/* ..\\ITEM\\PRELOAD.C (Code33, 1100:0000-056f in both games): loading the
 * shapes the map needs into the shape cache, the functions both games have
 * the same, their values each game's preload_game.h. */
#include "gamestate.h"
#include "preload.h"
#include "preload_game.h"
#include "item.h"
#include "type.h"
#include "itemcach.h"
#include "itemfind.h"
#include "globs.h"
#include "dtable.h"
#include "shaphand.h"
#include "cache.h"
#include "cprotmem.h"

/* The frame sets of shapes 0-3FFh and 400h-7FFh (1480:89a4). */
FrameSet *(&frameSets)[2] = gameGlobal<FrameSet *[2]>();

void FrameSet::clear()
{
    memset(bits, 0, sizeof bits);
}

void FrameSet::set(u16 frame)
{
    bits[frame >> 3] |= 1 << (frame & 7);
}

u8 FrameSet::test(u16 frame)
{
    return bits[frame >> 3] & (1 << (frame & 7));
}

namespace {
/* The frame set of shape, 0 for a bad shape (1100:00de). */
FrameSet *frameSet(u16 shape)
{
    if (shape < SHAPES) {
        return &frameSets[shape >> 10][shape & 0x3ff];
    }
    return nullptr;
}
}

void markFrame(u16 shape, u16 frame)
{
    FrameSet *set;

    if ((set = frameSet(shape)) != nullptr) {
        set->set(frame);
    }
}

/* Nothing calls it (1100:0137). */
u8 isFrameMarked(u16 shape, u16 frame)
{
    FrameSet *set;

    if ((set = frameSet(shape)) != nullptr) {
        return set->test(frame);
    }
    return 0;
}

void allocateFrameSets()
{
    for (i16 i = 0; i < 2; i++) {
        frameSets[i] = new FrameSet[0x400];
    }
}

u8 freeFrameSets()
{
    const bool ok = true;

    for (i16 i = 0; i < 2; i++) {
        if (frameSets[i] != nullptr) {
            delete[] frameSets[i];
        }
        frameSets[i] = nullptr;
    }
    return static_cast<u8>(ok);
}

bool markMapFrames()
{
    i16 row;
    i16 column;
    Item item;
    u16 next;
    u16 glob;

    allocateFrameSets();
    for (row = 0; row < CHUNKS; row++) {
        for (column = 0; column < CHUNKS; column++) {
            for (next = mapChunks[column][row]; next != 0U; next = itemNext[item.id]) {
                item = ItemRef(next);
                markFrame(itemShape[item.id], item.frame());
                glob = item.globQuality();
                if (typeFlags.infos[itemShape[item.id]].family == FAMILY_GLOB_EGG && (glob != 0U)) {
                    globHandler->markFrames(glob);
                }
            }
        }
    }
    return true;
}

/* No Regret's: the shape of the editor's NPC marker, whose NPC number picks
 * the NPC type (and the shape) to preload. */
constexpr int NPC_EGG_SHAPE = 0x4d0;

/* Not read. */
u16 &unknown22de = gameGlobal<u16>(0);
namespace {
/* No Regret's shapes whose frames are loaded when the main heap is large. */
u16 preloadList[] = {0x326, 0x31c, 0x31f, 0x320, 0x321, 0x323, 0x324, 0x325, 0x1d9, 0x1d8,
                     0x576, 0x656, 0x62d, 0x278, 0x58f, 0x5d6, 0x59c, 0x5ef, 0x001, 0xffff};

/* Frees the shapes the map does not use and, while the cache has room,
 * loads the frames of the shapes it uses and of the shapes in the preload
 * list (1100:0255). */
void preloadShapes()
{
    u16 frames;
    u16 width;
    u16 height;
    u8 *needed = new u8[SHAPES];
    ShapeInfo info;
    AreaSearch search;
    bool large;
    u16 shape;
    i16 frame;

    large = fragments[0].size >= static_cast<s32>(5000000L);
    if (needed == nullptr) {
        halt();
    }
    memset(needed, 0, SHAPES);
    search.init(0, 0, 0x7fff, 0x7fff, "$", 1);
    while (search.isValid()) {
        u16 foundShape = itemShape[search.id];

        if (foundShape != 0U) {
            if (foundShape == NPC_EGG_SHAPE) {
                const u16 number = search.npcNumber();
                const NpcType type = npcTypes[number];
                foundShape = type.shape;
                needed[foundShape] = 1;
            } else {
                info = typeFlags.info(foundShape);
                if (!info.editor) {
                    needed[foundShape] = 1;
                }
            }
        }
        search.next();
    }
    /* Shape 7FFh is never freed. */
    for (i16 i = 0; i < SHAPES - 1; i++) {
        if (needed[i] == 0U) {
            shapeCache->freeShape(i);
        }
    }
    search.init(0, 0, 0x7fff, 0x7fff, "$", 1);
    while (freeBytes > halfMinimumFree && search.isValid()) {
        shape = itemShape[search.id];
        if (shape != 0U) {
            info = typeFlags.info(shape);
            if (info.editor && shape == NPC_EGG_SHAPE) {
                const u16 number = search.npcNumber();
                const NpcType type = npcTypes[number];
                shape = type.shape;
                info = typeFlags.info(shape);
            }
            if (!info.editor && large) {
                shapeCache->shapeHeader(shape, &width, &height, &frames);
                for (frame = 0; frame < static_cast<i16>(frames) && freeBytes > halfMinimumFree; frame++) {
                    cachedShapeFrame(shape, frame);
                }
            }
        }
        search.next();
    }
    if (large) {
        for (i16 i = 0; freeBytes > halfMinimumFree && preloadList[i] != 0xffff; i++) {
            shape = preloadList[i];
            shapeCache->shapeHeader(shape, &width, &height, &frames);
            for (frame = 0; frame < static_cast<i16>(frames) && freeBytes > halfMinimumFree; frame++) {
                cachedShapeFrame(shape, frame);
            }
        }
    }
    /* The port's: the original leaves the table, a map's load each. */
    delete[] needed;
}
}

/* Loads the frames the map uses (the argument is not read): No Regret's
 * frees the shapes the map does not use and loads the frames of those it
 * uses and of the preload list while the cache has room (preloadShapes); No
 * Remorse's loads every frame the map uses, unless unknown1453 is set
 * (10f0:0255). */
void preloadMap(u8 /*unused*/)
{
    if (markMapFrames()) {
        if (PRELOAD_BY_SHAPES) {
            preloadShapes();
        } else {
            if (unknown1453 != 0U) {
                freeFrameSets();
                return;
            }
            for (i16 shape = 0; shape < 0x800; shape++) {
                for (i16 frame = 0; frame < 0x100; frame++) {
                    if (isFrameMarked(shape, frame) != 0U) {
                        cachedShapeFrame(shape, frame);
                    }
                }
            }
        }
    }
    freeFrameSets();
}
