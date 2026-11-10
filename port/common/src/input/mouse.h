/* The mouse of the game: MOUSE.C (11d0:0546-066e, file name unknown), the
 * mouse driver's (INT 33h) calls, is the platform's input in the port
 * (input.h: inputMouseStart for initMouse, inputMouseStop for
 * removeMouseHandler, inputMousePlace, inputMouseXRange and
 * inputMouseYRange); the handler the game gives it is MOUSECB.ASM's
 * (11d0:0514), in kernel.cpp. */
#ifndef PORT_MOUSE_H
#define PORT_MOUSE_H
#include "common.h"

/* The mouse's events to the kernel's mouseEvent, unless mouseHeld. */
void mouseCallback(u16 events, u16 buttons, u16 x, u16 y);
#endif
