/* No Regret's values of the movies (common/src/movie). */
#ifndef PORT_MOVIE_GAME_H
#define PORT_MOVIE_GAME_H
#include "subtitles.h"

/* The movies' host and keys are XANPLAY.C's (moviehost.cpp; No Remorse's
 * PLAYER.C's). */
inline constexpr bool XAN_MOVIE_HOST = true;

/* XANPLAY.C's subtitles (1440:036c): CAFont 3 in a strip of 15 lines cleared
 * with colour 1, centred on the 640 pixels, its pen on line 10, the glyphs'
 * colour 0 white (the movie's palette's entry 255) and all else the movie's
 * darkest colour, the strip on line 420; in hi-colour the pen on line 8 and
 * the strip's first 10 lines white on black. The other languages' glyphs in
 * font 3's ink, colour 0. */
inline constexpr SubtitleStyle SUBTITLE_STYLE = {.font = 3,
                                                 .stripWidth = 640,
                                                 .stripHeight = 15,
                                                 .stripColour = 1,
                                                 .centreWidth = 640,
                                                 .shift = 0,
                                                 .textY = 10,
                                                 .twoTone = true,
                                                 .screenX = 0,
                                                 .screenY = 420,
                                                 .gap = 0,
                                                 .room = 0,
                                                 .lowest = 0,
                                                 .hiColourTextY = 8,
                                                 .hiColourLines = 10,
                                                 .ink = 0};
#endif
