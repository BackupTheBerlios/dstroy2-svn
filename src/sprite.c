#include "sprite.h"

#include <math.h>

#include "dstroy.h"
#include "video.h"

Sprite* sprite_new( SpriteType type )
{
    Sprite* sprite = (Sprite*) malloc( sizeof( Sprite ) );
    sprite->moving = 0;
    sprite->frame = 0.;
    sprite->next = NULL;
    sprite->direction = 0;
    sprite->type = type;

    /* Tile */
    switch( type ) {
        case SpriteBuddy:  /* TODO: except for BuddyMonster */
            sprite->tile = '8';
            break;
        case SpriteBomb:
            sprite->tile = '0' + 40;
            break;
        default:
            sprite->tile = '0';
    }

    return sprite;
}

void sprite_delete( Sprite* sprite )
{
    if ( sprite )
        free( sprite );
}

void sprite_move( Sprite* sprite, float dt, int dx, int dy )
{
    int ndir;

    if ( !dx && !dy ) {
        sprite_stop( sprite );
        return;
    }

    /* New direction */
    if ( dy > 0 )
        ndir = 0;
    else if ( dy < 0 )
        ndir = 1;
    else if ( dx > 0 )
        ndir = 2;
    else
        ndir = 3;

    /* New direction, start with the first frame */
    if ( ndir != sprite->direction ) {
        sprite->frame = 0.;
        sprite->direction = ndir;
    }

    sprite->frame += dt * abs( dx + dy ) * BUDDY_FRAME_RATE;
    if ( sprite->frame >= 8. )
        sprite->frame = 0.;

    sprite->x += dx * dt * BUDDY_SPEED;
    sprite->y += dy * dt * BUDDY_SPEED;
    sprite->moving = 1;
}

void sprite_stop( Sprite* sprite )
{
    sprite->frame = 0.;
    sprite->moving = 0;
}

int sprite_animate( Sprite* sprite, float dt )
{
    sprite->frame += dt * (float) BOMB_FRAME_RATE;
    return sprite->frame < 11.;
}

char sprite_get_tile( Sprite* sprite )
{
    return sprite->tile + sprite->direction * 8 + (char) sprite->frame;
}

