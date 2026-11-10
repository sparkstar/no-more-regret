/* DTABLE.C (Code39, 1130:0000-062c, No Regret's addresses): the tables of
 * the weapons, their ammunition, the bombs and the inventory items, with
 * their names and gump frames, and the NPC types, read from npc.dat in
 * dtable.flx. The tables are the game's (<game>/src/game/dtable_game.cpp). */
#include "gamestate.h"
#include <cstdlib>
#include "dtable.h"
#include "flex.h"
#include "filespec.h"
#include "gamefile.h"

void readNpcTypes();

namespace {
u8 &dtableLoaded = gameGlobal<u8>(0);
}
NpcType *&npcTypes = gameGlobal<NpcType *>(nullptr);
namespace {
const char *dtableFile = "dtable.flx";
}

void loadDtable()
{
    npcTypes = new NpcType[NPC_TYPES];
    memset(npcTypes, 0, NPC_TYPES * sizeof(NpcType));
    dtableLoaded = 1;
    readNpcTypes();
}

/* The test is the wrong way round: the types are freed only when they were
 * never loaded, so they never are. */
void freeDtable()
{
    if (dtableLoaded == 0U) {
        dtableLoaded = 0;
        if (npcTypes != nullptr) {
            delete[] npcTypes;
            npcTypes = nullptr;
        }
    }
}

void readNpcTypes()
{
    Flex *flex = new Flex(makePath(nullptr, staticDirectory, dtableFile, nullptr), FILE_READ, -1);
    flex->readObject(0, npcTypes, static_cast<ui16>(NPC_TYPES * sizeof(NpcType)), 0);
    delete flex;
}

i16 npcType(u16 shape)
{
    for (i16 i = 0; i < NPC_TYPES; i++) {
        if (npcTypes[i].shape == shape) {
            return i;
        }
    }
    return -1;
}

namespace {
/* The index of shape in the tables of the weapons (1130:0145) and the bombs
 * (0373, below), 0 when it is not there. */
i16 weaponIndex(u16 shape)
{
    if (shape == 0U) {
        return 0;
    }
    for (i16 i = 0; i < WEAPON_TYPES; i++) {
        if (weaponTypes[i].shape == shape) {
            return i;
        }
    }
    return 0;
}
}

u16 clipSize(u16 shape)
{
    const i16 i = weaponIndex(shape);
    const u16 type = weaponTypes[i].ammoType;
    return ammoTypes[type].clipSize;
}

u16 shotEnergy(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponTypes[i].shotEnergy;
}

u16 fireSound(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponTypes[i].fireSound;
}

u16 shotDelay(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponTypes[i].shotDelay;
}

i16 ammoIndex(u16 shape)
{
    for (i16 i = 0; i < AMMO_TYPES; i++) {
        if (ammoTypes[i].shape == shape) {
            return i;
        }
    }
    return 0;
}

u16 ammoFrame(u16 shape)
{
    const i16 i = ammoIndex(shape);
    return ammoTypes[i].frame;
}

/* The fields of the tables of the ammunition (1130:025f) and the bombs
 * (03e6, 0405). */
/* UNCALLED: nothing calls it. */
u16 unknown025f(u16 shape)
{
    const i16 i = ammoIndex(shape);
    return ammoTypes[i].unknown04;
}

u16 ammoShape(u16 shape)
{
    const i16 i = weaponIndex(shape);
    const u16 type = weaponTypes[i].ammoType;
    return ammoTypes[type].shape;
}

const char *ammoName(u16 shape)
{
    const i16 i = ammoIndex(shape);
    return ammoNames[i];
}

u16 burstSize(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponTypes[i].burstSize;
}

u8 smallWeapon(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponTypes[i].small;
}

u16 weaponFrame(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponTypes[i].frame;
}

const char *weaponName(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponNames[i];
}

u16 reloadSound(u16 shape)
{
    const i16 i = weaponIndex(shape);
    return weaponTypes[i].reloadSound;
}

namespace {
i16 bombIndex(u16 shape)
{
    for (i16 i = 0; i < BOMB_TYPES; i++) {
        if (bombTypes[i].shape == shape) {
            return i;
        }
    }
    return 0;
}
}

u16 bombFrame(u16 shape)
{
    const i16 i = bombIndex(shape);
    return bombTypes[i].frame;
}

const char *bombName(u16 shape)
{
    const i16 i = bombIndex(shape);
    return bombNames[i];
}

/* UNCALLED: nothing calls it. */
u16 unknown03e6(u16 shape)
{
    const i16 i = bombIndex(shape);
    return bombTypes[i].unknown02;
}

/* UNCALLED: nothing calls it. */
u16 unknown0405(u16 shape)
{
    const i16 i = bombIndex(shape);
    return bombTypes[i].unknown04;
}

i16 inventoryIndex(u16 shape)
{
    for (i16 i = 0; i < INVENTORY_TYPES; i++) {
        if (inventoryTypes[i].shape == shape) {
            return i;
        }
    }
    return 0;
}

u16 inventoryFrame(u16 shape)
{
    const i16 i = inventoryIndex(shape);
    return inventoryTypes[i].frame;
}

const char *inventoryName(u16 shape)
{
    const i16 i = inventoryIndex(shape);
    return inventoryNames[i];
}

/* One of the weapons of the NPC type of shape, at random; nothing calls it
 * (1130:0497). The pointer to the type is never set: it adds the type's offset
 * to whatever the stack held, not to npcTypes. */
/* UNCALLED: nothing calls it. */
u8 randomWeapon(u16 shape)
{
    const i16 type = npcType(shape);
    i16 count;
    i16 choice;
    NpcType *npc;
#pragma GCC diagnostic push
    /* The original's: the pointer is never set (above); nothing calls it. */
#pragma GCC diagnostic ignored "-Wuninitialized"
    npc = npc + type;
#pragma GCC diagnostic pop
    if (npc->weapons[2] != 0U) {
        count = 3;
    } else if (npc->weapons[1] != 0U) {
        count = 2;
    } else if (npc->weapons[0] != 0U) {
        count = 1;
    } else {
        return 0;
    }
    choice = borlandRandom(count);
    return npc->weapons[choice];
}

i16 weaponOf(u16 shape)
{
    const i16 i = weaponIndex(shape);
    if (weaponTypes[i].fireType == 0U) {
        halt();
    } else {
        return weaponTypes[i].fireType;
    }
    return 0;
}

/* The name of the item of shape and its gump frame, from the table of its
 * family: "INVALID" and 0 for none (1130:056a, 05dc). */
const char *pickupName(u16 shape)
{
    if (weaponIndex(shape) != 0) {
        return weaponName(shape);
    }
    if (inventoryIndex(shape) != 0) {
        return inventoryName(shape);
    }
    if (bombIndex(shape) != 0) {
        return bombName(shape);
    }
    if (ammoIndex(shape) != 0) {
        return ammoName(shape);
    }
    return "INVALID";
}

u16 pickupFrame(u16 shape)
{
    u16 frame = weaponFrame(shape);
    if (frame == 0U) {
        frame = inventoryFrame(shape);
        if (frame == 0U) {
            frame = bombFrame(shape);
            if (frame == 0U) {
                frame = ammoFrame(shape);
            }
        }
    }
    return frame;
}
