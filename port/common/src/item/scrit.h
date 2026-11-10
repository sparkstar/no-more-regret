/* ..\\ITEM\\SCRIT.C (Code34, 1108:0415-0575): loop scripts, the item tests of
 * the searches (itemfind.h), made from tokens. */
#ifndef PORT_SCRIT_H
#define PORT_SCRIT_H
#include "common.h"

/* The tokens; Crusader tests >= and <= with ')' and '('. A token '@'+n (n 1-31)
 * is a list of n shapes, '`'+n of n frames. */
constexpr char SCRIPT_END = '$';
constexpr int SCRIPT_TRUE = 1;
constexpr int SCRIPT_FALSE = 0;
/* A word follows. */
constexpr char SCRIPT_INT = '%';
constexpr char SCRIPT_AND = '&';
constexpr char SCRIPT_OR = '+';
constexpr char SCRIPT_NOT = '!';
constexpr char SCRIPT_EQUAL = '=';
constexpr char SCRIPT_GREATER = '>';
constexpr char SCRIPT_LESS = '<';
constexpr char SCRIPT_GEQUAL = ')';
constexpr char SCRIPT_LEQUAL = '(';
constexpr char SCRIPT_NPCNUM = '#';
constexpr char SCRIPT_STATUS = '?';
constexpr char SCRIPT_Q = '*';
constexpr char SCRIPT_FAMILY = ':';
constexpr char SCRIPT_SHAPE = '@';
constexpr char SCRIPT_FRAME = '`';

class LoopScript {
public:
    /* The length, the tokens and SCRIPT_END. */
    u8 text[0x20];

    /* The tokens up to SCRIPT_END, each with its words (1108:0415). */
    // NOLINTNEXTLINE(google-explicit-constructor): a script is its tokens, as the original's
    LoopScript(i16 token, ...);
    /* The script as the searches take it. */
    // NOLINTNEXTLINE(google-explicit-constructor): passed as its text, as the original's
    operator char *() { return reinterpret_cast<char *>(text); }
    /* The bytes of token with its words: a token 'A'-'_' takes token-'@'
     * words, 'a'-7Fh token-'`' (1108:04f9). */
    i16 size(i16 token);
    /* Adds count bytes (1108:0535). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    void append(u8 count, ...);
};
ASSERT_SIZE(LoopScript, 0x20);
#endif
