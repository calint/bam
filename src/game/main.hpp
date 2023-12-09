#pragma once
// setup initial game state, callbacks from engine, game logic
// solves circular references between 'game_state' and game objects

// first include engine
#include "../engine.hpp"
// then the game state
#include "game_state.hpp"
// then the objects
#include "objects/bullet.hpp"
#include "objects/dummy.hpp"
#include "objects/hero.hpp"
#include "objects/ship1.hpp"
#include "objects/ship2.hpp"

// callback at boot
static void main_setup() {
  // scrolling vertically from bottom up
  tile_map_x = 0;
  tile_map_dx = 16;
  tile_map_y = tile_map_height * tile_height - display_height;
  tile_map_dy = -16;
  // tile_map_y = 0;
  // tile_map_dy = 1;

  hero *hro = new (objects.allocate_instance()) hero{};
  hro->x = display_width / 2 - sprite_width / 2;
  hro->y = 30;

  // bullet *blt = new (objects.allocate_instance()) bullet{};
  // blt->x = display_width / 2 - sprite_width / 2;
  // blt->y = 300;
  // blt->dy = -100;

  printf("------------------- game objects -------------------------\n");
  printf("----------------------------------------------------------\n");
  printf("            bullet: %zu B\n", sizeof(bullet));
  printf("             dummy: %zu B\n", sizeof(dummy));
  printf("          fragment: %zu B\n", sizeof(fragment));
  printf("              hero: %zu B\n", sizeof(hero));
  printf("             ship1: %zu B\n", sizeof(ship1));
  printf("             ship2: %zu B\n", sizeof(ship2));
  printf("    upgrade_picked: %zu B\n", sizeof(upgrade_picked));
  printf("           upgrade: %zu B\n", sizeof(upgrade));
  printf("----------------------------------------------------------\n");
}

static clk::time last_fire_ms = 0;
// keeps track of when the previous bullet was fired

// callback when screen is touched, happens before 'update'
static void main_on_touch_screen(int16_t x, int16_t y, int16_t z) {
  // fire eight times a second
  if (clk.ms - last_fire_ms > 125) {
    last_fire_ms = clk.ms;
    if (objects.can_allocate()) {
      bullet *blt = new (objects.allocate_instance()) bullet{};
      blt->x = float((x - touch_screen_min_x) * display_width /
                     touch_screen_range_x);
      blt->y = 300;
      blt->dy = -100;
    }
  }
}

// forward declaration of functions that start waves of objects
static void main_wave_1();
static void main_wave_2();
static void main_wave_3();
static void main_wave_4();
// static void main_wave_5();

// constant used to more easily position where waves are triggered
constexpr int tiles_per_screen = display_height / tile_height;

// pointer to function that creates wave
using wave_func_ptr = void (*)();

struct wave_trigger {
  float y = 0;
  wave_func_ptr func = nullptr;

  constexpr wave_trigger(float y_, wave_func_ptr func_) : y{y_}, func{func_} {}
  // note. constructor needed for C++11 to compile

} static constexpr wave_triggers[] = {
    {float((tile_map_height - tiles_per_screen * 1.0f) * tile_height),
     main_wave_4},
    {float((tile_map_height - tiles_per_screen * 1.5f) * tile_height),
     main_wave_1},
    {float((tile_map_height - tiles_per_screen * 2.0f) * tile_height),
     main_wave_2},
    {float((tile_map_height - tiles_per_screen * 2.5f) * tile_height),
     main_wave_3},
    {float((tile_map_height - tiles_per_screen * 3.5f) * tile_height),
     main_wave_4},
    {float((tile_map_height - tiles_per_screen * 4.5f) * tile_height),
     main_wave_3},
    {float((tile_map_height - tiles_per_screen * 5.0f) * tile_height),
     main_wave_2},
    {float((tile_map_height - tiles_per_screen * 5.5f) * tile_height),
     main_wave_1},
    {float((tile_map_height - tiles_per_screen * 6.0f) * tile_height),
     main_wave_4},
};

static constexpr int wave_triggers_len =
    sizeof(wave_triggers) / sizeof(wave_trigger);

static int wave_triggers_ix = 0;

// callback after frame has been rendered, happens after 'update'
static void main_on_frame_completed() {
  // update x position in pixels in the tile map
  tile_map_x += tile_map_dx * clk.dt;
  if (tile_map_x < 0) {
    tile_map_x = 0;
    tile_map_dx = -tile_map_dx;
  } else if (tile_map_x > (tile_map_width * tile_width - display_width)) {
    tile_map_x = tile_map_width * tile_width - display_width;
    tile_map_dx = -tile_map_dx;
  }
  // update y position in pixels in the tile map
  tile_map_y += tile_map_dy * clk.dt;
  if (tile_map_y < 0) {
    tile_map_y = 0;
    tile_map_dy = -tile_map_dy;
  } else if (tile_map_y > (tile_map_height * tile_height - display_height)) {
    tile_map_y = tile_map_height * tile_height - display_height;
    tile_map_dy = -tile_map_dy;
    wave_triggers_ix = 0;
  }

  if (not game_state.hero_is_alive) {
    hero *hro = new (objects.allocate_instance()) hero{};
    hro->x = float(rand()) * float(display_width) / float(RAND_MAX);
    hro->y = 30;
    hro->dx = float(rand()) * 64.0f / float(RAND_MAX);
  }

  // trigger waves
  if (wave_triggers_ix < wave_triggers_len and
      wave_triggers[wave_triggers_ix].y >= tile_map_y) {
    // Serial.printf("wave trigger  y=%f  trigger y = %f\n", tile_map_y,
    //               wave_triggers[wave_triggers_ix].y);
    wave_triggers[wave_triggers_ix].func();
    wave_triggers_ix++;
  }
}

void main_wave_1() {
  float x = 8;
  float y = -float(sprite_height);
  for (int i = 0; i < 8; i++) {
    ship1 *shp = new (objects.allocate_instance()) ship1{};
    shp->x = x;
    shp->y = y;
    shp->dy = 50;
    x += 32;
    y -= 8;
  }
}

void main_wave_2() {
  float x = 8;
  float y = -float(sprite_height);
  for (int i = 0; i < 8; i++) {
    ship1 *shp = new (objects.allocate_instance()) ship1{};
    shp->x = x;
    shp->y = y;
    shp->dy = 50;
    x += 32;
  }
}

void main_wave_3() {
  float y = -float(sprite_height);
  for (int j = 0; j < 8; j++, y -= 24) {
    float x = 8;
    for (int i = 0; i < 8; i++, x += 32) {
      ship1 *shp = new (objects.allocate_instance()) ship1{};
      shp->x = x;
      shp->y = y;
      shp->dy = 50;
    }
  }
}

void main_wave_4() {
  {
    ship2 *shp = new (objects.allocate_instance()) ship2{};
    shp->x = -float(sprite_width);
    shp->y = -float(sprite_height);
    shp->dy = 25;
    shp->dx = 12;
    shp->ddy = 20;
    shp->ddx = 10;
  }
  {
    ship2 *shp = new (objects.allocate_instance()) ship2{};
    shp->x = display_width;
    shp->y = -float(sprite_height);
    shp->dy = 25;
    shp->dx = -12;
    shp->ddy = 20;
    shp->ddx = -10;
  }
}

// void main_wave_5() {
//   float y = -float(sprite_height);
//   for (int j = 0; j < 12; j++, y -= 10) {
//     float x = 8;
//     for (int i = 0; i < 20; i++, x += 10) {
//       ship1 *shp = new (objects.allocate_instance()) ship1{};
//       shp->x = x;
//       shp->y = y;
//       shp->dy = 30;
//     }
//   }
// }