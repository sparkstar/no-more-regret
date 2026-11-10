/* No Regret's colours of the port's widgets (common/src/gump/widgets.h):
 * misc.pal's, as the options gump's art has them. */
#ifndef PORT_WIDGETS_GAME_H
#define PORT_WIDGETS_GAME_H
#include "common.h"

/* A choice chosen: the face and the edge of the art's green letters
 * (gumps.flx 0x29 SOUND VOLUME, 0x1f ON's frame 0), and of its red ones for
 * a warning (0x20 OFF's frame 0). */
inline constexpr u8 WIDGET_FACE = 144;
inline constexpr u8 WIDGET_EDGE = 143;
inline constexpr u8 WIDGET_WARNING_FACE = 1;
inline constexpr u8 WIDGET_WARNING_EDGE = 224;
/* A choice not chosen: the same buttons' frame 1. */
inline constexpr u8 WIDGET_DIM_FACE = 148;
inline constexpr u8 WIDGET_DIM_EDGE = 154;
inline constexpr u8 WIDGET_DIM_WARNING_FACE = 2;
inline constexpr u8 WIDGET_DIM_WARNING_EDGE = 230;
/* The frame round the row the keys stand on (the options gump's, 13b8). */
inline constexpr u8 WIDGET_FOCUS = 0x74;
#endif
