/* 1380:0000-04a5, file name unknown: the two text button classes and their
 * implicit destructors. See textbtn.h. */
#include "textbtn.h"

TextButtonGump::TextButtonGump(u8 code, NewGump *parentGump, const char *label, u8 style) : ButtonGump(code, parentGump)
{
    s16 width;
    s16 height;
    text = label;
    gumpFont->GumpFont::measure(text, &width, &height);
    bounds = Rect(0, 0, width + 8, height + 4);
    pressed = 0;
    color = style;
}

void TextButtonGump::paintSelf(i16 x, i16 y)
{
    Panel panel;
    const Rect face(x, y, x + RECT_WIDTH(bounds), y + RECT_HEIGHT(bounds));
    const u8 style = enabled != 0 ? color : color + 2;
    if (pressed == 0) {
        panel.raised(face, style);
    } else {
        panel.pressed(face, style);
    }
    const u16 baseline = gumpFont->GumpFont::textHeight(text);
    gumpFont->draw(x + pressed + 4, y + baseline + pressed + 2, "%s", text);
}

void TextButtonGump::showPressed()
{
    pressed = 1;
    paint(0);
}

void TextButtonGump::showReleased()
{
    pressed = 0;
    paint(0);
}

/* UNCALLED: nothing calls it. */
AutoRepeatTextButtonGump::AutoRepeatTextButtonGump(u8 code, NewGump *parentGump, const char *label, u8 style)
    : AutoRepeatButtonGump(code, parentGump)
{
    s16 width;
    s16 height;
    text = label;
    gumpFont->GumpFont::measure(text, &width, &height);
    bounds = Rect(0, 0, width + 8, height + 4);
    pressed = 0;
    color = style;
}

void AutoRepeatTextButtonGump::paintSelf(i16 x, i16 y)
{
    Panel panel;
    const Rect face(x, y, x + RECT_WIDTH(bounds), y + RECT_HEIGHT(bounds));
    const u8 style = enabled != 0 ? color : color + 2;
    if (pressed == 0) {
        panel.raised(face, style);
    } else {
        panel.pressed(face, style);
    }
    const u16 baseline = gumpFont->GumpFont::textHeight(text);
    gumpFont->draw(x + pressed + 4, y + baseline + pressed + 2, "%s", text);
}

void AutoRepeatTextButtonGump::showPressed()
{
    pressed = 1;
    paint(0);
}

void AutoRepeatTextButtonGump::showReleased()
{
    pressed = 0;
    paint(0);
}
