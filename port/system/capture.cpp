/* The capture of a replay's picture and sound (capture.h). */
#include "capture.h"
#include "nut.h"
#include "report.h"
#include "screen.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {

/* The capture's file, its first frame, whether the headers are out; the
 * output's samples as they come, and the sound made of them that the frames
 * have not taken (stereo pairs). */
std::FILE *out = nullptr;
std::uint64_t first = 0;
bool started = false;
/* The frame's width, the screen's at the first frame captured. */
u16 frameWidth = SCREEN_WIDTH;
Resampler resampler;
std::vector<i16> fifo;
/* A frame short of this many samples or fewer (the interpolation waits
 * for the input after the frame's time) holds its last sample; one shorter
 * than that has had the output off, silence. */
const std::size_t HELD_SAMPLES = 16;

/* The standard output for the capture alone: a copy of its descriptor in
 * binary, and the standard output itself to the standard error. */
std::FILE *standardOutput()
{
    std::fflush(stdout);
#ifdef _WIN32
    int copy = _dup(_fileno(stdout));
    if (copy < 0) {
        return nullptr;
    }
    _setmode(copy, _O_BINARY);
    _dup2(_fileno(stderr), _fileno(stdout));
    return _fdopen(copy, "wb");
#else
    int const copy = dup(STDOUT_FILENO);
    if (copy < 0) {
        return nullptr;
    }
    dup2(STDERR_FILENO, STDOUT_FILENO);
    return fdopen(copy, "wb");
#endif
}

void put(const std::vector<u8> &bytes)
{
    if (std::fwrite(bytes.data(), 1, bytes.size(), out) != bytes.size()) {
        portEnd(3, "the capture cannot be written");
    }
}

void closeCapture()
{
    if (out != nullptr) {
        std::fclose(out);
        out = nullptr;
    }
}

/* The sound of a frame: CAPTURE_FRAME_SAMPLES stereo pairs from the fifo,
 * made up when it is short. */
std::vector<i16> frameSound()
{
    const std::size_t wanted = 2 * static_cast<std::size_t>(CAPTURE_FRAME_SAMPLES);
    const std::size_t taken = std::min(fifo.size(), wanted);
    std::vector<i16> sound(fifo.begin(), fifo.begin() + static_cast<std::ptrdiff_t>(taken));
    fifo.erase(fifo.begin(), fifo.begin() + static_cast<std::ptrdiff_t>(taken));
    const bool hold = taken >= 2 && wanted - taken <= 2 * HELD_SAMPLES;
    const i16 left = hold ? sound[taken - 2] : 0;
    const i16 right = hold ? sound[taken - 1] : 0;
    while (sound.size() < wanted) {
        sound.push_back(left);
        sound.push_back(right);
    }
    return sound;
}

std::vector<u8> littleEndian(const std::vector<i16> &samples)
{
    std::vector<u8> bytes(samples.size() * 2);
    for (std::size_t k = 0; k < samples.size(); k++) {
        bytes[2 * k] = static_cast<u8>(samples[k]);
        bytes[(2 * k) + 1] = static_cast<u8>(static_cast<u16>(samples[k]) >> 8);
    }
    return bytes;
}

}

std::vector<u8> capturePicture(const ScreenView *view, u16 width)
{
    std::vector<u8> rgb(static_cast<std::size_t>(width) * CAPTURE_HEIGHT * 3, 0);
    if ((view == nullptr) || (view->width == 0U) || (view->height == 0U) || view->width <= 2 * view->sides) {
        return rgb;
    }
    /* The whole view, or its middle without the sides as 4:3 in the frame's
     * middle. */
    const bool whole = view->width == width && view->height == CAPTURE_HEIGHT;
    const u16 columns = whole ? width : std::min<u16>(SCREEN_WIDTH, width);
    const u16 left = static_cast<u16>((width - columns) / 2);
    const u16 skipped = whole ? 0 : view->sides;
    const u16 shown = whole ? view->width : static_cast<u16>(view->width - (2 * view->sides));
    const u16 dimmedRight = static_cast<u16>(view->width - view->sides);
    for (u16 y = 0; y < CAPTURE_HEIGHT; y++) {
        const u16 from = static_cast<u16>(static_cast<u32>(y) * view->height / CAPTURE_HEIGHT);
        u8 *row = &rgb[((static_cast<std::size_t>(y) * width) + left) * 3];
        for (u16 x = 0; x < columns; x++) {
            const u16 column = static_cast<u16>(skipped + (static_cast<u32>(x) * shown / columns));
            u32 colour = viewColour(*view, column, from);
            if (whole && view->dimmed && (column < view->sides || column >= dimmedRight)) {
                colour = (colour >> 1) & 0x7f7f7fU;
            }
            row[3 * x] = static_cast<u8>(colour >> 16);
            row[(3 * x) + 1] = static_cast<u8>(colour >> 8);
            row[(3 * x) + 2] = static_cast<u8>(colour);
        }
    }
    return rgb;
}

std::vector<i16> captureSamples(const u8 *bytes, u32 size, bool sixteen)
{
    std::vector<i16> samples;
    if (sixteen) {
        samples.reserve(size / 2);
        for (u32 k = 0; k + 1 < size; k += 2) {
            samples.push_back(static_cast<i16>(static_cast<u16>(bytes[k] | bytes[k + 1] << 8)));
        }
    } else {
        samples.reserve(size);
        for (u32 k = 0; k < size; k++) {
            samples.push_back(static_cast<i16>((bytes[k] - 0x80) * 256));
        }
    }
    return samples;
}

Resampled resample(Resampler state, const std::vector<i16> &samples)
{
    Resampled result;
    state.input.insert(state.input.end(), samples.begin(), samples.end());
    /* The whole pairs: a piece can end between a pair's two samples (the
     * output's units are a channel's, audio.cpp), and the other comes with
     * the next. Counted by the piece, each such end would lose half a pair
     * and the sound fall behind the picture. */
    state.in = state.kept + (state.input.size() / 2);
    for (;;) {
        const std::uint64_t place = state.out * state.rate;
        const std::uint64_t at = place / CAPTURE_RATE;
        const std::uint64_t part = place % CAPTURE_RATE;
        if (at + ((part != 0U) ? 1 : 0) >= state.in) {
            break;
        }
        const std::size_t index = static_cast<std::size_t>(at - state.kept) * 2;
        for (int channel = 0; channel < 2; channel++) {
            const std::int64_t a = state.input[index + channel];
            const std::int64_t b = (part != 0U) ? state.input[index + 2 + channel] : a;
            result.output.push_back(static_cast<i16>(
                (a * static_cast<std::int64_t>(CAPTURE_RATE - part) + b * static_cast<std::int64_t>(part)) /
                static_cast<std::int64_t>(CAPTURE_RATE)));
        }
        state.out++;
    }
    const std::uint64_t needed = state.out * state.rate / CAPTURE_RATE;
    if (needed > state.kept) {
        const std::uint64_t drop = std::min<std::uint64_t>(needed, state.in) - state.kept;
        state.input.erase(state.input.begin(), state.input.begin() + static_cast<std::ptrdiff_t>(drop * 2));
        state.kept += drop;
    }
    result.state = std::move(state);
    return result;
}

void startCapture(const std::string &path, std::uint64_t from)
{
    out = path == "-" ? standardOutput() : std::fopen(path.c_str(), "wb");
    if (out == nullptr) {
        portEnd(3, "the capture cannot be written to %s", path.c_str());
    }
    std::setvbuf(out, nullptr, _IOFBF, 1 << 20);
    first = from;
    std::atexit(closeCapture);
}

bool capturing()
{
    return out != nullptr;
}

void captureFrame(std::uint64_t frame)
{
    if (out == nullptr) {
        return;
    }
    const std::vector<i16> sound = frameSound();
    if (frame < first) {
        return;
    }
    if (!started) {
        frameWidth = wideFrameWidth();
        put(nut::headers({frameWidth, CAPTURE_HEIGHT, 60, CAPTURE_RATE, 2}));
        started = true;
    }
    const std::uint64_t index = frame - first;
    put(nut::syncpoint(index));
    const std::vector<u8> soundBytes = littleEndian(sound);
    put(nut::frameHeader(nut::AUDIO, index * CAPTURE_FRAME_SAMPLES, static_cast<u32>(soundBytes.size())));
    put(soundBytes);
    ScreenView view;
    const std::vector<u8> picture = capturePicture(viewWideScreen(view) ? &view : nullptr, frameWidth);
    put(nut::frameHeader(nut::VIDEO, index, static_cast<u32>(picture.size())));
    put(picture);
}

void captureAudio(const u8 *bytes, u32 size, u32 rate, bool sixteen)
{
    if ((out == nullptr) || (size == 0U)) {
        return;
    }
    if (rate != resampler.rate) {
        resampler = Resampler{};
        resampler.rate = rate;
    }
    Resampled made = resample(std::move(resampler), captureSamples(bytes, size, sixteen));
    resampler = std::move(made.state);
    fifo.insert(fifo.end(), made.output.begin(), made.output.end());
}
