/* No Regret's values of the attack (common/src/game/attack.h). */
#ifndef PORT_ATTACK_GAME_H
#define PORT_ATTACK_GAME_H
#include "common.h"

/* Attack's BCC size. */
inline constexpr u16 ATTACK_SIZE = 0x9c;
/* The feint lasts 8 to 17 seconds (feintEnd), and an NPC may feint again
 * (Attack::startFeint): No Remorse's; No Regret's feints once (feinted). */
inline constexpr bool TIMED_FEINT = false;
/* The boss: at the hardest level it waits BOSS_FIRE_DELAY ticks between
 * shots with the UV-9, GL-303 and AR-7 (Attack::Attack). */
inline constexpr u16 BOSS_SHAPE = 0x5b1;
inline constexpr i16 BOSS_FIRE_DELAY = 0x1e;
/* The boss fires a GL-303 grenade at every fifth and sixth shot (bossShots;
 * fireAtFrame). */
inline constexpr bool BOSS_GRENADES = true;
/* The attack without a tactic is No Regret's (Attack::attack): its own feint once, the attack target, kneeling
 * attackers that do not walk after a target too high, its alarm and hurt rules. */
inline constexpr bool ATTACK_NO_REGRET = true;
#endif
