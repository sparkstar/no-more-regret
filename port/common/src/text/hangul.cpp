/* The Hangul font of the translations. See hangul.h. */
#include "hangul.h"
#include "viewport.h"

#include <algorithm>

const HangulGlyph *hangulGlyph(char32_t code)
{
    const HangulGlyph *end = HANGUL_GLYPHS + HANGUL_GLYPH_COUNT;
    const HangulGlyph *found = std::lower_bound(
        HANGUL_GLYPHS, end, code, [](const HangulGlyph &glyph, char32_t key) { return glyph.code < key; });
    return found != end && found->code == code ? found : nullptr;
}

void drawHangul(const HangulGlyph &glyph, s16 x, s16 y, u8 ink)
{
    const ViewportPixels target = drawViewport->pixels();
    for (s16 row = 0; row < HANGUL_SIZE; row++) {
        const s32 line = y + row;
        if (line < target.top || line > target.bottom) {
            continue;
        }
        for (s16 column = 0; column < HANGUL_SIZE; column++) {
            const s32 place = x + column;
            if (place < target.left || place > target.right ||
                ((glyph.rows[row] >> (HANGUL_SIZE - 1 - column) & 1) == 0)) {
                continue;
            }
            target.pixels[target.rows[line] + place] = ink;
        }
    }
}
