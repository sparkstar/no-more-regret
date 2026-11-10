/* 1380 (file name unknown): text on the ordinary and auto-repeat buttons. */
#ifndef PORT_TEXTBUTTON_H
#define PORT_TEXTBUTTON_H
#include "button.h"
#include "gumpdraw.h"
class TextButtonGump : public ButtonGump {
    u16 pressed;
    u8 color;
    const char *text;

public:
    TextButtonGump(u8 code, NewGump *parentGump, const char *label, u8 style); /* 1380:0000 */
    void paintSelf(i16 x, i16 y) override;                                     /* 1380:00cc */
    void showPressed() override;                                               /* 1380:01e1 */
    void showReleased() override;                                              /* 1380:01fd */
};
class AutoRepeatTextButtonGump : public AutoRepeatButtonGump {
    u16 pressed;
    u8 color;
    const char *text;

public:
    /* UNCALLED: nothing calls it. */
    AutoRepeatTextButtonGump(u8 code, NewGump *parentGump, const char *label, u8 style); /* 1380:0219 */
    void paintSelf(i16 x, i16 y) override;                                               /* 1380:02e5 */
    void showPressed() override;                                                         /* 1380:03fa */
    void showReleased() override;                                                        /* 1380:0416 */
};
ASSERT_SIZE(TextButtonGump, 0x3c);
ASSERT_SIZE(AutoRepeatTextButtonGump, 0x40);
#endif
