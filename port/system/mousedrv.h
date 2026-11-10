/* The mouse driver (INT 33h) the port stands in for: the position, its
 * limits and the buttons, which the scripted input and the window's
 * mouse move. */
#ifndef PORT_MOUSEDRV_H
#define PORT_MOUSEDRV_H
#include "common.h"

struct MouseDriver {
    i16 x, y;
    i16 left, right, top, bottom;
    u16 buttons;
    bool operator==(const MouseDriver &) const = default;
};
extern MouseDriver mouseDriver;

/* AX=0: the driver's state at a reset, the cursor in the middle of the
 * virtual screen of 640x200, and no handler. */
void resetMouseDriver();
/* A mouse event: the place in the driver's coordinates and the buttons
 * (bits 0-2: left, right, middle), for the driver's next interrupt. */
void mouseInput(i16 x, i16 y, u16 buttons);
/* The same for the mouse's motion in mickeys, which the driver turns into
 * pixels at its speed (AX=0Fh). */
void mouseMotion(i16 mickeysX, i16 mickeysY, u16 buttons);
#endif
