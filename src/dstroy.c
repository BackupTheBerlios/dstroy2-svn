#include "dstroy.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>

#include "layer.h"
#include "maps.h"
#include "sprite.h"
#include "video.h"

static void xy_to_ab( float x, float y, int* a, int *b )
{
    *a = (int) ( x / TILE_W );
    *b = (int) ( y / TILE_H );
}

static void ab_to_xy( int a, int b, float *x, float *y )
{
    *x = (float) a * TILE_W;
    *y = (float) b * TILE_H;
}

int main( int argc, char* argv[] )
{
    SDL_Rect border;
    Layer **layers;
    SDL_Event event;
    int bpp = 0,
        flags = 0,
        use_gl = 0,
        use_planets = 1,
        num_of_layers = 2,
        bounce_around = 1,
        wrap = 0,
        alpha = 0,
        fullscreen = 0;
    int i;
    long tick1, tick2;
    float dt;
    Sprite *current_sprite, *buddy, *bomb = NULL;
    int allow_bomb = 1;

    /* Command line options */
    for(i = 1; i < argc; ++i) {
        if ( strncmp( argv[i], "-sdl", 4 ) == 0 )
            use_gl = 0;
        else if ( strncmp( argv[i], "-gl", 3 ) == 0 )
            use_gl = 1;
    }

    /* Init video and set tiles */
    SDL_Init( SDL_INIT_VIDEO );
    atexit( SDL_Quit );
    video_init( SCREEN_W, SCREEN_H, bpp, flags, use_gl );
    video_set_tiles( "gfx/tiles.bmp" );
    border = video_get_clip_rect();
    SDL_WM_SetCaption( "DStroy 2 - Move your Boody, the Meuries strike back!",
        "DStroy" );
    SDL_ShowCursor( 0 );

    /* Layer tab */
    layers = malloc( sizeof( Layer* ) * num_of_layers );

    /* Foreground */
    layers[0] = layer_new( &foreground_map );

    /* Middle */
    if ( num_of_layers >= 3 ) {
        for ( i = 1; i < num_of_layers - 1; ++i )
            layers[i] = layer_new( &middle_map );
    }

    /* Background */
    if ( num_of_layers >= 2 )
        layers[num_of_layers - 1] = layer_new( &background_map );

    /* Set up the depth order for the recursive rendering algorithm */
    for ( i = 0; i < num_of_layers - 1; ++i )
        layer_set_next( layers[i], layers[i+1] );

#if 0
    if( bounce_around && ( num_of_layers > 1 ) ) {
        /* ??? */
        for( i = 0; i < num_of_layers - 1; ++i ) {
            float a = 1.0 + i * 2.0 * 3.1415 / num_of_layers;
            float v = 200.0 / (i+1);
            layer_set_velocity( layers[i], v * cos(a), v * sin(a) );
            if( !wrap )
                layer_limit_bounce( layers[i] );
        }
    }
    else {
#endif
        /* Link all intermediate levels to the foreground layer */
        for ( i = 1; i < num_of_layers; ++i )
            layer_set_link( layers[i], layers[0], 1.2 / (float)(i+1) );
    /*}*/

    /* Create buddy and add it to layer 0 */
    buddy = sprite_new( SpriteBuddy );
    layer_add_sprite( layers[0], buddy );
    buddy->x = 300;
    buddy->y = 200;
    sprite_stop( buddy );

    /* Get initial tick for time calculation */
    tick1 = SDL_GetTicks();

    while ( SDL_PollEvent( &event ) >= 0 ) {
        int dx = 0, dy = 0;

        if ( event.type & ( SDL_KEYUP | SDL_KEYDOWN ) ) {
            Uint8 *keys = SDL_GetKeyState( &i );

            /* Exit */
            if ( keys[SDLK_ESCAPE] || keys[SDLK_q] )
                break;
            if ( keys[SDLK_LALT] && keys[SDLK_F4] )
                break;

            /* Up, Down, Left, Right */
            if ( keys[SDLK_UP] )
                dy -= 2;
            else if ( keys[SDLK_DOWN] )
                dy += 2;
            else if ( keys[SDLK_LEFT] )
                dx -= 2;
            else if ( keys[SDLK_RIGHT] )
                dx += 2;

            /* Shift = faster, Alt = slower */
            if ( keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT] ) {
                dx *= 2;
                dy *= 2;
            }
            if ( keys[SDLK_LALT] ) {
                dx = (int)( dx * .5 );
                dy = (int)( dy * .5 );
            }

            /* Fire */
            if ( keys[SDLK_RCTRL] ) {
                if ( allow_bomb ) {
                    int a, b;

                    /* Create bomb and add it to layer 0 */
                    bomb = sprite_new( SpriteBomb );
                    layer_add_sprite( layers[0], bomb );
                    xy_to_ab( buddy->x + TILE_W / 2., buddy->y + TILE_H / 2.,
                              &a, &b );
                    ab_to_xy( a, b, &bomb->x, &bomb->y );
                    sprite_stop( bomb );
                    allow_bomb = 0;
                }
            }
            else
                allow_bomb = 1;

            /* Toggle fullscreen */
            if ( keys[SDLK_f] ) {
                fullscreen = !fullscreen;
                video_set_fullscreen( fullscreen );
            }

            /* Zoom */
            if ( keys[SDLK_PLUS] )
                video_zoom( 0.99 );
            else if ( keys[SDLK_MINUS] )
                video_zoom( 1.01 );
        }

        /* Calculate time since last update */
        tick2 = SDL_GetTicks();
        dt = ( tick2 - tick1 ) * 0.001f;
        tick1 = tick2;

        /* Animate bombs */
        current_sprite = layers[0]->first_sprite;
        while ( current_sprite ) {
            Sprite* next = current_sprite->next;
            if ( current_sprite->type == SpriteBomb ) {
                if ( !sprite_animate( current_sprite, dt ) )
                    layer_remove_sprite( layers[0], current_sprite );
            }
            current_sprite = next;
        }

        /* Move your buddy */
        if ( dx || dy )
            sprite_move( buddy, dt, dx, dy );
        else if ( buddy->moving )
            sprite_stop( buddy );

        /* Center layer 0 on buddy */
        layer_center( layers[0], buddy->x, buddy->y );

        /* Animate all layers */
        /*for ( i = 0; i < num_of_layers; ++i )
            layer_animate( layers[i], dt, dx, dy );*/

        /* Update position of linked layers */
        for ( i = 0; i < num_of_layers; ++i ) {
            Layer *l = layers[i]->link;
            if ( l && ( layers[i]->flags & TL_LINKED ) ) {
                layer_set_pos( layers[i], l->pos_x * layers[i]->ratio,
                    l->pos_y * layers[i]->ratio );
            }
        }

        /* Render all layers (recursive!) */
        layer_render( layers[0], video_get_screen(), &border );

        /* Make changes visible */
        video_flip();

        /* Let operating system breath */
        /* TODO: Use a frame wait with a limit of 60 fps as it does not make
                 sense to use too much CPU resource */
#if 0
        SDL_Delay( 1000. / ( FPS - dt ) );
        printf( "Current FPS: %f\n", 1./dt );
        SDL_Delay( 1./dt - FPS );
#endif
        SDL_Delay( 1 );
    }

    /* Delete each layer + whole tab */
    for ( i = 0; i < num_of_layers; i++ )
        layer_delete( layers[i] );
    free( layers );

    video_close();
}

