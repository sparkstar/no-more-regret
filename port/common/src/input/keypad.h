/* Code119 (13b0:0000-0ad7, file name unknown): the keypad gump of the door and
 * computer locks. */
#ifndef PORT_KEYPAD_H
#define PORT_KEYPAD_H
#include "button.h"
#include "gumpshap.h"

/* A digit key: frame digit of gump shape 0Bh, digit+12 while pressed. digit
 * is the button's instance: 0-8 are 1-9, 9 is the back key, 10 is 0 and 11
 * the enter key (vtable 1480:6c46). */
class KeypadButton : public ButtonGump, protected GumpShape {
public:
    u16 digit;
    u8 unused3c;

    KeypadButton(u16 buttonDigit, NewGump *parentGump);
    void paintSelf(i16 x, i16 y) override;
    void showPressed() override;
    void showReleased() override;
};
ASSERT_SIZE(KeypadButton, 0x3d);

/* The keypad, gump shape 0Ah (vtable 1480:6bfe). Enter sends the number, or
 * -1 for Escape; with a code, the last four digits are checked against it
 * and a sound tells whether they matched. */
class KeypadGump : public NewGump, protected GumpShape {
public:
    i16 count; /* the digits typed */
    s32 value;
    i16 code;
    u8 unknown41[2];
    /* 1 when value matched code, FFh before the check. */
    u8 correct;
    i16 digits[8];
    u8 savedAnimationsHeld;

    explicit KeypadGump(i16 wantedCode);
    ~KeypadGump() override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    /* value from the digits. */
    void computeValue();
    /* Sets correct, sounds it and repaints. */
    void check();
};
ASSERT_SIZE(KeypadGump, 0x55);

/* Intrinsic 0BE: runs the keypad for code and returns what it sent. */
i16 showKeypad(i16 code);
#endif
