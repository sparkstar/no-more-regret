/* No Remorse's values of the items (common/src/item/item.cpp). */
#ifndef PORT_ITEM_GAME_H
#define PORT_ITEM_GAME_H
#include <array>
#include "common.h"

/* The crates the shape information does not mark (Item::isCrate): No
 * Remorse's marks none. */
inline constexpr std::array<u16, 8> CRATE_SHAPES = {0x432, 0x5a0, 0x1fd, 0x1fe, 0x8f, 0x59f, 0x2b3, 0x2ca};
/* An item is mostly on the screen with half its width and its whole height
 * there (Item::isMostlyOnScreen; No Regret's: half its height). */
inline constexpr bool ON_SCREEN_WHOLE_HEIGHT = true;
/* A dead NPC of the shapes that pass the test (576h, 58Fh, 596h, 59Ch)
 * stands and starts its activity again when it enters the fast area
 * (Item::enterFastArea) (No Regret's: only living NPCs; each game's own). */
inline constexpr bool DEAD_NPCS_RESTART = true;
/* The weapon types of the NPCs on the hardest level by their shapes, and of
 * the others (strongerWeaponType). */
inline constexpr std::array<StrongerWeapon, 13> STRONGER_WEAPONS = {
    StrongerWeapon{0x2fd, 4, 12, 3}, StrongerWeapon{0x319, 4, 12, 3}, StrongerWeapon{0x384, 3, 5, 10},
    StrongerWeapon{0x1b4, 4, 13, 9}, StrongerWeapon{0x528, 3, 9, 8},  StrongerWeapon{0x371, 3, 9, 10},
    StrongerWeapon{0x4d1, 2, 4, 11}, StrongerWeapon{0x383, 3, 10, 7}, StrongerWeapon{0x4e6, 3, 5, 11},
    StrongerWeapon{0x385, 4, 8, 9},  StrongerWeapon{0x338, 3, 5, 7},  StrongerWeapon{0x2cb, 2, 3, 7},
    StrongerWeapon{0x3ac, 2, 9, 13}};
inline constexpr i16 DEFAULT_STRONGER_WEAPON = 7;
/* An NPC made from its egg keeps the egg's armed flag (NpcData::armed), and
 * gets its weapon only when its shape is of the container family: both No Regret's
 * (Item::createNpc). */
inline constexpr bool EGG_MARKS_ARMED = false;
inline constexpr bool WEAPONS_ONLY_FOR_CONTAINERS = false;
#endif
