/* Code124 (13d8:0000-05cf, file name unknown): two dialogs drawn as one
 * frame of a gump shape, centred on the screen, with shape buttons. */
#ifndef PORT_SHAPEDLG_H
#define PORT_SHAPEDLG_H
#include "dialog.h"
#include "gumpshap.h"

/* A yes-or-no question. Its command is the instance of the button pressed;
 * the keys Y, J and O answer 1, the others 0 (vtable 1480:70e4). yes and no
 * place the two buttons; the callers build the Points in the argument area. */
class QuestionGump : public DialogGump, protected GumpShape {
public:
    u8 unused4c;

    QuestionGump(NewGumpId gumpId, NewGump *owner, u16 dialogShape, u16 yesShape, u16 noShape, Point yes, Point no);
    ~QuestionGump() override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    /* Not draggable. */
    void handleLeftButton(NewGumpEvent * /*event*/) override {}
};
ASSERT_SIZE(QuestionGump, 0x4d);

/* A notice with one button at place; Enter and Escape close it too (vtable
 * 1480:709c). */
class NoticeGump : public DialogGump, protected GumpShape {
public:
    u8 unused4c;

    NoticeGump(NewGumpId gumpId, NewGump *owner, u16 noticeShape, u16 buttonShape, Point place);
    ~NoticeGump() override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    void handleLeftButton(NewGumpEvent * /*event*/) override {}
};
ASSERT_SIZE(NoticeGump, 0x4d);
#endif
