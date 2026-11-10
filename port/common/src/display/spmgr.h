/* SPMGR.C (Code56): the sprites, shape frames drawn over the world at a
 * map point. SPRITE.C (Code43) keeps a sprite's number
 * (Sprite::handle). The camera draws them after the world: first it restores
 * the screen under their old places from gameBackground, then draws those
 * that moved or lie under a dirty rectangle, with the sprites they overlap. */
#ifndef PORT_SPMGR_H
#define PORT_SPMGR_H
#include "common.h"
#include "rect.h"
#include "basecam.h"
#include "mlist.h"
#include "viewport.h"
#include "flex.h"

constexpr int SPRITES = 64;

/* SpriteEntry::flags */
constexpr int SPRITE_DRAW = 0x1;        /* drawn in the next frame */
constexpr int SPRITE_MOVED = 0x2;       /* its old place is drawn again */
constexpr int SPRITE_REMAPPED = 0x1000; /* drawn through paletteTables->remap */

struct __attribute__((packed)) SpriteEntry {
    u16 shape; /* 0 for a free entry */
    u16 frame;
    u16 flags;
    i16 width, height; /* of the frame */
    u16 x, y;          /* the map point */
    u8 z;
    i16 originX, originY; /* the frame's offsets */
};
ASSERT_FILE_SIZE(SpriteEntry, 0x13);

/* The entries in use lie below it. */
extern u16 &spriteCount;
/* The screen without the sprites. */
extern Viewport *&gameBackground;
/* Sprite numbers SPRITE.C keeps. */
extern MList &spriteList;

void initSprites();
void closeSprites();
/* The number of a new sprite, -1 when the table is full. */
i16 createSprite(u16 shape, u16 frame, u16 flags, i16 x, i16 y, u8 z);
void freeSprite(i16 index);
u16 spriteFrame(i16 index);
void setSpriteFrame(i16 index, u16 frame);
/* Moves sprite to x,y,z. */
void showSprite(u16 sprite, i16 x, i16 y, u8 z);
void spritePosition(i16 index, u16 *x, u16 *y, u16 *z);
/* The sprite's map point on the screen. */
Point spritePoint(i16 index);
/* Draws the sprites that need it. */
void drawSprites();
/* Puts back the screen where sprites were drawn. */
void restoreBackground();
void clearSprites();
u16 spriteX(i16 index);
u16 spriteY(i16 index);
void freezeSprites();
/* Unfreezes and marks every sprite for drawing. */
void thawSprites();
/* Objects object to object+3 of a saved game. */
void loadSprites(Flex *flex, i16 object);
void saveSprites(Flex *flex, i16 object);
/* Marks every sprite for drawing (No Regret only). */
void redrawSprites();

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void clearSpriteMoved(i16 index);
/* UNCALLED: nothing calls it. */
void setSpriteShape(i16 index, u16 shape);
#endif
