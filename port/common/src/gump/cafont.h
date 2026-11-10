/* CAFONT.C (1310:0000-0354): the GUI font. Its glyphs are the frames of one
 * shape of the font archive, which initFonts opens into a FLEX cache; the
 * index selects the shape and the spacing added to each glyph. */
#ifndef PORT_CAFONT_H
#define PORT_CAFONT_H
#include "gumpfont.h"
#include "flexhand.h"

constexpr int CA_FONTS = 32; /* shapes in the font archive */

/* The font archive's cache (initFonts). */
extern FlexCacheHandler *&fontCache;

class CAFont : public GumpFont {
    i16 index;

public:
    explicit CAFont(i16 number);
    /* UNCALLED: nothing calls it. */
    CAFont();
    void drawGlyph(i16 screenX, i16 screenY, u8 ch) override;
    ~CAFont() override = default;
    u16 textHeight(const char *text) override { return GumpFont::textHeight(text); }
    i16 textWidth(const char *text) override { return GumpFont::textWidth(text); }
    void measure(const char *text, s16 *width, s16 *height) override { GumpFont::measure(text, width, height); }
    s16 glyphHeight(u8 ch) override;
    s16 glyphWidth(u8 ch) override;
    void glyphSize(u8 ch, s16 *width, s16 *height) override;
    /* The y offset of the shape's first frame. */
    s16 lineHeight() override;
    /* Selects a shape and its spacing; an unknown number selects 0. */
    void setIndex(i16 newIndex);
};
ASSERT_SIZE(CAFont, 0xc);

/* Opens and closes the font archive; a CAFont needs it open. */
void initFonts(char *path);

/* Nothing calls these, in the original either. */
void closeFonts();
#endif
