/* Shared GUI drawing interfaces: the panel routines, with the headers of the
 * GUI colours, shapes and font. Viewport drawing lives in viewport.h. */
#ifndef PORT_GUMPDRAW_H
#define PORT_GUMPDRAW_H
#include "newgump.h"
#include "viewport.h"
#include "gumpfont.h"
#include "gumpshap.h"
/* Code96 panels: a frame and fill, with a raised or pressed bevel. The
 * members never read the object; callers use a local one. */
class Panel {
public:
    void flat(Rect bounds, u8 color);    /* 12f8:0000 */
    void raised(Rect bounds, u8 color);  /* 12f8:004b */
    void pressed(Rect bounds, u8 color); /* 12f8:01a5 */
};
#endif
