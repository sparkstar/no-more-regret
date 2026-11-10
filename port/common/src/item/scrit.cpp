/* ..\\ITEM\\SCRIT.C (Code34, 1108:0415-0575): loop scripts. */
#include <cstdarg>
#include "scrit.h"

LoopScript::LoopScript(i16 token, ...)
{
    i16 length = 1;
    i16 next;
    va_list args;

#pragma GCC diagnostic push
/* The original's last named parameter is a 16-bit value, which the call
 * promotes: ISO C++ leaves va_start after it undefined, GCC's and Clang's
 * va_start do not use it. */
#pragma GCC diagnostic ignored "-Wvarargs"
    va_start(args, token);
#pragma GCC diagnostic pop
    for (next = token; next != SCRIPT_END; next = va_arg(args, int)) {
        const i16 bytes = size(next);
        if (static_cast<i16>(bytes + length) < 0x1f) {
            i16 word;
            text[length++] = next;
            for (i16 i = 1; i < bytes; i += 2) {
                word = va_arg(args, int);
                text[length++] = word & 0xff;
                text[length++] = word >> 8;
            }
        } else {
            halt();
        }
    }
    text[length] = SCRIPT_END;
    text[0] = length;
}

i16 LoopScript::size(i16 token)
{
    if (token == SCRIPT_INT) {
        return 3;
    }
    if (token > '@' && token < '`') {
        return (token * 2) - 0x7f;
    }
    if (token > '`' && token < 0x80) {
        return (token * 2) - 0xbf;
    }
    return 1;
}

/* UNCALLED: nothing calls it. */
void LoopScript::append(u8 count, ...)
{
    const i16 length = text[0];
    const i16 bytes = count;

    /* The copy starts at the high byte of count's argument word, one byte
     * before the variable arguments, over the SCRIPT_END. */
    text[0] = length + bytes - 1;
    memcpy(text + length, &count + 1, bytes);
}
