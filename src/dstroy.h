#ifndef _DSTROY_H_
#define _DSTROY_H_

#define FOREGROUND_VEL_X    77.0
#define FOREGROUND_VEL_Y    77.0
#define BACKGROUND_VEL      10.0

/* Screen size (unscaled) */
#define SCREEN_W        640
#define SCREEN_H        480
#define FPS             60

/* Tile size */
#define TILE_W          32
#define TILE_H          32

/* Tile palette size (pixels) */
#define PALETTE_W       256
#define PALETTE_H       256

/* Tile palette size (tiles) */
#define PALETTE_TW      (PALETTE_W / TILE_W)
#define PALETTE_TH      (PALETTE_H / TILE_H)

/* Map size (tiles) */
#define MAP_W           32
#define MAP_H           32

/* Sprite */
#define BUDDY_FRAME_RATE  15
#define BUDDY_SPEED       85
#define BOMB_FRAME_RATE    3

#endif

