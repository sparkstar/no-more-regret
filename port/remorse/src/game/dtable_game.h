/* No Remorse's inventory items' types and the shapes only it has (common/src/game/dtable.h). */
#ifndef PORT_DTABLE_GAME_H
#define PORT_DTABLE_GAME_H

inline constexpr int INVENTORY_TYPES = 0xc;

/* The bombs (No Remorse's table has every one), named as the table names
 * them (bombNames): */
inline constexpr int SHAPE_CONCUSSION_GRENADE = 0x34e;
inline constexpr int SHAPE_NERVE_GAS_GRENADE = 0x34f;
inline constexpr int SHAPE_BLAST_PAC = 0x39b;
/* The credits are in no table: the displays name them themselves. */
inline constexpr int SHAPE_CREDITS = 0x4ed; /* No Regret's is 112h */

/* The tables (dtable_game.cpp): the ammunition (WORLD.C's pickUp reads the
 * clip sizes itself), the weapons, the inventory items and the bombs, and
 * the names of each. */
extern AmmoType (&ammoTypes)[AMMO_TYPES];
extern WeaponType (&weaponTypes)[WEAPON_TYPES];
extern InventoryType (&inventoryTypes)[INVENTORY_TYPES];
extern BombType (&bombTypes)[BOMB_TYPES];
extern const char *(&weaponNames)[14];
extern const char *(&inventoryNames)[12];
extern const char *(&ammoNames)[6];
extern const char *(&bombNames)[9];
#endif
