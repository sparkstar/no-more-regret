/* No Regret's values of the joystick driver (common/src/input/joydrv.cpp). */
#ifndef PORT_JOYDRV_GAME_H
#define PORT_JOYDRV_GAME_H

/* The joystick is calibrated once (joystickUncalibrated, written to
 * crusader.cfg), and in a dialog from the options (calibrateJoystick's
 * gump). */
inline constexpr bool CALIBRATE_ONCE = true;
/* The buttons' actions as they start, crusader.cfg's button lines over them
 * (joystickActions). */
inline constexpr std::array<i16, 16> JOYSTICK_ACTIONS = {{0, 10, 2, 7, 11, -1, 8, 5, 1, 9, -1, -1, -1}};
/* The actions' numbers (joystickActions, JoystickDriver::run): Ctrl, Alt and
 * Shift held, and the first of the keys for the top modal gump. */
inline constexpr i16 JOY_CONTROL = 2;
inline constexpr i16 JOY_ALT = 3;
inline constexpr i16 JOY_SHIFT = 4;
inline constexpr i16 JOY_KEYS = 5;
/* Action JOY_JUMP jumps: No Regret's. */
inline constexpr bool JOY_JUMPS = true;
inline constexpr i16 JOY_JUMP = 1;
/* Actions JOY_KEYS + 5 and + 6 send w and o, again each 14h ticks while held:
 * No Regret's. */
inline constexpr bool JOY_REPEATED_KEYS = true;
/* Ctrl and Alt held give the control back to the player's NPC: No
 * Regret's. */
inline constexpr bool JOY_SHIFTS_TAKE_CONTROL = true;
/* The arrows sent as the fire button is released are presses: No Remorse's
 * (with its fix); No Regret's sends them released (kept). */
inline constexpr bool ARROWS_PRESSED_AFTER_FIRE = false;
#endif
