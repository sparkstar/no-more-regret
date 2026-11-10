/* No Remorse's sound archive (common/src/sound/dsfxman.cpp). */
#ifndef PORT_DSFXMAN_GAME_H
#define PORT_DSFXMAN_GAME_H
#include "file.h"

/* sound.flx: a TagFlex, read and written (12d8:03be, 03ca; No Regret's a
 * FastFlex, read). */
inline constexpr bool SOUND_FLEX_TAGGED = true;
inline constexpr int SOUND_FLEX_MODE = FILE_READWRITE;
#endif
