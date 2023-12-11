#pragma once
// first include engine
#include "../../engine.hpp"
// then objects
#include "fragment.hpp"
#include "game_object.hpp"
#include "utils.hpp"

class ufo2 final : public game_object {
  sprites_2x2 sprs;

public:
  ufo2() : game_object{ufo_cls}, sprs{this, 10} {
    col_bits = cb_hero;
    col_mask = cb_enemy | cb_enemy_bullet;

    health = 100;
  }

  void pre_render() override { sprs.set_sprites_positions(this); }

  void on_death_by_collision() override { create_fragments(x, y, 32, 150); }
};
