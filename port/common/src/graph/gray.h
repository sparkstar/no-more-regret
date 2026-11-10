/* 1270:0000-0252 (no file name): redraws a viewport in shades of gray by the
 * luminance of its palette. */
#ifndef PORT_GRAY_H
#define PORT_GRAY_H
#include "palette.h"
#include "viewport.h"

/* The same with the 16 fixed grays of entries 0-7 and F8h-FFh (1270:0157). */
void toFixedGray(Viewport *viewport, BasePalette *palette);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void toGray(Viewport *viewport, BasePalette *palette, u8 levels);
#endif
