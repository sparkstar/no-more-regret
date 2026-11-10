/* The capture: a replay's picture and sound as NUT (nut.h), to a file or to
 * the standard output for ffmpeg. A frame each sixtieth of a second of the
 * virtual time, the replay's frames (replay.h): the screen as the window
 * shows it, the frame's width x 480 of red, green and blue, and the sound
 * of the time before it, 735 samples of 44100 a second in two channels,
 * made from the output's (audio.h) by linear interpolation; silence while
 * the output is off. Picture and sound keep to the virtual time, so they
 * keep to each other. With the standard output the run's other output (the
 * text mode's, the reports) goes to the standard error.
 *
 * The frame's width is the screen's at the first frame captured: 640, or
 * the wide screen's (video.h's wideFrameWidth), which a capture without a
 * window takes from the shape of the widest display (window.h's
 * displayWideWidth; WIDE_DEFAULT without one wider than 4:3). A stream
 * keeps its size: the wide screen turned on or off later is drawn into the
 * frame the stream began with. */
#ifndef PORT_CAPTURE_H
#define PORT_CAPTURE_H
#include "common.h"
#include "video.h"
#include "screen.h"

#include <cstdint>
#include <string>
#include <vector>

const u16 CAPTURE_HEIGHT = SCREEN_HEIGHT;
const u32 CAPTURE_RATE = 44100;
const u32 CAPTURE_FRAME_SAMPLES = CAPTURE_RATE / 60;

/* The capture to path ("-": the standard output) from the frame from on. */
void startCapture(const std::string &path, std::uint64_t from);
bool capturing();
/* The replay's frame: its picture and the sound before it, from the
 * capture's first frame on. */
void captureFrame(std::uint64_t frame);
/* The output's bytes as they go out (audio.cpp): stereo samples of rate,
 * 16-bit signed or 8-bit unsigned. */
void captureAudio(const u8 *bytes, u32 size, u32 rate, bool sixteen);

/* The pure parts. */
/* The picture of a screen in a frame width x CAPTURE_HEIGHT, 3 bytes a
 * pixel, as the window draws it: a view as wide as the frame (the wide
 * screen with its sides) as it is, the sides at half their colours when
 * dimmed; else the view without its sides scaled to 640x480 (mode 13h's
 * 320x200 stretched), nearest pixel, in the frame's middle with black
 * beside it; all black without a view (a text mode). */
[[nodiscard]] std::vector<u8> capturePicture(const ScreenView *view, u16 width);
/* The output's bytes as 16-bit stereo samples, left first. */
[[nodiscard]] std::vector<i16> captureSamples(const u8 *bytes, u32 size, bool sixteen);
/* The resampling of one rate to CAPTURE_RATE: output sample n is at input
 * place n x rate / CAPTURE_RATE, between two input samples; it waits for
 * the second. The input kept is from the first one an output still needs. */
struct Resampler {
    u32 rate = 0;
    /* The whole pairs in and the samples out since the rate began, and
     * the input kept (stereo pairs, and a pair's first sample at its end
     * while the second has not come) from pair kept. */
    std::uint64_t in = 0;
    std::uint64_t out = 0;
    std::uint64_t kept = 0;
    std::vector<i16> input;
};
struct Resampled {
    Resampler state;
    std::vector<i16> output;
};
[[nodiscard]] Resampled resample(Resampler state, const std::vector<i16> &samples);
#endif
