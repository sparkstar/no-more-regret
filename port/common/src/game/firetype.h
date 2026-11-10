/* FIRETYPE.C (Code39, 1130:062d-069e, No Regret's addresses): the fire types,
 * what each kind of shot does. The types are the game's (firetype_game.cpp). */
#ifndef PORT_FIRETYPE_H
#define PORT_FIRETYPE_H
#include "common.h"

/* A fire type, 11h bytes; the note in front of the table names the fields
 * (FTYPE, LOW DMG, HIGH DMG, RANGE, NUM SHOTS, SHIELD COST, SHIELD MASK,
 * ACCURATE, CELLS/RND, RND DUR, NEAR SPRITE). */
struct FireType {
    u16 type;
    u16 lowDamage;
    u16 highDamage;
    char range;
    char shots;
    u16 shieldCost;
    u8 shieldMask;
    u8 accurate;
    i16 cellsPerRound;
    i16 roundDuration;
    u8 nearSprite;
};
ASSERT_SIZE(FireType, 0x11);

/* The fire types, by the weapons that fire them (the weapon table's
 * WeaponType::fireType, the same in both games) and by what the code does
 * with the others: an item's explosion (explode, the Bobo boomer's),
 * electricity (the Silencer shocked) and No Regret's boss's grenades
 * (fireAtFrame). The rest stay numbers: 8 the usecode's own hits and kills
 * (receiveHit's), 21 its PU_GUN's; 9, 12, 18 and 19 not traced. The types
 * from 10h are No Regret's. */
constexpr int FIRE_RP = 1; /* RP-22, RP-32 */
constexpr int FIRE_SG_A1 = 2;
constexpr int FIRE_GL_303 = 3;
constexpr int FIRE_EXPLOSION = 4;
constexpr int FIRE_PA = 5; /* PA-21, PA-31 */
constexpr int FIRE_PL_1 = 6;
constexpr int FIRE_ELECTRIC = 7;
constexpr int FIRE_AR_7 = 10;
constexpr int FIRE_BA = 11; /* BA-40, BA-41 */
constexpr int FIRE_AC_88 = 13;
constexpr int FIRE_UV_9 = 14;
constexpr int FIRE_EM_4 = 15;
constexpr int FIRE_BK_16 = 16;
constexpr int FIRE_LNR_81 = 17;
constexpr int FIRE_XP_5 = 20;
constexpr int FIRE_BOSS_GRENADE = 22;

/* FIRE_TYPES, the table's types, and the table (firetype_game.h). */
#include "firetype_game.h"

/* The entry of type; halts when there is none (1130:062d). */
FireType *fireType(u16 type);
/* A damage of type, between its lowDamage and highDamage (1130:0674). */
i16 damageOf(i16 type);
#endif
