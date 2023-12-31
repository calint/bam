#pragma once
// platform constants used by engine and game

// reviewed: 2023-12-11

#include "game/defs.hpp"

// display dimensions of screen ILI9341 depending on orientation
static constexpr int display_width = display_orientation == 0 ? 240 : 320;
static constexpr int display_height = display_orientation == 0 ? 320 : 240;

// calibration of touch screen
static constexpr int16_t touch_screen_min_x = 500;
static constexpr int16_t touch_screen_max_x = 3700;
static constexpr int16_t touch_screen_range_x =
    touch_screen_max_x - touch_screen_min_x;
static constexpr int16_t touch_screen_min_y = 400;
static constexpr int16_t touch_screen_max_y = 3700;
static constexpr int16_t touch_screen_range_y =
    touch_screen_max_y - touch_screen_min_y;
