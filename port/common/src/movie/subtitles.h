/* The movies' subtitles (the movies and subtitles): a movie's lines in
 * UTF-8 and the frames they appear at, read from either game's files,
 * and drawn in a strip as each game draws them (its SubtitleStyle,
 * movie_game.h). The text is UTF-8 from the files on, and the strip
 * draws it by code point from a list of fonts, so that other languages
 * and fonts need only be added. */
#ifndef PORT_SUBTITLES_H
#define PORT_SUBTITLES_H
#include "common.h"

#include <string>
#include <vector>

/* A line of a movie's subtitles. */
struct SubtitleCue {
    /* Its place among the file's lines, from 0. */
    u32 number = 0;
    /* The frame it appears at, as the player counts its frames (from 0); it
     * stays until the next cue appears. */
    u32 frame = 0;
    /* What it says, in UTF-8; empty: the strip is cleared. */
    std::string text;
};

/* A movie's cues, by their frames. */
class SubtitleTrack {
public:
    std::vector<SubtitleCue> cues;

    /* The cue that appears at frame; nullptr when none does. */
    [[nodiscard]] const SubtitleCue *appearing(u32 frame) const;
};

/* No Regret's IFF subtitles: in the chunk of language (its four letters,
 * ENGL for English), each line's frame (4 bytes), length and text (CP437);
 * a line appears at the frame of its number (XANPLAY.C's findSubtitle).
 * False when the file or the chunk is not there. */
bool readIffSubtitles(const char *path, const char *language, SubtitleTrack &track);

/* No Remorse's TXT subtitles: an "@frame N" line, then the line it says
 * (CP437), blank or another marker for none; each line appears at frame N.
 * False when the file is not there or has no text (z03.txt's lone marker):
 * no subtitles. Unlike the original (1470's parse and VideoPlayer's
 * showSubtitle), a line that begins with neither a letter nor '.' is shown
 * (03d.txt's "'cause", 14a.txt's " in which"), and a line is not one frame
 * later than the one before (its countdown's). */
bool readTextSubtitles(const char *path, SubtitleTrack &track);

/* NUL-ended CP437 text as UTF-8. */
std::string utf8FromCp437(const char *text);

/* How a game draws its subtitles (its movie_game.h): a strip in the draw
 * viewport's top left, cleared, the text centred in it and copied to the
 * screen. */
struct SubtitleStyle {
    /* The CAFont of the text (fonts.flx's shape; 3 and 4 are the same in
     * both games). */
    i16 font;
    /* The strip and the colour it is cleared with. */
    i16 stripWidth;
    i16 stripHeight;
    u8 stripColour;
    /* The text is centred in centreWidth and moved right by shift, its pen
     * on line textY of the strip. */
    i16 centreWidth;
    i16 shift;
    i16 textY;
    /* Two tones: the glyphs' pixels of colour 0 become the light colour and
     * all else the dark (No Regret's); else the glyphs as drawn. */
    bool twoTone;
    /* Where the strip goes on the screen: at x, on line screenY, or with
     * screenY -1 under the video: gap lines below it when room lines fit
     * below it above lowest, else on lowest. */
    i16 screenX;
    i16 screenY;
    i16 gap;
    i16 room;
    i16 lowest;
    /* On the 16-bit screen (No Regret's hi-colour movies): the pen's line,
     * and the strip's lines that go to the screen, white on black. */
    i16 hiColourTextY;
    i16 hiColourLines;
    /* The colour of the font's ink, which the glyphs of the other languages'
     * fonts (the Hangul font, hangul.h) are drawn in. */
    u8 ink;
};

/* Each line of the track in the game's language: its translation
 * (translate.h) in place of its text, its frames as they were. */
void translateSubtitles(SubtitleTrack &track);

/* Draws text (UTF-8) as style draws it, under the video whose last line is
 * videoBottom - 1, with the light and dark colours of a two-tone style, on
 * the 16-bit screen when hiColour. */
void drawSubtitle(const SubtitleStyle &style, const std::string &text, i16 videoBottom, u8 light, u8 dark,
                  bool hiColour);
#endif
