#include "layer.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sprite.h"
#include "video.h"

#define TM_EMPTY    0
#define TM_KEYED    1
#define TM_OPAQUE   2

/* Checks if tile is opaqe, empty or color keyed */
int tile_mode( char tile )
{
    switch ( tile ) {
        case '0':
            return TM_OPAQUE;
        case '1':
            return TM_KEYED;
        case '2':
        case '3':
            return TM_OPAQUE;
        case '4':
            return TM_KEYED;
    }
    return TM_EMPTY;
}

/* Initialivze layer; set up map and tile graphics data. */
Layer* layer_new( map_data_t *map )
{
    Layer* lr = malloc( sizeof( Layer ) );
    lr->next = NULL;
    lr->pos_x = lr->pos_y = 0.0;
    lr->vel_x = lr->vel_y = 0.0;
    lr->map = map;
    lr->link = NULL;
    lr->first_sprite = NULL;
    return lr;
}

void layer_delete( Layer *lr )
{
    if ( lr )
        free( lr );
}

/* Tell a layer which layer is next, or under this layer */
void layer_set_next( Layer *lr, Layer *next_lr )
{
    lr->next = next_lr;
}

/* Set position */
void layer_set_pos( Layer *lr, float x, float y )
{
    lr->pos_x = x;
    lr->pos_y = y;
}

/* Set velocity */
void layer_set_velocity( Layer *lr, float x, float y )
{
    lr->vel_x = x;
    lr->vel_y = y;
}

static void do_limit_bounce( Layer *lr )
{
    int dx = 0, dy = 0;
    int maxx = MAP_W * TILE_W - SCREEN_W;
    int maxy = MAP_H * TILE_H - SCREEN_H;

    if ( lr->pos_x > maxx ) {
        /* v.out = - v.in */
        /*lr->vel_x = -lr->vel_x;*/
        /*
         * Mirror over right limit. We need to do this
         * to be totally accurate, as we're in a time
         * discreet system! Ain't that obvious...? ;-)
         */
        lr->pos_x = maxx;
    }
    else if ( lr->pos_x < 0 ) {

        /* Basic physics again... */
        /*lr->vel_x = -lr->vel_x;*/
        /* Mirror over left limit */
        lr->pos_x = 0;
    }

    if ( lr->pos_y > maxy ) {
        /*lr->vel_y = -lr->vel_y;*/
        lr->pos_y = maxy;
    }
    else if ( lr->pos_y < 0 ) {
        /*lr->vel_y = -lr->vel_y;*/
        lr->pos_y = 0;
    }
}

/* Update animation (apply the velocity, that is) */
void layer_animate( Layer *lr, float dt, int x, int y )
{
    lr->pos_x += x * dt * lr->vel_x;
    lr->pos_y += y * dt * lr->vel_y;
}

/* Link the position of this layer to another layer, w/ scale ratio */
void layer_set_link( Layer *lr, Layer *to_lr, float ratio )
{
    lr->flags |= TL_LINKED;
    lr->link = to_lr;
    lr->ratio = ratio;
}

void layer_center( Layer *lr, float x, float y )
{
    lr->pos_x = x + TILE_W / 2. - SCREEN_W / 2.;
    lr->pos_y = y + TILE_H / 2. - SCREEN_H / 2.;
    do_limit_bounce( lr );
}

/*
 * Render layer to the specified surface,
 * clipping to the specified rectangle
 *
 * This version is slightly improved over the 
 * one in "Parallax 3"; it combines horizontal
 * runs of transparent and partially transparent
 * tiles before recursing down.
 */
void layer_render( Layer *lr, SDL_Surface *screen, SDL_Rect *rect )
{
    int max_x, max_y;
    int map_pos_x, map_pos_y;
    int mx, my, mx_start;
    int fine_x, fine_y;
    SDL_Rect pos;
    SDL_Rect local_clip;
    Sprite* sprite;

    /*
     * Set up clipping
     * (Note that we must first clip "rect" to the
     * current cliprect of the screen - or we'll screw
     * clipping up as soon as we have more than two
     * layers!)
     */
    if ( rect ) {
        pos = video_get_clip_rect();
        local_clip = *rect;
        /* Convert to (x2,y2) */
        pos.w += pos.x;
        pos.h += pos.y;
        local_clip.w += local_clip.x;
        local_clip.h += local_clip.y;
        if ( local_clip.x < pos.x )
            local_clip.x = pos.x;
        if ( local_clip.y < pos.y )
            local_clip.y = pos.y;
        if (local_clip.w > pos.w)
            local_clip.w = pos.w;
        if (local_clip.h > pos.h)
            local_clip.h = pos.h;
        /* Convert result back to w, h */
        local_clip.w -= local_clip.x;
        local_clip.h -= local_clip.y;
        /* Check if we actually have an area left! */
        if ( (local_clip.w <= 0) || (local_clip.h <= 0) )
            return;
        /* Set the final clip rect */
        video_set_clip_rect( &local_clip );
    }
    else
    {
        video_set_clip_rect( NULL );
        local_clip = video_get_clip_rect();
    }

    /* Position of clip rect in map space */
    map_pos_x = (int)lr->pos_x + video_get_clip_rect().x;
    map_pos_y = (int)lr->pos_y + video_get_clip_rect().y;

    /* The calculations would break with negative map coords... */
    if (map_pos_x < 0)
        map_pos_x += MAP_W*TILE_W*(-map_pos_x/(MAP_W*TILE_W)+1);
    if (map_pos_y < 0)
        map_pos_y += MAP_H*TILE_H*(-map_pos_y/(MAP_H*TILE_H)+1);

    /* Position on map in tiles */
    mx = map_pos_x / TILE_W;
    my = map_pos_y / TILE_H;

    /* Fine position - pixel offset; up to (1 tile - 1 pixel) */
    fine_x = map_pos_x % TILE_W;
    fine_y = map_pos_y % TILE_H;

    /* Draw all visible tiles */
    max_x = video_get_clip_rect().x + video_get_clip_rect().w;
    max_y = video_get_clip_rect().y + video_get_clip_rect().h;
    mx_start = mx;
    pos.h = TILE_H;
    for ( pos.y = video_get_clip_rect().y - fine_y;
            pos.y < max_y; pos.y += TILE_H ) {
        mx = mx_start;
        my %= MAP_H;
        for ( pos.x = video_get_clip_rect().x - fine_x; pos.x < max_x; ) {
            char tile;
            int tm, run_w;
            mx %= MAP_W;
            tile = (*lr->map)[my][mx];
            tm = tile_mode( tile );

            /* Calculate run length
             * ('tm' will tell what kind of run it is)
             */
            run_w = 1;
            if ( 1 ) {  /* detect_runs */
                while ( pos.x + run_w * TILE_W < max_x ) {
                    int tt = (*lr->map) [my]
                            [( mx + run_w ) % MAP_W];
                    tt = tile_mode( tt );
                    if ( tt != TM_OPAQUE )
                        tt = TM_EMPTY;
                    if ( tm != tt )
                        break;
                    ++run_w;
                }
            }

            /* Recurse to next layer */
            if ( ( tm != TM_OPAQUE ) && lr->next ) {
                ++lr->recursions;
                pos.w = run_w * TILE_W;
                /* !!! Recursive call !!! !*/
                layer_render(lr->next, screen, &pos);
                video_set_clip_rect( &local_clip );
            }

            /* Render our tiles */
            pos.w = TILE_W;
            while(run_w--)
            {
                mx %= MAP_W;
                tile = (*lr->map)[my][mx];
                tm = tile_mode(tile);
                if (tm != TM_EMPTY)
                {
                    ++lr->blits;
                    video_draw_tile( pos.x, pos.y, tile, tm == TM_OPAQUE );
                }
                ++mx;
                pos.x += TILE_W;
            }
        }
        ++my;
    }

    /* Draw all sprites that are in the rect */
    for ( sprite = lr->first_sprite; sprite; sprite = sprite->next ) {
        if ( !rect ||
             sprite->x + TILE_W > lr->pos_x + rect->x &&
             sprite->x < lr->pos_x + rect->x + rect->w &&
             sprite->y + TILE_H > lr->pos_y + rect->y &&
             sprite->y < lr->pos_y + rect->y + rect->h ) {
            video_draw_tile( sprite->x - lr->pos_x,
                sprite->y - lr->pos_y,
                sprite_get_tile( sprite ), 0 );
        }
    }
}

void layer_add_sprite( Layer *lr, Sprite* sprite )
{
    if ( !lr->first_sprite ) {
        lr->first_sprite = sprite;
        return;
    }

    if ( sprite->type == SpriteBomb ) {
        /* Prepend bomb => background */
        sprite->next = lr->first_sprite;
        lr->first_sprite = sprite;
    }
    else {
        /* Append other sprites => foreground */
        Sprite* last_sprite = lr->first_sprite;
        while ( last_sprite->next )
            last_sprite = last_sprite->next;
        last_sprite->next = sprite;
    }
}

void layer_remove_sprite( Layer *lr, Sprite* sprite )
{
    if ( !sprite )
        return;

    /* Remove sprite in the list */
    if ( lr->first_sprite == sprite )
        lr->first_sprite = sprite->next;
    else {
        Sprite *current = lr->first_sprite;
        while ( current ) {
            if ( current->next == sprite )
                current->next = sprite->next;
            current = current->next;
        }
    }

    sprite_delete( sprite );
}

void layer_reset_stats(Layer *lr)
{
    lr->blits = 0;
    lr->recursions = 0;
}

