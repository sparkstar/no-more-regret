/* Code5 (file name unknown): the game's options, read from and written
 * to crusader.cfg. */
#ifndef PORT_OPTIONS_H
#define PORT_OPTIONS_H
#include "common.h"

/* The movies play in 640x480 with 65536 colours (No Regret only). */
extern u8 &hiColorMovies;
/* crusader.cfg's noweasel: no weasel's shop between the missions (No
 * Remorse only). */
extern u8 &noWeasel;
/* The movies show their subtitles. No Remorse's PLAYER.C reads it far. */
extern u16 &movieSubtitles;

/* Reads crusader.cfg, or without one GOG's settings (options_game.h's
 * GOG_OPTIONS). */
void readOptions();
/* The settings of crusader.cfg's text: its key=value lines (# and / lines
 * are notes), the value's first word, both in lower case; with only, the
 * line of that key alone. */
void readOptionText(const char *text, const char *only);
/* Writes crusader.cfg anew. */
void writeOptions();
#endif
