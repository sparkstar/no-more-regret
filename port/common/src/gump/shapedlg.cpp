/* Code124 (13d8:0000-05cf, file name unknown). See shapedlg.h. */
#include <cctype>
#include "shapedlg.h"
#include "shapebtn.h"
#include "u9mouse.h"
#include "screen.h"

QuestionGump::QuestionGump(NewGumpId gumpId, NewGump *owner, u16 dialogShape, u16 yesShape, u16 noShape, Point yes,
                           Point no)
    : DialogGump(gumpId, owner, Rect(0, 0, 1, 1), guiColors->face), GumpShape(dialogShape)
{
    ShapeButtonGump *button;

    button = new ShapeButtonGump(1, this, yesShape);
    button->bounds.moveTo(yes.x, yes.y);
    button = new ShapeButtonGump(0, this, noShape);
    button->bounds.moveTo(no.x, no.y);
    bounds = dimensions();
    bounds.moveTo(static_cast<i16>(SCREEN_WIDTH - RECT_WIDTH(bounds)) >> 1,
                  static_cast<i16>(SCREEN_BOTTOM - RECT_HEIGHT(bounds)) >> 1);
    addAcceptedArguments('o', 'O', 'j', 'J', 'y', 'Y', 'n', 'N', 's', 'S', 0x1b, 0);
    gameMouse->resetShape(GAME_POINTER);
    gameMouse->show();
}

QuestionGump::~QuestionGump()
{
    gameMouse->hide();
}

void QuestionGump::handleCommand(NewGumpEvent *event)
{
    sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, event->sender->id.instance(), this, parent, 0L)));
    close();
}

void QuestionGump::handleKeyboard(NewGumpEvent *event)
{
    const u8 key = toupper(event->argument);

    if (key == 'Y' || key == 'J' || key == 'O') {
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 1L, this, parent, 0L)));
    } else {
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
    }
    close();
}

void QuestionGump::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

NoticeGump::NoticeGump(NewGumpId gumpId, NewGump *owner, u16 noticeShape, u16 buttonShape, Point place)
    : DialogGump(gumpId, owner, Rect(0, 0, 1, 1), guiColors->face), GumpShape(noticeShape)
{
    ShapeButtonGump *button;

    button = new ShapeButtonGump(1, this, buttonShape);
    button->bounds.moveTo(place.x, place.y);
    bounds = dimensions();
    bounds.moveTo(static_cast<i16>(SCREEN_WIDTH - RECT_WIDTH(bounds)) >> 1,
                  static_cast<i16>(SCREEN_BOTTOM - RECT_HEIGHT(bounds)) >> 1);
    addAcceptedArguments('\r', 0x1b, 0);
    gameMouse->resetShape(GAME_POINTER);
    gameMouse->show();
}

NoticeGump::~NoticeGump()
{
    gameMouse->hide();
}

void NoticeGump::handleCommand(NewGumpEvent * /*unused*/)
{
    close();
}

void NoticeGump::handleKeyboard(NewGumpEvent * /*unused*/)
{
    close();
}

void NoticeGump::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}
