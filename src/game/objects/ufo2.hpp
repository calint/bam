#pragma once
// first include engine
#include "../../engine.hpp"
// then objects
#include "fragment.hpp"
#include "game_object.hpp"

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

  void set_sprites_positions(game_object *obj) {
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

class ufo2 final : public game_object {
  sprites_2x2 sprs;

public:
  ufo2() : game_object{ufo_cls}, sprs{this, 10} {
    col_bits = cb_hero;
    col_mask = cb_enemy | cb_enemy_bullet;

    health = 100;
  }

  void pre_render() override { sprs.set_sprites_positions(this); }

  void on_death_by_collision() override { create_fragments(); }

private:
  static constexpr float frag_speed = 150;
  static constexpr int frag_count = 32;

  void create_fragments() {
    for (int i = 0; i < frag_count; i++) {
      fragment *frg = new (objects.allocate_instance()) fragment{};
      frg->die_at_ms = clk.ms + 500;
      frg->x = x;
      frg->y = y;
      frg->dx = float(random(-frag_speed, frag_speed));
      frg->dy = float(random(-frag_speed, frag_speed));
      frg->ddx = 2 * float(random(-frag_speed, frag_speed));
      frg->ddy = 2 * float(random(-frag_speed, frag_speed));
    }
  }
};
