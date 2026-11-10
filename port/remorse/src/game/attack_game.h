/* No Remorse's values of the attack (common/src/game/attack.h). */
#ifndef PORT_ATTACK_GAME_H
#define PORT_ATTACK_GAME_H
#include "common.h"

/* Attack's BCC size. */
inline constexpr u16 ATTACK_SIZE = 0x98;
/* The feint lasts 8 to 17 seconds (feintEnd), and an NPC may feint again
 * (Attack::startFeint); No Regret's feints once (feinted). */
inline constexpr bool TIMED_FEINT = true;
/* The boss: at the hardest level it waits BOSS_FIRE_DELAY ticks between
 * shots with the UV-9, GL-303 and AR-7 (Attack::Attack). */
inline constexpr u16 BOSS_SHAPE = 0x3ac;
inline constexpr i16 BOSS_FIRE_DELAY = 0xf;
/* The boss fires a GL-303 grenade at every fifth and sixth shot (bossShots;
 * fireAtFrame): No Regret's. */
inline constexpr bool BOSS_GRENADES = false;
/* The attack without a tactic is No Regret's (Attack::attack): not No Remorse's, whose NPCs speak while they attack,
 * feint for a while and walk after a target too high. */
inline constexpr bool ATTACK_NO_REGRET = false;
#endif
