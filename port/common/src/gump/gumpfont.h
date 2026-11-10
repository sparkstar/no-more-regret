/* 12b0:0000-0522, file name unknown: the base class of the GUI fonts, the
 * current font and text drawn on the draw viewport. A module of the build
 * directory: its data lies between FILEBOX.C's and GUMPSHAP.C's (1480:5c2c),
 * and INSTALL.EXE links it right before GUMPSHAP.C; REGRET.EXE's code segment
 * sits among the library's. */
#ifndef PORT_GUMPFONT_H
#define PORT_GUMPFONT_H
#include "common.h"
#include "rect.h"

class GumpFont {
public:
    /* The pen: where the next glyph goes. */
    s16 x, y;
    /* Added to the width and the height of each glyph (CAFONT.C). */
    s16 charSpacing, lineSpacing;

    GumpFont();
    virtual void drawGlyph(i16 x, i16 y, u8 ch) = 0;
    virtual ~GumpFont();
    /* The size of text as measure gives it. */
    virtual u16 textHeight(const char *text);
    virtual i16 textWidth(const char *text);
    /* The widest line of text and the height of its lines. */
    virtual void measure(const char *text, s16 *width, s16 *height);
    virtual s16 glyphHeight(u8 ch) = 0;
    virtual s16 glyphWidth(u8 ch) = 0;
    virtual void glyphSize(u8 ch, s16 *width, s16 *height) = 0;
    virtual s16 lineHeight() = 0;
    void moveTo(s16 px, s16 py)
    {
        x = px;
        y = py;
    }
    /* Draws ch at the pen and moves the pen past it. A newline goes down a
     * line and back to the left edge of the draw viewport, a return only back. */
    void putChar(u8 ch);
    void putChar(s16 px, s16 py, u8 ch)
    {
        moveTo(px, py);
        putChar(ch);
    }
    void putText(const char *text);
    /* Puts the glyphs of text that end within width of the pen; returns how
     * many it put. */
    i16 putClipped(s16 width, const char *text);
    /* printf-style text at (x,y), clipped to width, or at the pen. The text is
     * formatted into formatBuffer unless format is formatBuffer. */
    [[gnu::format(printf, 4, 5)]] void draw(s16 x, s16 y, const char *format, ...);
    [[gnu::format(printf, 5, 6)]] void drawClipped(s16 x, s16 y, s16 width, const char *format, ...);
    [[gnu::format(printf, 2, 3)]] void print(const char *format, ...);
    /* The box text covers when drawn at (x,y). */
    void bounds(Rect *box, s16 x, s16 y, const char *text);
};
ASSERT_SIZE(GumpFont, 0xa);

/* Makes font the current font for the object's scope. */
class FontTarget {
    GumpFont *previous;

public:
    /* UNCALLED: nothing calls it. */
    explicit FontTarget(GumpFont *font);
    /* UNCALLED: nothing calls it. */
    ~FontTarget();
};
ASSERT_SIZE(FontTarget, 0x4);

/* Makes font both the system font and the current font. */
void setFont(GumpFont *font);
/* The font of the debug text (Code52) and the font the GUI draws with. */
extern GumpFont *&systemFont;
extern GumpFont *&gumpFont;
#endif
