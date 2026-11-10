/* KEYBOARD.C (11e0:0000-02d0): key events and their translation to
 * characters. */
#ifndef PORT_KEYBOARD_H
#define PORT_KEYBOARD_H
#include "common.h"

/* KeyEvent::shift */
constexpr int KEY_RIGHT_SHIFT = 0x1;
constexpr int KEY_LEFT_SHIFT = 0x2;
constexpr int KEY_CONTROL = 0x4; /* either control key (the BIOS's bit) */
constexpr int KEY_ALT = 0x8;     /* either alt key (the BIOS's bit) */
constexpr int KEY_SCROLL_LOCK = 0x10;
constexpr int KEY_NUM_LOCK = 0x20;
constexpr int KEY_CAPS_LOCK = 0x40;
constexpr int KEY_INSERT = 0x80;
constexpr int KEY_LEFT_CONTROL = 0x100;
constexpr int KEY_LEFT_ALT = 0x200;
constexpr int KEY_RIGHT_CONTROL = 0x400;
constexpr int KEY_RIGHT_ALT = 0x800;
/* Scan codes. The keypad sends the codes of the cursor keys; KeyEvent::extended
 * tells the separate cursor keys apart. */
constexpr int SCAN_ESCAPE = 0x01;
constexpr int SCAN_1 = 0x02;
constexpr int SCAN_2 = 0x03;
constexpr int SCAN_3 = 0x04;
constexpr int SCAN_4 = 0x05;
constexpr int SCAN_5 = 0x06;
constexpr int SCAN_6 = 0x07;
constexpr int SCAN_7 = 0x08;
constexpr int SCAN_8 = 0x09;
constexpr int SCAN_9 = 0x0a;
constexpr int SCAN_0 = 0x0b;
constexpr int SCAN_MINUS = 0x0c;
constexpr int SCAN_EQUALS = 0x0d;
constexpr int SCAN_BACKSPACE = 0x0e;
constexpr int SCAN_TAB = 0x0f;
constexpr int SCAN_Q = 0x10;
constexpr int SCAN_W = 0x11;
constexpr int SCAN_E = 0x12;
constexpr int SCAN_R = 0x13;
constexpr int SCAN_T = 0x14;
constexpr int SCAN_Y = 0x15;
constexpr int SCAN_U = 0x16;
constexpr int SCAN_I = 0x17;
constexpr int SCAN_O = 0x18;
constexpr int SCAN_P = 0x19;
constexpr int SCAN_ENTER = 0x1c;
constexpr int SCAN_CONTROL = 0x1d;
constexpr int SCAN_A = 0x1e;
constexpr int SCAN_S = 0x1f;
constexpr int SCAN_D = 0x20;
constexpr int SCAN_F = 0x21;
constexpr int SCAN_G = 0x22;
#define SCAN_H 0x23
constexpr int SCAN_J = 0x24;
constexpr int SCAN_K = 0x25;
constexpr int SCAN_L = 0x26;
constexpr int SCAN_LEFT_SHIFT = 0x2a;
constexpr int SCAN_Z = 0x2c;
constexpr int SCAN_X = 0x2d;
constexpr int SCAN_C = 0x2e;
constexpr int SCAN_V = 0x2f;
constexpr int SCAN_B = 0x30;
constexpr int SCAN_N = 0x31;
constexpr int SCAN_M = 0x32;
constexpr int SCAN_SLASH = 0x35;
constexpr int SCAN_RIGHT_SHIFT = 0x36;
constexpr int SCAN_PAD_STAR = 0x37;
constexpr int SCAN_ALT = 0x38;
constexpr int SCAN_SPACE = 0x39;
constexpr int SCAN_F1 = 0x3b;
constexpr int SCAN_F2 = 0x3c;
constexpr int SCAN_F3 = 0x3d;
constexpr int SCAN_F4 = 0x3e;
constexpr int SCAN_F5 = 0x3f;
constexpr int SCAN_F6 = 0x40;
constexpr int SCAN_F7 = 0x41;
constexpr int SCAN_F8 = 0x42;
constexpr int SCAN_F9 = 0x43;
constexpr int SCAN_F10 = 0x44;
constexpr int SCAN_SCROLL_LOCK = 0x46;
constexpr int SCAN_HOME = 0x47; /* keypad 7 */
constexpr int SCAN_UP = 0x48;
constexpr int SCAN_PAGE_UP = 0x49; /* keypad 9 */
constexpr int SCAN_PAD_MINUS = 0x4a;
constexpr int SCAN_LEFT = 0x4b;
constexpr int SCAN_CENTRE = 0x4c; /* keypad 5 */
constexpr int SCAN_RIGHT = 0x4d;
constexpr int SCAN_PAD_PLUS = 0x4e;
constexpr int SCAN_END = 0x4f; /* keypad 1 */
constexpr int SCAN_DOWN = 0x50;
constexpr int SCAN_PAGE_DOWN = 0x51;  /* keypad 3 */
constexpr int SCAN_INSERT = 0x52;     /* keypad 0 */
constexpr int SCAN_DELETE = 0x53;     /* keypad's . */
constexpr int SCAN_TRANSLATED = 0x53; /* the keys up to the keypad's Del */
/* The release bit of a scan code, the code without it, and the prefix of the
 * extended keys. */
constexpr int SCAN_RELEASED = 0x80;
constexpr int SCAN_MASK = 0x7f;
constexpr int SCAN_PREFIX = 0xe0;
/* Ends the argument lists of asciiIn and scanIn. */
constexpr int KEY_END = 0xfe;
/* The ASCII code of Esc, which C has no escape sequence for. */
constexpr int ASCII_ESCAPE = 0x1b;

class KeyEvent {
public:
    u8 ascii;
    u8 scan;
    u16 shift;
    u32 time;
    u8 pressed;
    u8 extended;
    u16 repeat;

    KeyEvent() = default;
    void clear();
    /* Sets ascii from scan and the shift state. */
    void translate();
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] bool isModifier() const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] bool isShifted() const;
    [[nodiscard]] bool isAlted() const;
    [[nodiscard]] bool isControlled() const;
};
ASSERT_SIZE(KeyEvent, 0xc);

/* Whether the key's scan code is one of the arguments, which end with
 * KEY_END. */
/* UNCALLED: nothing calls it. */
bool scanIn(KeyEvent *key, ...);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
bool asciiIn(KeyEvent *key, ...);
#endif
