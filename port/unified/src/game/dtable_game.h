/* Written by port/tools/make_unified_values.py from regret/src/game/dtable_game.h and remorse/src/game/dtable_game.h:
 * the one program's run-time values; do not edit. */

#ifndef PORT_DTABLE_GAME_H
#define PORT_DTABLE_GAME_H
#include <span>

extern int INVENTORY_TYPES;
extern int SHAPE_CREDITS;
extern std::span<AmmoType> ammoTypes;
extern std::span<WeaponType> weaponTypes;
extern std::span<InventoryType> inventoryTypes;
extern std::span<BombType> bombTypes;
extern std::span<const char *> weaponNames;
extern std::span<const char *> inventoryNames;
extern std::span<const char *> ammoNames;
extern std::span<const char *> bombNames;
#endif
