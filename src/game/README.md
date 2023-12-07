# game code

intention:
* developing a toy game using platform independent engine

table of contents (in include order by program file):
* `main.hpp` setup initial game state, callbacks from engine, game logic
* `objects/*` game objects
* `game_state.hpp` game state used by objects
* `resources/*` partial files defining tiles, sprites, palettes and tile map
* `defs.hpp` constants used by engine and game objects

appendix:
* `png-to-resources/extract.sh` tool for extracting resources from png files

# overview

## main.hpp
### function `main_setup`
* initiates the game by creating initial objects and sets tile map position and velocity
### function `main_on_touch_screen`
* handles user interaction with touch screen
### function `main_on_frame_completed`
* implements game logic

## objects/*
* see `objects/README.md`

## game_state.hpp
* included by objects that access game state
* included by `main.hpp` after the objects
* provides a way for game objects to share information with `main.hpp`
* used in `main_on_frame_completed` to solve circular reference problems

## resources/*
* `tile_map.hpp` size defined in `defs.hpp`
* `tiles.hpp`, `sprite_imgs.hpp` and `palette_*.hpp` generated from png files by tool `png-to-resources/extract.sh`
* 256 tile and 256 sprite images are default settings in `defs.hpp`
* sprite and tile images are constant data stored in program memory
* separate palettes for tiles and sprites

## defs.hpp
### `enum object_class`
* each game object class has an entry named with suffix `_cls`
### `collision_bits`
* named bits with constants used by objects to define collision bits and mask
### `object_instance_max_size_B`
* maximum size of any game object instance
* set to 256B but should be maximum game object instance size rounded upwards to nearest power of 2 number

## limitations
* due to target device not being able to allocate large (>150KB) chunks of contiguous memory some limitations are imposed
* concurrent sprites limited to 255 due to collision map having to be 8-bit to fit the screen pixels in a contiguous block of memory
* concurrent objects limited to 255 being a natural sizing considering sprites
* limits defined in `defs.hpp`
