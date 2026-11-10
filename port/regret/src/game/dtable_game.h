/* No Regret's inventory items' types and the shapes only it has (common/src/game/dtable.h). */
#ifndef PORT_DTABLE_GAME_H
#define PORT_DTABLE_GAME_H

inline constexpr int INVENTORY_TYPES = 0x13;

/* The shapes in the tables, named as the tables name them (weaponNames,
 * ammoNames, inventoryNames). The weapons: */
inline constexpr int SHAPE_BK_16 = 0x5f6;
inline constexpr int SHAPE_LNR_81 = 0x5f5;
inline constexpr int SHAPE_XP_5 = 0x198;
/* Their ammunition: */
inline constexpr int SHAPE_BK_16_CLIP = 0x615;
inline constexpr int SHAPE_LNR_81_CLIP = 0x614;
/* The inventory items: */
inline constexpr int SHAPE_DISRUPTER = 0x616;
inline constexpr int SHAPE_SPIDER_MINE = 0x11e;
inline constexpr int SHAPE_DATA_PICK = 0x613;
inline constexpr int SHAPE_MINE_DET = 0x61c;
inline constexpr int SHAPE_PORTABLE_BETTY = 0x63e;
inline constexpr int SHAPE_RADIATION_SHIELD = 0x191;
inline constexpr int SHAPE_VIR_IMAGER = 0x4ed;
/* The bombs' table has no shape for the concussion and nerve gas grenades.
 * The credits are in no table: the displays name them themselves. */
inline constexpr int SHAPE_CREDITS = 0x112;

/* The tables (dtable_game.cpp): the ammunition (WORLD.C's pickUp reads the
 * clip sizes itself, 1480:2590), the weapons (1480:2614), the inventory
 * items and the bombs, and the names of each. */
extern AmmoType (&ammoTypes)[AMMO_TYPES];
extern WeaponType (&weaponTypes)[WEAPON_TYPES];
extern InventoryType (&inventoryTypes)[INVENTORY_TYPES];
extern BombType (&bombTypes)[BOMB_TYPES];
extern const char *(&weaponNames)[18];
extern const char *(&inventoryNames)[19];
extern const char *(&ammoNames)[8];
extern const char *(&bombNames)[8];
#endif
