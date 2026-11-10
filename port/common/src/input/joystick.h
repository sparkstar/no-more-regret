/* SYSTEM\JOYSTICK.ASM (12d0:0000-00fc): the joystick and PIT timer routines
 * (joystick.asm), the parts C code uses; in C++ over the platform's game
 * port (joystick.cpp). */
#ifndef PORT_JOYSTICK_H
#define PORT_JOYSTICK_H
#include "common.h"

extern "C" {
/* PIT channel 0's reload value, which the original's reads of the
 * counter take; the port's clock knows its own (clockTickPhase). */
extern u32 &timerPeriod;
/* Bits of the axes that still answer. */
extern u8 &joystickAxes;
/* The first of sampleJoystick's two readings, and the buttons, 1 pressed. */
extern u32 &joystickFirstX;
extern u32 &joystickFirstY;
extern u8 &joystickButtons;
/* Reads the joystick twice. */
void sampleJoystick();
}
#endif
