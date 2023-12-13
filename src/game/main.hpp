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
#include "objects/ufo2.hpp"
// then other
#include "objects/utils.hpp"

// callback from 'setup'
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

  printf("------------------- game object sizes --------------------\n");
  printf("       game object: %zu B\n", sizeof(game_object));
  printf("            bullet: %zu B\n", sizeof(bullet));
  printf("             dummy: %zu B\n", sizeof(dummy));
  printf("          fragment: %zu B\n", sizeof(fragment));
  printf("              hero: %zu B\n", sizeof(hero));
  printf("             ship1: %zu B\n", sizeof(ship1));
  printf("             ship2: %zu B\n", sizeof(ship2));
  printf("    upgrade_picked: %zu B\n", sizeof(upgrade_picked));
  printf("           upgrade: %zu B\n", sizeof(upgrade));
  printf("              ufo2: %zu B\n", sizeof(ufo2));
}

static clk::time last_fire_ms = 0;
// keeps track of when the previous bullet was fired

// callback when screen is touched, happens before 'update'
static void main_on_touch(int16_t x, int16_t y, int16_t z) {
  // fire eight times a second
  if (clk.ms - last_fire_ms > 125) {
    last_fire_ms = clk.ms;
    if (object *mem = objects.allocate_instance()) {
      bullet *blt = new (mem) bullet{};
      // printf("%d  %d\n", x, y);
      blt->x = display_x_for_touch(x);
      blt->y = display_height - 30;
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

static constexpr float y_for_screen_percentage(float offset_as_screen_percentage) {
  return float(display_height * offset_as_screen_percentage);
}

struct wave_trigger {
  float since_last_wave_y = 0;
  wave_func_ptr func = nullptr;

  constexpr wave_trigger(float y_, wave_func_ptr func_)
      : since_last_wave_y{y_}, func{func_} {}
  // note. constructor needed for C++11 to compile

} static constexpr wave_triggers[] = {
    {y_for_screen_percentage(0.5f), main_wave_4},
    {y_for_screen_percentage(0.5f), main_wave_1},
    {y_for_screen_percentage(1.0f), main_wave_2},
    {y_for_screen_percentage(0.5f), main_wave_3},
    {y_for_screen_percentage(1.0f), main_wave_4},
    {y_for_screen_percentage(1.0f), main_wave_3},
    {y_for_screen_percentage(0.5f), main_wave_2},
    {y_for_screen_percentage(0.5f), main_wave_1},
    {y_for_screen_percentage(0.5f), main_wave_4},
};

static constexpr float wave_triggers_bottom_screen_y =
    tile_map_height * tile_height - display_height;

static constexpr int wave_triggers_len =
    sizeof(wave_triggers) / sizeof(wave_trigger);

static int wave_triggers_ix = 0;

static float wave_triggers_next_y =
    wave_triggers_bottom_screen_y - wave_triggers[0].since_last_wave_y;

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
    wave_triggers_next_y =
        wave_triggers_bottom_screen_y - wave_triggers[0].since_last_wave_y;
  }

  if (not game_state.hero_is_alive) {
    hero *hro = new (objects.allocate_instance()) hero{};
    hro->x = random_float(0, display_width);
    hro->y = 30;
    hro->dx = random_float(-64, 64);
  }

  // trigger waves
  if (wave_triggers_ix < wave_triggers_len and
      wave_triggers_next_y >= tile_map_y) {
    wave_triggers[wave_triggers_ix].func();
    wave_triggers_ix++;
    if (wave_triggers_ix < wave_triggers_len) {
      wave_triggers_next_y -= wave_triggers[wave_triggers_ix].since_last_wave_y;
    }
    // Serial.printf("wave trigger  ix=%d  trigger_y=%f  trigger y = %f\n",
    //               wave_triggers_ix, wave_triggers_next_y);
  }
}

static void main_wave_1() {
  constexpr int count = display_width / (sprite_width * 3 / 2);
  constexpr int dx = display_width / count;
  // printf("wave1: count=%d  dx=%d\n", count, dx);
  float x = 0;
  float y = -sprite_height;
  for (int i = 0; i < count; i++) {
    ship1 *shp = new (objects.allocate_instance()) ship1{};
    shp->x = x;
    shp->y = y;
    shp->dy = 50;
    x += dx;
    y -= sprite_width / 2;
  }
}

static void main_wave_2() {
  constexpr int count = display_width / (sprite_width * 3 / 2);
  constexpr int dx = display_width / count;
  // printf("wave2: count=%d  dx=%d\n", count, dx);
  float x = 0;
  float y = -sprite_height;
  for (int i = 0; i < count; i++, x += dx) {
    ship1 *shp = new (objects.allocate_instance()) ship1{};
    shp->x = x;
    shp->y = y;
    shp->dy = 50;
  }
}

static void main_wave_3() {
  constexpr int count = display_width / (sprite_width * 3 / 2);
  constexpr int dx = display_width / count;
  // printf("wave3: count=%d  dx=%d\n", count, dx);
  float y = -sprite_height;
  for (int j = 0; j < count; j++, y -= 24) {
    float x = 0;
    for (int i = 0; i < count; i++, x += dx) {
      ship1 *shp = new (objects.allocate_instance()) ship1{};
      shp->x = x;
      shp->y = y;
      shp->dy = 50;
    }
  }
}

static void main_wave_4() {
  ufo2 *ufo = new (objects.allocate_instance()) ufo2{};
  ufo->x = display_width / 2;
  ufo->y = -sprite_height;
  ufo->dy = 5;

  {
    ship2 *shp = new (objects.allocate_instance()) ship2{};
    shp->x = -sprite_width;
    shp->y = -sprite_height;
    shp->dy = 25;
    shp->dx = 12;
    shp->ddy = 20;
    shp->ddx = 10;
  }
  {
    ship2 *shp = new (objects.allocate_instance()) ship2{};
    shp->x = display_width;
    shp->y = -sprite_height;
    shp->dy = 25;
    shp->dx = -12;
    shp->ddy = 20;
    shp->ddx = -10;
  }
}

// static void main_wave_5() {
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