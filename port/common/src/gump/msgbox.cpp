/* 1350:0000-040b, file name unknown: MessageBoxGump and a helper that runs
 * one. See msgbox.h. */
#include "msgbox.h"
#include "textbtn.h"
#include "dispatch.h"

#include <string>

/* Shows a message box and waits until it is closed. */
/* UNCALLED: nothing calls it. */
void showMessage(char *message)
{
    /* The intrinsic's result 0 after the box. */
    openModalGump(new MessageBoxGump(NewGumpId(), nullptr, message, guiColors->face), static_cast<u32>(-1),
                  [](s32 /*result*/) { setRestResult(0); });
}

MessageBoxGump::MessageBoxGump(NewGumpId boxId, NewGump *owner, const char *message, u8 boxBackground)
    : DialogGump(boxId, owner, Rect(0, 0, 4, 4), boxBackground)
{
    TextButtonGump *ok;
    text = message;
    ok = new TextButtonGump(0, this, "Ok", boxBackground + 2);
    bounds = Rect(0, 0, RECT_WIDTH(ok->bounds) + 8, RECT_HEIGHT(ok->bounds) + 12);
    gumpFont->GumpFont::measure(text, &textWidth, &textHeight);
    if (RECT_WIDTH(bounds) < static_cast<i16>(textWidth + 8)) {
        bounds.right = textWidth + 8;
    }
    bounds.bottom += textHeight;
    /* The button at the bottom center, the dialog at the center of the screen. */
    ok->bounds.moveTo((RECT_WIDTH(bounds) >> 1) - (RECT_WIDTH(ok->bounds) >> 1),
                      RECT_HEIGHT(bounds) - (RECT_HEIGHT(ok->bounds) + 4));
    bounds.moveTo((static_cast<i16>(drawViewport->right() - drawViewport->left() + 1) >> 1) - (RECT_WIDTH(bounds) >> 1),
                  (static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1) >> 1) -
                      (RECT_HEIGHT(bounds) >> 1));
    if (parent != nullptr) {
        bounds.moveBy(-parent->getAbsoluteX(), -parent->getAbsoluteY());
    }
    addAcceptedArguments(0x1b, 0x0d, 0);
}

void MessageBoxGump::handleCommand(NewGumpEvent * /*unused*/)
{
    close();
}

void MessageBoxGump::handleKeyboard(NewGumpEvent * /*unused*/)
{
    close();
}

void MessageBoxGump::paintSelf(i16 x, i16 y)
{
    char *cursor;
    char *line;
    bool more;
    s16 height;
    s16 width;
    s16 offset;
    s16 count;

    DialogGump::paintSelf(x, y);
    count = 1;
    /* The original ends each line in the message itself (a 0 for the
     * line break, put back after): its strings were writable. The port's
     * literals are not (a write into one faults), so it splits a copy the
     * same way. */
    std::string copy(text);
    line = cursor = copy.data();
    more = false;
    do {
        if (more) {
            more = false;
            *cursor = '\n';
            ++cursor;
            ++count;
            line = cursor;
        }
        while (*cursor != '\0' && *cursor != '\n') {
            ++cursor;
        }
        if (*cursor == '\n') {
            more = true;
            *cursor = '\0';
        }
        gumpFont->GumpFont::measure(line, &width, &height);
        offset = (RECT_WIDTH(bounds) >> 1) - (width >> 1);
        /* The original formats the line again; Borland's printf writes a lone
         * % as it is, which "%s" does too. */
        gumpFont->draw(x + offset, y + (height * count) + 4, "%s", line);
    } while (more);
}
