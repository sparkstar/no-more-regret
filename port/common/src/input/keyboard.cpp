/* KEYBOARD.C (11e0:0000-02d0): key events and their characters. See
 * keyboard.h. */
#include "gamestate.h"
#include <cstdarg>
#include "keyboard.h"

/* The characters of the scan codes below SCAN_TRANSLATED: plain, shifted,
 * with num lock, shifted with num lock, shifted with caps and num lock,
 * shifted with caps lock, with caps and num lock, with caps lock. */
u8 (&keyTables)[8][SCAN_TRANSLATED] = gameGlobal<u8[8][SCAN_TRANSLATED]>(
    {{0,   0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x8, 0x9, 'q', 'w', 'e', 'r',  't',
      'y', 'u',  'i', 'o', 'p', '[', ']', 0xd, 0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
      0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,    0,
      0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,   0,   0},
     {0,   0x1b, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x8, 0x9, 'Q', 'W', 'E', 'R', 'T',
      'Y', 'U',  'I', 'O', 'P', '{', '}', 0xd, 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
      0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,
      0,   0,    0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0'},
     {0,   0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x8, 0x9, 'q', 'w', 'e', 'r',  't',
      'y', 'u',  'i', 'o', 'p', '[', ']', 0xd, 0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
      0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,    0,
      0,   0,    0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0'},
     {0,   0x1b, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x8, 0x9, 'Q', 'W', 'E', 'R', 'T',
      'Y', 'U',  'I', 'O', 'P', '{', '}', 0xd, 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
      0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,
      0,   0,    0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0'},
     {0,   0x1b, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x8, 0x9, 'q', 'w', 'e', 'r', 't',
      'y', 'u',  'i', 'o', 'p', '{', '}', 0xd, 0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', '~',
      0,   '|',  'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,
      0,   0,    0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0'},
     {0,   0x1b, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x8, 0x9, 'Q', 'W', 'E', 'R', 'T',
      'Y', 'U',  'I', 'O', 'P', '{', '}', 0xd, 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
      0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,
      0,   0,    0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0'},
     {0,   0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x8, 0x9, 'Q', 'W', 'E', 'R', 'T',
      'Y', 'U',  'I', 'O', 'P', '{', '}', 0xd, 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
      0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,
      0,   0,    0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0'},
     {0,   0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x8, 0x9, 'Q', 'W', 'E', 'R', 'T',
      'Y', 'U',  'I', 'O', 'P', '{', '}', 0xd, 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
      0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,
      0,   0,    0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0'}});

void KeyEvent::clear()
{
    memset(this, 0, sizeof(KeyEvent));
}

void KeyEvent::translate()
{
    if (scan < SCAN_TRANSLATED) {
        if ((extended == 0U) || scan == SCAN_ENTER || scan == SCAN_SLASH) {
            if (((shift & KEY_LEFT_SHIFT) != 0) || ((shift & KEY_RIGHT_SHIFT) != 0)) {
                if ((shift & KEY_CAPS_LOCK) != 0) {
                    if ((shift & KEY_NUM_LOCK) != 0) {
                        ascii = keyTables[4][scan];
                        return;
                    }
                    ascii = keyTables[5][scan];
                    return;
                }
                if ((shift & KEY_NUM_LOCK) != 0) {
                    ascii = keyTables[3][scan];
                    return;
                }
                ascii = keyTables[1][scan];
                return;
            }
            if ((shift & KEY_CAPS_LOCK) != 0) {
                if ((shift & KEY_NUM_LOCK) != 0) {
                    ascii = keyTables[6][scan];
                    return;
                }
                ascii = keyTables[7][scan];
                return;
            }
            if ((shift & KEY_NUM_LOCK) != 0) {
                ascii = keyTables[2][scan];
                return;
            }
            ascii = keyTables[0][scan];
            return;
        }
    }
    ascii = 0;
}

/* UNCALLED: nothing calls it. */
bool KeyEvent::isModifier() const
{
    return scan == SCAN_LEFT_SHIFT || scan == SCAN_RIGHT_SHIFT || scan == SCAN_ALT || scan == SCAN_CONTROL;
}

/* UNCALLED: nothing calls it. */
bool KeyEvent::isShifted() const
{
    return ((shift & KEY_LEFT_SHIFT) != 0) || ((shift & KEY_RIGHT_SHIFT) != 0);
}

bool KeyEvent::isAlted() const
{
    return ((shift & KEY_LEFT_ALT) != 0) || ((shift & KEY_RIGHT_ALT) != 0);
}

bool KeyEvent::isControlled() const
{
    return ((shift & KEY_LEFT_CONTROL) != 0) || ((shift & KEY_RIGHT_CONTROL) != 0);
}

/* Whether the key's character is one of the arguments, which end with
 * KEY_END. */
/* UNCALLED: nothing calls it. */
bool asciiIn(KeyEvent *key, ...)
{
    va_list arguments;
    i16 value;
    i16 count;
    bool found;
    i16 wanted;

    found = false;
    va_start(arguments, key);
    count = 0;
    do {
        value = va_arg(arguments, int);
        count++;
        if (count == 32) {
            halt();
        }
    } while (value != KEY_END);
    va_start(arguments, key);
    for (wanted = key->ascii; count != 0; count--) {
        value = va_arg(arguments, int);
        if (wanted == value) {
            found = true;
            break;
        }
    }
    return found;
}

/* UNCALLED: nothing calls it. */
bool scanIn(KeyEvent *key, ...)
{
    va_list arguments;
    i16 value;
    i16 count;
    bool found;
    i16 wanted;

    found = false;
    va_start(arguments, key);
    count = 0;
    do {
        value = va_arg(arguments, int);
        count++;
        if (count == 32) {
            halt();
        }
    } while (value != KEY_END);
    va_start(arguments, key);
    for (wanted = key->scan; count != 0; count--) {
        value = va_arg(arguments, int);
        if (wanted == value) {
            found = true;
            break;
        }
    }
    return found;
}
