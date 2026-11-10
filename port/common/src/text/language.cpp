/* The language of the game's text. See language.h. */
#include <cstring>
#include "gamestate.h"
#include "language.h"

Language &gameLanguage = gameSetting<Language>(Language::ENGLISH);

namespace {

struct LanguageName {
    Language language;
    const char *code;
};

const LanguageName LANGUAGE_NAMES[] = {
    {Language::ENGLISH, "en"},
    {Language::KOREAN, "ko"},
};

}

Language languageOfCode(const char *code)
{
    for (const LanguageName &name : LANGUAGE_NAMES) {
        if (strcmp(name.code, code) == 0) {
            return name.language;
        }
    }
    return Language::ENGLISH;
}

const char *languageCode(Language language)
{
    for (const LanguageName &name : LANGUAGE_NAMES) {
        if (name.language == language) {
            return name.code;
        }
    }
    return "en";
}
