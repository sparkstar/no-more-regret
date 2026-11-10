/* 1308 (file name unknown): the abstract button and its input policies.
 * ButtonBaseGump is a reconstructed name for the base vtable 1480:580e. */
#ifndef PORT_BUTTON_H
#define PORT_BUTTON_H
#include "newgump.h"
/* buttonState: a button is held, the pointer is on it, and the right mouse
 * button holds it. A button sends EVENT_COMMAND with 1 when it is clicked,
 * 2 with the right button. */
constexpr int BUTTON_HELD = 1;
constexpr int BUTTON_INSIDE = 2;
constexpr int BUTTON_RIGHT = 4;
extern u16 &buttonState;
class ButtonBaseGump : public NewGump {
protected:
    u8 enabled;

public:
    /* UNCALLED: nothing calls it. */
    ButtonBaseGump(u8 code, NewGump *parentGump) : NewGump(NewGumpId(GUMP_BUTTON, code), parentGump, EVENT_LEFT_BUTTON)
    {
        buttonState = 0;
        enabled = 1;
    }
    void handleLeftButton(NewGumpEvent *event) override;
    void handleRightButton(NewGumpEvent *event) override;
    void handleMouseMotion(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override = 0;
    virtual void showPressed() = 0;
    virtual void showReleased() = 0;
};
class ButtonGump : public ButtonBaseGump {
public:
    ButtonGump(u8 code, NewGump *parentGump);
    void handleLeftButton(NewGumpEvent *event) override;
};
class CyclingButtonGump : public ButtonBaseGump {
protected:
    s16 current, count;

public:
    /* UNCALLED: nothing calls it. */
    CyclingButtonGump(u8 code, NewGump *parentGump);
};
class AutoRepeatButtonGump : public ButtonGump {
protected:
    u32 deadline;

public:
    AutoRepeatButtonGump(u8 code, NewGump *parentGump);
    void execute(NewGumpEvent *event) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void handleMouseMotion(NewGumpEvent *event) override;
};
ASSERT_SIZE(ButtonBaseGump, 0x35);
ASSERT_SIZE(ButtonGump, 0x35);
ASSERT_SIZE(CyclingButtonGump, 0x39);
ASSERT_SIZE(AutoRepeatButtonGump, 0x39);
#endif
