#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <SDL/SDL.h>

void video_init( int w, int h, int bpp, Uint32 flags, int use_gl );
SDL_Surface* video_get_screen();
void video_set_fullscreen( int fullscreen );
void video_zoom( float factor );

void (*video_set_tiles) ( const char* path );
void (*video_draw_tile) ( float x, float y, char tile, int opaque );
void (*video_flip ) (void);
SDL_Rect (*video_get_clip_rect ) ();
void (*video_set_clip_rect) ( SDL_Rect* rect );
void (*video_close ) (void);

#endif

