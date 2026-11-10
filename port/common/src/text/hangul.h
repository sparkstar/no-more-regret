/* The Hangul font of the translations (the game's text in other languages):
 * the 2350 syllables of KS X 1001 in 9x9 pixels, from port/common/fonts/ko.png
 * (Galmuri9's glyphs, trimmed to their cells) through port/tools/make_text.py
 * (hangul_glyphs.cpp). The game's own fonts have the other characters. */
#ifndef PORT_HANGUL_H
#define PORT_HANGUL_H
#include "common.h"

constexpr int HANGUL_GLYPH_COUNT = 2350;
/* A glyph's width and height in pixels. */
constexpr int HANGUL_SIZE = 9;
/* The width a syllable takes in a line: its glyph and a pixel between. */
constexpr int HANGUL_ADVANCE = (HANGUL_SIZE + 1);

/* A syllable's glyph: each row's pixels as bits, bit 8 the leftmost. */
struct HangulGlyph {
    char32_t code;
    u16 rows[HANGUL_SIZE];
};

/* By code point (hangul_glyphs.cpp). */
extern const HangulGlyph HANGUL_GLYPHS[HANGUL_GLYPH_COUNT];

/* The glyph of a syllable; nullptr for one the font does not have. */
const HangulGlyph *hangulGlyph(char32_t code);
/* Draws a glyph with its top left corner at (x, y) of the draw viewport,
 * its pixels in colour ink, clipped to the viewport (the terminal and the
 * subtitles draw it in their font's ink). */
void drawHangul(const HangulGlyph &glyph, s16 x, s16 y, u8 ink);
#endif
