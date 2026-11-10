/* FORMAT.C (13b0:0000-0418, No Remorse only): text laid out in screen
 * rectangles for the bark gump. */
#ifndef PORT_FORMAT_H
#define PORT_FORMAT_H
#include "common.h"
#include "rect.h"

/* Copies text into formatBuffer (at most 3FFh bytes) broken into lines that
 * fit the rectangles one after another in font: '\n' ends a line, FFh a
 * rectangle; '%' is four spaces, '~' ends the line and '*' the rectangle.
 * When the text ends on a rectangle's first line within half its width,
 * the rectangle ends before it (back to the last line break) become line
 * breaks and that line break the rectangle's end (13b0:0000). */
/* UNCALLED: nothing calls it. */
char *formatText(char *text, i16 font, Rect *rects, i16 count);
#endif
