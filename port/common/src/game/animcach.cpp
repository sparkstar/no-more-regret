/* ANIMCACH.C (Code50, 1188): the cache of the shapes' animations. */
#include "gamestate.h"
#include "animcach.h"
#include "distance.h"
#include "filespec.h"
#include "gamefile.h"

void closeAnimFile();
void openAnimFile();

/* The slot to fill next, 1-9 (1480:34de). */
char &nextSlot = gameGlobal<char>(0);
/* The slots (1480:34df) and the shape in each, FFFFh for none (8ea5). */
ActorAnim *&animCache = gameGlobal<ActorAnim *>(nullptr);
/* anim.dat (1480:8e4c). */
AnimFile &animFile = gameGlobal<AnimFile>();
u16 (&cachedShapes)[ANIM_CACHE] = gameGlobal<u16[ANIM_CACHE]>();

namespace {
/* 1188:0000 */
void resetAnims(ActorAnim *anims)
{
    memset(static_cast<void *>(anims), 0, sizeof(ActorAnim));
}
}

void initAnimCache()
{
    i16 i;
    for (i = 0; i < ANIM_CACHE; i++) {
        cachedShapes[i] = 0xffff;
    }
    animCache = new ActorAnim[ANIM_CACHE];
    if (animCache != nullptr) {
        for (i = 0; i < ANIM_CACHE; i++) {
            resetAnims(&animCache[i]);
        }
    } else {
        halt();
    }
}

void closeAnimCache()
{
    if (animCache != nullptr) {
        closeAnimFile();
        delete[] animCache;
        animCache = nullptr;
    }
}

namespace {
/* Frees the animations of slot (1188:00ab). */
void freeSlot(i16 slot)
{
    i16 anim;
    AnimAction *action;
    for (anim = 0; anim < ANIM_ACTIONS; anim++) {
        if (animCache[slot].actions[anim] != nullptr) {
            action = animCache[slot].actions[anim];
            action->release();
            delete action;
            animCache[slot].setAction(anim, nullptr);
        }
    }
    cachedShapes[slot] = 0xffff;
}

/* The slot of shape, -1 when it is not cached (1188:0147). */
i16 findSlot(u16 shape)
{
    for (i16 i = 0; i < ANIM_CACHE; i++) {
        if (cachedShapes[i] == shape) {
            return i;
        }
    }
    return -1;
}

/* Reads the animations of shape into the next slot and returns the slot, -1
 * when shape has none (1188:0179). */
i16 loadShape(u16 shape)
{
    i16 slot;
    i16 anim;
    ActorAnim *table;
    AnimAction *action;
    s32 offset;
    table = new ActorAnim;
    if (cachedShapes[0] == 0xffff) {
        if (animFile.isOpen() == 0U) {
            openAnimFile();
        }
        nextSlot = 0;
        cachedShapes[0] = 1;
        loadShape(1);
        if (shape == 1) {
            delete table;
            return 0;
        }
    }
    freeSlot(nextSlot);
    if (!animFile.readTable(shape, table)) {
        delete table;
        return -1;
    }
    slot = nextSlot++;
    if (nextSlot == ANIM_CACHE) {
        nextSlot = 1;
    }
    for (anim = 0; anim < ANIM_ACTIONS; anim++) {
        if ((static_cast<u8>(table->offsets[anim] != 0)) != 0U) {
            action = new AnimAction;
            animCache[slot].setAction(anim, action);
            offset = table->offsets[anim];
            animFile.read(&action->info, 4, offset);
            action->allocate();
            animFile.read(action->frameList, action->size(), -1);
        } else {
            animCache[slot].setAction(anim, nullptr);
        }
    }
    cachedShapes[slot] = shape;
    delete table;
    return slot;
}

/* The animations of shape, cached first; 0 when it has none (1188:03d4). */
ActorAnim *actorAnim(u16 shape)
{
    i16 slot = findSlot(shape);
    if (slot == -1) {
        slot = loadShape(shape);
        if (slot == -1) {
            return nullptr;
        }
        return &animCache[slot];
    }
    return &animCache[slot];
}
}

void getAnimFrame(u16 shape, u16 anim, u8 direction, u16 index, AnimFrame *frame)
{
    ActorAnim *table;
    AnimAction *action;
    if (direction == NO_DIRECTION) {
        halt();
    }
    table = actorAnim(shape);
    if ((static_cast<u8>(table->actions[anim] != nullptr)) != 0U) {
        action = table->actions[anim];
        if (action == nullptr) {
            halt();
        }
        frame->copy(action->frameAt(direction, index));
    } else {
        halt();
    }
}

AnimInfo animInfo(u16 shape, u16 anim)
{
    ActorAnim *table = actorAnim(shape);
    if ((table != nullptr) && ((static_cast<u8>(table->actions[anim] != nullptr)) != 0U)) {
        AnimAction *action = table->actions[anim];
        return action->info;
    }
    AnimInfo none;
    return none;
}

/* Closes anim.dat, freeing every slot (1188:086d), and opens it (08aa). */
void closeAnimFile()
{
    i16 slot;
    if (animFile.isOpen() != 0U) {
        animFile.close();
        for (slot = 0; slot < ANIM_CACHE; slot++) {
            freeSlot(slot);
        }
    }
}

void openAnimFile()
{
    animFile.open(makePath(nullptr, staticDirectory, gameFiles[GAME_ANIM], nullptr), FILE_READWRITE, 0);
}

/* Nothing calls them (1188:08d9, 08e6): closes anim.dat, and 1 when shape
 * has animations. */
/* UNCALLED: nothing calls it. */
void unknown08d9()
{
    animFile.close();
}

/* UNCALLED: nothing calls it. */
bool hasAnims(u16 shape)
{
    if (findSlot(shape) == -1) {
        ActorAnim table;
        if (animFile.isOpen() == 0U) {
            openAnimFile();
        }
        if (!animFile.readTable(shape, &table)) {
            return false;
        }
    }
    return true;
}

bool hasAnim(u16 shape, u16 anim)
{
    i16 slot = findSlot(shape);
    if (slot == -1) {
        slot = loadShape(shape);
        if (slot == -1) {
            return false;
        }
    }
    return animCache[slot].actions[anim] != nullptr;
}
