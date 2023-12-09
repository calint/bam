#pragma once
// platform independent game engine code

// include platform constants
#include "platform.hpp"

#include "o1store.hpp"

#include <cstdint>
#include <cstring>
#include <limits>

// palette used when rendering tiles
// converts uint8_t to uint16_t rgb 565 (red being the highest bits)
// note. lower and higher byte swapped
static constexpr uint16_t palette_tiles[256]{
#include "game/resources/palette_tiles.hpp"
};

// palette used when rendering sprites
static constexpr uint16_t palette_sprites[256]{
#include "game/resources/palette_sprites.hpp"
};

static constexpr uint8_t tiles[tiles_count][tile_width * tile_height]{
#include "game/resources/tiles.hpp"
};

static tile_ix tile_map[tile_map_height][tile_map_width]{
#include "game/resources/tile_map.hpp"
};

// returns number of shifts to convert a 2^n number to 1
static constexpr int count_right_shifts_until_1(unsigned num) {
  return (num <= 1) ? 0 : 1 + count_right_shifts_until_1(num >> 1);
}

// the right shift of 'x' to get the x in tiles map
static constexpr unsigned tile_width_shift =
    count_right_shifts_until_1(tile_width);

// the right shift of 'y' to get the y in tiles map
static constexpr unsigned tile_height_shift =
    count_right_shifts_until_1(tile_height);
;

// the bits that are the partial tile position between 0 and not including
// 'tile_width'
static constexpr unsigned tile_width_and = (1 << tile_width_shift) - 1;

// the bits that are the partial tile position between 0 and not including
// 'tile_height'
static constexpr unsigned tile_height_and = (1 << tile_height_shift) - 1;

// tile map controls
static float tile_map_x = 0;
static float tile_map_dx = 0;
static float tile_map_y = 0;
static float tile_map_dy = 0;

// used when rendering
static constexpr int16_t sprite_width_neg = -int16_t(sprite_width);
static constexpr int16_t sprite_height_neg = -int16_t(sprite_height);

// images used by sprites
static constexpr uint8_t sprite_imgs[sprite_imgs_count]
                                    [sprite_width * sprite_height]{
#include "game/resources/sprite_imgs.hpp"
                                    };

// the reserved 'sprite_ix' in 'collision_map' representing 'no sprite pixel'
static constexpr sprite_ix sprite_ix_reserved =
    std::numeric_limits<sprite_ix>::max();

// forward declaration of type
class object;

class sprite {
public:
  sprite **alloc_ptr = nullptr;
  object *obj = nullptr;
  uint8_t const *img = nullptr;
  int16_t scr_x = 0;
  int16_t scr_y = 0;
};

using sprites_store = o1store<sprite, sprites_count, 1>;

static sprites_store sprites{};

// pixel precision collision detection between on screen sprites
// allocated at 'engine_setup()'
static constexpr unsigned collision_map_size =
    sizeof(sprite_ix) * display_width * display_height;
static sprite_ix *collision_map;

// note. allocating collision_map in static ram gives device error:
// rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
// static sprite_ix collision_map[collision_map_size];

class object {
public:
  object **alloc_ptr;
  // note. no default value since it would overwrite the 'o1store' assigned
  // value at 'allocate_instance()'

  object *col_with = nullptr;
  collision_bits col_bits = 0;
  collision_bits col_mask = 0;
  // note: used to declare interest in collisions with objects whose
  // 'col_bits' bitwise AND with this 'col_mask' is not 0

  object() {}
  // note. constructor must be defined because the default constructor
  // overwrites the 'o1store' assigned 'alloc_ptr' at the 'new in place'

  virtual ~object() {}

  // returns true if object has died
  // note. regarding classes overriding 'update(...)'
  // after 'update(...)' 'col_with' should be 'nullptr'
  virtual auto update() -> bool { return false; }

  // called before rendering the sprites
  virtual void pre_render() {}
};

using object_store =
    o1store<object, objects_count, 2, object_instance_max_size_B>;

class objects : public object_store {
public:
  void update() {
    object **end = allocated_list_end();
    // note. important to get the 'end' outside the loop because objects may
    // allocate new objects in the loop and that would change the 'end'
    for (object **it = allocated_list(); it < end; it++) {
      object *obj = *it;
      if (obj->update()) {
        obj->~object();
        free_instance(obj);
      }
    }
  }

  void pre_render() {
    object **end = allocated_list_end();
    // note. important to get the 'end' outside the loop because objects may
    // allocate new objects in the loop and that would change the 'end'
    for (object **it = allocated_list(); it < end; it++) {
      object *obj = *it;
      obj->pre_render();
    }
  }
} static objects{};

// helper class managing current frame time, dt, frames per second calculation
class clk {
public:
  using time = unsigned long;

private:
  unsigned interval_ms_ = 5000;
  unsigned frames_rendered_since_last_update_ = 0;
  time last_fps_update_ms_ = 0;
  time prv_ms_ = 0;
  unsigned locked_dt_ms_ = 0;

public:
  // current time since boot in milliseconds
  time ms = 0;

  // frame delta time in seconds
  float dt = 0;

  // current frames per second calculated at interval specified at 'init'
  unsigned fps = 0;

  // called at setup with current time and frames per seconds calculation
  // interval
  void init(const unsigned long time_ms,
            const unsigned interval_of_fps_calculation_ms,
            const unsigned locked_dt_ms) {
    interval_ms_ = interval_of_fps_calculation_ms;
    if (locked_dt_ms) {
      locked_dt_ms_ = locked_dt_ms;
      dt = 1.0f / locked_dt_ms;
    } else {
      prv_ms_ = ms = time_ms;
    }
    last_fps_update_ms_ = time_ms;
  }

  // called before every frame to update state
  // returns true if new frames per second calculation was done
  auto on_frame(const unsigned long time_ms) -> bool {
    if (locked_dt_ms_) {
      ms += locked_dt_ms_;
    } else {
      ms = time_ms;
      dt = 0.001f * (ms - prv_ms_);
      prv_ms_ = ms;
    }
    frames_rendered_since_last_update_++;
    const unsigned long dt_ms = time_ms - last_fps_update_ms_;
    if (dt_ms >= interval_ms_) {
      fps = frames_rendered_since_last_update_ * 1000 / dt_ms;
      frames_rendered_since_last_update_ = 0;
      last_fps_update_ms_ = time_ms;
      return true;
    }
    return false;
  }
} static clk{};

// callback from 'main.cpp'
static void engine_setup() {
  collision_map = static_cast<sprite_ix *>(malloc(collision_map_size));
  if (!collision_map) {
    printf("!!! could not allocate collision map\n");
    while (true)
      ;
  }
}

// forward declaration of platform specific function
static void render(const unsigned x, const unsigned y);

// forward declaration of user provided callback
static void main_on_frame_completed();

// callback from 'main.cpp'
// update and render the state of the engine
static void engine_loop() {
  // call 'update()' on allocated objects
  objects.update();

  // deallocate the objects freed during 'objects.update()'
  objects.apply_free();

  // deallocate the sprites freed during 'objects.update()'
  sprites.apply_free();

  // clear collisions map
  memset(collision_map, sprite_ix_reserved, collision_map_size);

  // prepare objects for render
  objects.pre_render();

  // render tiles, sprites and collision map
  render(unsigned(tile_map_x), unsigned(tile_map_y));

  // game logic hook
  main_on_frame_completed();
}
