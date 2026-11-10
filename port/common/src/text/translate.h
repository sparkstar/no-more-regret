/* The translations of the game's text (the game's text in other languages). A text is
 * found by the FNV-1a 64-bit hash of its original's bytes, so that it is
 * translated once wherever the game has it, and in another version of the
 * data too:
 * - the usecode's literals: the interpreter's OP_PUSH_STRING makes a
 *   literal's translation in its place; the usecode joins the translated
 *   pieces and numbers as it joins the original's;
 * - the movies' subtitles: each line as the port reads it (UTF-8,
 *   subtitles.h), replaced before the movie plays; its frames stay.
 * The usecode, the subtitle files, the saves and English stay as they were.
 * The translations come from port/lang/<language>/<game>/usecode.jsonl and
 * subtitles.jsonl through port/tools/make_text.py (each game's
 * translations.cpp). */
#ifndef PORT_TRANSLATE_H
#define PORT_TRANSLATE_H
#include <cstddef>
#include <cstdint>
#include "common.h"
#include "language.h"

/* A text's translation, UTF-8, with the original's controls (in the
 * usecode's: ^ a new row, * a pause). */
struct Translation {
    std::uint64_t hash;
    const char *text;
};

/* A language's translations of one kind of text, by hash. */
struct Translations {
    Language language;
    const Translation *texts;
    std::size_t count;
};

/* The FNV-1a 64-bit hash of length bytes at text (port/tools/usecode_text.py's
 * and subtitle_text.py's). */
std::uint64_t textHash(const char *text, std::size_t length);
/* The translation in the game's language of the usecode's literal of length
 * bytes at text; nullptr in English and for a literal it does not translate. */
const char *usecodeTranslation(const char *text, std::size_t length);
/* The same of a subtitle's line (UTF-8, as subtitles.h reads it). */
const char *subtitleTranslation(const char *text, std::size_t length);
/* The game's tables (translate_game.h). */
#include "translate_game.h"
#endif
