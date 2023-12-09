#pragma once
// constants used by engine, game objects and 'main.hpp'

#include <cstdint>

// portrait or landscape orientation of screen
// 0: portrait, 1: landscape
static constexpr uint8_t display_orientation = 0;

// lock dt to 32 ms per frame (~31 fps) for deterministic behavior
static constexpr int clk_locked_dt_ms = 32;
// use measured time to increase dt
// static constexpr int clk_locked_dt_ms = 0;

// number of sprite images
static constexpr int sprite_imgs_count = 256;
// defined in 'resources/sprite_imgs.hpp'

// type used to index in the 'sprite_imgs' array
using sprite_img_ix = uint8_t;

// sprite dimensions
static constexpr int sprite_width = 16;
static constexpr int sprite_height = 16;

// number of tile images
static constexpr int tiles_count = 256;
// defined in 'resources/tiles.hpp'

// type used to index in the 'tiles' array from 'tile_map'
using tile_ix = uint8_t;

// tile dimensions
static constexpr int tile_width = 16;
static constexpr int tile_height = 16;

//
// example configuration of more sprites and tiles
//
// static constexpr int sprite_imgs_count = 512;
// using sprite_img_ix = uint16_t;
// static constexpr int tiles_count = 512;
// using tile_ix = uint16_t;

// tile map dimension
static constexpr int tile_map_width = 15;
static constexpr int tile_map_height = 320;
// defined in 'resources/tile_map.hpp'

// type used to index a 'sprite'
// note. 8-bit for 'collision_map' to fit in a contiguous block on heap
using sprite_ix = uint8_t;

// sprites available for allocation using 'sprites'
// note. maximum is one less than limit of type 'sprite_ix' due to the reserved
// sprite index (maximum limit) used at collision detection
static constexpr int sprites_count = 255;

// objects available for allocation using 'objects'
static constexpr int objects_count = 255;

// enumeration of game object classes
// defined in 'objects/*'
enum object_class : uint8_t {
  hero_cls,
  bullet_cls,
  dummy_cls,
  fragment_cls,
  ship1_cls,
  ship2_cls,
  upgrade_cls,
  upgrade_picked_cls
};

// size that fits any instance of game object
static constexpr int object_instance_max_size_B = 256;

// define the size of collision bits
using collision_bits = uint16_t;

// collision bits
static constexpr collision_bits cb_none = 0;
static constexpr collision_bits cb_hero = 1 << 0;
static constexpr collision_bits cb_hero_bullet = 1 << 1;
static constexpr collision_bits cb_fragment = 1 << 2;
static constexpr collision_bits cb_enemy = 1 << 3;
static constexpr collision_bits cb_enemy_bullet = 1 << 4;
static constexpr collision_bits cb_upgrade = 1 << 5;
