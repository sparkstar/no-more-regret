/* Code119 (13b0:0000-0ad7, file name unknown). See keypad.h. */
#include "keypad.h"
#include "dispatch.h"
#include "dsfxman.h"
#include "gametime.h"
#include "kernel.h"
#include "viewport.h"
#include "world.h"
#include "sounds.h"

/* The keypad's buttons (their codes): 0-8 are the digits 1-9, then Back,
 * 0 and Enter. */
constexpr int KEYPAD_BACK = 9;
constexpr int KEYPAD_ZERO = 10;
constexpr int KEYPAD_ENTER = 11;

KeypadButton::KeypadButton(u16 buttonDigit, NewGump *parentGump) : ButtonGump(buttonDigit, parentGump), GumpShape(0xb)
{
    this->digit = buttonDigit;
    setFrame(this->digit);
}

void KeypadButton::showPressed()
{
    setFrame(digit + 12);
}

void KeypadButton::showReleased()
{
    if (frame != digit) {
        const i16 sound = digit == 9 ? 0x3a : 0x3b;

        playEffect(sound);
    }
    setFrame(digit);
}

void KeypadButton::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

i16 showKeypad(i16 code)
{
    freezeWorld();
    /* The world goes on after the gump has closed, the intrinsic's result
     * its. */
    openModalGump(new KeypadGump(code), static_cast<u32>(-1), [](s32 result) {
        const i16 entered = static_cast<i16>(result);
        thawWorld();
        setRestResult(entered);
    });
    return 0;
}

KeypadGump::KeypadGump(i16 wantedCode) : NewGump(NewGumpId(), nullptr, 0), GumpShape(0xa)
{
    savedAnimationsHeld = animationsHeld;
    animationsHeld = 1;
    count = 0;
    this->code = wantedCode;
    correct = 0xff;
    bounds = dimensions();
    eventMask |= EVENT_KEY;
    addAcceptedArguments(-1, 0);
    bounds.moveTo(static_cast<i16>(drawViewport->right() - drawViewport->left() + 1 - RECT_WIDTH(bounds)) >> 1,
                  static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1 - RECT_HEIGHT(bounds)) >> 1);
    KeypadButton *button = new KeypadButton(0, this);
    button->bounds.set(12, 25, 38, 49);
    button = new KeypadButton(1, this);
    button->bounds.set(39, 25, 65, 49);
    button = new KeypadButton(2, this);
    button->bounds.set(66, 25, 91, 49);
    button = new KeypadButton(3, this);
    button->bounds.set(12, 50, 38, 73);
    button = new KeypadButton(4, this);
    button->bounds.set(39, 50, 65, 73);
    button = new KeypadButton(5, this);
    button->bounds.set(66, 50, 91, 73);
    button = new KeypadButton(6, this);
    button->bounds.set(12, 74, 38, 97);
    button = new KeypadButton(7, this);
    button->bounds.set(39, 74, 65, 97);
    button = new KeypadButton(8, this);
    button->bounds.set(66, 74, 91, 97);
    button = new KeypadButton(KEYPAD_BACK, this);
    button->bounds.set(12, 98, 38, 123);
    button = new KeypadButton(KEYPAD_ZERO, this);
    button->bounds.set(39, 98, 65, 123);
    button = new KeypadButton(KEYPAD_ENTER, this);
    button->bounds.set(66, 98, 91, 123);
    paint(0);
}

KeypadGump::~KeypadGump()
{
    animationsHeld = savedAnimationsHeld;
    flushKeys();
    gumpKeys.clear();
}

void KeypadGump::handleCommand(NewGumpEvent *event)
{
    const u8 key = event->sender->id.instance();

    switch (key) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case KEYPAD_ZERO:
        if (count < 8) {
            digits[count++] = key == KEYPAD_ZERO ? 0 : key + 1;
        }
        break;
    case KEYPAD_BACK:
        if (count != 0) {
            count--;
        }
        break;
    case KEYPAD_ENTER:
        computeValue();
        check();
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, value, this, parent, 0L)));
        close();
        return;
    }
    paint(0);
}

void KeypadGump::handleKeyboard(NewGumpEvent *event)
{
    /* The shifted digits of a US keyboard. */
    switch (event->argument) {
    case ')':
        event->argument = '0';
        break;
    case '!':
        event->argument = '1';
        break;
    case '@':
        event->argument = '2';
        break;
    case '#':
        event->argument = '3';
        break;
    case '$':
        event->argument = '4';
        break;
    case '%':
        event->argument = '5';
        break;
    case '^':
        event->argument = '6';
        break;
    case '&':
        event->argument = '7';
        break;
    case '*':
        event->argument = '8';
        break;
    case '(':
        event->argument = '9';
        break;
    }
    switch (event->argument) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        if (count < 8) {
            digits[count++] = event->argument & 0xf;
            playEffect(SOUND_KEYPAD_DIGIT);
        }
        paint(0);
        return;
    case '\b':
        if (count != 0) {
            count--;
        }
        playEffect(SOUND_KEYPAD_ERASE);
        paint(0);
        return;
    case ASCII_ESCAPE:
        value = 0;
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, -1L, this, parent, 0L)));
        close();
        return;
    case '\r':
        if (count != 0) {
            computeValue();
            check();
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, value, this, parent, 0L)));
            close();
        }
    }
}

void KeypadGump::computeValue()
{
    i16 i;

    value = 0;
    for (i = 0; i < count; i++) {
        value *= 10;
        value += digits[i];
    }
}

void KeypadGump::paintSelf(i16 x, i16 y)
{
    draw(x, y);
    if (count != 0) {
        GumpShape digit(0xc);
        i16 left = 25;
        const i16 top = 12;
        i16 i;

        for (i = 0; i < count; i++) {
            digit.setFrame(digits[i] == 0 ? 9 : digits[i] - 1);
            digit.draw(x + left, y + top);
            left += digit.dimensions().width() + 1;
        }
    }
}

void KeypadGump::check()
{
    if (code == 0) {
        return;
    }
    if (value == static_cast<s32>(74697689L)) {
        correct = 1;
        value = code;
    } else {
        value %= 10000;
        correct = static_cast<u8>(code == value);
    }
    paint(1);
    const i16 sound = (correct != 0U) ? 0x32 : 0x31;
    playEffect(sound);
    paint(1);
}
