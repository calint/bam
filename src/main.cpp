//
// intended for: ESP32-2432S028R
//    ESP32 Arduino LVGL WIFI & Bluetooth Development Board 2.8"
//    240 * 320 Smart Display Screen 2.8 inch LCD TFT Module With Touch WROOM
//
//          from: http://www.jczn1688.com/
//  purchased at: https://www.aliexpress.com/item/1005004502250619.html
//     resources: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
//

// note. design decision of 'hpp' source files
// * the program is one file split into logical sections using includes
// * increases opportunities for optimization by the compiler
// * directory 'game' contains the user code that interfaces with 'engine.hpp'
// * order of include and content of 'defs.hpp', 'game.hpp', 'main.hpp' solves
//   circular references and gives user the necessary callbacks to interface
//   with engine

// note. design decision regarding 'unsigned'
// due to sign conversion warnings and subtle bugs in mixed signedness
// operations signed constants and variables are used where the bit width of the
// type is wide enough to fit the largest values

// note. why some buffers are allocated at 'setup'
// Due to a technical limitation, the maximum statically allocated DRAM usage is
// 160KB. The remaining 160KB (for a total of 320KB of DRAM) can only be
// allocated at runtime as heap.
// -- https://stackoverflow.com/questions/71085927/how-to-extend-esp32-heap-size

// note. 38476 B static memory left for freertos not to crash

#include <Arduino.h>

// main entry file to user code
#include "game/main.hpp"

// platform specific definitions and objects
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// rgb led
static constexpr uint8_t cyd_led_blue = 17;
static constexpr uint8_t cyd_led_red = 4;
static constexpr uint8_t cyd_led_green = 16;

// ldr (light dependant resistor)
// analog read of pin gives: 0 for full brightness, higher values is darker
static constexpr uint8_t cyd_ldr = 34;

// setup touch screen
// https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/2-TouchTest/2-TouchTest.ino
static constexpr uint8_t xpt2046_irq = 36;  // Interrupt Request
static constexpr uint8_t xpt2046_mosi = 32; // Master Out Slave In
static constexpr uint8_t xpt2046_miso = 39; // Master In Slave Out
static constexpr uint8_t xpt2046_clk = 25;  // Clock
static constexpr uint8_t xpt2046_cs = 33;   // Chip Select

static SPIClass spi{HSPI};
static XPT2046_Touchscreen touch_screen{xpt2046_cs, xpt2046_irq};
static TFT_eSPI display{};

// number of scanlines to render before DMA transfer
constexpr int dma_n_scanlines = 8;
// note. performance on device:
//  1: 23 fps
//  2: 27 fps
//  4: 29 fps
//  8: 31 fps
// 16: 31 fps
// 32: 32 fps

// alternating buffers for rendering scanlines while DMA is active
// note. allocating buffers in static memory may leads to freertos crash due to
//       not having enough memory (dma_n_scanlines > 40 when width is 240):
//       assert failed: vApplicationGetIdleTaskMemory port_common.c:194
//       (pxTCBBufferTemp != NULL)
static uint16_t dma_buf_1[display_width * dma_n_scanlines];
static uint16_t dma_buf_2[display_width * dma_n_scanlines];
static constexpr int dma_buf_size_B = sizeof(dma_buf_1);

// allocated in 'setup'
// static constexpr int dma_buf_size_B =
//     sizeof(uint16_t) * display_width * dma_n_scanlines;
// static uint16_t *dma_buf_1;
// static uint16_t *dma_buf_2;

// renders a scanline
// note. inline because it is only called from render(...)
static inline void render_scanline(uint16_t *render_buf_ptr,
                                   sprite_ix *collision_map_row_ptr, int tile_x,
                                   int tile_x_fract,
                                   tile_ix const *tiles_map_row_ptr,
                                   const int16_t scanline_y,
                                   const int tile_line_times_tile_width) {

  // used later by sprite renderer to overwrite tiles pixels
  uint16_t *scanline_ptr = render_buf_ptr;
  // pointer to first tile to render
  tile_ix const *tiles_map_ptr = tiles_map_row_ptr + tile_x;
  // for all horizontal pixels
  int remaining_x = display_width;
  while (remaining_x) {
    // pointer to tile data to render
    uint8_t const *tile_data_ptr =
        tiles[*tiles_map_ptr] + tile_line_times_tile_width + tile_x_fract;
    // calculate number of pixels to render
    int render_n_pixels = 0;
    if (tile_x_fract) {
      // note. assumes display width is at least a tile width
      render_n_pixels = tile_width - tile_x_fract;
      tile_x_fract = 0;
    } else {
      render_n_pixels = remaining_x < tile_width ? remaining_x : tile_width;
    }
    // decrease remaining pixels to render before using that variable
    remaining_x -= render_n_pixels;
    while (render_n_pixels--) {
      *render_buf_ptr++ = palette_tiles[*tile_data_ptr++];
    }
    // next tile
    tiles_map_ptr++;
  }

  // render sprites
  // note. although grossly inefficient algorithm the DMA is busy while
  //       rendering

  sprite *spr = sprites.all_list();
  const int len = sprites.all_list_len();
  for (sprite_ix i = 0; i < len; i++, spr++) {
    if (!spr->img or spr->scr_y > scanline_y or
        spr->scr_y + sprite_height <= scanline_y or
        spr->scr_x <= -sprite_width or spr->scr_x >= display_width) {
      // sprite has no image or
      // not within scanline or
      // is outside the screen x-wise
      continue;
    }
    // pointer to sprite data to be rendered
    uint8_t const *spr_data_ptr =
        spr->img + (scanline_y - spr->scr_y) * sprite_width;
    // pointer to destination of sprite data
    uint16_t *scanline_dst_ptr = scanline_ptr + spr->scr_x;
    // initial number of pixels to be rendered
    int render_n_pixels = sprite_width;
    // pointer to collision map for first pixel of sprite
    sprite_ix *collision_pixel = collision_map_row_ptr + spr->scr_x;
    if (spr->scr_x < 0) {
      // adjustments if sprite x is negative
      spr_data_ptr -= spr->scr_x;
      scanline_dst_ptr -= spr->scr_x;
      render_n_pixels += spr->scr_x;
      collision_pixel -= spr->scr_x;
    } else if (spr->scr_x + sprite_width > display_width) {
      // adjustment if sprite partially outside screen (x-wise)
      render_n_pixels = display_width - spr->scr_x;
    }
    // render line from sprite to scanline and check collisions
    object *obj = spr->obj;
    while (render_n_pixels--) {
      // write pixel from sprite data or skip if 0
      const uint8_t color_ix = *spr_data_ptr;
      if (color_ix) {
        // if not transparent pixel
        *scanline_dst_ptr = palette_sprites[color_ix];
        if (*collision_pixel != sprite_ix_reserved) {
          // if other sprite has written to this pixel
          sprite *spr2 = sprites.instance(*collision_pixel);
          object *other_obj = spr2->obj;
          if (obj->col_mask & other_obj->col_bits) {
            obj->col_with = other_obj;
          }
          if (other_obj->col_mask & obj->col_bits) {
            other_obj->col_with = obj;
          }
        }
        // set pixel collision value to sprite index
        *collision_pixel = i;
      }
      spr_data_ptr++;
      collision_pixel++;
      scanline_dst_ptr++;
    }
  }
}

// returns number of shifts to convert a 2^n number to 1
static constexpr int count_right_shifts_until_1(int num) {
  return (num <= 1) ? 0 : 1 + count_right_shifts_until_1(num >> 1);
}

// renders tile map and sprites
static void render(const int x, const int y) {
  // extract whole number and fractions from x, y
  constexpr int tile_width_shift = count_right_shifts_until_1(tile_width);
  constexpr int tile_height_shift = count_right_shifts_until_1(tile_height);
  constexpr int tile_width_and = (1 << tile_width_shift) - 1;
  constexpr int tile_height_and = (1 << tile_height_shift) - 1;
  const int tile_x = x >> tile_width_shift;
  const int tile_x_fract = x & tile_width_and;
  int tile_y = y >> tile_height_shift;
  int tile_y_fract = y & tile_height_and;
  // current scanline screen y
  int16_t scanline_y = 0;
  // pointer to start of current row of tiles
  tile_ix const *tiles_map_row_ptr = tile_map[tile_y];
  // pointer to collision map starting at top left of screen
  sprite_ix *collision_map_row_ptr = collision_map;
  // keeps track of how many scanlines have been rendered since last DMA
  // transfer
  int dma_scanline_count = 0;
  // select first buffer for rendering
  uint16_t *render_buf_ptr = dma_buf_1;
  // which dma buffer to use next
  bool dma_buf_use_first = false;
  // pointer to the buffer that DMA will copy to screen
  uint16_t *dma_buf = render_buf_ptr;
  // for all lines on display
  int remaining_y = display_height;
  while (remaining_y) {
    // render from tiles map and sprites to the 'render_buf_ptr'
    int render_n_tile_lines =
        remaining_y < tile_height ? remaining_y : tile_height;
    // prepare loop variables
    int render_n_scanlines = 0;
    int tile_line = 0;
    int tile_line_times_tile_width = 0;
    if (tile_y_fract) {
      // note. assumes display height is at least a tile height -1
      render_n_scanlines = tile_height - tile_y_fract;
      tile_line = tile_y_fract;
      tile_line_times_tile_width = tile_y_fract * tile_height;
      tile_y_fract = 0;
    } else {
      render_n_scanlines = render_n_tile_lines;
      tile_line = 0;
      tile_line_times_tile_width = 0;
    }
    // render a row from tile map
    while (tile_line < render_n_tile_lines) {
      render_scanline(render_buf_ptr, collision_map_row_ptr, tile_x,
                      tile_x_fract, tiles_map_row_ptr, scanline_y,
                      tile_line_times_tile_width);
      tile_line++;
      tile_line_times_tile_width += tile_width;
      render_buf_ptr += display_width;
      collision_map_row_ptr += display_width;
      scanline_y++;
      dma_scanline_count++;
      if (dma_scanline_count == dma_n_scanlines) {
        display.pushPixelsDMA(dma_buf,
                              unsigned(display_width * dma_n_scanlines));
        dma_scanline_count = 0;
        // swap to the other render buffer
        dma_buf = render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
        dma_buf_use_first = not dma_buf_use_first;
      }
    }
    tile_y++;
    remaining_y -= render_n_scanlines;
    tiles_map_row_ptr += tile_map_width;
  }
  // in case 'dma_n_scanlines' and 'display_height' not evenly divisible there
  // will be remaining scanlines to write
  constexpr int dma_n_scanlines_trailing = display_height % dma_n_scanlines;
  if (dma_n_scanlines_trailing) {
    display.pushPixelsDMA(dma_buf,
                          unsigned(display_width * dma_n_scanlines_trailing));
  }
}

void setup() {
  // setup rgb led pins
  pinMode(cyd_led_red, OUTPUT);
  pinMode(cyd_led_green, OUTPUT);
  pinMode(cyd_led_blue, OUTPUT);

  // set rgb led to yellow
  digitalWrite(cyd_led_red, LOW);
  digitalWrite(cyd_led_green, LOW);
  digitalWrite(cyd_led_blue, HIGH);

  Serial.begin(115200);
  sleep(1); // arbitrary wait 1 second for serial to connect

  printf("\n\n");
  printf("------------------- platform -----------------------------\n");
  printf("        chip model: %s\n", ESP.getChipModel());
  printf("            screen: %u x %u px\n", display_width, display_height);
  printf("     free heap mem: %u B\n", ESP.getFreeHeap());
  printf("largest free block: %u B\n", ESP.getMaxAllocHeap());
  printf("------------------- type sizes ---------------------------\n");
  printf("              bool: %zu B\n", sizeof(bool));
  printf("              char: %zu B\n", sizeof(char));
  printf("               int: %zu B\n", sizeof(int));
  printf("              long: %zu B\n", sizeof(long));
  printf("         long long: %zu B\n", sizeof(long long));
  printf("             float: %zu B\n", sizeof(float));
  printf("            double: %zu B\n", sizeof(double));
  printf("             void*: %zu B\n", sizeof(void *));
  printf("------------------- object sizes -------------------------\n");
  printf("            sprite: %zu B\n", sizeof(sprite));
  printf("            object: %zu B\n", sizeof(object));
  printf("              tile: %zu B\n", sizeof(tiles[0]));
  printf("------------------- in program memory --------------------\n");
  printf("     sprite images: %zu B\n", sizeof(sprite_imgs));
  printf("             tiles: %zu B\n", sizeof(tiles));
  printf("------------------- globals ------------------------------\n");
  printf("   DMA buf 1 and 2: %u B\n", 2 * dma_buf_size_B);
  printf("          tile map: %zu B\n", sizeof(tile_map));
  printf("           sprites: %zu B\n", sizeof(sprites));
  printf("           objects: %zu B\n", sizeof(objects));

  // set rgb led blue
  digitalWrite(cyd_led_red, HIGH);
  digitalWrite(cyd_led_green, HIGH);
  digitalWrite(cyd_led_blue, LOW);

  // setup ldr pin
  pinMode(cyd_ldr, INPUT);

  // start the spi for the touch screen and init the library
  spi.begin(xpt2046_clk, xpt2046_miso, xpt2046_mosi, xpt2046_cs);
  touch_screen.begin(spi);
  touch_screen.setRotation(display_orientation);

  // initiate display
  display.init();
  display.setRotation(display_orientation);
  display.setAddrWindow(0, 0, display_width, display_height);
  display.initDMA(true);

  // set random seed for deterministic behavior
  randomSeed(0);

  // initiate clock to current time, frames-per-second calculation every 2
  // seconds and locked dt
  clk.init(millis(), 2000, clk_locked_dt_ms);

  engine_setup();

  main_setup();

  // set rgb led green
  digitalWrite(cyd_led_red, HIGH);
  digitalWrite(cyd_led_green, LOW);
  digitalWrite(cyd_led_blue, HIGH);

  printf("------------------- on heap ------------------------------\n");
  printf("      sprites data: %zu B\n", sprites.allocated_data_size_B());
  printf("      objects data: %zu B\n", objects.allocated_data_size_B());
  printf("     collision map: %zu B\n", collision_map_size_B);
  printf("------------------- after setup --------------------------\n");
  printf("     free heap mem: %zu B\n", ESP.getFreeHeap());
  printf("largest free block: %zu B\n", ESP.getMaxAllocHeap());
  printf("----------------------------------------------------------\n");
}

void loop() {
  if (clk.on_frame(millis())) {
    printf("t=%lu  fps=%u  ldr=%u  objs=%u  sprs=%u\n", clk.ms, clk.fps,
           analogRead(cyd_ldr), objects.allocated_list_len(),
           sprites.allocated_list_len());
  }

  if (touch_screen.tirqTouched() and touch_screen.touched()) {
    const TS_Point pt = touch_screen.getPoint();
    main_on_touch_screen(pt.x, pt.y, pt.z);
  }

  engine_loop();
}
