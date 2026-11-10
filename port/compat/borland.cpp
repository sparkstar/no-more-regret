/* Borland C++ 3.1's library functions that the game calls and the C
 * library does not have, or has with other results (borland.h). Each gives
 * what the original's RTL (REGRET.EXE, segment 1000) gives; port/test's
 * borland_test compares them. */
#include "borland.h"
#include "gamestate.h"

#include <cctype>
#include <cstring>

namespace {

/* RAND.C: the seed starts at 1; the game never calls srand. */
std::uint32_t &seed = gameGlobal<std ::uint32_t>(1);

/* The cases the RTL folds to (ASCII only): the comparisons compare capitals,
 * strlwr makes small letters. */
int upper(int c)
{
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
}

int lower(int c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

}

int borlandRand()
{
    seed = seed * 0x015a4e35U + 1;
    return static_cast<int>(seed >> 16) & 0x7fff;
}

void borlandSrand(unsigned value)
{
    seed = static_cast<std::uint16_t>(value);
}

/* STRICMP.ASM and STRNICMP.ASM: at the end of s1 the difference is with
 * s2's byte as it is; elsewhere between the bytes folded to capitals. */
PORT_BORLAND_LIBRARY int stricmp(const char *s1, const char *s2)
{
    for (;; s1++, s2++) {
        const int a = static_cast<unsigned char>(*s1);
        const int b = static_cast<unsigned char>(*s2);
        if (a == 0) {
            return -b;
        }
        if (upper(a) != upper(b)) {
            return upper(a) - upper(b);
        }
    }
}

PORT_BORLAND_LIBRARY int strnicmp(const char *s1, const char *s2, std::size_t count)
{
    for (; count > 0; count--, s1++, s2++) {
        const int a = static_cast<unsigned char>(*s1);
        const int b = static_cast<unsigned char>(*s2);
        if (a == 0) {
            return -b;
        }
        if (upper(a) != upper(b)) {
            return upper(a) - upper(b);
        }
    }
    return 0;
}

PORT_BORLAND_LIBRARY char *strlwr(char *s)
{
    for (char *p = s; *p != 0; p++) {
        *p = static_cast<char>(lower(static_cast<unsigned char>(*p)));
    }
    return s;
}

/* LTOA1.C: radix 10 is signed; the others show the 16 bits unsigned. */
PORT_BORLAND_LIBRARY char *itoa(int value, char *string, int radix)
{
    std::uint32_t magnitude;
    char *p = string;
    if (radix < 2 || radix > 36) {
        *string = 0;
        return string;
    }
    if (radix == 10 && static_cast<std::int16_t>(value) < 0) {
        *p++ = '-';
        magnitude = static_cast<std::uint32_t>(-static_cast<std::int32_t>(static_cast<std::int16_t>(value)));
    } else {
        magnitude = radix == 10 ? static_cast<std::uint32_t>(static_cast<std::int16_t>(value))
                                : static_cast<std::uint16_t>(value);
    }
    char digits[33];
    int n = 0;
    do {
        const int digit = static_cast<int>(magnitude % static_cast<unsigned>(radix));
        digits[n++] = static_cast<char>(digit < 10 ? '0' + digit : 'a' + digit - 10);
        magnitude /= static_cast<unsigned>(radix);
    } while (magnitude != 0U);
    while (n != 0) {
        *p++ = digits[--n];
    }
    *p = 0;
    return string;
}
