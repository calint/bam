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

// note. why some buffers are allocated at 'setup'
// Due to a technical limitation, the maximum statically allocated DRAM usage is
// 160KB. The remaining 160KB (for a total of 320KB of DRAM) can only be
// allocated at runtime as heap.
// -- https://stackoverflow.com/questions/71085927/how-to-extend-esp32-heap-size

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

// buffers for rendering a chunk while the other is transferred to the screen
// using DMA. allocated in setup
static constexpr unsigned dma_buf_size =
    sizeof(uint16_t) * display_width * tile_height;
static uint16_t *dma_buf_1;
static uint16_t *dma_buf_2;

// note. allocating buffers in static memory leads to crash:
//    assert failed: vApplicationGetIdleTaskMemory port_common.c:194
//    (pxTCBBufferTemp != NULL)
// static uint16_t dma_buf_1[dma_buf_size];
// static uint16_t dma_buf_2[dma_buf_size];

static void render_scanline(uint16_t *render_buf_ptr,
                            sprite_ix *collision_map_scanline_ptr,
                            unsigned tile_x, unsigned tile_dx,
                            tile_ix const *tiles_map_row_ptr,
                            const int16_t scanline_y, const unsigned tile_sub_y,
                            const unsigned tile_sub_y_times_tile_width) {

  // used later by sprite renderer to overwrite tiles pixels
  uint16_t *scanline_ptr = render_buf_ptr;

  tile_ix const *tiles_map_ptr = tiles_map_row_ptr + tile_x;

  unsigned remaining_x = display_width;

  while (remaining_x) {
    uint8_t const *tile_data_ptr =
        tiles[*tiles_map_ptr] + tile_sub_y_times_tile_width + tile_dx;
    unsigned render_n_pixels = 0;
    if (tile_dx) {
      render_n_pixels = tile_width - tile_dx;
      tile_dx = 0;
    } else {
      render_n_pixels = remaining_x < tile_width ? remaining_x : tile_width;
    }
    remaining_x -= render_n_pixels;
    while (render_n_pixels--) {
      *render_buf_ptr++ = palette_tiles[*tile_data_ptr++];
    }
    tile_x++;
    tiles_map_ptr++;
  }

  // render sprites
  // note. although grossly inefficient algorithm the DMA is busy while
  // rendering one tile height of sprites and tiles

  sprite *spr = sprites.all_list();
  const unsigned len = sprites.all_list_len();
  for (unsigned i = 0; i < len; i++, spr++) {
    if (!spr->img or spr->scr_y > scanline_y or
        spr->scr_y + int16_t(sprite_height) <= scanline_y or
        spr->scr_x <= sprite_width_neg or spr->scr_x > int16_t(display_width)) {
      // sprite has no image or
      // not within scanline or
      // is outside the screen x-wise
      continue;
    }
    uint8_t const *spr_data_ptr =
        spr->img + (scanline_y - spr->scr_y) * sprite_width;
    uint16_t *scanline_dst_ptr = scanline_ptr + spr->scr_x;
    unsigned render_n_pixels = sprite_width;
    sprite_ix *collision_pixel = collision_map_scanline_ptr + spr->scr_x;
    if (spr->scr_x < 0) {
      // adjustment if x is negative
      spr_data_ptr -= spr->scr_x;
      scanline_dst_ptr -= spr->scr_x;
      render_n_pixels += spr->scr_x;
      collision_pixel -= spr->scr_x;
    } else if (spr->scr_x + sprite_width > display_width) {
      // adjustment if sprite partially outside screen (x-wise)
      render_n_pixels = display_width - spr->scr_x;
    }
    // render scanline of sprite
    object *obj = spr->obj;
    while (render_n_pixels--) {
      // write pixel from sprite data or skip if 0
      const uint8_t color_ix = *spr_data_ptr;
      if (color_ix) {
        *scanline_dst_ptr = palette_sprites[color_ix];
        if (*collision_pixel != sprite_ix_reserved) {
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

// buffer: one tile height, palette, 8-bit tiles from tiles map, 8-bit sprites
// 31 fps with DMA, 22 fps without
static void render(const unsigned x, const unsigned y) {
  display.startWrite();

  const unsigned tile_x = x >> tile_width_shift;
  const unsigned tile_dx = x & tile_width_and;
  unsigned tile_y = y >> tile_height_shift;
  unsigned tile_dy = y & tile_height_and;

  // selects buffer to write while DMA reads the other buffer
  bool dma_buf_use_first = true;
  // y in frame for current tiles row to be copied by DMA
  unsigned frame_y = 0;
  // current line y on screen
  int16_t scanline_y = 0;
  // pointer to start of current row of tiles
  tile_ix const *tiles_map_row_ptr = tile_map[tile_y];
  // pointer to collision map starting at top left of screen
  sprite_ix *collision_map_scanline_ptr = collision_map;
  unsigned remaining_y = display_height;
  while (remaining_y) {
    // swap between two rendering buffers to not overwrite DMA accessed
    // buffer
    uint16_t *render_buf_ptr = dma_buf_use_first ? dma_buf_1 : dma_buf_2;
    dma_buf_use_first = not dma_buf_use_first;
    // pointer to the buffer that the DMA will copy to screen
    uint16_t *dma_buf = render_buf_ptr;
    // render from tiles map and sprites to the 'render_buf_ptr'
    unsigned render_n_tile_lines =
        remaining_y < tile_height ? remaining_y : tile_height;
    unsigned render_n_scanlines = 0;
    unsigned tile_sub_y = 0;
    unsigned tile_sub_y_times_tile_width = 0;
    if (tile_dy) {
      render_n_scanlines = tile_height - tile_dy;
      tile_sub_y = tile_dy;
      tile_sub_y_times_tile_width = tile_dy * tile_height;
      tile_dy = 0;
    } else {
      render_n_scanlines = render_n_tile_lines;
      tile_sub_y = 0;
      tile_sub_y_times_tile_width = 0;
    }
    while (tile_sub_y < render_n_tile_lines) {
      render_scanline(render_buf_ptr, collision_map_scanline_ptr, tile_x,
                      tile_dx, tiles_map_row_ptr, scanline_y, tile_sub_y,
                      tile_sub_y_times_tile_width);
      tile_sub_y++;
      tile_sub_y_times_tile_width += tile_width;
      render_buf_ptr += display_width;
      collision_map_scanline_ptr += display_width;
      scanline_y++;
    }

    display.setAddrWindow(0, frame_y, display_width, render_n_scanlines);
    display.pushPixelsDMA(dma_buf, display_width * render_n_scanlines);

    tile_y++;
    frame_y += render_n_scanlines;
    remaining_y -= render_n_scanlines;
    tiles_map_row_ptr += tile_map_width;
  }

  display.endWrite();
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

  // heap_caps_dump_all();
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

  // allocate DMA buffers
  dma_buf_1 = static_cast<uint16_t *>(malloc(dma_buf_size));
  dma_buf_2 = static_cast<uint16_t *>(malloc(dma_buf_size));
  if (!dma_buf_1 or !dma_buf_2) {
    printf("!!! could not allocate DMA buffers");
    while (true)
      ;
  }

  engine_setup();

  printf("------------------- after init ---------------------------\n");
  printf("     free heap mem: %zu B\n", ESP.getFreeHeap());
  printf("largest free block: %zu B\n", ESP.getMaxAllocHeap());
  printf("------------------- globals ------------------------------\n");
  printf("          tile map: %zu B\n", sizeof(tile_map));
  printf("           sprites: %zu B\n", sizeof(sprites));
  printf("           objects: %zu B\n", sizeof(objects));
  printf("------------------- on heap ------------------------------\n");
  printf("      sprites data: %zu B\n", sprites.allocated_data_size_B());
  printf("      objects data: %zu B\n", objects.allocated_data_size_B());
  printf("     collision map: %zu B\n", collision_map_size);
  printf("   DMA buf 1 and 2: %zu B\n", 2 * dma_buf_size);
  printf("------------------- in program memory --------------------\n");
  printf("     sprite images: %zu B\n", sizeof(sprite_imgs));
  printf("             tiles: %zu B\n", sizeof(tiles));
  printf("----------------------------------------------------------\n");

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
  display.initDMA(true);

  // set random seed for deterministic behavior
  randomSeed(0);

  // initiate clock to current time and frames-per-second calculation every 2
  // seconds
  clk.init(millis(), 2000, clk_locked_dt_ms);

  main_setup();

  // set rgb led green
  digitalWrite(cyd_led_red, HIGH);
  digitalWrite(cyd_led_green, LOW);
  digitalWrite(cyd_led_blue, HIGH);
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
