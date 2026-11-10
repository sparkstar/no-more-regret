/* UTF-8 for the port's texts (the translations of terminals and the
 * subtitles, and the port's widgets): how many bytes follow a
 * character's first, and a text's code points. */
#ifndef PORT_UTF8_H
#define PORT_UTF8_H
#include "common.h"

#include <string>

/* The bytes after lead in its character, 0 to 3; -1 for a byte that starts
 * no character. */
inline int utf8Following(u8 lead)
{
    if (lead < 0x80) {
        return 0;
    }
    if ((lead & 0xe0) == 0xc0) {
        return 1;
    }
    if ((lead & 0xf0) == 0xe0) {
        return 2;
    }
    if ((lead & 0xf8) == 0xf0) {
        return 3;
    }
    return -1;
}

/* The code points of UTF-8 text; a byte that begins no sequence stands for
 * U+FFFD. */
[[nodiscard]] inline std::u32string codePoints(const std::string &text)
{
    std::u32string out;
    size_t at = 0;
    while (at < text.size()) {
        const u8 lead = static_cast<u8>(text[at]);
        const int more = utf8Following(lead);
        if (more < 0 || at + more >= text.size()) {
            out += U'\xfffd';
            at++;
            continue;
        }
        char32_t point = (more != 0) ? lead & (0x3f >> more) : lead;
        for (int k = 1; k <= more; k++) {
            point = point << 6 | (static_cast<u8>(text[at + k]) & 0x3f);
        }
        out += point;
        at += more + 1;
    }
    return out;
}
#endif
