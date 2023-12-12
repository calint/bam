#pragma once
// first include engine
#include "../../engine.hpp"
// then objects
#include "fragment.hpp"

class sprites_2x2 final {
  // three additional sprites
  sprite *sprs[3];

public:
  sprites_2x2(game_object *obj, int top_left_index_in_16_sprites_row) {
    obj->spr = sprites.allocate_instance();
    obj->spr->obj = obj;
    // additional 3 sprites
    for (int i = 0; i < 3; i++) {
      sprs[i] = sprites.allocate_instance();
      sprs[i]->obj = obj;
    }
    obj->spr->img = sprite_imgs[top_left_index_in_16_sprites_row];
    sprs[0]->img = sprite_imgs[top_left_index_in_16_sprites_row + 1];
    sprs[1]->img = sprite_imgs[top_left_index_in_16_sprites_row + 16];
    sprs[2]->img = sprite_imgs[top_left_index_in_16_sprites_row + 16 + 1];
  }

  ~sprites_2x2() {
    for (int i = 0; i < 3; i++) {
      sprs[i]->img = nullptr;
      sprites.free_instance(sprs[i]);
    }
  }

  void pre_render(game_object *obj) {
    obj->spr->scr_x = int16_t(obj->x - sprite_width);
    obj->spr->scr_y = int16_t(obj->y - sprite_height);
    sprs[0]->scr_x = int16_t(obj->x);
    sprs[0]->scr_y = int16_t(obj->y - sprite_height);
    sprs[1]->scr_x = int16_t(obj->x - sprite_width);
    sprs[1]->scr_y = int16_t(obj->y);
    sprs[2]->scr_x = int16_t(obj->x);
    sprs[2]->scr_y = int16_t(obj->y);
  }
};

static void create_fragments(float orig_x, float orig_y, int frag_count,
                             float frag_speed, int life_time_ms) {
  for (int i = 0; i < frag_count; i++) {
    fragment *frg = new (objects.allocate_instance()) fragment{};
    frg->die_at_ms = clk.ms + life_time_ms;
    frg->x = orig_x;
    frg->y = orig_y;
    frg->dx = float(random(-frag_speed, frag_speed));
    frg->dy = float(random(-frag_speed, frag_speed));
    frg->ddx = 2 * float(random(-frag_speed, frag_speed));
    frg->ddy = 2 * float(random(-frag_speed, frag_speed));
  }
}

static float display_x_for_touch(int16_t x) {
  static constexpr float fact_x = float(display_width) / touch_screen_range_x;
  return float((x - touch_screen_min_x) * fact_x);
}

static float display_y_for_touch(int16_t y) {
  static constexpr float fact_y = float(display_height) / touch_screen_range_y;
  return float((y - touch_screen_min_y) * fact_y);
}