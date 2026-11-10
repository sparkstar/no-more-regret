/* The sound's output (audio.h): the units the ring's output has passed by a
 * time are the virtual time's, from its start S: (t - S) x rate x 2 / PIT_HZ.
 * The samples go out as the time passes them (passTime calls audioTime): the
 * game writes the ring only at those times, so the bytes are the ones an
 * output reading all the while gets. They go to REGRET_AUDIO, a WAV file, to
 * the window's audio (window.h) and to a replay's capture (capture.h). */
#include "audio.h"
#include "gamememory.h"
#include "pcstate.h"
#include "capture.h"
#include "pc.h"
#include "report.h"
#include "window.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

/* The output: its ring, its units, its start, the units it has passed and
 * taken, and where it stopped. */
bool playing = false;
bool sixteen = false;
const u8 *ring = nullptr;
u32 units = 0;
u32 rate = 22050;
std::uint64_t start = 0;
std::uint64_t taken = 0;
std::uint64_t stopped = 0;

FILE *wav = nullptr;
/* The WAV's samples' depth: the output's when it opened; the bytes of the
 * other depth (asylum.h's ASS_SetDepth) go into it in its own. */
bool wavSixteen = false;
/* The tap's (pcstate.h). */
bool tapping = false;
std::vector<u8> tapped;
u32 wavBytes = 0;

/* The units passed by now. */
std::uint64_t unitsBy(std::uint64_t now)
{
    return now < start ? 0 : (now - start) * rate * 2 / PIT_HZ;
}

void writeWavHeader()
{
    u8 header[44];
    auto put32 = [&](int at, u32 v) {
        for (int k = 0; k < 4; k++) {
            header[at + k] = static_cast<u8>(v >> 8 * k);
        }
    };
    auto put16 = [&](int at, u16 v) {
        header[at] = static_cast<u8>(v);
        header[at + 1] = static_cast<u8>(v >> 8);
    };
    std::memcpy(header, "RIFF", 4);
    put32(4, 36 + wavBytes);
    std::memcpy(header + 8, "WAVEfmt ", 8);
    put32(16, 16);
    put16(20, 1);
    put16(22, 2);
    put32(24, rate);
    const u16 align = wavSixteen ? 4 : 2;
    put32(28, rate * align);
    put16(32, align);
    put16(34, wavSixteen ? 16 : 8);
    std::memcpy(header + 36, "data", 4);
    put32(40, wavBytes);
    std::fseek(wav, 0, SEEK_SET);
    std::fwrite(header, 1, sizeof(header), wav);
    std::fseek(wav, 0, SEEK_END);
}

void closeWav()
{
    if (wav == nullptr) {
        return;
    }
    writeWavHeader();
    std::fclose(wav);
    wav = nullptr;
}

void openWav()
{
    const char *path = std::getenv("REGRET_AUDIO");
    if ((path == nullptr) || (wav != nullptr)) {
        return;
    }
    wav = std::fopen(path, "wb");
    if (wav == nullptr) {
        portEnd(3, "cannot write %s", path);
    }
    wavSixteen = sixteen;
    writeWavHeader();
    std::atexit(closeWav);
}

/* The output's bytes of one depth in another: a 16-bit sample's high
 * byte, an 8-bit one's offset from 80h as the high byte. */
std::vector<u8> outputDepth(const std::vector<u8> &bytes, bool from16, bool to16)
{
    if (from16 == to16) {
        return bytes;
    }
    std::vector<u8> made;
    if (to16) {
        made.reserve(bytes.size() * 2);
        for (const u8 byte : bytes) {
            made.push_back(0);
            made.push_back(static_cast<u8>(byte ^ 0x80));
        }
        return made;
    }
    made.reserve(bytes.size() / 2);
    for (std::size_t k = 0; k + 1 < bytes.size(); k += 2) {
        made.push_back(static_cast<u8>(bytes[k + 1] ^ 0x80));
    }
    return made;
}

/* The units passed to count: the bytes out. */
void take(std::uint64_t count)
{
    if (count <= taken) {
        return;
    }
    const u32 size = sixteen ? 2 : 1;
    std::vector<u8> bytes;
    bytes.reserve(static_cast<std::size_t>(count - taken) * size);
    for (; taken < count; taken++) {
        const u8 *unit = ring + (static_cast<u32>(taken % units) * size);
        bytes.insert(bytes.end(), unit, unit + size);
    }
    if (tapping) {
        tapped.insert(tapped.end(), bytes.begin(), bytes.end());
    }
    if (wav != nullptr) {
        const std::vector<u8> written = outputDepth(bytes, sixteen, wavSixteen);
        std::fwrite(written.data(), 1, written.size(), wav);
        std::fflush(wav);
        wavBytes += static_cast<u32>(written.size());
    }
    windowAudio(bytes.data(), static_cast<u32>(bytes.size()), rate, sixteen);
    captureAudio(bytes.data(), static_cast<u32>(bytes.size()), rate, sixteen);
}

}

void audioOutputStart(const std::uint8_t *newRing, std::uint32_t newUnits, std::uint32_t newRate, bool words)
{
    const SystemSide systemSide;
    playing = true;
    sixteen = words;
    ring = newRing;
    units = newUnits;
    rate = newRate;
    start = pcTime();
    taken = 0;
    openWav();
}

void audioOutputStop()
{
    const SystemSide systemSide;
    if (!playing) {
        return;
    }
    take(unitsBy(pcTime()));
    stopped = unitsBy(pcTime());
    playing = false;
}

std::uint64_t audioOutputTaken()
{
    const SystemSide systemSide;
    return playing ? unitsBy(pcTime()) : stopped;
}

void audioOutputTime(std::uint64_t now)
{
    if (playing) {
        take(unitsBy(now));
    }
}

AudioState audioState()
{
    return {playing, sixteen, ring, units, rate, start, taken, stopped};
}

void putAudioState(const AudioState &state)
{
    playing = state.playing;
    sixteen = state.sixteen;
    ring = state.ring;
    units = state.units;
    rate = state.rate;
    start = state.start;
    taken = state.taken;
    stopped = state.stopped;
}

void audioTap(bool on)
{
    tapping = on;
    tapped.clear();
}

std::vector<u8> takeAudioTap()
{
    std::vector<u8> bytes;
    bytes.swap(tapped);
    return bytes;
}
