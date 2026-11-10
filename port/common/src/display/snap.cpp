/* Code14 (1068, file name unknown): the process that keeps the camera on the
 * snap eggs. */
#include "gamestate.h"
#include "snap.h"
#include "camera.h"
#include "control.h"
#include "crusys.h"
#include "dispatch.h"
#include "gumpshap.h"
#include "item.h"
#include "itemdata.h"
#include "itemutil.h"
#include "kernel.h"
#include "msgbox.h"
#include "npc.h"
#include "type.h"

SnapProcess *&snapProcess = gameGlobal<SnapProcess *>(nullptr);

SnapProcess::SnapProcess()
{
    nameProcess(pid, "SnapProcess");
    makeSystem();
    snapProcess = this;
    clearEggs();
}

void SnapProcess::clearEggs()
{
    area.left = 0;
    area.top = 0;
    area.right = -1;
    area.bottom = -1;
    currentEgg = 0;
    nextEgg = 0;
    eggCount = 0;
    holdsCamera = 0;
    memset(eggs, 0, sizeof(eggs));
}

void SnapProcess::run()
{
    if (worldActive == 0U) {
        holdsCamera = 0;
        return;
    }
    if (area.right < 0) {
        findEgg();
        return;
    }
    if (!npcInArea()) {
        holdsCamera = 0;
        findEgg();
        return;
    }
    holdsCamera = 1;
}

/* The quality's high byte gives the half size in 32 units, x in the high
 * nibble; the map and NPC number bytes, read as signed, move the area. */
Rect SnapProcess::eggArea(u16 egg)
{
    Rect eggBounds;
    const Item eggItem(egg);
    const u8 quality = eggItem.qualityHigh();
    const i16 xRange = quality >> 4;
    const i16 yRange = quality & 0xf;
    const i16 xOffset = static_cast<char>(eggItem.map());
    const i16 yOffset = static_cast<char>(eggItem.npcNumber());

    eggBounds.left = eggBounds.right = itemX[eggItem.id];
    eggBounds.top = eggBounds.bottom = itemY[eggItem.id];
    eggBounds.left -= xRange << 5;
    eggBounds.top -= yRange << 5;
    eggBounds.right += xRange << 5;
    eggBounds.bottom += yRange << 5;
    eggBounds.left += xOffset << 5;
    eggBounds.top += yOffset << 5;
    eggBounds.right += xOffset << 5;
    eggBounds.bottom += yOffset << 5;
    return eggBounds;
}

void SnapProcess::addEgg(u16 egg)
{
    i16 i;

    /* The box waits where it is: the list holds the snap eggs in the fast
     * area, and the games' maps never have more than 47 in one. */
    if (eggCount == SNAP_EGGS) {
        runModalGump(new MessageBoxGump(NewGumpId(), nullptr, "Too many snap eggs!", guiColors->face), -1);
        return;
    }
    for (i = 0; i < eggCount; i++) {
        if (eggs[i] == egg) {
            return;
        }
    }
    for (i = 0; i < eggCount; i++) {
        if (eggs[i] == 0) {
            eggs[i] = egg;
            return;
        }
    }
    eggs[eggCount++] = egg;
}

void SnapProcess::removeEgg(u16 egg)
{
    i16 i;

    if (eggCount == 0) {
        return;
    }
    if (currentEgg == egg) {
        currentEgg = 0;
        area.left = 0;
        area.right = -1;
    } else if (nextEgg == egg) {
        nextEgg = 0;
    }
    for (i = 0; i < eggCount; i++) {
        if (eggs[i] == egg) {
            eggs[i] = 0;
            if (static_cast<i16>(eggCount - 1) == i) {
                while ((eggCount != 0) && eggs[static_cast<i16>(eggCount - 1)] == 0) {
                    eggCount--;
                }
                return;
            }
        }
    }
}

bool SnapProcess::npcInArea()
{
    i16 width;
    i16 depth;
    i16 height;

    if (currentEgg == 0U) {
        return false;
    }
    const Npc npc(controlledNpcNumber());
    ShapeInfo *info = &typeFlags.infos[itemShape[npc.id]];
    info->box(width, depth, height);
    const i16 x = itemX[npc.id];
    const i16 y = itemY[npc.id];
    const i16 z = itemZ[npc.id];
    i16 eggZ;
    const Item egg(currentEgg);
    eggZ = itemZ[egg.id];
    const Rect r = eggArea(egg.id);
    const bool inside = x >= r.left && static_cast<i16>(x - width + 1) <= r.right && y >= r.top &&
                        static_cast<i16>(y - depth + 1) <= r.bottom;
    if (inside && (static_cast<i16>(eggZ + 0x30) < z || static_cast<i16>(eggZ - 0x30) > z)) {
        return false;
    }
    if (inside) {
        return true;
    }
    return false;
}

void SnapProcess::findEgg()
{
    i16 eggZ;
    i16 width = 0;
    i16 depth = 0;
    i16 height;
    bool inside;

    if (eggCount == 0) {
        return;
    }
    const Npc npc(controlledNpcNumber());
    currentEgg = 0;
    if (nextEgg != 0U) {
        currentEgg = nextEgg;
        nextEgg = 0;
        area = nextArea;
        const i16 x = itemX[npc.id];
        const i16 y = itemY[npc.id];
        const i16 z = itemZ[npc.id];
        const Item egg(currentEgg);
        eggZ = itemZ[egg.id];
        const Rect r = area;
        /* Original behaviour: width and depth are only set on the other
         * path, so this test reads whatever the frame held (0 in the port);
         * every branch after it returns, so its result changes nothing. */
        inside = x >= r.left && static_cast<i16>(x - width + 1) <= r.right && y >= r.top &&
                 static_cast<i16>(y - depth + 1) <= r.bottom;
        if (inside) {
            if (static_cast<i16>(eggZ + 0x30) < z) {
                return;
            }
            if (static_cast<i16>(eggZ - 0x30) > z) {
                return;
            }
        }
        return;
    }
    ShapeInfo *info = &typeFlags.infos[itemShape[npc.id]];
    info->box(width, depth, height);
    const i16 x = itemX[npc.id];
    const i16 y = itemY[npc.id];
    const i16 z = itemZ[npc.id];
    i16 i;
    for (i = 0; i < eggCount; i++) {
        if (eggs[i] != 0U) {
            Item egg(eggs[i]);
            eggZ = itemZ[egg.id];
            const Rect r = eggArea(egg.id);
            inside = x >= r.left && static_cast<i16>(x - width + 1) <= r.right && y >= r.top &&
                     static_cast<i16>(y - depth + 1) <= r.bottom;
            if (inside && (static_cast<i16>(eggZ + 0x30) < z || static_cast<i16>(eggZ - 0x30) > z)) {
                inside = false;
            }
            if (inside) {
                i16 screenX;
                i16 screenY;
                u16 mapX;
                u16 mapY;
                i16 j;
                area = r;
                currentEgg = egg.id;
                mapToScreen(itemX[egg.id], itemY[egg.id], &screenX, &screenY);
                screenY -= itemZ[egg.id];
                screenToMap(screenX, screenY, &mapX, &mapY);
                moveCamera(mapX, mapY, 0, 0);
                holdsCamera = 1;
                /* Original behaviour: the other eggs are read and nothing is
                 * done with them. */
                for (j = 0; j < eggCount; j++) {
                    if (j != i) {
                        egg.id = eggs[j];
                    }
                }
                return;
            }
        }
    }
}

void SnapProcess::load(File *file)
{
    Process::load(file);
    snapProcess = this;
}

void SnapProcess::showAreas()
{
    i16 i;

    if (eggCount == 0) {
        return;
    }
    for (i = 0; i < eggCount; i++) {
        if (eggs[i] != 0U) {
            const Item egg(eggs[i]);
            const u8 quality = egg.qualityHigh();
            const i16 xRange = quality >> 4;
            const i16 yRange = quality & 0xf;
            const i16 xOffset = static_cast<char>(egg.map());
            const i16 yOffset = static_cast<char>(egg.npcNumber());
            showBox(itemX[egg.id] + (xRange << 5) + (xOffset << 5), itemY[egg.id] + (yRange << 5) + (yOffset << 5),
                    itemZ[egg.id], xRange * 2, yRange * 2, 9);
        }
    }
}
