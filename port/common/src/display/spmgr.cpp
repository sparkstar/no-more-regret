/* SPMGR.C (Code56): the sprites. See spmgr.h. A sprite number out of range
 * halts in both games: No Regret's original broke into a debugger there (INT
 * 3, which the port does not have) and went on, No Remorse's halts. Freeing
 * one out of range does nothing, as in No Regret's. */
#include "spmgr.h"
#include "gamestate.h"
#include <cstdio>
#include "gamepal.h"
#include "itemutil.h"
#include "shape.h"
#include "shaphand.h"
#include "screen.h"

void markSprite(i16 index);
Rect spriteRect(i16 index);

u8 &spritesReady = gameGlobal<u8>(0);
u16 &spriteCount = gameGlobal<u16>(0);
SpriteEntry *&sprites = gameGlobal<SpriteEntry *>(nullptr);
Viewport *&gameBackground = gameGlobal<Viewport *>(nullptr);
/* Set while the world is frozen (WORLD.C): the sprites are not drawn. */
u8 &spritesFrozen = gameGlobal<u8>(0);
/* Where sprites were drawn, to restore from gameBackground. */
RectList &drawnAreas = gameGlobal<RectList>();
MList &spriteList = gameGlobal<MList>();

void initSprites()
{
    if (spritesReady != 0U) {
        return;
    }
    sprites = new SpriteEntry[SPRITES];
    gameBackground = new Viewport(VIEWPORT_LARGE, 0, 0, SCREEN_RIGHT, SCREEN_BOTTOM);
    clearSprites();
    if (spriteList.valid()) {
        spriteList.clear();
    }
    spritesReady = 1;
}

void closeSprites()
{
    if (spriteList.valid()) {
        spriteList.clear();
    }
    if (spritesReady == 0U) {
        return;
    }
    if (sprites != nullptr) {
        delete[] sprites;
        sprites = nullptr;
    }
    if (gameBackground != nullptr) {
        delete gameBackground;
        gameBackground = nullptr;
    }
}

i16 createSprite(u16 shape, u16 frame, u16 flags, i16 x, i16 y, u8 z)
{
    if (shape == 0) {
        haltMessage("Invalid type for sprite.");
    }
    for (i16 i = 0; i < SPRITES; i++) {
        if (sprites[i].shape == 0) {
            sprites[i].width = 0;
            sprites[i].height = 0;
            sprites[i].flags = flags;
            sprites[i].shape = shape;
            showSprite(i, x, y, z);
            setSpriteFrame(i, frame);
            if (static_cast<ui16>(i) >= spriteCount) {
                spriteCount = i + 1;
            }
            markSprite(i);
            return i;
        }
    }
    return -1;
}

void freeSprite(i16 index)
{
    if (index == -1 || index >= SPRITES) {
        return;
    }
    if (spriteList.valid()) {
        spriteList.remove(index);
    }
    addDirtyRect(spriteRect(index));
    sprites[index].shape = 0;
    sprites[index].x = 0;
    sprites[index].width = 0;
    sprites[index].height = 0;
    if (static_cast<ui16>(spriteCount - 1) == static_cast<ui16>(index)) {
        spriteCount--;
        while ((spriteCount != 0U) && (sprites[static_cast<ui16>(spriteCount - 1)].shape == 0U)) {
            spriteCount--;
        }
    }
}

/* Nothing calls it. */
/* UNCALLED: nothing calls it. */
void clearSpriteMoved(i16 index)
{
    sprites[index].flags &= ~SPRITE_MOVED;
}

void markSprite(i16 index)
{
    if (index == -1 || index >= SPRITES) {
        halt();
    }
    sprites[index].flags |= SPRITE_DRAW;
}

u16 spriteFrame(i16 index)
{
    if (index == -1 || index >= SPRITES) {
        halt();
    }
    return sprites[index].frame;
}

void setSpriteFrame(i16 index, u16 frame)
{
    if (index == -1 || index >= SPRITES) {
        halt();
    }
    const Rect old = spriteRect(index);
    sprites[index].frame = frame;
    /* Read and never used, as in setSpriteShape (the original's). */
    [[maybe_unused]] const i16 width = sprites[index].width;
    ShapeFrame *image = cachedShapeFrame(sprites[index].shape, frame);
    sprites[index].width = image->width;
    sprites[index].height = image->height;
    sprites[index].originX = image->xOffset;
    sprites[index].originY = image->yOffset;
    sprites[index].flags |= SPRITE_MOVED;
    addDirtyRect(old);
}

/* Nothing calls it. */
/* UNCALLED: nothing calls it. */
void setSpriteShape(i16 index, u16 shape)
{
    if (index == -1 || index >= SPRITES) {
        halt();
    }
    const Rect old = spriteRect(index);
    const i16 width = sprites[index].width;
    const i16 height = sprites[index].height;
    sprites[index].shape = shape;
    sprites[index].frame = 0;
    ShapeFrame *image = cachedShapeFrame(shape, 0);
    sprites[index].width = image->width;
    sprites[index].height = image->height;
    sprites[index].originX = image->xOffset;
    sprites[index].originY = image->yOffset;
    if (width < image->width || height < image->height) {
        sprites[index].flags |= SPRITE_MOVED;
    } else {
        markSprite(index);
    }
    addDirtyRect(old);
}

void showSprite(u16 sprite, i16 x, i16 y, u8 z)
{
    if (sprites[sprite].x != static_cast<ui16>(x) || sprites[sprite].y != static_cast<ui16>(y) ||
        sprites[sprite].z != z) {
        /* x is 0 until the sprite is first placed. */
        if (static_cast<int>(sprites[sprite].x == 0U) == 0) {
            addDirtyRect(spriteRect(sprite));
        }
        sprites[sprite].x = x;
        sprites[sprite].y = y;
        sprites[sprite].z = z;
        sprites[sprite].flags |= SPRITE_MOVED;
    }
}

void spritePosition(i16 index, u16 *x, u16 *y, u16 *z)
{
    *x = sprites[index].x;
    *y = sprites[index].y;
    *z = sprites[index].z;
}

namespace {
/* The sprite's frame on the screen. */
Rect spriteBounds(i16 index)
{
    i16 x;
    i16 y;
    Rect bounds;

    mapToScreen(sprites[index].x, sprites[index].y, &x, &y);
    y -= sprites[index].z;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = sprites[index].width;
    bounds.bottom = sprites[index].height;
    const i16 left = x - sprites[index].originX;
    const i16 top = y - sprites[index].originY;
    const i16 dx = left - bounds.left;
    const i16 dy = top - bounds.top;
    bounds.moveBy(dx, dy);
    return bounds;
}
}

Point spritePoint(i16 index)
{
    Point point;

    mapToScreen(sprites[index].x, sprites[index].y, &point.x, &point.y);
    point.y -= sprites[index].z;
    return point;
}

/* The sprite's frame clipped to the screen. */
Rect spriteRect(i16 index)
{
    i16 x;
    i16 y;
    Rect bounds;

    mapToScreen(sprites[index].x, sprites[index].y, &x, &y);
    y -= sprites[index].z;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = sprites[index].width;
    bounds.bottom = sprites[index].height;
    bounds.moveTo(x - sprites[index].originX, y - sprites[index].originY);
    if (bounds.left < 0) {
        bounds.left = 0;
    }
    if (bounds.top < 0) {
        bounds.top = 0;
    }
    if (bounds.right > SCREEN_RIGHT) {
        bounds.right = SCREEN_RIGHT;
    }
    if (bounds.bottom > SCREEN_BOTTOM) {
        bounds.bottom = SCREEN_BOTTOM;
    }
    return bounds;
}

void drawSprites()
{
    u16 i;

    if (spritesFrozen != 0U) {
        return;
    }
    RectLink *link = nullptr;
    const Rect clip = drawViewport->bounds();
    while (dirtyRects.forward(reinterpret_cast<GumpLink **>(&link))) {
        for (i = 0; i < spriteCount; i++) {
            if ((sprites[i].flags & (SPRITE_DRAW | SPRITE_MOVED)) == 0) {
                Rect bounds = spriteBounds(i);
                if (rectsOverlap(&bounds, &link->bounds)) {
                    sprites[i].flags |= SPRITE_MOVED;
                }
            }
        }
    }
    RectList moved;
    for (i = 0; i < spriteCount; i++) {
        if (((sprites[i].flags & SPRITE_MOVED) != 0) && (sprites[i].shape != 0U)) {
            sprites[i].flags &= ~SPRITE_MOVED;
            moved.add(spriteBounds(i));
            markSprite(i);
        }
    }
    moved.merge();
    link = nullptr;
    while (moved.forward(reinterpret_cast<GumpLink **>(&link))) {
        Rect *area = &link->bounds;
        gameBackground->blit(drawViewport, area->left, area->top, area->right, area->bottom, area->left, area->top);
    }
    moved.clear();
    for (i = 0; i < spriteCount; i++) {
        if ((sprites[i].shape != 0U) && ((sprites[i].flags & SPRITE_DRAW) != 0)) {
            Rect bounds = spriteBounds(i);
            for (u16 other = 0; other < spriteCount; other++) {
                if (i == other) {
                    drawnAreas.add(bounds);
                    ShapeFrame *image = cachedShapeFrame(sprites[i].shape, sprites[i].frame);
                    if (image != nullptr) {
                        if ((sprites[i].flags & SPRITE_REMAPPED) != 0) {
                            drawShapeRemapped(bounds.left + sprites[i].originX, bounds.top + sprites[i].originY, image,
                                              paletteTables->remap);
                        } else {
                            drawShape(bounds.left + sprites[i].originX, bounds.top + sprites[i].originY, image);
                        }
                        addDirtyRect(Rect(bounds.left, bounds.top, bounds.right, bounds.bottom));
                    } else {
                        halt();
                    }
                } else if (((sprites[other].flags & SPRITE_DRAW) == 0) && (sprites[other].shape != 0U)) {
                    Rect under = spriteBounds(other);
                    if (rectsOverlap(&bounds, &under)) {
                        ShapeFrame *image = cachedShapeFrame(sprites[other].shape, sprites[other].frame);
                        if (image != nullptr) {
                            if ((sprites[other].flags & SPRITE_REMAPPED) != 0) {
                                drawShapeRemapped(under.left + sprites[other].originX,
                                                  under.top + sprites[other].originY, image, paletteTables->remap);
                            } else {
                                drawShape(under.left + sprites[other].originX, under.top + sprites[other].originY,
                                          image);
                            }
                            addDirtyRect(Rect(under.left, under.top, under.right, under.bottom));
                            drawnAreas.add(under);
                        } else {
                            halt();
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < spriteCount; i++) {
        sprites[i].flags &= ~(SPRITE_DRAW | SPRITE_MOVED);
    }
    drawViewport->setBounds(clip);
}

void restoreBackground()
{
    RectLink *link;

    drawnAreas.merge();
    link = nullptr;
    while (drawnAreas.forward(reinterpret_cast<GumpLink **>(&link))) {
        drawViewport->blit(gameBackground, link->bounds.left, link->bounds.top, link->bounds.right, link->bounds.bottom,
                           link->bounds.left, link->bounds.top);
    }
    drawnAreas.clear();
}

void clearSprites()
{
    spriteCount = 0;
    for (i16 i = 0; i < SPRITES; i++) {
        sprites[i].shape = 0;
        sprites[i].x = 0;
    }
}

u16 spriteX(i16 index)
{
    return sprites[index].x;
}

u16 spriteY(i16 index)
{
    return sprites[index].y;
}

void freezeSprites()
{
    spritesFrozen = 1;
}

void thawSprites()
{
    spritesFrozen = 0;
    for (u16 i = 0; i < spriteCount; i++) {
        if (sprites[i].shape != 0U) {
            markSprite(i);
        }
    }
}

void loadSprites(Flex *flex, i16 object)
{
    memset(sprites, 0, SPRITES * sizeof(SpriteEntry));
    drawnAreas.clear();
    flex->readObject(object, &spritesFrozen, 1, 0);
    flex->readObject(object + 1, &spriteCount, 2, 0);
    flex->readObject(object + 2, sprites, static_cast<ui16>(spriteCount * sizeof(SpriteEntry)), 0);
    flex->seekObject(object + 3);
    spriteList.load(flex);
    for (u16 i = 0; i < spriteCount; i++) {
        sprites[i].flags |= SPRITE_MOVED;
    }
}

void saveSprites(Flex *flex, i16 object)
{
    flex->writeObject(object, &spritesFrozen, 1, 0, 1);
    flex->writeObject(object + 1, &spriteCount, 2, 0, 1);
    flex->writeObject(object + 2, sprites, static_cast<ui16>(spriteCount * sizeof(SpriteEntry)), 0, 1);
    FlexEntry entry;
    flex->readEntry(object + 3, &entry);
    flex->seek(0, SEEK_END);
    entry.offset = flex->size();
    spriteList.save(flex);
    entry.size = flex->size() - entry.offset;
    flex->writeEntry(object + 3, &entry);
}

void redrawSprites()
{
    for (u16 i = 0; i < spriteCount; i++) {
        if (sprites[i].shape != 0U) {
            sprites[i].flags |= SPRITE_DRAW | SPRITE_MOVED;
        }
    }
}
