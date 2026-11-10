/* No Remorse's values of the activities (common/src/game/activity.h). */
#ifndef PORT_ACTIVITY_GAME_H
#define PORT_ACTIVITY_GAME_H
#include "common.h"

/* Activity's BCC size; Stand and Guard add nothing, Pace 4 bytes, Loiter 7. */
inline constexpr u16 ACTIVITY_SIZE = 0x42;
/* Surrender's BCC size (the constructor asks for 45h bytes, 1100:0391: soundPlayed). */
inline constexpr u16 SURRENDER_SIZE = 0x45;
/* An activity times its NPC's idle actions (interval, lastTime; Activity): not No Remorse's. */
inline constexpr bool IDLE_TIMING = false;
/* An activity or attack that stands its NPC puts the weapon away (weaponDrawn;
 * Activity, Attack). */
inline constexpr bool STANDING_PUTS_WEAPON_AWAY = true;
/* A surrendering NPC that sees the controlled NPC speaks once, by its shape
 * (Surrender::run). */
inline constexpr bool SURRENDER_SPEAKS_ONCE = true;
#endif
