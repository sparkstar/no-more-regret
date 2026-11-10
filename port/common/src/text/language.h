/* The language of the game's text (the game's text in other languages):
 * enhanced.cfg's language= (enhanced.h), English when it is not there. It is the player's setting,
 * not the game's state: the saves do not hold it. */
#ifndef PORT_LANGUAGE_H
#define PORT_LANGUAGE_H
#include "common.h"

enum class Language : u8 { ENGLISH, KOREAN };

extern Language &gameLanguage;

/* The language of a code of enhanced.cfg ("en", "ko"); English for a code
 * the port does not know. */
Language languageOfCode(const char *code);
/* The code enhanced.cfg writes for the language. */
const char *languageCode(Language language);
#endif
