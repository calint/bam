#pragma once
// platform constants used by main, engine and game

// reviewed: 2023-12-11
// reviewed: 2024-05-01

#include "game/defs.hpp"

// display dimensions depending on orientation
static constexpr int display_width =
    display_orientation == 0 ? TFT_WIDTH : TFT_HEIGHT;
static constexpr int display_height =
    display_orientation == 0 ? TFT_HEIGHT : TFT_WIDTH;

// calibration of touch screen
static constexpr int16_t touch_screen_min_x = TOUCH_SCREEN_MIN_X;
static constexpr int16_t touch_screen_max_x = TOUCH_SCREEN_MAX_X;
static constexpr int16_t touch_screen_range_x =
    touch_screen_max_x - touch_screen_min_x;
static constexpr int16_t touch_screen_min_y = TOUCH_SCREEN_MIN_Y;
static constexpr int16_t touch_screen_max_y = TOUCH_SCREEN_MAX_Y;
static constexpr int16_t touch_screen_range_y =
    touch_screen_max_y - touch_screen_min_y;
