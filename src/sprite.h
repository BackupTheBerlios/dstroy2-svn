#ifndef _SPRITE_H_
#define _SPRITE_H_

/* SpriteType */
typedef enum sprite_type_T {
    SpriteBuddy = 0, SpriteMonster, SpriteBomb
} SpriteType;

/* SpriteState */
typedef enum sprite_state_t {
    SpriteNormal = 0, SpriteInvisible, SpriteInvincible, SpriteBuddyMonster,
    SpriteBipBip, SpriteSlow
} SpriteState;

/* Sprite */
typedef struct sprite_t
{
    SpriteType type;

    /* Position, Geometry */
    float x;
    float y;
    int direction;            /* up = 0, down, left, right */
    int tile;                 /* tile number */
    float frame;              /* current frame: 0 .. numframes */
    int top;
    int bottom;
    int left;
    int right;
    int moving;               /* is the sprite moving? */

    /* Properties */
    int id;                   /* < 0 : monster, >= 0 : buddy */
    SpriteState state;        /* state (depends on bonus) */
    float timer;              /* timer for temporary bonus */
    struct sprite_t *next;
} Sprite;

/* Constructor / Destructor */
Sprite* sprite_new( SpriteType type );
void sprite_delete( Sprite* sprite );

/* Methods */
void sprite_move( Sprite* sprite, float dt, int dx, int dy );
void sprite_stop( Sprite* sprite );
int sprite_animate( Sprite* sprite, float dt );
char sprite_get_tile( Sprite* sprite );

#endif

