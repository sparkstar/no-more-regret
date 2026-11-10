/* No Remorse's colours of the port's widgets (common/src/gump/widgets.h):
 * misc.pal's, as the options gump's art has them. */
#ifndef PORT_WIDGETS_GAME_H
#define PORT_WIDGETS_GAME_H
#include "common.h"

/* A choice chosen: the face and the edge of the art's green letters
 * (gumps.flx 0x29 SOUND VOLUME, 0x1f ON's frame 0), and of its red ones for
 * a warning (0x20 OFF's frame 0). */
inline constexpr u8 WIDGET_FACE = 12;
inline constexpr u8 WIDGET_EDGE = 28;
inline constexpr u8 WIDGET_WARNING_FACE = 14;
inline constexpr u8 WIDGET_WARNING_EDGE = 26;
/* A choice not chosen: the same buttons' frame 1. */
inline constexpr u8 WIDGET_DIM_FACE = 45;
inline constexpr u8 WIDGET_DIM_EDGE = 139;
inline constexpr u8 WIDGET_DIM_WARNING_FACE = 94;
inline constexpr u8 WIDGET_DIM_WARNING_EDGE = 16;
/* The frame round the row the keys stand on (the options gump's, 13c8). */
inline constexpr u8 WIDGET_FOCUS = 0xc;
#endif
