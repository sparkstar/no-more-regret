/* No Remorse's values of the joystick driver (common/src/input/joydrv.cpp). */
#ifndef PORT_JOYDRV_GAME_H
#define PORT_JOYDRV_GAME_H

/* The joystick is calibrated once (joystickUncalibrated, written to
 * crusader.cfg), and in a dialog from the options (calibrateJoystick's
 * gump): No Regret's; No Remorse's calibrates it in text mode each time. */
inline constexpr bool CALIBRATE_ONCE = false;
/* The buttons' actions as they start, crusader.cfg's button lines over them
 * (joystickActions). */
inline constexpr std::array<i16, 16> JOYSTICK_ACTIONS = {{0, 1, 3, 2, 4, 9, -1, 6, -1, -1, -1, -1, -1}};
/* The actions' numbers (joystickActions, JoystickDriver::run): Ctrl, Alt and
 * Shift held, and the first of the keys for the top modal gump. */
inline constexpr i16 JOY_CONTROL = 1;
inline constexpr i16 JOY_ALT = 2;
inline constexpr i16 JOY_SHIFT = 3;
inline constexpr i16 JOY_KEYS = 4;
/* Action JOY_JUMP jumps: No Regret's; No Remorse's has no such action. */
inline constexpr bool JOY_JUMPS = false;
inline constexpr i16 JOY_JUMP = -1;
/* Actions JOY_KEYS + 5 and + 6 send w and o, again each 14h ticks while held:
 * No Regret's; No Remorse's JOY_KEYS + 5 sends q, w, i or o as the stick turns left, right, up or down. */
inline constexpr bool JOY_REPEATED_KEYS = false;
/* Ctrl and Alt held give the control back to the player's NPC: No
 * Regret's. */
inline constexpr bool JOY_SHIFTS_TAKE_CONTROL = false;
/* The arrows sent as the fire button is released are presses: No Remorse's
 * (with its fix); No Regret's sends them released (kept). */
inline constexpr bool ARROWS_PRESSED_AFTER_FIRE = true;
#endif
