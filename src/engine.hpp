#pragma once
// platform independent game engine code

// include platform constants
#include "platform.hpp"

#include "o1store.hpp"
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

// tile dimensions
static constexpr unsigned tile_width = 16;
static constexpr unsigned tile_height = 16;

// the right shift of 'x' to get the x in tiles map
static constexpr unsigned tile_width_shift = 4;

// the bits that are the partial tile position between 0 and not including
// 'tile_width'
static constexpr unsigned tile_width_and = 15;

// the right shift of 'y' to get the y in tiles map
static constexpr unsigned tile_height_shift = 4;

// the bits that are the partial tile position between 0 and not including
// 'tile_height'
static constexpr unsigned tile_height_and = 15;

class tile {
public:
  const uint8_t data[tile_width * tile_height];
} static constexpr tiles[tile_count]{
#include "game/resources/tile_imgs.hpp"
};

class tile_map {
public:
  tile_ix cell[tile_map_height][tile_map_width];
} static constexpr tile_map{{
#include "game/resources/tile_map.hpp"
}};

// tile map controls
static float tile_map_x = 0;
static float tile_map_dx = 0;
static float tile_map_y = 0;
static float tile_map_dy = 0;

// sprite dimensions
static constexpr unsigned sprite_width = 16;
static constexpr unsigned sprite_height = 16;

static constexpr int16_t sprite_width_neg = -int16_t(sprite_width);
// used when rendering

// images used by sprites
static constexpr uint8_t sprite_imgs[sprite_imgs_count]
                                    [sprite_width * sprite_height]{
#include "game/resources/sprite_imgs.hpp"
                                    };

using sprite_ix = uint8_t;
// data type used to index a sprite
// note. for 'collision_map' to fit in a contiguous block of heap it must be
// 8-bit

// the reserved 'sprite_ix' in 'collision_map' representing 'no sprite pixel'
static constexpr sprite_ix sprite_ix_reserved =
    std::numeric_limits<sprite_ix>::max();

// forward declaration of type
class object;

class sprite {
public:
  object *obj = nullptr;
  uint8_t const *img = nullptr;
  int16_t scr_x = 0;
  int16_t scr_y = 0;
  sprite **alloc_ptr = nullptr;
};

using sprites_store = o1store<sprite, 255, 1>;
// note. 255 because sprite_ix a.k.a. uint8_t max size is 255
// note. sprite 255 is reserved which gives 255 [0:254] usable sprites

static sprites_store sprites{};

// pixel precision collision detection between on screen sprites
// allocated at 'engine_setup()'
static sprite_ix *collision_map;
static constexpr unsigned collision_map_size =
    sizeof(sprite_ix) * display_width * display_height;

// helper class managing current frame time, dt, frames per second calculation
class clk {
public:
  using time = unsigned long;

private:
  unsigned interval_ms_ = 5000;
  unsigned frames_rendered_since_last_update_ = 0;
  time last_fps_update_ms_ = 0;
  time prv_ms_ = 0;

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
            const unsigned interval_of_fps_calculation_ms) {
    last_fps_update_ms_ = prv_ms_ = ms;
    interval_ms_ = interval_of_fps_calculation_ms;
    ms = time_ms;
  }

  // called before every frame to update state
  // returns true if new frames per second calculation was done
  auto on_frame(const unsigned long time_ms) -> bool {
    ms = time_ms;
    dt = 0.001f * (ms - prv_ms_);
    prv_ms_ = ms;
    frames_rendered_since_last_update_++;
    const unsigned long dt_ms = ms - last_fps_update_ms_;
    if (dt_ms >= interval_ms_) {
      fps = frames_rendered_since_last_update_ * 1000 / dt_ms;
      frames_rendered_since_last_update_ = 0;
      last_fps_update_ms_ = ms;
      return true;
    }
    return false;
  }
} static clk{};

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

using object_store = o1store<object, 255, 2, object_instance_max_size_B>;

class objects : public object_store {
public:
  void update() {
    object **it = allocated_list();
    const unsigned len = allocated_list_len();
    for (unsigned i = 0; i < len; i++, it++) {
      object *obj = *it;
      if (obj->update()) {
        obj->~object();
        free_instance(obj);
      }
    }
  }

  void pre_render() {
    object **it = allocated_list();
    const unsigned len = allocated_list_len();
    for (unsigned i = 0; i < len; i++, it++) {
      object *obj = *it;
      obj->pre_render();
    }
  }
} static objects{};

static void engine_setup() {
  // allocate collision map
  collision_map = (sprite_ix *)malloc(collision_map_size);
  if (!collision_map) {
    Serial.printf("!!! could not allocate collision map");
    while (true)
      ;
  }
}

// forward declaration of platform specific function
static void render(const unsigned x, const unsigned y);

// forward declaration of user provided callback
static void main_on_frame_completed();

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