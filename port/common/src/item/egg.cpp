/* Code21 (No Regret's 10a0:0000-0b4b, No Remorse's 1090:0000-0b4c; file
 * name unknown, among the..\ITEM modules): eggs, teleport eggs and the
 * process that watches an egg. Both games' are No Regret's: an egg hatches
 * for the NPC the player controls (No Remorse's original: the Silencer, even
 * while the player controls another), and a teleport does not empty the
 * caches (No Remorse's did, compactMemory, which plays no part in the
 * port). */
#include "egg.h"
#include "gamestate.h"
#include "waits.h"
#include "crusys.h"
#include "item.h"
#include "npc.h"
#include "type.h"
#include "itemcach.h"
#include "itemfind.h"
#include "scrit.h"
#include "kernel.h"
#include "uprocess.h"
#include "control.h"
#include "camera.h"
#include "gravity.h"
#include "sndsrc.h"
#include "dsfxman.h"
#include "sndmgr.h"
#include "sndcache.h"

u8 &showEggs = gameGlobal<u8>(0);
/* A teleport to the old place is under way (1480:12df). */
u16 &teleporting = gameGlobal<u16>(0);

NpcHold::NpcHold()
{
    wasHeld = controlledNpc.held;
    controlledNpc.held = 1;
}

void NpcHold::run()
{
    if (wasHeld == 0U) {
        controlledNpc.held = 0;
    }
    terminate(0);
}

Teleporter::Teleporter(u16 eggNumber)
{
    nameProcess(pid, "Teleporter");
    Item eggItem(eggNumber);
    Item *teleportEgg = &eggItem;
    map = teleportEgg->eggId();
    this->egg = teleportEgg->qualityLow();
    const u8 hold = teleportEgg->qualityHigh();
    start(hold);
}

Teleporter::Teleporter(u16 targetMap, u16 targetEgg, u8 hold)
{
    this->map = targetMap;
    this->egg = targetEgg;
    start(hold);
    if (effectsStarted != 0U) {
        resetSoundSources();
        stopAllEffects();
        resetChannels();
        compactBlocks();
    }
}

Teleporter::Teleporter(u16 targetEgg, u8 hold)
{
    map = currentMap;
    this->egg = targetEgg;
    start(hold);
}

void Teleporter::start(u8 hold)
{
    killItem(controlledNpc.id, FIRST_ITEM_TYPE);
    if ((hold != 0U) && (teleporting == 0U)) {
        block(new NpcHold);
    }
}

void Teleporter::run()
{
    if (teleporting != 0U) {
        terminate(0);
        return;
    }
    const u16 x = itemX[controlledNpc.id];
    const u16 y = itemY[controlledNpc.id];
    const u8 z = itemZ[controlledNpc.id];
    const u16 oldMap = npcMap(&controlledNpc.id);
    u8 sameMap = 1;
    if (map != currentMap) {
        sameMap = 0;
        if (worldActive != 0U) {
            controlledNpc.teleport(0, 0, 0, map);
        } else {
            moveCamera(0, 0, 0, map);
        }
        if (waitOpened()) {
            /* The egg after the map's loading's wait. */
            giveRest([this, x, y, z, oldMap, sameMap]() {
                arrive(x, y, z, oldMap, sameMap);
                return false;
            });
            return;
        }
    }
    arrive(x, y, z, oldMap, sameMap);
}

/* Teleporter::run's on the map: the egg, or back where it was. */
void Teleporter::arrive(u16 x, u16 y, u8 z, u16 oldMap, u8 sameMap)
{
    const AreaSearch search(0, 0, -1, -1,
                            LoopScript(SCRIPT_FAMILY, SCRIPT_INT, FAMILY_TELEPORT_EGG, SCRIPT_EQUAL, SCRIPT_FRAME,
                                       SCRIPT_INT, 1, SCRIPT_EQUAL, SCRIPT_AND, SCRIPT_Q, SCRIPT_INT, 0xff, SCRIPT_AND,
                                       SCRIPT_INT, egg, SCRIPT_EQUAL, SCRIPT_AND, SCRIPT_END),
                            -1);
    if (search.isValid()) {
        Support support;

        if (worldActive != 0U) {
            controlledNpc.teleport(itemX[search.id], itemY[search.id], itemZ[search.id], map);
        } else {
            moveCamera(itemX[search.id], itemY[search.id], itemZ[search.id], map);
        }
        const ItemPoint point(itemX[search.id], itemY[search.id], itemZ[search.id]);
        testSupport(&support, 1, point, 1, controlledNpc.id);
        if (support.standing() == 0U) {
            fall(&controlledNpc);
        }
    } else {
        if (sameMap == 0U) {
            teleporting = 1;
            if (worldActive != 0U) {
                controlledNpc.teleport(x, y, z, oldMap);
            } else {
                moveCamera(x, y, z, oldMap);
            }
            if (waitOpened()) {
                giveRest([this]() {
                    teleporting = 0;
                    debugPrintf(50, "No destination for this egg!");
                    arrived();
                    return false;
                });
                return;
            }
            teleporting = 0;
        }
        debugPrintf(50, "No destination for this egg!");
    }
    arrived();
}

/* Teleporter::run's end. */
void Teleporter::arrived()
{
    if (hasFlags(PROCESS_DEAD) == 0U) {
        terminate(0);
    }
}

/* Intrinsics 86h and 9Ah: the teleport's process (10a0:04c4, 04ed). */
u16 teleportToEgg(u16 map, u16 egg, u8 hold)
{
    Teleporter *teleporter = new Teleporter(map, egg, hold);

    return teleporter->pid;
}

u16 teleportToEgg(u16 egg, u8 hold)
{
    Teleporter *teleporter = new Teleporter(egg, hold);

    return teleporter->pid;
}

i16 Item::eggXRange() const
{
    return itemNpc[id] >> 4;
}

i16 Item::eggYRange() const
{
    return itemNpc[id] & 0xf;
}

void Item::setEggXRange(u8 range) const
{
    itemNpc[id] &= 0xf;
    itemNpc[id] |= range << 4;
}

void Item::setEggYRange(u8 range) const
{
    itemNpc[id] &= 0xf0;
    itemNpc[id] |= range & 0xf;
}

u16 Item::eggId() const
{
    return itemMap[id];
}

void Item::setEggId(u8 newEggId) const
{
    itemMap[this->id] = newEggId;
}

u16 Item::hatch()
{
    Teleporter *teleporter;
    u16 result;

    if (family() == FAMILY_TELEPORT_EGG) {
        teleporter = new Teleporter(id);
        return teleporter->pid;
    }
    if (callEvent(this, 1L << 7, 7, &result, 0)) {
        return result;
    }
    return 0;
}

u16 Item::unhatch()
{
    u16 result;

    if (callEvent(this, 1L << 21, 21, &result, 0)) {
        return result;
    }
    return 0;
}

EggHatcher::EggHatcher(u16 egg)
{
    /* Not read. */
    const ItemRef hatched(egg);

    setItem(egg);
    setType(EGG_HATCHER_TYPE);
    inRange = 0;
    nameProcess(pid, "EggHatcher");
}

void EggHatcher::run()
{
    ItemRef egg(item);
    const Item avatar(controlledActor);
    i16 left;
    i16 top;
    i16 right;
    i16 bottom;
    u16 x;
    u16 y;
    i16 z;
    i16 width;
    i16 depth;
    i16 height;
    const ItemRef npc(avatar.id);
    ShapeInfo *info = &typeFlags.infos[itemShape[npc.id]];
    i16 eggZ;
    bool inside;

    info->box(width, depth, height);
    x = itemX[npc.id];
    y = itemY[npc.id];
    z = itemZ[npc.id];
    eggZ = itemZ[egg.id];
    if (egg.family() == FAMILY_MONSTER_EGG) {
        return;
    }
    left = right = itemX[egg.id];
    top = bottom = itemY[egg.id];
    left -= egg.eggXRange() << 5;
    top -= egg.eggYRange() << 5;
    right += egg.eggXRange() << 5;
    bottom += egg.eggYRange() << 5;
    inside = x >= static_cast<ui16>(left) && static_cast<ui16>(x - width + 1) <= static_cast<ui16>(right) &&
             y >= static_cast<ui16>(top) && static_cast<ui16>(y - depth + 1) <= static_cast<ui16>(bottom);
    if (inside && (static_cast<i16>(eggZ + 48) < z || static_cast<i16>(eggZ - 48) > z)) {
        inside = false;
    }
    if (inRange != 0U) {
        if (!inside) {
            inRange = 0;
            if (egg.family() != FAMILY_MONSTER_EGG) {
                egg.unhatch();
                return;
            }
        }
    } else if (inside && (worldActive != 0U)) {
        inRange = 1;
        if (egg.family() != FAMILY_MONSTER_EGG) {
            egg.hatch();
        }
    }
}

void EggHatcher::show()
{
    const ItemRef egg(item);

    if (egg.family() == FAMILY_MONSTER_EGG) {
        showBox(itemX[egg.id] + (egg.eggXRange() << 5), egg.y() + (egg.eggXRange() << 5), egg.z(), egg.eggXRange() << 1,
                egg.eggXRange() << 1, 0xd);
        return;
    }
    if ((showEggs != 0U) && (((inRange == 0U) && (worldActive != 0U)) || (worldActive == 0U))) {
        showBox(itemX[egg.id] + (egg.eggXRange() << 5), itemY[egg.id] + (egg.eggYRange() << 5), itemZ[egg.id],
                egg.eggXRange() << 1, egg.eggYRange() << 1, 0xd);
    }
}
