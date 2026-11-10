/* The translations of the game's text. See translate.h. */
#include "translate.h"

#include <algorithm>

namespace {

/* The translation of a text in the game's language among tables. */
const char *translationIn(const Translations *tables, std::size_t count, const char *text, std::size_t length)
{
    if (gameLanguage == Language::ENGLISH) {
        return nullptr;
    }
    for (std::size_t table = 0; table < count; table++) {
        const Translations &translations = tables[table];
        if (translations.language != gameLanguage) {
            continue;
        }
        const std::uint64_t hash = textHash(text, length);
        const Translation *end = translations.texts + translations.count;
        const Translation *found =
            std::lower_bound(translations.texts, end, hash,
                             [](const Translation &translation, std::uint64_t key) { return translation.hash < key; });
        if (found != end && found->hash == hash) {
            return found->text;
        }
        return nullptr;
    }
    return nullptr;
}

}

std::uint64_t textHash(const char *text, std::size_t length)
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (std::size_t i = 0; i < length; i++) {
        hash ^= static_cast<u8>(text[i]);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

const char *usecodeTranslation(const char *text, std::size_t length)
{
    return translationIn(tableStart(USECODE_TRANSLATION_TABLES), USECODE_TRANSLATION_TABLE_COUNT, text, length);
}

const char *subtitleTranslation(const char *text, std::size_t length)
{
    return translationIn(tableStart(SUBTITLE_TRANSLATION_TABLES), SUBTITLE_TRANSLATION_TABLE_COUNT, text, length);
}
