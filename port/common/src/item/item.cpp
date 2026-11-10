/* ..\\ITEM\\ITEM.C (Code23, No Regret's 10b0:0000-4582, No Remorse's
 * 10a0:0000-4485; the addresses are No Regret's): the items, the games'
 * differences as each game's values (item_game.h). */
#include "item.h"
#include "item_game.h"
#include <cstring>
#include "bark.h"
#include "dispatch.h"
#include "flicplay.h"
#include "movie.h"
#include "crusys.h"
#include "npc.h"
#include "type.h"
#include "itemcach.h"
#include "itemdam.h"
#include "itemfind.h"
#include "itemutil.h"
#include "distance.h"
#include "collide.h"
#include "egg.h"
#include "alert.h"
#include "kprocess.h"
#include "kernel.h"
#include "priority.h"
#include "uprocess.h"
#include "sndsrc.h"
#include "dsfxman.h"
#include "shaphand.h"
#include "viewport.h"
#include "rect.h"
#include "camera.h"
#include "spmgr.h"
#include "gravity.h"
#include "missile.h"
#include "snap.h"
#include "dtable.h"
#include "tracker.h"
#include "crusader.h"
#include "control.h"
#include "animcach.h"
#include "animprim.h"
#include "dlist.h"
#include "dlitem.h"
#include "random.h"
#include "shapes.h"

namespace {
/* The 16-unit step of each of the 8 directions (step). */
i16 stepX[8] = {-16, 0, 16, 16, 16, 0, -16, -16};
i16 stepY[8] = {-16, -16, -16, 0, 16, 16, 16, 0};

/* Halts at the caller's place unless item is a valid number (10b0:0000). */
void checkItem(u16 item, std::source_location place = CALL_PLACE)
{
    if (item == 0 || item >= ITEMS) {
        halt(place);
    }
}
}

ReleaseProcess::ReleaseProcess(u16 releasedItem)
{
    setItem(releasedItem);
    setType(RELEASE_TYPE);
}

void ReleaseProcess::run()
{
    Item(item).release();
    terminate(0);
}

bool Item::isAvatar() const
{
    return id == 1;
}

bool Item::isControlled() const
{
    return id == controlledActor;
}

void Item::screenPosition(i16 *x, i16 *y) const
{
    mapToScreen(itemX[id], itemY[id], x, y);
    *y -= z();
}

/* UNCALLED: nothing calls it. */
u8 Item::isFlipped() const
{
    return ((flags() & ITEM_FLIPPED) != 0) ? 1 : 0;
}

u16 Item::x() const
{
    u16 item = id;

    while ((itemFlags[item] & ITEM_CONTAINED) != 0) {
        item = itemX[item];
    }
    return itemX[item];
}

u16 Item::y() const
{
    u16 item = id;

    while ((itemFlags[item] & ITEM_CONTAINED) != 0) {
        item = itemX[item];
    }
    return itemY[item];
}

u16 Item::gumpX() const
{
    u16 x = 0;

    if ((flags() & ITEM_CONTAINED) != 0) {
        x = static_cast<u8>(itemY[id] & 0xff);
    }
    return x;
}

u16 Item::gumpY() const
{
    u16 y = 0;

    if ((flags() & ITEM_CONTAINED) != 0) {
        y = static_cast<u8>(itemY[id] >> 8);
    }
    return y;
}

void Item::setGumpPosition(i16 x, i16 y) const
{
    if ((itemFlags[id] & ITEM_CONTAINED) != 0) {
        if (x > 0x7f) {
            x = 0x7f;
        }
        if (y > 0xff) {
            y = 0xff;
        }
        itemY[id] = (x & 0xff) + ((y & 0xff) << 8);
    }
}

u8 Item::z() const
{
    u16 item = id;

    while ((itemFlags[item] & ITEM_CONTAINED) != 0) {
        item = itemX[item];
    }
    return itemZ[item];
}

u16 Item::centreX() const
{
    return itemX[id] - (typeFlags.infos[itemShape[id]].x << 3);
}

u16 Item::centreY() const
{
    return itemY[id] - (typeFlags.infos[itemShape[id]].y << 3);
}

u8 Item::centreZ() const
{
    return z() + (typeFlags.infos[itemShape[id]].z << 2);
}

u8 Item::damagePoints() const
{
    return itemDamagePoints[id];
}

void Item::setDamagePoints(u8 points) const
{
    itemDamagePoints[id] = points;
}

void Item::setX(u16 x) const
{
    itemX[id] = x;
}

void Item::setY(u16 y) const
{
    itemY[id] = y;
}

void Item::setZ(u8 z) const
{
    if (z >= 0xfb) {
        halt();
    }
    itemZ[id] = z;
}

ItemPoint Item::point() const
{
    u16 item = id;
    ItemPoint point;

    while ((itemFlags[item] & ITEM_CONTAINED) != 0) {
        item = itemX[item];
    }
    point.x = itemX[item];
    point.y = itemY[item];
    point.z = itemZ[item];
    return point;
}

u16 Item::next() const
{
    return itemNext[id];
}

i16 Item::weight() const
{
    return typeFlags.infos[itemShape[id]].weight;
}

i16 Item::totalWeight() const
{
    i16 total = weight();

    if (contents() != 0U) {
        total += Container(id).contentsWeight();
    }
    return total;
}

i16 Item::volume() const
{
    ShapeInfo *info = &typeFlags.infos[itemShape[id]];

    if ((flags() & ITEM_INVISIBLE) != 0) {
        return 0;
    }
    if (info->family == FAMILY_QUANTITY) {
        return static_cast<i16>((info->volume * quality()) + 99) / 100;
    }
    if (info->family == FAMILY_REAGENT) {
        return static_cast<i16>((info->volume * quality()) + 9) / 10;
    }
    if (info->family == FAMILY_CONTAINER && info->volume == 0) {
        return 1;
    }
    return info->volume;
}

i16 Item::capacity() const
{
    const ShapeInfo info = typeFlags.infos[itemShape[id]];

    if (info.family == FAMILY_CONTAINER) {
        return info.volume;
    }
    return 0;
}

u16 Item::surfaceWeight() const
{
    u16 total = 0;
    SurfaceSearch above(id, "$", 1, 1);

    while (above.isValid()) {
        total += above.totalWeight();
        above.next();
    }
    return total;
}

void Item::setNext(u16 item) const
{
    itemNext[id] = item;
}

u16 Item::shape() const
{
    return itemShape[id];
}

void Item::setShape(u16 shape) const
{
    const ShapeInfo old = typeFlags.infos[this->shape()];
    const ShapeInfo info = typeFlags.infos[shape];

    if ((old.occludes || old.targetable) && (isBroken() == 0U)) {
        if (!info.occludes && !info.targetable) {
            removeTarget(id);
        }
    } else if ((info.occludes || info.targetable) && !old.occludes && !old.targetable && (isBroken() == 0U)) {
        addTarget(id);
    }
    itemShape[id] = shape;
}

u16 Item::frame() const
{
    u16 frame = itemFrame[id];

    if (isNpc()) {
        Npc npc(id);
        frame |= npc.frameHigh() << 8;
    }
    return frame;
}

void Item::setFrame(u16 frame) const
{
    itemFrame[id] = frame;
    if (isNpc()) {
        Npc npc(id);
        npc.setFrameHigh(frame >> 8);
    }
    if ((itemFlags[id] & ITEM_DRAWN) != 0) {
        if (itemZ[id] < cameraLevel() || typeFlags.infos[itemShape[id]].draw) {
            touch();
        }
    }
}

/* UNCALLED: nothing calls it. */
void Item::setFrameNow(u16 frame) const
{
    itemFrame[id] = frame;
    if (isNpc()) {
        Npc npc(id);
        npc.setFrameHigh(frame >> 8);
    }
    if ((itemFlags[id] & ITEM_DRAWN) == 0) {
        return;
    }
    if (itemZ[id] < cameraLevel() || typeFlags.infos[itemShape[id]].draw) {
        const u16 node = itemNode(id);
        if ((nodeOf(node)->flags & (NODE_CHANGED | NODE_DRAWN)) == 0) {
            changedNodes.add(node);
            nodeOf(node)->flags |= NODE_CHANGED;
        }
        DL_Node *entry = nodeOf(node);
        Rect dirty(&entry->bounds);
        dirty.left = dirty.left - scrollX;
        dirty.top = dirty.top - scrollY;
        dirty.right = dirty.right - scrollX;
        dirty.bottom = dirty.bottom - scrollY;
        if ((dirtySet != 0U) && dirty.touches(screenRect) && !dirty.touches(dirtyArea)) {
            flushDirty();
        }
        if (dirty.touches(screenRect)) {
            clipRect(&dirty, &screenRect);
            if (dirtySet != 0U) {
                mergeRect(&dirtyArea, &dirty);
            } else {
                dirtyArea.left = dirty.left;
                dirtyArea.top = dirty.top;
                dirtyArea.right = dirty.right;
                dirtyArea.bottom = dirty.bottom;
                dirtySet = 1;
            }
            Rect area(entry->bounds.left - scrollX, entry->bounds.top - scrollY, entry->bounds.right - scrollX,
                      entry->bounds.bottom - scrollY);
            area.intersect(drawViewport->bounds());
            if (updateSet != 0U) {
                mergeRect(&updateArea, &lvalue(Rect(&area)));
                return;
            }
            updateArea = ScreenArea(&area);
            updateSet = 1;
        }
    }
}

void Item::setNpc(u8 npc) const
{
    itemNpc[id] = npc;
}

void Item::setNpcNumber(u8 npc) const
{
    itemNpc[id] = npc;
}

u16 Item::npcNumber() const
{
    return itemNpc[id];
}

void Item::setMap(i16 map) const
{
    itemMap[id] = map;
}

i16 Item::map() const
{
    return itemMap[id];
}

u16 Item::q() const
{
    return itemQuality[id];
}

i16 Item::familyQuality() const
{
    if (typeFlags.infos[itemShape[id]].family == FAMILY_QUALITY) {
        return itemQuality[id];
    }
    return 0;
}

void Item::setFamilyQuality(u16 quality) const
{
    if (typeFlags.infos[itemShape[id]].family == FAMILY_QUALITY) {
        itemQuality[id] = quality;
    }
}

u16 Item::eggType() const
{
    if (typeFlags.infos[itemShape[id]].family == FAMILY_EGG) {
        return itemQuality[id] & 0xff;
    }
    return 0;
}

void Item::setEggQuality(u16 quality) const
{
    if (typeFlags.infos[itemShape[id]].family == FAMILY_EGG) {
        itemQuality[id] = quality;
    }
}

i16 Item::quality() const
{
    return itemQuality[id];
}

void Item::setQuality(u16 quality)
{
    itemQuality[id] = quality;
    combine();
}

u16 Item::contents() const
{
    if (family() == FAMILY_CONTAINER || family() == FAMILY_MONSTER_EGG) {
        return itemQuality[id];
    }
    return 0;
}

u16 Item::container() const
{
    if ((itemFlags[id] & ITEM_CONTAINED) != 0) {
        return itemX[id];
    }
    return 0;
}

u16 Item::outerContainer() const
{
    Item outer(container());

    if (!outer.isValid()) {
        return 0;
    }
    Item next(outer.container());
    while (next.isValid()) {
        outer = next;
        next = outer.container();
    }
    return outer.id;
}

u16 Item::globQuality() const
{
    if (typeFlags.infos[itemShape[id]].family == FAMILY_GLOB_EGG) {
        return itemQuality[id];
    }
    return 0;
}

void Item::setGlobQuality(u16 quality) const
{
    if (typeFlags.infos[itemShape[id]].family == FAMILY_GLOB_EGG) {
        itemQuality[id] = quality;
    }
}

void Item::setQ(u16 quality) const
{
    itemQuality[id] = quality;
}

void Item::setContents(u16 item) const
{
    if (typeFlags.infos[itemShape[id]].family == FAMILY_CONTAINER ||
        typeFlags.infos[itemShape[id]].family == FAMILY_MONSTER_EGG) {
        itemQuality[id] = item;
    }
}

i16 Item::family() const
{
    return typeFlags.infos[itemShape[id]].family;
}

u8 Item::typeFlag(i16 bit) const
{
    u8 *bytes = reinterpret_cast<u8 *>(&typeFlags.infos[itemShape[id]]);

    return (bytes[bit >> 3] >> (bit & 7)) & 1;
}

u16 Item::flags() const
{
    return itemFlags[id];
}

bool Item::legalCreateAtPoint(u16 shape, u16 frame, ItemPoint *point)
{
    if (shape >= SHAPES) {
        return false;
    }
    Sweep sweep;
    if (sweep.fits(shape, point, 0)) {
        create(shape, frame);
        popToCoords(point->x, point->y, point->z);
        return true;
    }
    return false;
}

bool Item::legalCreateAtCoords(u16 shape, u16 frame, u16 x, u16 y, u8 z)
{
    if (shape >= SHAPES) {
        return false;
    }
    ItemPoint point(x, y, z);
    Sweep sweep;
    if (sweep.fits(shape, &point, 0)) {
        create(shape, frame);
        popToCoords(point.x, point.y, point.z);
        return true;
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool Item::legalCreateInContainer(u16 shape, u16 frame, u16 container, i16 flag)
{
    if (shape >= SHAPES) {
        return false;
    }
    Container box(container);
    if (box.canHold(shape, flag)) {
        create(shape, frame);
        popToContainer(container);
        return true;
    }
    return false;
}

bool Item::create(u16 shape, u16 frame)
{
    if (shape >= SHAPES) {
        id = 0;
        return false;
    }
    id = newItem();
    itemShape[id] = shape;
    itemFrame[id] = frame;
    if (damageTable->takesDamage(shape) != 0U) {
        DamageEntry *entry = &damageTable->entries[shape];
        const i16 points = entry->damagePoints;
        setDamagePoints(points);
    } else {
        setDamagePoints(1);
    }
    {
        const i16 family = Item(id).family();
        switch (family) {
        case FAMILY_AMMO:
        case FAMILY_BOMB:
            setQuality(1);
            break;
        case FAMILY_WEAPON:
            const u16 rounds = clipSize(this->shape());
            setQuality(rounds);
            break;
        }
    }
    if ((snapProcess != nullptr) && shape == SNAP_EGG) {
        snapProcess->removeEgg(id);
    }
    return true;
}

void Item::popToCoords(u16 x, u16 y, u8 z)
{
    ::popToCoords(x, y, z);
}

void Item::popToContainer(u16 container)
{
    ::popToContainer(container);
}

void Item::pop()
{
    popItem();
}

void Item::popToEnd(u16 container)
{
    ::popToEnd(container);
}

void Item::push() const
{
    pushItem(id);
}

void Item::destroyContents() const
{
    if (family() == FAMILY_CONTAINER || family() == FAMILY_MONSTER_EGG) {
        Container(id).destroyContents();
    }
}

/* A crate: No Regret's shape information has a crate bit; No Remorse's has
 * none, and its Item::destroy compares the shapes of CRATE_SHAPES itself
 * (10a0:1365), which the port does here. */
bool Item::isCrate() const
{
    const u16 number = shape();
    const ShapeInfo info = typeFlags.infos[number];

    return (info.crate != 0U) || isOneOf(number, CRATE_SHAPES);
}

u8 Item::hasDeathCry() const
{
    const ShapeInfo info = typeFlags.infos[shape()];

    return info.deathCry;
}

u16 Item::nothing()
{
    return 0;
}

i16 Item::isEthereal() const
{
    return static_cast<i16>(z() == ETHEREAL_Z);
}

/* The item says text in a bark gump for a while: 5.5 ticks for each 32
 * characters, at least one line's worth; the pid of the gump's notifier
 * (10a0:290e). */
/* UNCALLED: nothing calls it. */
u16 Item::bark(char *text) const
{
    i16 lines = strlen(text) / 32;

    if (lines == 0) {
        lines++;
    }
    const i16 time = static_cast<i16>(lines * 11) >> 1;
    BarkGump *gump = new BarkGump(modals[modalTop], id, text, 1, time);

    return gump->notifier;
}

namespace {
/* The usecode's movies' end: the intrinsic's result 0 after the movie, a
 * wait of the steps. */
void movieDone(bool /*cancelled*/)
{
    setRestResult(0);
}
}

/* The usecode's movies (10a0:3a36-3ad7), each a request of the movies' entry:
 * with every argument of the ring player (centred by a, the screen kept by c
 * or greyed with a fade by d; b was unused; the CD's usecode never calls it),
 * as a cutscene on the screen greyed with a fade, as a cutscene on the greyed
 * screen, the last two again without their unused argument, and as an overlay
 * on the screen as it is. */
/* UNCALLED: nothing calls it. */
u16 Item::playMovie(char *name, i16 x, i16 y, i16 a, i16 /*unused*/, i16 c, i16 d)
{
    MovieBackground background = MovieBackground::GREY;
    if (c != 0) {
        background = MovieBackground::KEEP;
    } else if (d != 0) {
        background = MovieBackground::GREY_FADE;
    }
    ::openMovie(
        MovieRequest{.name = name, .centred = a != 0, .x = x, .y = y, .background = background, .holdMusic = true},
        movieDone);
    return 0;
}

u16 Item::playMovieCutscene(char *name, i16 x, i16 y, i16 /*unused*/)
{
    ::openMovie(MovieRequest{.name = name, .x = x, .y = y, .background = MovieBackground::GREY_FADE, .holdMusic = true},
                movieDone);
    return 0;
}

u16 Item::playMovieCutsceneAlt(char *name, i16 x, i16 y, i16 /*unused*/)
{
    ::openMovie(MovieRequest{.name = name, .x = x, .y = y, .background = MovieBackground::GREY, .holdMusic = true},
                movieDone);
    return 0;
}

u16 Item::playMovieCutsceneAlt(char *name, i16 x, i16 y)
{
    return playMovieCutsceneAlt(name, x, y, 1);
}

u16 Item::playMovieCutscene(char *name, i16 x, i16 y)
{
    return playMovieCutscene(name, x, y, 1);
}

u16 Item::playMovieOverlay(char *name, i16 x, i16 y)
{
    flicGreyScreen = 0;
    ::openMovie(MovieRequest{.name = name,
                             .x = x,
                             .y = y,
                             .background = MovieBackground::KEEP,
                             .mayDouble = false,
                             .maySubtitle = false},
                movieDone);
    return 0;
}

u16 Item::enterFastArea()
{
    u16 result = 0;

    setFlags(flags() | ITEM_FAST_AREA);
    if (((isOccluding() != 0U) || (isTargetable() != 0U)) && (isBroken() == 0U)) {
        addTarget(id);
    }
    if (isNpc() && (Npc(id).isDead() != 0U)) {
        const u16 shape = itemShape[id];
        if (shape != SHAPE_BURNING && shape != SHAPE_SKELETON_DEATH && shape != SHAPE_EM_DEATH &&
            shape != SHAPE_PLASMA_DEATH) {
            return 0;
        }
    }
    const i16 family = this->family();
    if (family == FAMILY_EGG || (family == FAMILY_TELEPORT_EGG && (frame() == 0U)) || family == FAMILY_MONSTER_EGG) {
        if (family == FAMILY_MONSTER_EGG && (worldActive != 0U)) {
            MonsterEgg egg(id);
            egg.hatch();
            return 0;
        }
        new EggHatcher(id);
    }
    if ((snapProcess != nullptr) && itemShape[id] == SNAP_EGG) {
        snapProcess->addEgg(id);
    }
    if (currentMap == 0U) {
        return 0;
    }
    if (isNpc()) {
        Npc npc(id);
        if ((npc.isDead() == 0U) || DEAD_NPCS_RESTART) {
            const u16 activity = npcs[id].activity;
            npcs[id].lastActivity = 0;
            npcs[id].activity = 0;
            if (hasAnim(shape(), ANIM_STAND)) {
                setFirstFrame(id, ANIM_STAND);
            }
            npcs[npc.id].weaponDrawn = 0;
            npcs[npc.id].inCombat = 0;
            npc.setActivity(activity);
        }
    }
    if (isNpc() || typeFlags.infos[itemShape[id]].noisy) {
        if (callEvent(this, 1L << 15, 15, &result, 0)) {
            return result;
        }
    }
    return 0;
}

/* What stands on a crate falls when it goes (isCrate: No Regret's reads the
 * shape's crate bit, No Remorse's knows the crates' shapes). */
void Item::destroy()
{
    if ((snapProcess != nullptr) && itemShape[id] == SNAP_EGG) {
        snapProcess->removeEgg(id);
    }
    if (id == 0) {
        return;
    }
    if ((worldActive != 0U) && (effectsStarted != 0U)) {
        if (isNpc()) {
            forgetItemSounds(id);
        }
        stopItemLoops(id);
    }
    if (isNpc()) {
        freeNpcNumber(this);
    }
    if (isCrate()) {
        SurfaceSearch above(id, "$", 1, 1);
        while (above.isValid()) {
            fall(&above);
            above.next();
        }
    }
    removeItem(id);
}

/* UNCALLED: nothing calls it. */
void Item::removeContents() const
{
    if (family() == FAMILY_CONTAINER || family() == FAMILY_MONSTER_EGG) {
        Container(id).removeContents();
    }
}

void Item::move(u16 x, u16 y, u8 z)
{
    if (z >= 0xfb) {
        halt();
    }
    pushItem(id);
    ::popToCoords(x, y, z);
    justMoved();
    if (targeting == 0 || id != targetItem) {
        return;
    }
    Npc npc(controlledActor);
    if (npc.isValid() && (npc.isDead() == 0U)) {
        const Npc target(findTarget(controlledActor, npc.direction(), 1));
        if (id == target.id) {
            const ShapeInfo info = typeFlags.info(shape());
            i16 size = info.z;
            if (size >= 4) {
                size >>= 1;
                size++;
            } else if (size >= 2) {
                size >>= 1;
            }
            if (!isNpc()) {
                if (size >= 2) {
                    size -= 2;
                } else if (size == 1) {
                    size = 0;
                }
            }
            size *= 8;
            size += z;
            /* The reticle goes on the middle of the box's long side. */
            i16 spriteX;
            i16 spriteY;
            if (info.x > info.y) {
                spriteX = info.x >> 1;
                spriteX *= 16;
                spriteX = x - spriteX;
                spriteY = y;
            } else if (info.y > info.x) {
                spriteY = info.y >> 1;
                spriteY *= 16;
                spriteY = y - spriteY;
                spriteX = x;
            } else {
                spriteX = x;
                spriteY = y;
            }
            showSprite(targetSprite, spriteX, spriteY, size);
            return;
        }
        if (target.isValid() && (target.isOccluding() != 0U)) {
            setTarget(target.id);
            return;
        }
        setTarget(0);
    }
}

void Item::moveToContainer(u16 container)
{
    pushItem(id);
    ::popToContainer(container);
    justMoved();
}

bool Item::legalMoveToPoint(ItemPoint *point, u16 flags, u16 force) const
{
    Sweep sweep;

    if (force == 0) {
        force = 4;
    }
    if (sweep.move(id, point, force)) {
        sweep.apply();
        return true;
    }
    if ((flags & 1) == 0) {
        sweep.apply();
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool Item::legalMoveToContainer(u16 *container)
{
    Container box(*container);
    if (box.canAdd(id)) {
        moveToContainer(*container);
        setGumpPosition(0, 0);
        return true;
    }
    return false;
}

bool Item::collideMove(ItemPoint *point, u16 flags, u16 moveFlags, u16 target, u8 *blocked, u16 *hit) const
{
    HitSweep sweep;

    if (moveFlags == 0) {
        moveFlags = MOVE_NO_FLOOR | MOVE_CLIMB;
    }
    const bool moved = sweep.move(id, point, moveFlags, target);
    if (hit != nullptr) {
        *hit = sweep.hitItem;
    }
    if (moved) {
        if (blocked != nullptr) {
            *blocked = 0;
        }
        grab();
        sweep.apply();
        return true;
    }
    if (blocked != nullptr) {
        if (sweep.targetFirst != 0U) {
            *blocked = 1;
        } else {
            *blocked = 0;
        }
    }
    if ((flags & 1) == 0) {
        grab();
        sweep.apply();
    }
    return false;
}

void Item::moveTo(const ItemPoint &point) const
{
    pushItem(id);
    ::popToCoords(point.x, point.y, point.z);
}

/* UNCALLED: nothing calls it. */
void Item::step(char direction) const
{
    const i16 x = itemX[id];
    const i16 y = itemY[id];
    const u8 z = itemZ[id];

    pushItem(id);
    ::popToCoords(x + stepX[static_cast<int>(direction)], y + stepY[static_cast<int>(direction)], z);
}

char Item::directionTo(u16 x, u16 y) const
{
    return ::direction(itemX[id], itemY[id], x, y);
}

i16 Item::directionFrom(u16 x, u16 y) const
{
    return static_cast<i16>(directionTo(x, y) + 4) % 8;
}

i16 Item::directionFromItem(u16 item) const
{
    const Item other(item);

    return directionFrom(itemX[other.id], itemY[other.id]);
}

char Item::directionToItem(u16 item) const
{
    const Item other(item);

    return directionTo(itemX[other.id], itemY[other.id]);
}

void Item::footpad(i16 *x, i16 *y, i16 *z) const
{
    const u16 shape = itemShape[id];
    ShapeInfo *info = &typeFlags.infos[shape];

    if ((flags() & ITEM_FLIPPED) != 0) {
        *y = info->x;
        *x = info->y;
    } else {
        *x = info->x;
        *y = info->y;
    }
    *z = info->z;
}

bool Item::touches(u16 item) const
{
    bool touching = false;

    if (((itemFlags[id] & ITEM_CONTAINED) == 0) && ((itemFlags[item] & ITEM_CONTAINED) == 0)) {
        i16 width;
        i16 depth;
        i16 height;
        i16 otherWidth;
        i16 otherDepth;
        i16 otherHeight;
        u8 z;
        u8 otherZ;

        footpad(&width, &depth, &height);
        Item(item).footpad(&otherWidth, &otherDepth, &otherHeight);
        const u16 x = itemX[id];
        const u16 otherX = itemX[item];
        width <<= 4;
        otherWidth <<= 4;
        if (static_cast<ui16>(otherX - otherWidth) <= x && static_cast<ui16>(x - width) <= otherX) {
            if (static_cast<ui16>(otherX - otherWidth) == x || static_cast<ui16>(x - width) == otherX) {
                touching = true;
            }
            const u16 y = itemY[id];
            const u16 otherY = itemY[item];
            depth <<= 4;
            otherDepth <<= 4;
            if (static_cast<ui16>(otherY - otherDepth) <= y && static_cast<ui16>(y - depth) <= otherY) {
                if (static_cast<ui16>(otherY - otherDepth) == y || static_cast<ui16>(y - depth) == otherY) {
                    touching = true;
                }
                z = itemZ[id];
                otherZ = itemZ[item];
                height <<= 3;
                otherHeight <<= 3;
                if (z <= static_cast<i16>(otherZ + otherHeight) && otherZ <= static_cast<i16>(z + height)) {
                    if (z == static_cast<i16>(otherZ + otherHeight) || otherZ == static_cast<i16>(z + height)) {
                        touching = true;
                    }
                    return touching;
                }
            }
        }
    }
    return false;
}

bool Item::overlaps(u16 item) const
{
    if (((itemFlags[id] & ITEM_CONTAINED) == 0) && ((itemFlags[item] & ITEM_CONTAINED) == 0)) {
        i16 width;
        i16 depth;
        i16 height;
        i16 otherWidth;
        i16 otherDepth;
        i16 otherHeight;
        u8 z;
        u8 otherZ;

        footpad(&width, &depth, &height);
        Item(item).footpad(&otherWidth, &otherDepth, &otherHeight);
        const u16 x = itemX[id];
        const u16 otherX = itemX[item];
        width <<= 4;
        otherWidth <<= 4;
        if (static_cast<ui16>(otherX - otherWidth) < x && static_cast<ui16>(x - width) < otherX) {
            const u16 y = itemY[id];
            const u16 otherY = itemY[item];
            depth <<= 4;
            otherDepth <<= 4;
            if (static_cast<ui16>(otherY - otherDepth) < y && static_cast<ui16>(y - depth) < otherY) {
                z = itemZ[id];
                otherZ = itemZ[item];
                height <<= 3;
                otherHeight <<= 3;
                if (z < static_cast<i16>(otherZ + otherHeight) && otherZ < static_cast<i16>(z + height)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Item::overlapsXY(u16 item) const
{
    if (((itemFlags[id] & ITEM_CONTAINED) == 0) && ((itemFlags[item] & ITEM_CONTAINED) == 0)) {
        i16 width;
        i16 depth;
        i16 height;
        i16 otherWidth;
        i16 otherDepth;
        i16 otherHeight;

        footpad(&width, &depth, &height);
        Item(item).footpad(&otherWidth, &otherDepth, &otherHeight);
        const u16 x = itemX[id];
        const u16 otherX = itemX[item];
        width <<= 4;
        otherWidth <<= 4;
        if (static_cast<ui16>(otherX - otherWidth) < x && static_cast<ui16>(x - width) < otherX) {
            const u16 y = itemY[id];
            const u16 otherY = itemY[item];
            depth <<= 4;
            otherDepth <<= 4;
            if (static_cast<ui16>(otherY - otherDepth) < y && static_cast<ui16>(y - depth) < otherY) {
                return true;
            }
        }
    }
    return false;
}

bool Item::isOn(u16 item) const
{
    i16 width;
    i16 depth;
    i16 height;

    if (overlapsXY(item)) {
        Item(item).footpad(&width, &depth, &height);
        height <<= 3;
        if (itemZ[id] == static_cast<i16>(itemZ[item] + height)) {
            return true;
        }
    }
    return false;
}

bool Item::isCompletelyOn(u16 item) const
{
    if (((itemFlags[id] & ITEM_CONTAINED) == 0) && ((itemFlags[item] & ITEM_CONTAINED) == 0)) {
        i16 width;
        i16 depth;
        i16 height;
        i16 otherWidth;
        i16 otherDepth;
        i16 otherHeight;

        footpad(&width, &depth, &height);
        Item(item).footpad(&otherWidth, &otherDepth, &otherHeight);
        const u16 x = itemX[id];
        const u16 otherX = itemX[item];
        width <<= 4;
        otherWidth <<= 4;
        if (x <= otherX && static_cast<ui16>(x - width) >= static_cast<ui16>(otherX - otherWidth)) {
            const u16 y = itemY[id];
            const u16 otherY = itemY[item];
            depth <<= 4;
            otherDepth <<= 4;
            if (y <= otherY && static_cast<ui16>(y - depth) >= static_cast<ui16>(otherY - otherDepth)) {
                otherHeight <<= 3;
                if (itemZ[id] == static_cast<i16>(itemZ[item] + otherHeight)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Item::isAbove(u16 item) const
{
    i16 width;
    i16 depth;
    i16 height;

    if (overlapsXY(item)) {
        Item(item).footpad(&width, &depth, &height);
        height <<= 3;
        if (itemZ[id] >= static_cast<i16>(itemZ[item] + height)) {
            return true;
        }
    }
    return false;
}

bool Item::isBelow(u16 item) const
{
    i16 width;
    i16 depth;
    i16 height;

    if (overlapsXY(item)) {
        footpad(&width, &depth, &height);
        height <<= 3;
        if (static_cast<i16>(itemZ[id] + height) <= itemZ[item]) {
            return true;
        }
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool Item::lift(i16 delta) const
{
    Item above(0);
    i16 count = 0;
    Sweep sweep;
    i16 left;
    i16 top;
    i16 width;
    i16 depth;
    i16 height;
    ShapeInfo info;
    ItemPoint point;

    typeFlags.infos[shape()].box(width, depth, height);
    const i16 x = itemX[id];
    const i16 y = itemY[id];
    left = x - width + 1;
    top = y - depth + 1;
    AreaSearch search(left, top, x + 0x100, y + 0x100, "$", 1);
    while (search.isValid()) {
        above = search;
        search.next();
        info = typeFlags.infos[above.shape()];
        if (above.isOn(id) && !info.fixed && (info.weight != 0U)) {
            above.push();
            count++;
        }
    }
    if (delta < 0) {
        grab();
    }
    if (!legalMoveToPoint(&lvalue(ItemPoint(itemX[id], itemY[id], itemZ[id] + delta)), 0, 0)) {
        while (count != 0) {
            above.pop();
            count--;
        }
        return false;
    }
    while (count != 0) {
        above.id = etherealTop;
        point = above.point();
        point.z += delta;
        if (sweep.fits(above.shape(), &point, 0)) {
            above.setZ(above.z() + delta);
        } else {
            fall(&above);
        }
        above.pop();
        count--;
    }
    return true;
}

void Item::getPoint(ItemPoint *point) const
{
    point->x = itemX[id];
    point->y = itemY[id];
    point->z = itemZ[id];
}

bool Item::isNpc() const
{
    /* The original fills an NPC it never reads. */
    Npc npc(0);

    npc.id = id;
    return id != 0 && id < 0x100;
}

bool Item::isInNpc() const
{
    u16 item = id;

    while ((item != 0U) && ((Item(item).flags() & ITEM_CONTAINED) != 0)) {
        item = Item(item).container();
        if (Item(item).isNpc()) {
            return true;
        }
    }
    return false;
}

void Item::touch() const
{
    if ((flags() & ITEM_ETHEREAL) == 0) {
        updateItem(id);
        redrawItem(id);
    }
}

void Item::setFlags(u16 flags) const
{
    checkItem(id);
    itemFlags[id] = flags;
}

void Item::orFlags(u16 flags) const
{
    itemFlags[id] |= flags;
}

void Item::andFlags(u16 flags) const
{
    itemFlags[id] &= flags;
}

u16 Item::look()
{
    u16 result;

    if (callEvent(this, 1L << 0, 0, &result, 0)) {
        return result;
    }
    return 0;
}

void Item::setBroken() const
{
    if (isBroken() == 0U) {
        removeTarget(id);
        setFlags(flags() | ITEM_BROKEN);
    }
}

void Item::clearBroken() const
{
    if (isBroken() != 0U) {
        addTarget(id);
        setFlags(flags() & ~ITEM_BROKEN);
    }
}

u8 Item::isBroken() const
{
    return ((flags() & ITEM_BROKEN) != 0) ? 1 : 0;
}

u16 Item::use()
{
    u16 result;

    if (isNpc()) {
        Npc npc(id);
        if (npc.isDead() == 0U) {
            if ((npc.isInCombat() != 0U) && !npc.isSilencer()) {
                return 0;
            }
            if (callEvent(this, 1L << 1, 1, &result, 0)) {
                return result;
            }
            return 0;
        }
    } else {
        if (callEvent(this, 1L << 1, 1, &result, 0)) {
            return result;
        }
        return 0;
    }
    return 0;
}

u16 Item::hit(u16 hitter, i16 force)
{
    u32 argument = force;
    u16 result;

    argument <<= 16;
    argument |= hitter;
    if (callEvent(this, 1L << 5, 5, &result, argument)) {
        return result;
    }
    return 0;
}

u16 Item::gotHit(u16 hitter, i16 force, u8 *handled)
{
    u16 points = 0;
    u8 usecode;
    const u16 classId = classOf(this);

    if ((classes->unitFile != nullptr) && ((classes->info(classId).events & (1L << 6)) != 0)) {
        usecode = 1;
    } else {
        usecode = 0;
    }
    if (hitter == 0x4000) {
        points = force & 0xff;
    }
    if (handled != nullptr) {
        *handled = usecode;
    }
    if (damageTable->isDamageable(id) == 0U) {
        if (damageTable->breakItem(id, points)) {
            killItemProcesses(id, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
        }
        return 0;
    }
    s32 argument = force;
    argument <<= 16;
    argument |= hitter;
    if ((damageTable->takesDamage(shape()) == 0U) || damageTable->wouldBreak(id, points)) {
        if (usecode != 0U) {
            /* The original keeps callEvent's success flag in result, over the
             * event's result, and returns it. */
            u16 result = static_cast<u16>(callEvent(this, 1L << 6, 6, &result, argument));
            if (z() != ETHEREAL_Z) {
                static_cast<void>(damageTable->breakItem(id, points));
            }
            return result;
        }
        receiveHit(this, hitter, force);
    } else {
        static_cast<void>(damageTable->breakItem(id, points));
    }
    return 0;
}

u16 Item::anim()
{
    u16 result;

    if (callEvent(this, 1L << 2, 2, &result, 0)) {
        return result;
    }
    return 0;
}

void Item::grab() const
{
    SurfaceSearch above(id, "$", 1, 3);

    while (above.isValid()) {
        if (above.side == 1) {
            fall(&above);
        } else if (!itemHasType(above.id, RELEASE_TYPE)) {
            new ReleaseProcess(above.id);
        }
        above.next();
    }
}

u16 Item::release()
{
    u16 result;

    if (callEvent(this, 1L << 9, 9, &result, 0)) {
        return result;
    }
    return 0;
}

u16 Item::equip(i16 value)
{
    u16 result;

    if (callEvent(this, 1L << 10, 10, &result, value)) {
        return result;
    }
    return 0;
}

u16 Item::unequip(i16 value)
{
    u16 result;

    if (callEvent(this, 1L << 11, 11, &result, value)) {
        return result;
    }
    return 0;
}

u16 Item::cacheIn()
{
    u16 result;

    if (currentMap == 0U) {
        return 0;
    }
    if (callEvent(this, 1L << 4, 4, &result, 0)) {
        return result;
    }
    return 0;
}

u16 Item::combine()
{
    u16 result;

    if (callEvent(this, 1L << 12, 12, &result, 0)) {
        return result;
    }
    return 0;
}

u16 Item::calledFromAnim()
{
    u16 result;

    if (callEvent(this, 1L << 14, 14, &result, 0)) {
        return result;
    }
    return 0;
}

u16 Item::leaveFastArea()
{
    u16 result;

    setFlags(flags() & ~ITEM_FAST_AREA);
    /* The original asks for the family and drops it. */
    static_cast<void>(family());
    if (((isOccluding() != 0U) || (isTargetable() != 0U)) && (isBroken() == 0U)) {
        removeTarget(id);
    }
    killItemProcesses(id, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    if ((snapProcess != nullptr) && itemShape[id] == SNAP_EGG) {
        snapProcess->removeEgg(id);
    }
    if (currentMap == 0U) {
        return 0;
    }
    if (isNpc() && (Npc(id).isDead() != 0U)) {
        return 0;
    }
    if ((effectsStarted != 0U) && typeFlags.infos[itemShape[id]].sound) {
        stopItemSounds(id);
    }
    if (isNpc() || typeFlags.infos[itemShape[id]].noisy) {
        if (callEvent(this, 1L << 16, 16, &result, 0)) {
            return result;
        }
    }
    return 0;
}

u16 Item::cast(u16 value)
{
    u16 result;

    if (callEvent(this, 1L << 17, 17, &result, value)) {
        return result;
    }
    return 0;
}

u16 Item::justMoved()
{
    u16 result;

    if (callEvent(this, 1L << 18, 18, &result, 0)) {
        return result;
    }
    return 0;
}

/* UNCALLED: nothing calls it. */
u16 Item::unknownEvent14(u16 value)
{
    u16 result;

    if (callEvent(this, 1L << 20, 20, &result, value)) {
        return result;
    }
    return 0;
}

u16 Item::unknown2ff3()
{
    return 0;
}

u16 Item::avatarStoleSomething(u16 value)
{
    u16 result;

    if (isNpc() && (Npc(id).isDead() != 0U)) {
        return 0;
    }
    if (callEvent(this, 1L << 19, 19, &result, static_cast<i16>(value))) {
        return result;
    }
    return 0;
}

bool itemAt(u16 *item, i16 x, i16 y)
{
    const u16 node = nodeAt(x, y);
    DL_ItemNode *entry = static_cast<DL_ItemNode *>(nodeOf(node));

    if ((node != 0U) && entry->kind == NODE_ITEM) {
        *item = entry->item;
        return true;
    }
    *item = 0;
    return false;
}

u16 Item::unknown30a9()
{
    return 0;
}

u16 Item::unknown30b0()
{
    return 0;
}

void Item::unknown30b7() {}

void Item::unknown30bc() {}

bool Item::unknown30c1()
{
    return false;
}

u8 Item::isOccluding() const
{
    return typeFlags.infos[itemShape[id]].occludes;
}

u8 Item::isTargetable() const
{
    return typeFlags.infos[itemShape[id]].targetable;
}

u8 Item::isFixed() const
{
    return typeFlags.infos[itemShape[id]].fixed;
}

/* The last argument is not read. */
bool Item::canReach(u16 item, i16 margin, ItemPoint *point, u8 /*unused*/) const
{
    Item other(item);
    Rect box(0, 0, 0, 0);
    Rect otherBox(0, 0, 0, 0);
    i16 width;
    i16 depth;
    i16 height;
    i16 otherWidth;
    i16 otherDepth;
    i16 otherHeight;

    while ((other.flags() & ITEM_CONTAINED) != 0) {
        item = other.container();
        other = Item(item);
    }
    if (touches(item)) {
        return true;
    }
    typeFlags.infos[shape()].box(width, depth, height);
    typeFlags.infos[other.shape()].box(otherWidth, otherDepth, otherHeight);
    box.right = itemX[id];
    box.bottom = itemY[id];
    box.left = box.right - width - margin;
    box.top = box.bottom - depth - margin;
    if (point != nullptr) {
        otherBox.right = point->x;
        otherBox.bottom = point->y;
    } else {
        otherBox.right = itemX[other.id];
        otherBox.bottom = itemY[other.id];
    }
    otherBox.left = otherBox.right - otherWidth - margin;
    otherBox.top = otherBox.bottom - otherDepth - margin;
    if (box.touches(otherBox)) {
        Sweep sweep;
        ItemPoint start;
        ItemPoint end;
        start = this->point();
        if (point != nullptr) {
            end = *point;
        } else {
            end = other.point();
        }
        if (start.z<end.z &&static_cast<i16>(start.z + height)> end.z) {
            start.z = end.z;
        } else if (start.z < static_cast<i16>(end.z + otherHeight)) {
            end.z = start.z;
        }
        if (sweep.lineOfSight(start, end, item, id) || (sweep.targetFirst != 0U)) {
            return true;
        }
        if (point != nullptr) {
            end = *point;
        } else {
            end = other.point();
        }
        start = this->point();
        start.x -= width >> 1;
        start.y -= depth >> 1;
        end.x -= otherWidth >> 1;
        end.y -= otherDepth >> 1;
        if (height > 16) {
            start.z += height - 8;
        }
        end.z += otherHeight >> 1;
        if (sweep.lineOfSight(start, end, item, id) || (sweep.targetFirst != 0U)) {
            return true;
        }
    }
    return false;
}

bool Item::canReach(u16 item, i16 margin) const
{
    return canReach(item, margin, nullptr, 1);
}

/* UNCALLED: nothing calls it. */
void Item::loadFrame() const
{
    cachedShapeFrame(shape(), frame());
}

i16 Item::distanceTo(u16 item) const
{
    const Item other(item);
    i16 width;
    i16 depth;
    i16 height;
    i16 otherWidth;
    i16 otherDepth;
    i16 otherHeight;
    i16 x;
    i16 y;
    i16 left;
    i16 top;
    i16 otherX;
    i16 otherY;
    i16 otherLeft;
    i16 otherTop;
    i16 distance;
    i16 gap;

    typeFlags.infos[shape()].box(width, depth, height);
    typeFlags.infos[other.shape()].box(otherWidth, otherDepth, otherHeight);
    x = itemX[id];
    y = itemY[id];
    left = x - width;
    top = y - depth;
    otherX = itemX[other.id];
    otherY = itemY[other.id];
    otherLeft = otherX - otherWidth;
    otherTop = otherY - otherDepth;
    distance = 0;
    gap = left - otherX;
    if (gap > distance) {
        distance = gap;
    }
    gap = top - otherY;
    if (gap > distance) {
        distance = gap;
    }
    gap = otherLeft - x;
    if (gap > distance) {
        distance = gap;
    }
    gap = otherTop - y;
    if (gap > distance) {
        distance = gap;
    }
    return distance;
}

i16 Item::distanceToShapeAt(u16 shape, i16 x, i16 y) const
{
    i16 width;
    i16 depth;
    i16 height;
    i16 otherWidth;
    i16 otherDepth;
    i16 otherHeight;
    i16 itemX0;
    i16 itemY0;
    i16 left;
    i16 top;
    i16 otherX;
    i16 otherY;
    i16 otherLeft;
    i16 otherTop;
    i16 distance;
    i16 gap;
    ShapeInfo *info;
    ShapeInfo *otherInfo;

    info = &typeFlags.infos[this->shape()];
    info->box(width, depth, height);
    otherInfo = &typeFlags.infos[shape];
    otherInfo->box(otherWidth, otherDepth, otherHeight);
    itemX0 = itemX[id];
    itemY0 = itemY[id];
    left = itemX0 - width;
    top = itemY0 - depth;
    otherX = x;
    otherY = y;
    otherLeft = otherX - otherWidth;
    otherTop = otherY - otherDepth;
    distance = 0;
    gap = left - otherX;
    if (gap > distance) {
        distance = gap;
    }
    gap = top - otherY;
    if (gap > distance) {
        distance = gap;
    }
    gap = otherLeft - itemX0;
    if (gap > distance) {
        distance = gap;
    }
    gap = otherTop - itemY0;
    if (gap > distance) {
        distance = gap;
    }
    return distance;
}

u16 Item::qualityHigh() const
{
    return (static_cast<s16>(itemQuality[id]) >> 8) & 0xff;
}

void Item::setQualityHigh(u16 value) const
{
    itemQuality[id] &= 0xff;
    itemQuality[id] |= value << 8;
}

u16 Item::qualityLow() const
{
    return itemQuality[id] & 0xff;
}

void Item::setQualityLow(u16 value) const
{
    itemQuality[id] &= 0xff00;
    itemQuality[id] |= value & 0xff;
}

void Item::shoot(ItemPoint *target, u16 speed, u16 gravity) const
{
    const MissileTracker tracker(point(), *target, speed, gravity);

    tracker.launch(id);
}

void Item::playSound(i16 sound) const
{
    if (effectsStarted == 0U) {
        return;
    }
    playItemSound(sound, id);
}

void Item::playLoop(i16 sound) const
{
    if (effectsStarted == 0U) {
        return;
    }
    playItemLoop(sound, id);
}

void Item::cancelSound(i16 sound) const
{
    if (effectsStarted == 0U) {
        return;
    }
    stopItemSound(sound, id);
}

void Item::restartSound(i16 sound) const
{
    if (effectsStarted == 0U) {
        return;
    }
    restartItemSound(sound, id);
}

void Item::stopSound(i16 sound) const
{
    if (effectsStarted == 0U) {
        return;
    }
    stopItemSound(sound, id);
}

void Item::stopSounds() const
{
    if (effectsStarted == 0U) {
        return;
    }
    stopItemSounds(id);
}

u8 Item::isSoundPlaying(i16 sound) const
{
    if (effectsStarted == 0U) {
        return 0;
    }
    return isItemSoundPlaying(sound, id);
}

void Item::unknown3a2e() {}

void Item::setSoundVolume(i16 sound, char volume) const
{
    if (effectsStarted == 0U) {
        return;
    }
    setItemSoundVolume(sound, id, volume);
}

u8 Item::inFastArea() const
{
    return ((flags() & ITEM_FAST_AREA) != 0) ? 1 : 0;
}

bool Item::fits(u16 shape, ItemPoint *point)
{
    if (shape < SHAPES) {
        Sweep sweep;
        if (sweep.fits(shape, point, 0)) {
            return true;
        }
    }
    return false;
}

bool Item::isOnScreen() const
{
    u16 width;
    u16 height;
    s16 originX;
    s16 originY;
    i16 screenX;
    i16 screenY;
    const Rect view(screenViewport->left(), screenViewport->top(), screenViewport->right(), screenViewport->bottom());

    mapToScreen(itemX[id], itemY[id], &screenX, &screenY);
    screenY -= itemZ[id];
    shapeCache->frameMetrics(itemShape[id], frame(), &width, &height, &originX, &originY);
    Rect area(0, 0, width - 1, height - 1);
    area.moveTo(screenX - originX, screenY - originY);
    return view.overlaps(area);
}

/* A movement key was pressed lately (10b0:4437). */
u16 moveKeyDownRecently()
{
    return pendingKeys;
}

/* 10b0:443f */
u8 isNpcType(u16 shape)
{
    if (shape == 0x7fe) {
        return 1;
    }
    const ShapeInfo info = typeFlags.info(shape);
    return info.npc;
}

bool Item::isCentreOn(u16 item) const
{
    i16 width;
    i16 depth;
    i16 height;

    Item(item).footpad(&width, &depth, &height);
    width <<= 4;
    depth <<= 4;
    height <<= 3;
    const u16 x = centreX();
    const u16 y = centreY();
    const u16 right = itemX[item];
    const u16 bottom = itemY[item];
    const u16 left = right - width;
    const u16 top = bottom - depth;
    return x < right && x > left && y < bottom && y > top && itemZ[id] == static_cast<i16>(itemZ[item] + height);
}

/* A weapon type for an NPC of shape on the hardest level (10b0:3a71): its row
 * of the game's STRONGER_WEAPONS, the first type at one chance in chance
 * (always when chance is 0), else the second; DEFAULT_STRONGER_WEAPON for the
 * other shapes. The original switches on the shapes; the rows keep its order
 * and its random draws. */
i16 strongerWeaponType(u16 shape)
{
    for (const StrongerWeapon &row : STRONGER_WEAPONS) {
        if (row.shape == shape) {
            if (row.chance == 0 || oneIn(row.chance)) {
                return row.first;
            }
            return row.second;
        }
    }
    return DEFAULT_STRONGER_WEAPON;
}

u16 Item::createNpc(u16 npcEgg) const
{
    Npc npc(0);
    const Item egg(npcEgg);
    u16 waypointGroup;
    u16 tactic;
    u16 activity0;
    u16 activity1;
    u16 activity2;
    const Npc eggNpc(npcEgg);
    bool armed;
    i16 difficulty;
    u8 unknown12;
    char direction;
    u16 shape;
    u16 weapon1;
    u16 weapon2;
    u16 x;
    u16 y;
    u16 z;
    u16 slot;

    waypointGroup = qualityLow();
    activity1 = qualityHigh();
    activity0 = egg.qualityHigh();
    const i16 map = this->map();
    armed = false;
    if ((map & 4) != 0) {
        armed = true;
    }
    difficulty = (map & 3) + 1;
    activity2 = egg.map();
    i16 number = npcNumber();
    unknown12 = number >> 4;
    direction = number & 0xf;
    const u16 type = egg.npcNumber();
    {
        const NpcType info = npcTypes[type];
        shape = info.shape;
    }
    weapon1 = npcTypes[type].weaponType;
    weapon2 = npcTypes[type].weaponType2;
    const i16 level = controlledNpc.difficulty;
    if (difficulty > level) {
        return 0;
    }
    if (level == 4) {
        weapon1 = strongerWeaponType(shape);
        weapon2 = weapon1;
    }
    x = itemX[id];
    y = itemY[id];
    z = itemZ[id];
    Item weapon(0);
    number = (direction + direction + 4) & 0xf;
    if (npc.create(shape, number)) {
        if (EGG_MARKS_ARMED) {
            if (armed) {
                npcs[npc.id].armed = 1;
            } else {
                npcs[npc.id].armed = 0;
            }
        }
        tactic = 0;
        npc.setCombatTactic(tactic);
        npc.popToCoords(x, y, z);
        npc.setDirection(direction * 2);
        npc.setHome(ItemPoint(x, y, z));
        npc.setUnknown12(unknown12);
        npc.setDefaultActivity0(activity0);
        npc.setDefaultActivity1(activity1);
        npc.setDefaultActivity2(activity2);
        npc.orFlags(ITEM_DISPOSABLE);
        npc.setWaypointGroup(waypointGroup);
        if (!WEAPONS_ONLY_FOR_CONTAINERS || npc.family() == FAMILY_CONTAINER) {
            if (armed && (weapon2 != 0U)) {
                slot = npc.weaponSlot();
                if (slot != 0U) {
                    weapon.create(weaponTypes[weapon2].shape, 0);
                    weapon.popToCoords(0, 0, 0);
                    npc.setEquip(slot, weapon.id);
                    npc.setActiveWeapon(slot);
                }
            } else if (weapon1 != 0U) {
                slot = npc.weaponSlot();
                if (slot != 0U) {
                    weapon.create(weaponTypes[weapon1].shape, 0);
                    weapon.popToCoords(0, 0, 0);
                    npc.setEquip(slot, weapon.id);
                    npc.setActiveWeapon(slot);
                }
            }
        }
        return npc.id;
    }
    halt();

    /* The original ends here without a value (BCC warns; halt does not
     * return). */
}

bool isVisible(u16 x, u16 y, u8 z, u16 shape, u16 frame)
{
    u16 width;
    u16 height;
    s16 originX;
    s16 originY;
    i16 screenX;
    i16 screenY;
    const Rect view(screenViewport->left(), screenViewport->top(), screenViewport->right(), screenViewport->bottom());

    mapToScreen(x, y, &screenX, &screenY);
    screenY -= z;
    shapeCache->frameMetrics(shape, frame, &width, &height, &originX, &originY);
    Rect area(0, 0, width - 1, height - 1);
    area.moveTo(screenX - originX, screenY - originY);
    area.intersect(view);
    return static_cast<s16>(width) >> 1 <= area.width() && static_cast<s16>(height) >> 1 <= area.height();
}

bool Item::isMostlyOnScreen() const
{
    u16 width;
    u16 height;
    s16 originX;
    s16 originY;
    i16 screenX;
    i16 screenY;
    const Rect view(screenViewport->left(), screenViewport->top(), screenViewport->right(), screenViewport->bottom());

    mapToScreen(itemX[id], itemY[id], &screenX, &screenY);
    screenY -= itemZ[id];
    shapeCache->frameMetrics(itemShape[id], frame(), &width, &height, &originX, &originY);
    Rect area(0, 0, width - 1, height - 1);
    area.moveTo(screenX - originX, screenY - originY);
    area.intersect(view);
    const bool tallEnough = ON_SCREEN_WHOLE_HEIGHT ? static_cast<ui16>(area.height()) == height
                                                   : static_cast<s16>(height) >> 1 <= area.height();
    return static_cast<s16>(width) >> 1 <= area.width() && tallEnough;
}
