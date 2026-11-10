/* 12b0:0000-0522, file name unknown: the GUI font base class. See
 * gumpfont.h. */
#include <cstdarg>
#include "gamestate.h"
#include <cstdio>
#include "gumpfont.h"
#include "viewport.h"
#include "system.h"

GumpFont *&systemFont = gameGlobal<GumpFont *>(nullptr);
GumpFont *&gumpFont = gameGlobal<GumpFont *>(nullptr);

GumpFont::~GumpFont() = default;

GumpFont::GumpFont()
{
    x = 0;
    y = 0;
    charSpacing = 0;
    /* The original clears charSpacing twice and leaves lineSpacing. */
    charSpacing = 0;
}

void GumpFont::putChar(u8 ch)
{
    s16 width;
    s16 height;

    glyphSize(ch, &width, &height);
    drawGlyph(x, y, ch);
    if (ch == '\n') {
        y += height;
        x = drawViewport->bounds().left;
        return;
    }
    if (ch == '\r') {
        x = drawViewport->bounds().left;
        return;
    }
    x += width;
}

void GumpFont::putText(const char *text)
{
    while (*text != 0) {
        putChar(*text);
        text++;
    }
}

i16 GumpFont::putClipped(s16 width, const char *text)
{
    const s16 start = x;
    s16 charWidth;
    s16 charHeight;
    i16 count = 0;

    while (*text != 0) {
        glyphSize(*text, &charWidth, &charHeight);
        if (static_cast<i16>(x + charWidth) > static_cast<i16>(start + width)) {
            break;
        }
        putChar(*text);
        text++;
        count++;
    }
    return count;
}

void GumpFont::measure(const char *text, s16 *width, s16 *height)
{
    s16 lineWidth = 0;
    s16 widest = 0;
    s16 total = 0;
    s16 charHeight = 0;

    while (*text != 0) {
        glyphSize(*text, width, &charHeight);
        if (*text == '\n') {
            total += charHeight;
            if (lineWidth > widest) {
                widest = lineWidth;
            }
            lineWidth = 0;
        }
        lineWidth += *width;
        text++;
    }
    total += charHeight;
    if (lineWidth > widest) {
        widest = lineWidth;
    }
    *width = widest;
    *height = total;
}

u16 GumpFont::textHeight(const char *text)
{
    s16 height;
    s16 width;

    measure(text, &width, &height);
    return height;
}

i16 GumpFont::textWidth(const char *text)
{
    s16 height;
    s16 width;

    measure(text, &width, &height);
    return width;
}

void GumpFont::draw(s16 px, s16 py, const char *format, ...)
{
    if (format != formatBuffer) {
        va_list arguments;

        va_start(arguments, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, arguments);
    }
    moveTo(px, py);
    putText(formatBuffer);
}

void GumpFont::drawClipped(s16 px, s16 py, s16 width, const char *format, ...)
{
    if (format != formatBuffer) {
        va_list arguments;

        va_start(arguments, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, arguments);
    }
    moveTo(px, py);
    putClipped(width, formatBuffer);
}

void GumpFont::print(const char *format, ...)
{
    if (format != formatBuffer) {
        va_list arguments;

        va_start(arguments, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, arguments);
    }
    putText(formatBuffer);
}

void GumpFont::bounds(Rect *box, s16 px, s16 py, const char *text)
{
    box->left = box->top = 0;
    measure(text, &box->right, &box->bottom);
    box->moveTo(px, py);
    box->moveBy(0, -lineHeight());
}

void setFont(GumpFont *font)
{
    systemFont = font;
    gumpFont = font;
}

/* UNCALLED: nothing calls it. */
FontTarget::FontTarget(GumpFont *font)
{
    previous = gumpFont;
    gumpFont = font;
}

/* UNCALLED: nothing calls it. */
FontTarget::~FontTarget()
{
    gumpFont = previous;
}
