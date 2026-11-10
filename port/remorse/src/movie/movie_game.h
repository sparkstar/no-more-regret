/* No Remorse's values of the movies (common/src/movie). */
#ifndef PORT_MOVIE_GAME_H
#define PORT_MOVIE_GAME_H
#include "subtitles.h"

/* The movies' host and keys are No Regret's XANPLAY.C's (moviehost.cpp);
 * No Remorse's are PLAYER.C's. */
inline constexpr bool XAN_MOVIE_HOST = false;

/* PLAYER.C's subtitles (showSubtitle): CAFont 4 in a strip of 601 by 16
 * cleared with colour 0, centred on 600 pixels and 4 to the right (the
 * original measures all but the last two characters and the line's end
 * before them: in that font, 8 pixels a glyph, one glyph less), its pen on
 * line 2, the glyphs as drawn; the strip at x 20, 8 lines under the video,
 * or on line 470 when fewer than 10 lines are left above it. No hi-colour.
 * The other languages' glyphs in font 4's ink, colour 8. */
inline constexpr SubtitleStyle SUBTITLE_STYLE = {.font = 4,
                                                 .stripWidth = 601,
                                                 .stripHeight = 16,
                                                 .stripColour = 0,
                                                 .centreWidth = 600,
                                                 .shift = 4,
                                                 .textY = 2,
                                                 .twoTone = false,
                                                 .screenX = 20,
                                                 .screenY = -1,
                                                 .gap = 8,
                                                 .room = 10,
                                                 .lowest = 470,
                                                 .hiColourTextY = 0,
                                                 .hiColourLines = 0,
                                                 .ink = 8};
#endif
