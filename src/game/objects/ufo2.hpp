#pragma once
// first include engine
#include "../../engine.hpp"
// then objects
#include "game_object.hpp"
#include "ship2.hpp"
#include "utils.hpp"

class ufo2 final : public game_object {
  sprites_2x2 sprs;

public:
  ufo2() : game_object{ufo_cls}, sprs{this, 10} {
    col_bits = cb_hero;
    col_mask = cb_enemy | cb_enemy_bullet;

    health = 100;
  }

  void pre_render() override { sprs.pre_render(this); }

  auto on_collision(game_object *obj) -> bool override {
    ship2 *shp = new (objects.allocate_instance()) ship2{};
    shp->x = obj->x;
    shp->y = obj->y - sprite_height;
    shp->dx = float(random(-100, 100));
    shp->ddx = -shp->dx * 0.25f;
    shp->dy = -100;
    shp->ddy = 100;

    return game_object::on_collision(obj);
  }

  void on_death_by_collision() override {
    create_fragments(x, y, 32, 150, 1000);
  }
};
