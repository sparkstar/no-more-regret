/* Written by port/tools/make_unified_values.py from regret/src/input/joydrv_game.h and remorse/src/input/joydrv_game.h:
 * the one program's run-time values; do not edit. */

#ifndef PORT_JOYDRV_GAME_H
#define PORT_JOYDRV_GAME_H
#include <span>

extern bool CALIBRATE_ONCE;
extern std::array<i16, 16> JOYSTICK_ACTIONS;
extern i16 JOY_CONTROL;
extern i16 JOY_ALT;
extern i16 JOY_SHIFT;
extern i16 JOY_KEYS;
extern bool JOY_JUMPS;
extern i16 JOY_JUMP;
extern bool JOY_REPEATED_KEYS;
extern bool JOY_SHIFTS_TAKE_CONTROL;
extern bool ARROWS_PRESSED_AFTER_FIRE;
#endif
