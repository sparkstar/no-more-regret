/* No Regret's values of the items (common/src/item/item.cpp). */
#ifndef PORT_ITEM_GAME_H
#define PORT_ITEM_GAME_H
#include <array>
#include "common.h"

/* The crates the shape information does not mark (Item::isCrate): none, No
 * Regret's marks them all. */
inline constexpr std::array<u16, 0> CRATE_SHAPES{};
/* An item is mostly on the screen with half its width and half its height
 * there (Item::isMostlyOnScreen). */
inline constexpr bool ON_SCREEN_WHOLE_HEIGHT = false;
/* A dead NPC of the shapes that pass the test (576h, 58Fh, 596h, 59Ch)
 * stands and starts its activity again when it enters the fast area
 * (Item::enterFastArea): No Remorse's (each game's own). */
inline constexpr bool DEAD_NPCS_RESTART = false;
/* The weapon types of the NPCs on the hardest level by their shapes, and of
 * the others (strongerWeaponType). */
inline constexpr std::array<StrongerWeapon, 14> STRONGER_WEAPONS = {
    StrongerWeapon{0x1b4, 4, 13, 9},  StrongerWeapon{0x528, 3, 9, 8},   StrongerWeapon{0x371, 3, 9, 10},
    StrongerWeapon{0x4d1, 2, 4, 11},  StrongerWeapon{0x383, 3, 10, 7},  StrongerWeapon{0x4e6, 3, 5, 11},
    StrongerWeapon{0x385, 4, 8, 9},   StrongerWeapon{0x338, 3, 5, 7},   StrongerWeapon{0x2cb, 2, 3, 7},
    StrongerWeapon{0x308, 2, 10, 11}, StrongerWeapon{0x57a, 2, 13, 15}, StrongerWeapon{0x30c, 2, 4, 15},
    StrongerWeapon{0x319, 2, 8, 9},   StrongerWeapon{0x5e2, 0, 14, 14}};
inline constexpr i16 DEFAULT_STRONGER_WEAPON = 15;
/* An NPC made from its egg keeps the egg's armed flag (NpcData::armed), and
 * gets its weapon only when its shape is of the container family
 * (Item::createNpc). */
inline constexpr bool EGG_MARKS_ARMED = true;
inline constexpr bool WEAPONS_ONLY_FOR_CONTAINERS = true;
#endif
