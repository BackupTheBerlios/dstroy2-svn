#ifndef _LAYER_H_
#define _LAYER_H_

#include "dstroy.h"
#include <SDL/SDL.h>

typedef char map_data_t[MAP_H][MAP_W];
struct sprite_t;

typedef struct layer_t
{
    /* Next layer in recursion order */
    struct layer_t *next;

    /* Position and speed */
    float pos_x, pos_y;
    float vel_x, vel_y;

    /* Various flags */
    int flags;

    /* Map and tile data */
    map_data_t *map;
    SDL_Surface *tiles;
    SDL_Surface *opaque_tiles;

    /* Position link */
    struct layer_t *link;
    float ratio;

    /* Statistics */
    int calls;
    int blits;
    int recursions;

    /* Sprites */
    struct sprite_t *first_sprite;
} Layer;

/* Flag definitions */
#define TL_LINKED  0x00000001

/* Constructor / Destructor */
Layer* layer_new( map_data_t *map );
void layer_delete( Layer *lr );

/* Methods */
void layer_set_next( Layer *lr, Layer *next_l );
void layer_set_pos( Layer *lr, float x, float y );
void layer_set_velocity( Layer *lr, float x, float y );
void layer_animate( Layer *lr, float dt, int dx, int dy );
void layer_center( Layer *lr, float x, float y );
void layer_add_sprite( Layer *lr, struct sprite_t* sprite );
void layer_remove_sprite( Layer *lr, struct sprite_t* sprite );
void layer_limit_bounce( Layer *lr );
void layer_set_link( Layer *lr, Layer *to_l, float ratio );
void layer_render( Layer *lr, SDL_Surface *screen, SDL_Rect *rect );
void layer_reset_stats( Layer *lr );

#endif

