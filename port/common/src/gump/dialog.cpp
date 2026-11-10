/* 12f8:0000-053e, file name unknown: panel drawing and DialogGump, followed by
 * the kept copies of the inline NewGump::execute, handleCommand and
 * handleKeyboard. See dialog.h. */
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "dialog.h"
#include "gumpdraw.h"
#include "cafont.h"
#include "system.h"

void Panel::flat(Rect bounds, u8 color)
{
    drawViewport->rectangle(0, bounds.left, bounds.top, bounds.right, bounds.bottom);
    drawViewport->clear(color, bounds.left + 1, bounds.top + 1, bounds.right - 1, bounds.bottom - 1);
}

void Panel::raised(Rect bounds, u8 color)
{
    drawViewport->rectangle(0, bounds.left, bounds.top, bounds.right, bounds.bottom);
    drawViewport->clear(color, bounds.left + 1, bounds.top + 1, bounds.right - 1, bounds.bottom - 1);
    drawVLine(bounds.left + 1, bounds.top + 1, bounds.bottom - 2, color - 2);
    drawHLine(bounds.left + 2, bounds.top + 1, bounds.right - 2, color - 2);
    drawHLine(bounds.left + 2, bounds.bottom - 1, bounds.right - 1, color + 2);
    drawVLine(bounds.right - 1, bounds.top + 2, bounds.bottom - 2, color + 2);
}

void Panel::pressed(Rect bounds, u8 color)
{
    drawViewport->rectangle(0, bounds.left, bounds.top, bounds.right, bounds.bottom);
    drawViewport->clear(color, bounds.left + 1, bounds.top + 1, bounds.right - 1, bounds.bottom - 1);
    drawViewport->line(bounds.left + 1, bounds.top + 1, bounds.left + 1, bounds.bottom - 1, color + 2);
    drawViewport->line(bounds.left + 2, bounds.top + 1, bounds.right - 1, bounds.top + 1, color + 2);
}

DialogGump::DialogGump(NewGumpId gumpId, NewGump *owner, Rect area, u8 dialogBackground) : DraggableGump(gumpId, owner)
{
    eventMask |= EVENT_COMMAND;
    bounds = area;
    color = dialogBackground;
    font = gumpFont;
    flat = 0;
}

DialogGump::~DialogGump()
{
    if (font != gumpFont) {
        delete font;
        font = nullptr;
    }
}

/* UNCALLED: nothing calls it. */
void DialogGump::setFont(i16 index)
{
    font = new CAFont(index);
}

void DialogGump::paintSelf(i16 x, i16 y)
{
    Panel panel;
    Rect area(&bounds);
    area.moveTo(x, y);
    if (flat != 0U) {
        panel.flat(area, color);
    } else {
        panel.raised(area, color);
    }
}

void DialogGump::handleLeftButton(NewGumpEvent *event)
{
    DraggableGump::handleLeftButton(event);
}

void DialogGump::print(i16 x, i16 y, const char *format, ...)
{
    va_list args;
    char text[256];
    if (font == nullptr) {
        return;
    }
    if (format != formatBuffer) {
        va_start(args, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, args);
    }
    strcpy(text, formatBuffer);
    font->drawClipped(getAbsoluteX() + x, getAbsoluteY() + y, width() - x, "%s", text);
}
