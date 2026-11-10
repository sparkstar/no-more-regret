/* No Remorse's values of the alarm (common/src/item/alert.cpp). */
#ifndef PORT_ALERT_GAME_H
#define PORT_ALERT_GAME_H

/* Turning the alarm on or off does nothing when it already is: No Regret's
 * (setAlertActive, clearAlertActive). */
inline constexpr bool ALERT_ONLY_ON_CHANGE = false;
/* The alarm swaps the doors of shapes 660h-663h too: No Regret's. */
inline constexpr bool ALERT_SWAPS_LIGHTS = false;
#endif
