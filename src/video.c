#include "video.h"

#include <stdlib.h>
#include <SDL/SDL_opengl.h>

#include "dstroy.h"

void set_clip_rect_gl( SDL_Rect* rect );
static int intersect_rect( const SDL_Rect *A, const SDL_Rect *B,
                           SDL_Rect *intersection );


/* -----------------
         SDL
 ------------------*/

SDL_Surface *screen = NULL;
SDL_Surface *tiles = NULL;
SDL_Surface *opaque_tiles = NULL;

void init_sdl( int w, int h, int bpp, Uint32 flags )
{
    screen = SDL_SetVideoMode( w, h, bpp, flags );
    if ( !screen ) {
        fprintf( stderr, "Failed to open screen!\n" );
        exit( -1 );
    }
}

void set_tiles_sdl( const char* path )
{
    SDL_Surface *tiles_bmp;

    tiles_bmp = SDL_LoadBMP( path );
    if( !tiles_bmp ) {
        fprintf( stderr, "Could not load graphics!\n" );
        exit( -1 );
    }
    tiles = SDL_DisplayFormat( tiles_bmp );
    opaque_tiles = SDL_DisplayFormat( tiles_bmp );
    SDL_FreeSurface( tiles_bmp );

    /* Set colorkey for non-opaque tiles to bright magenta */
    SDL_SetColorKey( tiles,
        SDL_SRCCOLORKEY | SDL_RLEACCEL,
        SDL_MapRGB( tiles->format, 255, 0, 255 ) );

    if ( 0 )  /*alpha*/
        SDL_SetAlpha( tiles, SDL_SRCALPHA | SDL_RLEACCEL, 0/*alpha*/ );
}

void flip_sdl()
{
    SDL_Flip( screen );
}

void video_set_fullscreen( int fullscreen )
{
    if ( fullscreen ) {
        screen = SDL_SetVideoMode( SCREEN_W, SCREEN_H, 0/*TODO:depth*/,
            screen->flags | SDL_FULLSCREEN );
    }
    else {
        screen = SDL_SetVideoMode( SCREEN_W, SCREEN_H, 0/*TODO:depth*/,
            screen->flags ^ SDL_FULLSCREEN );
    }
}

SDL_Rect get_clip_rect_sdl()
{
    return screen->clip_rect;
}

void set_clip_rect_sdl( SDL_Rect* rect )
{
    SDL_SetClipRect( screen, rect );
}

void close_sdl()
{
    SDL_FreeSurface(tiles);
    SDL_Quit();
}

void draw_tile_sdl( float x, float y, char tile, int opaque )
{
    SDL_Rect source_rect, dest_rect;

    /* Study the following expression. Typo trap! :-) */
    if ( ' ' == tile )
        return;

    source_rect.x = ( ( tile - '0' ) % PALETTE_TW ) * TILE_W;
    source_rect.y = ( ( tile - '0' ) / PALETTE_TW ) * TILE_H;
    source_rect.w = TILE_W;
    source_rect.h = TILE_H;

    dest_rect.x = (int) (x);
    dest_rect.y = (int) (y);

    SDL_BlitSurface( opaque ? opaque_tiles : tiles, &source_rect, screen, &dest_rect );
}


/* -----------------
      Open GL
 ------------------*/

GLint gl_tiles, gl_tiles_mask;
int vscreen_w, vscreen_h;
SDL_Rect clip_rect;

/* OpenGL "state optimizer" hack from glSDL */
static struct
{
    int do_blend;
    int do_texture;
    GLint texture;
    GLenum sfactor, dfactor;
} glstate;

static __inline__ void gl_do_blend(int on)
{
        if(glstate.do_blend == on)
                return;

        if(on)
                glEnable(GL_BLEND);
        else
                glDisable(GL_BLEND);
        glstate.do_blend = on;
}

static __inline__ void gl_do_texture(int on)
{
    if(glstate.do_texture == on)
        return;

    if(on)
        glEnable(GL_TEXTURE_2D);
    else
        glDisable(GL_TEXTURE_2D);
    glstate.do_texture = on;
}

static __inline__ void gl_texture( GLuint tx )
{
    if ( tx == glstate.texture )
        return;

    glBindTexture( GL_TEXTURE_2D, tx );
    glstate.texture = tx;
}

static void gl_reset(void)
{
        glstate.do_blend = -1;
        glstate.do_blend = -1;
        glstate.texture = -1;
        glstate.sfactor = 0xffffffff;
        glstate.dfactor = 0xffffffff;
}

void init_gl( int w, int h, int bpp, Uint32 flags )
{
    GLint gl_doublebuf;
    GLint maxtexsize;

    flags |= SDL_OPENGL;

    gl_doublebuf = flags & SDL_DOUBLEBUF;
    if ( bpp == 15 ) {
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    }
    else if ( bpp == 16 ) {
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    }
    else if ( bpp >= 24 ) {
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    }

    if ( bpp )
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, bpp );

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, gl_doublebuf );

    /*TODO: use scale*/
    vscreen_w = w * 1.;
    vscreen_h = h * 1.;

    screen = SDL_SetVideoMode( w, h, bpp, flags );
    set_clip_rect_gl( NULL );  // HELP
    if ( !screen ) {
        fprintf(stderr, "Failed to open screen!\n");
        return;
    }

    /*
     * Just because my driver f*cks up if there's console
     * output when it's messing with textures... :-(
     */
    SDL_Delay( 1000 );

    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxtexsize );
    if ( maxtexsize < 256) {
        fprintf(stderr, "Need at least 256x256 textures!\n");
        SDL_Quit();
        return;
    }

    /*
     * Set up OpenGL for 2D rendering.
     */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glViewport(0, 0, screen->w, screen->h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, vscreen_w, vscreen_h, 0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, 0.0f);

    gl_reset();
}

static SDL_Surface * create_tiles_surface( const char *path, int id, GLint *pi )
{
    SDL_Surface *tmp, *tmp2;

    tmp = SDL_LoadBMP( path );
    if ( !tmp ) {
        fprintf( stderr, "Could not load graphics!\n" );
        SDL_Quit();
        return NULL;
    }

    tmp2 = SDL_CreateRGBSurface( SDL_SWSURFACE, 256, 256, 24,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        0x00ff0000, 0x0000ff00, 0x000000ff, 0
#else
        0x000000ff, 0x0000ff00, 0x00ff0000, 0
#endif
    );

    SDL_BlitSurface( tmp, NULL, tmp2, NULL );
    SDL_FreeSurface( tmp );

    glGenTextures( id, pi );
    glBindTexture( GL_TEXTURE_2D, *pi );
    glPixelStorei( GL_UNPACK_ROW_LENGTH, tmp2->pitch /
                   tmp2->format->BytesPerPixel );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, tmp2->w, tmp2->h, 0, GL_RGB,
                  GL_UNSIGNED_BYTE, tmp2->pixels );

    return tmp2;
}

void set_tiles_gl( const char* path )
{
    SDL_Surface *s1, *s2;

    s1 = create_tiles_surface( "gfx/tiles.bmp", 1, &gl_tiles );
    if ( !s1 )
        exit( -1 );
    /*s2 = create_tiles_surface( "gfx/tiles_mask.bmp", 2, &gl_tiles_mask );
    if ( !s2 )
        exit( -1 );*/

    glFlush();
    SDL_FreeSurface( s1 );
    /*SDL_FreeSurface( s2 );*/
}

void flip_gl()
{
    SDL_GL_SwapBuffers();
}

void video_zoom( float factor )
{
    vscreen_w = (int) ( vscreen_w * factor );
    vscreen_h = (int) ( vscreen_h * factor );
    glViewport(0, 0, screen->w, screen->h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, vscreen_w, vscreen_h, 0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, 0.0f);

    gl_reset();
}

SDL_Rect get_clip_rect_gl()
{
    return clip_rect;
}

void set_clip_rect_gl( SDL_Rect* rect )
{
    SDL_Rect full_rect;

    full_rect.x = 0;
    full_rect.y = 0;
    full_rect.w = SCREEN_W;
    full_rect.h = SCREEN_H;

    if ( !rect )
        clip_rect = full_rect;
    else
        intersect_rect( &full_rect, rect, &clip_rect );
}

void close_gl()
{
    glDeleteTextures( 1, &gl_tiles );
    SDL_Quit();
}

void draw_tile_gl( float x, float y, char tile, int opaque )
{
    SDL_Rect rect;
    float tx1, ty1, tx2, ty2, x1, y1, x2, y2;

    /* Study the following expression. Typo trap! :-) */
    if ( ' ' == tile )
        return;

    /* Calc the destination rect (intersection with clip_rect) */
    rect.x = x;
    rect.y = y;
    rect.w = TILE_W;
    rect.h = TILE_H;
    intersect_rect( &rect, &clip_rect, &rect );

    if ( rect.w <= 0 || rect.h <= 0 )  /* mandatory? */
        return;

    tx1 = (float)( ( tile - '0' ) % PALETTE_TW ) / PALETTE_TW;
    tx1 += (float) ( rect.x - x ) / PALETTE_W + 0.5 / PALETTE_W;
    ty1 = (float)( ( tile - '0' ) / PALETTE_TH ) / PALETTE_TH;
    ty1 += (float) ( rect.y - y ) / PALETTE_H + 0.5 / PALETTE_W;
    tx2 = tx1 + ( (float) rect.w / PALETTE_W ) - 1. / PALETTE_W;
    ty2 = ty1 + ( (float) rect.h / PALETTE_H ) - 1. / PALETTE_W;
    x1 = (float) rect.x;
    y1 = (float) rect.y;
    x2 = x1 + (float) rect.w;
    y2 = y1 + (float) rect.h;

    gl_do_texture( 1 );
    gl_do_blend( 0 );
    gl_texture( gl_tiles );

    glBegin( GL_QUADS );
    glColor4ub( 255, 255, 255, 255 );
    glTexCoord2f( tx1, ty1 ); glVertex2f( x1, y1 );
    glTexCoord2f( tx2, ty1 ); glVertex2f( x2, y1 );
    glTexCoord2f( tx2, ty2 ); glVertex2f( x2, y2 );
    glTexCoord2f( tx1, ty2 ); glVertex2f( x1, y2 );
    glEnd();

#if 0
    glEnable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );

    if ( 1 )  /* masking */
        glBlendFunc( GL_DST_COLOR,GL_ZERO );

    if ( 1 )  /* scene */
        glTranslatef
#endif
}


/* -----------------
     Common Stuff
 ------------------*/

static int intersect_rect( const SDL_Rect *A, const SDL_Rect *B,
                           SDL_Rect *intersection )
{
    int Amin, Amax, Bmin, Bmax;

    /* Horizontal intersection */
    Amin = A->x;
    Amax = Amin + A->w;
    Bmin = B->x;
    Bmax = Bmin + B->w;
    if ( Bmin > Amin )
         Amin = Bmin;
    intersection->x = Amin;
    if ( Bmax < Amax )
        Amax = Bmax;
    intersection->w = Amax - Amin > 0 ? Amax - Amin : 0;

    /* Vertical intersection */
    Amin = A->y;
    Amax = Amin + A->h;
    Bmin = B->y;
    Bmax = Bmin + B->h;
    if ( Bmin > Amin )
        Amin = Bmin;
    intersection->y = Amin;
    if ( Bmax < Amax )
        Amax = Bmax;
    intersection->h = Amax - Amin > 0 ? Amax - Amin : 0;

    return ( intersection->w && intersection->h );
}

void video_init( int w, int h, int bpp, Uint32 flags, int use_gl )
{
    flags |= SDL_DOUBLEBUF;

    if ( use_gl ) {
        /* Mode GL */
        init_gl( w, h, bpp, flags );
        video_set_tiles = set_tiles_gl;
        video_draw_tile = draw_tile_gl;
        video_flip = flip_gl;
        video_get_clip_rect = get_clip_rect_gl;
        video_set_clip_rect = set_clip_rect_gl;
        video_close = close_gl;
    }
    else {
        /* Mode SDL */
        init_sdl( w, h, bpp, flags );
        video_set_tiles = set_tiles_sdl;
        video_draw_tile = draw_tile_sdl;
        video_flip = flip_sdl;
        video_get_clip_rect = get_clip_rect_sdl;
        video_set_clip_rect = set_clip_rect_sdl;
        video_close = close_sdl;
    }
}

SDL_Surface* video_get_screen()
{
    return screen;
}

