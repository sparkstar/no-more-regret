/* CAFONT.C (1310:0000-0354, No Regret's addresses). See cafont.h. */
#include "cafont.h"
#include "gamestate.h"
#include "cafont_game.h"

#include <array>

FlexCacheHandler *&fontCache = gameGlobal<FlexCacheHandler *>(nullptr);
/* Added to each glyph's width and height, per shape: the width's the
 * game's (cafont_game.h). */
std::array<s16, CA_FONTS> &charSpacings = gameGlobal<std::array<s16, CA_FONTS>>(CHAR_SPACINGS);
s16 (&lineSpacings)[CA_FONTS] = gameGlobal<s16[CA_FONTS]>({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

s16 CAFont::lineHeight()
{
    ShapeFrame *frame = fontCache->getFrame(index, 0);
    return frame->yOffset;
}

void CAFont::drawGlyph(i16 screenX, i16 screenY, u8 ch)
{
    drawShape(screenX, screenY, fontCache->getFrame(index, ch));
}

s16 CAFont::glyphHeight(u8 ch)
{
    ShapeFrame *frame = fontCache->getFrame(index, ch);
    return frame->height + lineSpacing;
}

s16 CAFont::glyphWidth(u8 ch)
{
    ShapeFrame *frame = fontCache->getFrame(index, ch);
    return frame->width + charSpacing;
}

void CAFont::glyphSize(u8 ch, s16 *width, s16 *height)
{
    ShapeFrame *frame = fontCache->getFrame(index, ch);
    *width = frame->width + charSpacing;
    *height = frame->height + lineSpacing;
}

CAFont::CAFont(i16 number)
{
    if (fontCache == nullptr) {
        haltMessage("FontHandler not inited");
    }
    setIndex(number);
}

/* UNCALLED: nothing calls it. */
CAFont::CAFont()
{
    if (fontCache == nullptr) {
        halt();
    }
    setIndex(0);
}

void initFonts(char *path)
{
    if (fontCache != nullptr) {
        halt();
    }
    fontCache = new FlexCacheHandler(CA_FONTS, 1, path);
}

void closeFonts()
{
    if (fontCache != nullptr) {
        delete fontCache;
        fontCache = nullptr;
    }
}

/* In the original the parameter is index too and hides the member: the
 * first branch assigns the parameter to itself, which leaves an empty branch
 * in the code. */
void CAFont::setIndex(i16 newIndex)
{
    if (newIndex < CA_FONTS) {
        /* The original's index = index: the parameter to itself, nothing. */
    } else {
        newIndex = 0;
    }
    this->index = newIndex;
    charSpacing = charSpacings[newIndex];
    lineSpacing = lineSpacings[newIndex];
}
