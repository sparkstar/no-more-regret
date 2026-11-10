/* The movies' baseline of frames (host.h's hostMovieFrame, the movies and
 * subtitles): with REGRET_FRAMES set to a file, a line for each movie the
 * script plays ("movie", its name, form and place) and one for each of its
 * frames finished on the screen: the frame and a hash of 64 bits of the
 * screen as the mode shows it (screenHash: its colours, in eight lanes of
 * words at once, the tests' times), so that two builds of the port can be
 * compared frame by frame (port/test/scenario/movieframes.py). */
#include "host.h"
#include "gamememory.h"
#include "report.h"
#include "script.h"
#include "video.h"

#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

/* REGRET_FRAMES' file, opened at the first line; nullptr without it. */
std::FILE *framesFile()
{
    static std::FILE *file = [] {
        const char *path = std::getenv("REGRET_FRAMES");
        if (!path) {
            return static_cast<std ::FILE *>(nullptr);
        }
        std::FILE *opened = std::fopen(path, "w");
        if (!opened) {
            portRefuse("REGRET_FRAMES %s cannot be written", path);
        }
        return opened;
    }();
    return file;
}

const int LANES = 8;
const std::uint64_t LANE_START = 0xcbf29ce484222325ULL;
const std::uint64_t MULTIPLIER = 0x9e3779b97f4a7c15ULL;
const int ROTATION = 29;

/* A word into a lane: xor, an odd multiplier and a rotation, each of which
 * a lane can be told back from, so that a word other than another's makes
 * the lane another. */
std::uint64_t mixWord(std::uint64_t lane, std::uint64_t word)
{
    return std::rotl((lane ^ word) * MULTIPLIER, ROTATION);
}

/* The lanes of a mode (8 or 16 bits) at the start. */
void startLanes(std::uint64_t (&lanes)[LANES], u8 bits)
{
    for (int k = 0; k < LANES; k++) {
        lanes[k] = LANE_START ^ (static_cast<std::uint64_t>(bits) << 8 | static_cast<std::uint64_t>(k));
    }
}

/* The lanes into one hash, through MurmurHash3's finaliser. */
std::uint64_t endLanes(const std::uint64_t (&lanes)[LANES])
{
    std::uint64_t hash = LANE_START;
    for (const std::uint64_t lane : lanes) {
        hash = mixWord(hash, lane);
    }
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;
    return hash;
}

/* Two pixels' colours as a word, the first in the low half. */
std::uint64_t pairAt(const u32 (&colours)[256], const u8 *pixels)
{
    return static_cast<std::uint64_t>(colours[pixels[0]]) | static_cast<std::uint64_t>(colours[pixels[1]]) << 32;
}

/* Eight bytes as a word, the first the lowest. */
std::uint64_t wordAt(const u8 *bytes)
{
    std::uint64_t word;
    std::memcpy(&word, bytes, sizeof word);
    return word;
}

/* The 8-bit screen by its colours: two pixels' red, green and blue to a
 * word, the words of a row round the lanes in turn (a row's last pixels
 * past a multiple of 16 to the first, one to a word). The colours, not the
 * pixels' bytes: a screen the same colours through other entries of the
 * palette, or with another palette's entries no pixel shows, is the same
 * screen. */
std::uint64_t hashIndexed(const ScreenView &view)
{
    u32 colours[256];
    for (unsigned entry = 0; entry < 256; entry++) {
        const u8 *rgb = view.palette + (3 * entry);
        colours[entry] = static_cast<u32>(rgb[0]) << 16 | static_cast<u32>(rgb[1]) << 8 | static_cast<u32>(rgb[2]);
    }
    std::uint64_t lanes[LANES];
    startLanes(lanes, view.bits);
    for (u16 y = 0; y < view.height; y++) {
        const u8 *row = view.pixels + (static_cast<size_t>(y) * view.pitch);
        u16 x = 0;
        // The lanes one by one, so that they stay in registers.
        for (; x + 2 * LANES <= view.width; x += 2 * LANES) {
            const u8 *at = row + x;
            lanes[0] = mixWord(lanes[0], pairAt(colours, at));
            lanes[1] = mixWord(lanes[1], pairAt(colours, at + 2));
            lanes[2] = mixWord(lanes[2], pairAt(colours, at + 4));
            lanes[3] = mixWord(lanes[3], pairAt(colours, at + 6));
            lanes[4] = mixWord(lanes[4], pairAt(colours, at + 8));
            lanes[5] = mixWord(lanes[5], pairAt(colours, at + 10));
            lanes[6] = mixWord(lanes[6], pairAt(colours, at + 12));
            lanes[7] = mixWord(lanes[7], pairAt(colours, at + 14));
        }
        for (; x < view.width; x++) {
            lanes[0] = mixWord(lanes[0], colours[row[x]]);
        }
    }
    return endLanes(lanes);
}

/* The 16-bit screen by its pixels' bytes, eight to a word: each value is a
 * colour of its own (video.h's viewColour gives each another), so the bytes
 * are the colours. A row's last bytes past a multiple of 64 go to the first
 * lane, a pixel to a word. */
std::uint64_t hashHicolour(const ScreenView &view)
{
    std::uint64_t lanes[LANES];
    startLanes(lanes, view.bits);
    const size_t rowBytes = 2U * view.width;
    for (u16 y = 0; y < view.height; y++) {
        const u8 *row = view.pixels + (static_cast<size_t>(y) * view.pitch);
        size_t at = 0;
        for (; at + 8 * LANES <= rowBytes; at += 8 * LANES) {
            const u8 *bytes = row + at;
            lanes[0] = mixWord(lanes[0], wordAt(bytes));
            lanes[1] = mixWord(lanes[1], wordAt(bytes + 8));
            lanes[2] = mixWord(lanes[2], wordAt(bytes + 16));
            lanes[3] = mixWord(lanes[3], wordAt(bytes + 24));
            lanes[4] = mixWord(lanes[4], wordAt(bytes + 32));
            lanes[5] = mixWord(lanes[5], wordAt(bytes + 40));
            lanes[6] = mixWord(lanes[6], wordAt(bytes + 48));
            lanes[7] = mixWord(lanes[7], wordAt(bytes + 56));
        }
        for (; at < rowBytes; at += 2) {
            lanes[0] =
                mixWord(lanes[0], static_cast<std::uint64_t>(row[at]) | static_cast<std::uint64_t>(row[at + 1]) << 8);
        }
    }
    return endLanes(lanes);
}

/* The hash of the screen as the mode shows it; 0 in a text mode. */
std::uint64_t screenHash()
{
    ScreenView view;
    if (!viewScreen(view)) {
        return 0;
    }
    if (view.bits == 16) {
        return hashHicolour(view);
    }
    return hashIndexed(view);
}
}

void beginMovieFrames(const std::string &name, i16 form, i16 x, i16 y)
{
    if (std::FILE *file = framesFile()) {
        std::fprintf(file, "movie %s %d %d %d\n", name.c_str(), form, x, y);
    }
}

void hostMovieFrame(std::uint32_t frame)
{
    const SystemSide systemSide;
    if (std::FILE *file = framesFile()) {
        std::fprintf(file, "%u %016llx\n", frame, static_cast<unsigned long long>(screenHash()));
    }
}
