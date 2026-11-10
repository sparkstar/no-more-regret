/* 1270:0000-0252, file name unknown: redraws a viewport in shades of gray. See
 * gray.h. */
#include "common.h"
#include "gray.h"
#include "palette.h"
#include "viewport.h"

/* Turns the first levels entries of palette into a gray ramp and each pixel
 * into the level of its color's luminance (1270:0000). */
/* UNCALLED: nothing calls it. */
void toGray(Viewport *viewport, BasePalette *palette, u8 levels)
{
    const Palette yiq(*palette);

    yiq.toYiq();
    if (levels > 64) {
        levels = 64;
    }
    for (i16 i = 0; i < levels; i++) {
        palette->data[i].red = palette->data[i].green = palette->data[i].blue = i * (64 / levels);
    }
    u8 *pixel = nullptr;
    for (i16 y = 0; y < static_cast<i16>(viewport->bottom() - viewport->top() + 1); y++) {
        pixel = viewport->address(0, y);
        for (i16 x = 0; x < static_cast<i16>(viewport->right() - viewport->left() + 1); x++, pixel++) {
            *pixel = yiq.data[*pixel].red / (64 / levels);
        }
    }
}

void toFixedGray(Viewport *viewport, BasePalette *palette)
{
    const Palette yiq(*palette);

    yiq.toYiq();
    u8 *pixel = nullptr;
    u8 level = 0;
    for (i16 y = 0; y < static_cast<i16>(viewport->bottom() - viewport->top() + 1); y++) {
        pixel = viewport->address(0, y);
        for (i16 x = 0; x < static_cast<i16>(viewport->right() - viewport->left() + 1); x++, pixel++) {
            level = yiq.data[*pixel].red >> 2;
            if (level < 8) {
                *pixel = level;
            } else {
                *pixel = level + 0xf0;
            }
        }
    }
}
