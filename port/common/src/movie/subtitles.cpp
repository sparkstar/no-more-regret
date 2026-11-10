/* The movies' subtitles. See subtitles.h. */
#include <cstdlib>
#include <cstring>
#include "subtitles.h"
#include "cafont.h"
#include "hangul.h"
#include "translate.h"
#include "file.h"
#include "utility.h"
#include "utf8.h"
#include "viewport.h"
#include "x32code.h"
#include "screen.h"

#include <memory>

namespace {

/* The bytes of a line of the 16-bit screen (640 pixels). */
const long HI_COLOUR_PITCH = SCREEN_WIDTH * 2;

/* CP437's upper half as code points; its lower half is ASCII's. */
const char16_t CP437_HIGH[128] = {
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee,
    0x00ec, 0x00c4, 0x00c5, 0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6,
    0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192, 0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa,
    0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510, 0x2514,
    0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550,
    0x256c, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c,
    0x2588, 0x2584, 0x258c, 0x2590, 0x2580, 0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
    0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229, 0x2261, 0x00b1, 0x2265, 0x2264, 0x2320,
    0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0};

/* The CP437 byte of a code point; -1 when CP437 has none. */
int cp437Byte(char32_t point)
{
    if (point < 0x80) {
        return static_cast<int>(point);
    }
    for (int i = 0; i < 128; i++) {
        if (static_cast<char32_t>(CP437_HIGH[i]) == point) {
            return 0x80 + i;
        }
    }
    return -1;
}

void appendUtf8(std::string &out, char32_t point)
{
    if (point < 0x80) {
        out += static_cast<char>(point);
    } else if (point < 0x800) {
        out += static_cast<char>(0xc0 | point >> 6);
        out += static_cast<char>(0x80 | (point & 0x3f));
    } else if (point < 0x10000) {
        out += static_cast<char>(0xe0 | point >> 12);
        out += static_cast<char>(0x80 | (point >> 6 & 0x3f));
        out += static_cast<char>(0x80 | (point & 0x3f));
    } else {
        out += static_cast<char>(0xf0 | point >> 18);
        out += static_cast<char>(0x80 | (point >> 12 & 0x3f));
        out += static_cast<char>(0x80 | (point >> 6 & 0x3f));
        out += static_cast<char>(0x80 | (point & 0x3f));
    }
}

/* A font the strip draws code points with. */
class SubtitleFont {
public:
    virtual ~SubtitleFont() = default;
    virtual bool has(char32_t point) = 0;
    /* The width of a run of code points it has, and the run drawn with its
     * pen at x, y of the draw viewport. */
    virtual i16 width(const std::u32string &run) = 0;
    virtual void draw(i16 x, i16 y, const std::u32string &run) = 0;
};

/* A font of fonts.flx: the code points of CP437 whose glyph the shape has,
 * drawn as GumpFont draws text. */
class ShapeFont : public SubtitleFont {
public:
    explicit ShapeFont(i16 number) : font(number), glyphs(fontCache->frameCount(number)) {}
    bool has(char32_t point) override
    {
        const int byte = cp437Byte(point);
        return byte >= 0 && byte < glyphs;
    }
    i16 width(const std::u32string &run) override { return font.textWidth(bytes(run).c_str()); }
    void draw(i16 x, i16 y, const std::u32string &run) override
    {
        font.moveTo(x, y);
        font.putText(bytes(run).c_str());
    }

private:
    CAFont font;
    int glyphs;

    static std::string bytes(const std::u32string &run)
    {
        std::string out;
        for (const char32_t point : run) {
            out += static_cast<char>(cp437Byte(point));
        }
        return out;
    }
};

/* The Hangul font (hangul.h) beside a font of fonts.flx: its syllables
 * drawn in the style's ink with their bottoms on the shape font's (the
 * bottom of its 'A'), a syllable HANGUL_ADVANCE wide. */
class HangulFont : public SubtitleFont {
public:
    HangulFont(i16 shapeFont, u8 newInk) : ink(newInk)
    {
        ShapeFrame *frame = fontCache->getFrame(shapeFont, 'A');
        bottom = static_cast<s16>(frame->height - 1 - frame->yOffset);
    }
    bool has(char32_t point) override { return hangulGlyph(point) != nullptr; }
    i16 width(const std::u32string &run) override { return static_cast<i16>(run.size() * HANGUL_ADVANCE); }
    void draw(i16 x, i16 y, const std::u32string &run) override
    {
        for (const char32_t point : run) {
            drawHangul(*hangulGlyph(point), x, static_cast<s16>(y + bottom - (HANGUL_SIZE - 1)), ink);
            x += HANGUL_ADVANCE;
        }
    }

private:
    u8 ink;
    /* The last line of the shape font's glyphs from the pen's line. */
    s16 bottom;
};

/* The text in runs, each of the first font of fonts that has its code
 * points; code points no font has are left out. */
struct Run {
    SubtitleFont *font;
    std::u32string points;
};

std::vector<Run> runs(const std::vector<std::unique_ptr<SubtitleFont>> &fonts, const std::string &text)
{
    std::vector<Run> out;
    for (const char32_t point : codePoints(text)) {
        SubtitleFont *chosen = nullptr;
        for (const auto &font : fonts) {
            if (font->has(point)) {
                chosen = font.get();
                break;
            }
        }
        if (chosen == nullptr) {
            continue;
        }
        if (out.empty() || out.back().font != chosen) {
            out.push_back({chosen, {}});
        }
        out.back().points += point;
    }
    return out;
}

/* The strip's place on the screen. */
i16 stripTop(const SubtitleStyle &style, i16 videoBottom)
{
    if (style.screenY >= 0) {
        return style.screenY;
    }
    return videoBottom + style.room < style.lowest ? static_cast<i16>(videoBottom + style.gap) : style.lowest;
}

}

std::string utf8FromCp437(const char *text)
{
    std::string out;
    for (; *text != 0; text++) {
        const u8 byte = static_cast<u8>(*text);
        appendUtf8(out, byte < 0x80 ? static_cast<char32_t>(byte) : static_cast<char32_t>(CP437_HIGH[byte - 0x80]));
    }
    return out;
}

const SubtitleCue *SubtitleTrack::appearing(u32 frame) const
{
    for (const SubtitleCue &cue : cues) {
        if (cue.frame == frame) {
            return &cue;
        }
    }
    return nullptr;
}

namespace {
/* The whole of a file the game opens by path; false when it cannot. */
bool readWhole(const char *path, std::vector<u8> &data)
{
    if (fileExists(path) == 0) {
        return false;
    }
    File file;
    file.open(path, 0, 0);
    if (file.isOpen() == 0U) {
        return false;
    }
    data.resize(file.size());
    const u32 read = data.empty() ? 0 : file.read(data.data(), data.size());
    file.close();
    data.resize(read);
    return true;
}
}

bool readIffSubtitles(const char *path, const char *language, SubtitleTrack &track)
{
    track.cues.clear();
    std::vector<u8> data;
    if (!readWhole(path, data) || data.size() < 8) {
        return false;
    }
    /* The first place the language's letters are (findSubtitles), then the
     * chunk's size, big-endian. */
    size_t at = 0;
    while (at + 8 <= data.size() && memcmp(&data[at], language, 4) != 0) {
        at++;
    }
    if (at + 8 > data.size()) {
        return false;
    }
    size_t end = at + 8 +
                 (static_cast<u32>(data[at + 4]) << 24 | static_cast<u32>(data[at + 5]) << 16 |
                  static_cast<u32>(data[at + 6]) << 8 | data[at + 7]);
    end = end < data.size() ? end : data.size();
    at += 8;
    /* The lines up to the chunk's end or a negative frame. */
    for (u32 number = 0; at + 5 <= end; number++) {
        const s32 frame = static_cast<s32>(static_cast<u32>(data[at]) | static_cast<u32>(data[at + 1]) << 8 |
                                           static_cast<u32>(data[at + 2]) << 16 | static_cast<u32>(data[at + 3]) << 24);
        if (frame < 0) {
            break;
        }
        const u8 length = data[at + 4];
        const std::string text(reinterpret_cast<const char *>(&data[at + 5]),
                               at + 5 + length <= data.size() ? length : 0);
        track.cues.push_back({number, static_cast<u32>(frame), utf8FromCp437(text.c_str())});
        at += 5 + length;
    }
    return true;
}

bool readTextSubtitles(const char *path, SubtitleTrack &track)
{
    track.cues.clear();
    std::vector<u8> data;
    if (!readWhole(path, data)) {
        return false;
    }
    /* The lines, without their CR and LF. */
    std::vector<std::string> lines;
    std::string line;
    for (const u8 byte : data) {
        if (byte == '\n') {
            lines.push_back(line);
            line.clear();
        } else if (byte != '\r') {
            line += static_cast<char>(byte);
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }
    /* Each marker's frame (from its eighth character) and the line after it
     * when that is no marker and not blank. */
    struct Marker {
        s32 frame;
        std::string text;
    };
    std::vector<Marker> markers;
    for (size_t i = 0; i < lines.size(); i++) {
        if (lines[i].empty() || lines[i][0] != '@') {
            continue;
        }
        Marker marker{lines[i].size() > 7 ? static_cast<s32>(atoi(lines[i].c_str() + 7)) : 0, {}};
        if (i + 1 < lines.size() && (lines[i + 1].empty() || lines[i + 1][0] != '@') &&
            lines[i + 1].find_first_not_of(" \t") != std::string::npos) {
            marker.text = utf8FromCp437(lines[i + 1].c_str());
        }
        markers.push_back(marker);
    }
    if (markers.empty()) {
        return false;
    }
    /* Each line appears at its marker's frame and stays until the next one
     * appears (the original's countdown showed the k-th k frames late: each
     * line stayed a frame more; set to the file's frames). A
     * file without text shows no strip, as the original, which turned the
     * subtitles off (z03.txt's lone marker). */
    bool text = false;
    for (size_t k = 0; k < markers.size(); k++) {
        if (markers[k].frame >= 0) {
            track.cues.push_back({static_cast<u32>(k), static_cast<u32>(markers[k].frame), markers[k].text});
            text = text || !markers[k].text.empty();
        }
    }
    if (!text) {
        track.cues.clear();
    }
    return text;
}

void translateSubtitles(SubtitleTrack &track)
{
    for (SubtitleCue &cue : track.cues) {
        if (const char *translated = subtitleTranslation(cue.text.data(), cue.text.size())) {
            cue.text = translated;
        }
    }
}

void drawSubtitle(const SubtitleStyle &style, const std::string &text, i16 videoBottom, u8 light, u8 dark,
                  bool hiColour)
{
    /* The game's font, and the Hangul font for what it lacks. */
    std::vector<std::unique_ptr<SubtitleFont>> fonts;
    fonts.push_back(std::make_unique<ShapeFont>(style.font));
    fonts.push_back(std::make_unique<HangulFont>(style.font, style.ink));
    std::vector<Run> parts = runs(fonts, text);
    i16 width = 0;
    for (const Run &run : parts) {
        width += run.font->width(run.points);
    }
    i16 x = static_cast<i16>((static_cast<i16>(style.centreWidth - width) / 2) + style.shift);
    auto drawText = [&](i16 y) {
        i16 pen = x;
        for (const Run &run : parts) {
            run.font->draw(pen, y, run.points);
            pen += run.font->width(run.points);
        }
    };
    if (hiColour) {
        /* Drawn in a strip of its own, then white on black over the draw
         * viewport's first bytes, whose first lines go to the screen. */
        Viewport strip(VIEWPORT_LARGE, 0, 0, style.stripWidth - 1, style.stripHeight - 1);
        strip.clear(style.stripColour);
        {
            const DrawTarget target(&strip);
            drawText(style.hiColourTextY);
        }
        u8 *pixels = strip.address(0, 0);
        u16 *mask = reinterpret_cast<u16 *>(drawViewport->address(0, 0));
        for (i16 i = 0; i < style.stripWidth * style.stripHeight; i++) {
            mask[i] = (pixels[i] != 0U) ? 0 : 0xffff;
        }
        memcpy(svgaMemory + (style.screenY * HI_COLOUR_PITCH), drawViewport->address(0, 0),
               static_cast<size_t>(style.hiColourLines) * HI_COLOUR_PITCH);
        return;
    }
    drawViewport->clear(style.stripColour, 0, 0, style.stripWidth - 1, style.stripHeight - 1);
    drawText(style.textY);
    if (style.twoTone) {
        for (i16 y = 0; y < style.stripHeight; y++) {
            u8 *row = drawViewport->address(0, y);
            for (i16 column = 0; column < style.stripWidth; column++) {
                row[column] = row[column] == 0 ? light : dark;
            }
        }
    }
    screenViewport->blit(drawViewport, 0, 0, style.stripWidth - 1, style.stripHeight - 1, style.screenX,
                         stripTop(style, videoBottom));
}
