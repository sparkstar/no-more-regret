/* No Regret's values of the activities (common/src/game/activity.h). */
#ifndef PORT_ACTIVITY_GAME_H
#define PORT_ACTIVITY_GAME_H
#include "common.h"

/* Activity's BCC size; Stand and Guard add nothing, Pace 4 bytes, Loiter 7. */
inline constexpr u16 ACTIVITY_SIZE = 0x48;
/* Surrender's BCC size. */
inline constexpr u16 SURRENDER_SIZE = 0x4a;
/* An activity times its NPC's idle actions (interval, lastTime; Activity). */
inline constexpr bool IDLE_TIMING = true;
/* An activity or attack that stands its NPC puts the weapon away (weaponDrawn;
 * Activity, Attack): No Remorse's. */
inline constexpr bool STANDING_PUTS_WEAPON_AWAY = false;
/* A surrendering NPC that sees the controlled NPC speaks once, by its shape
 * (Surrender::run); No Regret's moans its hurt sounds now and then. */
inline constexpr bool SURRENDER_SPEAKS_ONCE = false;
#endif
