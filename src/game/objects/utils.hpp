#pragma once
// first include engine
#include "../../engine.hpp"
// then objects
#include "fragment.hpp"

class sprites_2x2 final {
  // three additional sprites
  sprite *spr[3];

public:
  sprites_2x2(game_object *obj, int top_left_index_in_16_sprites_row) {
    obj->spr = sprites.allocate_instance();
    obj->spr->obj = obj;
    // additional 3 sprites
    for (int i = 0; i < 3; i++) {
      spr[i] = sprites.allocate_instance();
      spr[i]->obj = obj;
    }
    obj->spr->img = sprite_imgs[top_left_index_in_16_sprites_row];
    spr[0]->img = sprite_imgs[top_left_index_in_16_sprites_row + 1];
    spr[1]->img = sprite_imgs[top_left_index_in_16_sprites_row + 16];
    spr[2]->img = sprite_imgs[top_left_index_in_16_sprites_row + 16 + 1];
  }

  ~sprites_2x2() {
    for (int i = 0; i < 3; i++) {
      spr[i]->img = nullptr;
      sprites.free_instance(spr[i]);
    }
  }

  void pre_render(game_object *obj) {
    obj->spr->scr_x = int16_t(obj->x - sprite_width);
    obj->spr->scr_y = int16_t(obj->y - sprite_height);
    spr[0]->scr_x = int16_t(obj->x);
    spr[0]->scr_y = int16_t(obj->y - sprite_height);
    spr[1]->scr_x = int16_t(obj->x - sprite_width);
    spr[1]->scr_y = int16_t(obj->y);
    spr[2]->scr_x = int16_t(obj->x);
    spr[2]->scr_y = int16_t(obj->y);
  }
};

void create_fragments(float orig_x, float orig_y, int frag_count,
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