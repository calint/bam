#!/bin/bash
set -e
cd $(dirname "$0")

./read-palette.py sprites.png > ../resources/palette_sprites.hpp
./read-sprites.py sprites.png > ../resources/sprite_imgs.hpp

./read-palette.py tiles.png > ../resources/palette_tiles.hpp
./read-sprites.py tiles.png > ../resources/tiles.hpp