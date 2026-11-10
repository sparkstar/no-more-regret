/* DTABLE.C (Code39, 1130:0000-062c, No Regret's addresses): the NPC types
 * (npc.dat in dtable.flx) and the tables of the weapons, their ammunition, the
 * bombs and the inventory items: the shapes, the gump frames that show them and
 * their names. The tables are the game's (<game>/src/game/dtable_game.cpp). */
#ifndef PORT_DTABLE_H
#define PORT_DTABLE_H
#include "common.h"

/* An NPC type, 8Eh bytes. */
struct NpcType {
    u8 unknown00[2];
    u16 maxHp;
    u8 unknown04[0x14];
    /* The weapon types (weaponTypes) it carries. */
    u16 weaponType;
    u16 weaponType2;
    u8 unknown1c[2];
    u16 defaultActivity0;
    u8 unknown20[0x1e];
    u16 shape;
    u16 defaultActivity1;
    u16 defaultActivity2;
    u8 unknown44[3];
    /* The weapons an NPC of the type may get (randomWeapon). */
    u8 weapons[3];
    u8 unknown4a[0x44];
};
ASSERT_SIZE(NpcType, 0x8e);

constexpr int NPC_TYPES = 0x40;
constexpr int AMMO_TYPES = 0x10;
constexpr int WEAPON_TYPES = 0x15;
constexpr int BOMB_TYPES = 9;
/* The shapes both games have, named as the tables name them (weaponNames,
 * ammoNames, inventoryNames, bombNames); each game's others and the credits
 * are its own (dtable_game.h). The weapons: */
constexpr int SHAPE_BA_40 = 0x32e;
constexpr int SHAPE_BA_41 = 0x32f;
constexpr int SHAPE_PA_21 = 0x330;
constexpr int SHAPE_EM_4 = 0x38c;
constexpr int SHAPE_SG_A1 = 0x332;
constexpr int SHAPE_RP_22 = 0x333;
constexpr int SHAPE_RP_32 = 0x334;
constexpr int SHAPE_AR_7 = 0x38e;
constexpr int SHAPE_GL_303 = 0x388;
constexpr int SHAPE_PA_31 = 0x38a;
constexpr int SHAPE_PL_1 = 0x38d;
constexpr int SHAPE_AC_88 = 0x38b;
constexpr int SHAPE_UV_9 = 0x386;
/* Their ammunition: */
constexpr int SHAPE_BA_41_CLIP = 0x33d;
constexpr int SHAPE_SG_A1_CLIP = 0x33e;
constexpr int SHAPE_RP_32_CLIP = 0x33f;
constexpr int SHAPE_AR_7_CLIP = 0x340;
constexpr int SHAPE_GL_303_CLIP = 0x341;
/* The inventory items: */
constexpr int SHAPE_MEDICAL_KIT = 0x351;
constexpr int SHAPE_DATA_LINK = 0x4d4;
constexpr int SHAPE_INHIBITOR = 0x52d;
constexpr int SHAPE_IONIC_SHIELD = 0x52e;
constexpr int SHAPE_ENERGY_CUBE = 0x582;
constexpr int SHAPE_PLASMA_SHIELD = 0x52f;
constexpr int SHAPE_DET_PAC = 0x55f;
constexpr int SHAPE_GRAVITON_SHIELD = 0x530;
constexpr int SHAPE_CHEMICAL_BATTERY = 0x3a2;
constexpr int SHAPE_FUSION_BATTERY = 0x3a3;
constexpr int SHAPE_FISSION_BATTERY = 0x3a4;
/* The bombs: */
constexpr int SHAPE_GRENADE = 0x343;
constexpr int SHAPE_EMP_GRENADE = 0x350;
constexpr int SHAPE_SPIDER_BOMB = 0x560;
constexpr int SHAPE_LAND_MINE = 0x39a;
constexpr int SHAPE_FUSION_PAC = 0x39c;
/* The keycard, in no table: the displays name it themselves. */
constexpr int SHAPE_KEYCARD = 0x111;

/* An ammunition type, 8 bytes. */
struct AmmoType {
    u16 shape;
    u16 clipSize;
    u16 unknown04;
    /* The gump frame that shows it (ammoFrame). */
    u16 frame;
};
ASSERT_SIZE(AmmoType, 8);

/* A weapon type, 12h bytes. */
struct WeaponType {
    u16 shape;
    /* The sound of a shot (fireAtFrame plays it). */
    u16 fireSound;
    /* The timeout of the controller after a shot of the controlled NPC. */
    u16 shotDelay;
    /* Its ammunition, an index in the ammunition table. */
    u16 ammoType;
    /* The shots of one pull of the trigger: 2 for the BA-41 and the RP-22, 3
     * for the RP-32, else 1 (the controller's setShots). */
    u16 burstSize;
    u8 shotEnergy;
    u8 unknown0b;
    /* 1 for a small weapon (the BA-40, BA-41 and PA-21), which the NPC holds
     * with the _SMALL animations (Npc::usesSmallAnims). */
    u8 small;
    /* The gump frame that shows it (weaponFrame). */
    u16 frame;
    u8 reloadSound;
    /* Its fire type (firetype.h). */
    u16 fireType;
};
ASSERT_SIZE(WeaponType, 0x12);

/* An inventory item's type and a bomb's. */
struct InventoryType {
    u16 shape;
    u16 frame;
};
ASSERT_SIZE(InventoryType, 4);
struct BombType {
    u16 shape;
    u16 unknown02;
    u16 unknown04;
    u16 frame;
};
ASSERT_SIZE(BombType, 8);

extern NpcType *&npcTypes; /* 1480:28fb */

/* Reads npc.dat into npcTypes (1130:0000, 0075) and frees it (0041). */
void loadDtable();
void freeDtable();
/* The npc.dat type of an NPC of shape (1130:010a). */
i16 npcType(u16 shape);
/* The rounds a weapon of shape holds (1130:0180). */
u16 clipSize(u16 shape);
/* The energy a shot of the weapon of shape takes (1130:01ad). */
u16 shotEnergy(u16 shape);
/* The shape of the ammunition of the weapon of shape, 0 for an energy
 * weapon (1130:027e). */
u16 ammoShape(u16 shape);
/* The fields of the weapon type of shape: fireSound, shotDelay, burstSize and
 * small (1130:01cf, 01ef, 02ce, 02ee). */
u16 fireSound(u16 shape);
u16 shotDelay(u16 shape);
u16 burstSize(u16 shape);
u8 smallWeapon(u16 shape);
/* The index of shape in the tables of the ammunition (1130:020f) and the
 * inventory items (0424), 0 when it is not there. */
i16 ammoIndex(u16 shape);
i16 inventoryIndex(u16 shape);
/* The sound of reloading the weapon of shape (1130:0351). */
u16 reloadSound(u16 shape);
/* The fire type of the weapon of shape; halts when it has none
 * (1130:0528). */
i16 weaponOf(u16 shape);
/* The frame of the gump shape that shows an item of shape in the pickup and
 * status displays, and the item's name, for each family that has one: the
 * weapons (1130:030e, 032e), the ammunition (0240, 02ab), the bombs (03a4,
 * 03c3) and the inventory items (0455, 0474). */
u16 weaponFrame(u16 shape);
const char *weaponName(u16 shape);
u16 ammoFrame(u16 shape);
const char *ammoName(u16 shape);
u16 bombFrame(u16 shape);
const char *bombName(u16 shape);
u16 inventoryFrame(u16 shape);
const char *inventoryName(u16 shape);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
u16 unknown025f(u16 shape);
/* UNCALLED: nothing calls it. */
u16 unknown03e6(u16 shape);
/* UNCALLED: nothing calls it. */
u16 unknown0405(u16 shape);
/* UNCALLED: nothing calls it. */
u8 randomWeapon(u16 shape);
/* The name of the item of shape and its gump frame, from the table of its
 * family: "INVALID" and 0 for none (1118:056a, 05dc). No Remorse's weasel's
 * shop shows them (WEZZY.C). */
const char *pickupName(u16 shape);
u16 pickupFrame(u16 shape);
/* The inventory items' types, the shapes only one game has and the tables
 * are the game's (dtable_game.h). */
#include "dtable_game.h"
#endif
