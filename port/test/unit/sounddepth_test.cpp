/* The port's depth of the sound: 16 the 16-bit output; 8 the 8-bit
 * output's mix put out as words at the 16-bit output's level, a quarter
 * of the 8-bit DMA's output (asyapi.h's setDepth). Against the 8-bit
 * DMA's output, which soundout8 holds to the original, every song of both
 * CDs; the effects, the movies' stream and a loud sum against the 16-bit
 * output; a change while the music and an effect play, which goes on in
 * the same ring; the depth kept when the arena is put back, the data made
 * the depth's. */
#include "asylib.h"
#include "asylum.h"
#include "amfload.h"
#include "audio.h"
#include "gamememory.h"
#include "host.h"
#include "pc.h"
#include "pcstate.h"
#include "window.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

int checks = 0;
int failures = 0;
std::vector<u8> outBytes;
bool outSixteen = false;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH sounddepth: %s\n", what.c_str());
    }
}

/* A looped square wave in SOUND.FLX's form (signed bytes) between size
 * and -size, a half period every 25 samples. */
std::vector<u8> squareSound(u32 length, u8 size)
{
    std::vector<u8> sound(0x14 + length);
    auto put32 = [&](u32 at, u32 value) {
        for (u32 k = 0; k < 4; k++) {
            sound[at + k] = (u8)(value >> (8 * k));
        }
    };
    put32(4, length);
    put32(8, 0);
    put32(0xc, length);
    sound[0x12] = (u8)22050;
    sound[0x13] = (u8)(22050 >> 8);
    for (u32 k = 0; k < length; k++) {
        sound[0x14 + k] = (u8)((k / 25) % 2 == 0 ? size : -size);
    }
    return sound;
}

std::vector<u8> tables()
{
    const asylum::Tables &t = asylum::data.tables;
    return std::vector<u8>(t.bytes + (asylum::CLIP_AT - asylum::TABLES_AT),
                           t.bytes + (asylum::CONFIG_AT - asylum::TABLES_AT));
}

void start(u16 bits, u16 dma = 5)
{
    asylum::loadData(asylum::data);
    ASS_SetDepth(bits);
    AssConfig config{3, 0x220, 7, dma, 22050, 1, 1};
    ASS_Init(&config);
    outBytes.clear();
}

std::vector<u8> startedTables(u16 dma, u16 bits)
{
    start(bits, dma);
    std::vector<u8> made = tables();
    ASS_Uninit();
    return made;
}

void play(double seconds)
{
    const std::uint64_t period = PIT_HZ / 60;
    const std::uint64_t end = pcTime() + (std::uint64_t)(seconds * PIT_HZ);
    while (pcTime() < end) {
        passTime(pcTime() + period);
        ASS_UpdateDAC();
    }
}

std::vector<i16> samples(const std::vector<u8> &bytes)
{
    std::vector<i16> made;
    for (std::size_t k = 0; k + 1 < bytes.size(); k += 2) {
        made.push_back((i16)(bytes[k] | (bytes[k + 1] << 8)));
    }
    return made;
}

/* The driver's first click left out of the levels. */
const std::size_t SKIP = 22050 / 10 * 2;

double power(const std::vector<i16> &made)
{
    double sum = 0;
    for (std::size_t k = SKIP; k < made.size(); k++) {
        sum += (double)made[k] * made[k];
    }
    return sum;
}

/* The 8-bit precision's level against the 16-bit output's, in dB. */
double levelOf(const std::vector<i16> &eight, const std::vector<i16> &sixteen)
{
    const double a = power(sixteen);
    const double b = power(eight);
    return a > 0 && b > 0 ? 10 * std::log10(b / a) : 100;
}

/* The same run at both depths: the same samples, each within 64 a channel
 * and 64 of the 16-bit one's (the 8-bit tables' lost bits; the 16-bit ones'
 * negatives one place over at 41h too, where the 8-bit output's is the
 * identity; the oversampling's halves), the same level. */
void compare(const std::vector<u8> &sixteen, const std::vector<u8> &eight, const std::string &name, int channels)
{
    const int error = 64 * (channels + 1);
    const std::vector<i16> a = samples(sixteen);
    const std::vector<i16> b = samples(eight);
    check(a.size() == b.size() && a.size() > SKIP, name + ": the same samples");
    int worst = 0;
    for (std::size_t k = SKIP; k < std::min(a.size(), b.size()); k++) {
        worst = std::max(worst, std::abs((int)a[k] - b[k]));
    }
    check(worst <= error, name + ": each sample within " + std::to_string(error) + " of the 16-bit output's (" +
                              std::to_string(worst) + ")");
    const double level = levelOf(b, a);
    check(std::abs(level) < 0.2, name + ": the 16-bit output's level (" + std::to_string(level) + " dB)");
}

/* The run at bits: its output, which is words. */
template <class Run> std::vector<u8> runAt(u16 bits, Run run, u16 dma = 5)
{
    start(bits, dma);
    run();
    ASS_Uninit();
    check(outSixteen == (bits != 0 || dma >= 4), "depth " + std::to_string(bits) + ": the output's samples' size");
    return outBytes;
}

/* The file of a CD at relative, each part in whatever case the CD has it
 * (one copy names its files in lower case, another in capitals); as named
 * where there is none. */
std::filesystem::path anyCase(const std::filesystem::path &root, const std::filesystem::path &relative)
{
    auto upper = [](std::string text) {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return text;
    };
    std::filesystem::path path = root;
    for (const auto &part : relative) {
        std::filesystem::path found = path / part;
        std::error_code error;
        for (const auto &entry : std::filesystem::directory_iterator(path, error)) {
            if (upper(entry.path().filename().string()) == upper(part.string())) {
                found = entry.path();
                break;
            }
        }
        path = found;
    }
    return path;
}

struct Song {
    std::vector<u8> bytes;
    AmfModule module{};

    explicit Song(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        bytes = std::vector<u8>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        if (bytes.size() < sizeof(AmfHeader)) {
            std::fprintf(stderr, "sounddepth: missing or short AMF: %s\n", path.string().c_str());
            std::exit(1);
        }
        module.header = reinterpret_cast<AmfHeader *>(bytes.data());
        std::size_t at = sizeof(AmfHeader);
        module.patterns = bytes.data() + at;
        at += module.header->patterns * AMF_PATTERN_SIZE;
        for (std::size_t i = 0; i < AMF_SAMPLES; i++) {
            const std::size_t length = module.header->sample[i].length;
            if (at > bytes.size() || length > bytes.size() - at) {
                std::fprintf(stderr, "sounddepth: short AMF samples: %s\n", path.string().c_str());
                std::exit(1);
            }
            module.samples[i] = length == 0 ? nullptr : bytes.data() + at;
            at += length;
        }
    }
};

/* Every song of a CD for 6 seconds: at 8 bits the 8-bit DMA's output,
 * which soundout8 holds to the original, a quarter (its byte's offset from
 * 80h times 64; the songs' sums never clip there), 800h samples sooner
 * (the 8-bit DMA's ring, 2000h bytes, has 1000h samples, the words' 800h:
 * the mix comes out a round later); against the 16-bit output, the level
 * the pans leave (the 8-bit mix's own, the 16-bit one's the song's), far
 * from the 8-bit DMA's 12 dB. */
const std::size_t LATER = 0x800 * 2;

void songs(const std::filesystem::path &directory, double &lowest, double &highest)
{
    std::vector<std::filesystem::path> paths;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        const std::string extension = entry.path().extension().string();
        if (extension == ".amf" || extension == ".AMF") {
            paths.push_back(entry.path());
        }
    }
    std::sort(paths.begin(), paths.end());
    check(!paths.empty(), directory.string() + ": the CD's songs");
    for (const auto &path : paths) {
        Song song(path);
        auto run = [&] {
            ASS_PlayMusic(&song.module);
            ASS_StartMusic();
            play(6);
        };
        const std::vector<u8> native = runAt(0, run, 1);
        const std::vector<i16> eight = samples(runAt(8, run));
        const std::vector<i16> sixteen = samples(runAt(16, run));
        const std::string name = path.filename().string();
        check(native.size() == eight.size() && eight.size() > SKIP + LATER, name + ": the 8-bit DMA's samples");
        std::size_t differ = 0;
        for (std::size_t k = LATER; k < std::min(native.size(), eight.size()); k++) {
            differ += eight[k - LATER] != (native[k] - 0x80) * 64 ? 1 : 0;
        }
        check(differ == 0, name + ": the 8-bit DMA's output, a quarter (" + std::to_string(differ) + " samples off)");
        const double level = levelOf(eight, sixteen);
        check(power(eight) > 0 && std::abs(level) < 6,
              name + ": near the 16-bit output's level (" + std::to_string(level) + " dB)");
        lowest = std::min(lowest, level);
        highest = std::max(highest, level);
    }
}

/* The ring, the output's place, the song and the channels as a change of
 * depth between 16 and 8 leaves them. */
struct Playing {
    std::vector<u8> ring;
    AudioState audio;
    u16 tickLeft, mixPointer, fillHalf, halfBuffer;
    u8 order, row, speedCount;
    std::vector<u32> positions;

    Playing()
        : ring(std::begin(asylum::data.ring), std::end(asylum::data.ring)), audio(audioState()),
          tickLeft(asylum::data.tickLeft), mixPointer(asylum::data.mixPointer), fillHalf(asylum::data.fillHalf),
          halfBuffer(asylum::data.halfBuffer), order(asylum::data.order), row(asylum::data.row),
          speedCount(asylum::data.speedCount)
    {
        for (const asylum::Channel &c : asylum::data.channels) {
            positions.push_back(c.position);
            positions.push_back(c.fraction);
            positions.push_back(c.step);
            positions.push_back(c.active);
        }
    }

    bool operator==(const Playing &other) const
    {
        return ring == other.ring && audio.start == other.audio.start && audio.units == other.audio.units &&
               audio.taken == other.audio.taken && audio.sixteen == other.audio.sixteen && tickLeft == other.tickLeft &&
               mixPointer == other.mixPointer && fillHalf == other.fillHalf && halfBuffer == other.halfBuffer &&
               order == other.order && row == other.row && speedCount == other.speedCount &&
               positions == other.positions;
    }
};

/* The largest step between a channel's samples one after the other, from
 * the sample from on. */
int largestStep(const std::vector<i16> &made, std::size_t from)
{
    int largest = 0;
    for (std::size_t k = from + 2; k < made.size(); k++) {
        largest = std::max(largest, std::abs((int)made[k] - made[k - 2]));
    }
    return largest;
}

void switches(AmfModule &song, std::vector<u8> &sound)
{
    auto begin = [&](u16 bits) {
        start(bits);
        ASS_PlayMusic(&song);
        ASS_StartMusic();
        ASS_PlaySFX(sound.data(), 0, 64, 0, 1);
        play(0.5);
    };
    /* The steps of the two depths' runs, which a change's click would pass. */
    int steps = 0;
    for (const u16 bits : {16, 8}) {
        begin(bits);
        play(2);
        steps = std::max(steps, largestStep(samples(outBytes), SKIP));
        ASS_Uninit();
    }

    begin(16);
    for (int round = 0; round < 4; round++) {
        for (const u16 bits : {8, 16}) {
            const Playing before;
            ASS_SetDepth(bits);
            check(ASS_GetOutputBits() == bits && ASS_GetDepth() == bits, "the depth chosen");
            check(asylum::data.sixteenBit == (bits == 16 ? 1 : 0) && asylum::data.eightBitWords == (bits == 8 ? 1 : 0),
                  "the depth's mix");
            check(Playing() == before, "a change keeps the ring, the output's place, the song and the channels");
            bool mixers = true;
            for (const asylum::Channel &c : asylum::data.channels) {
                mixers &= c.mixer == asylum::MIX_NONE ||
                          (bits == 16) == (c.mixer >= asylum::MIX_MONO16 && c.mixer <= asylum::MIX_STREAM16MONO);
            }
            check(mixers, "the channels' mixers of the depth");
            if (bits == 16) {
                bool pans = true;
                for (u16 i = 0; i < asylum::MUSIC_CHANNELS; i++) {
                    const asylum::Channel &c = asylum::data.channels[i];
                    pans &= c.panLeft == c.musicPanLeft && c.panRight == c.musicPanRight;
                }
                check(pans, "the song's pans back from the 8-bit mix");
            }
            outBytes.clear();
            play(0.25);
            check(outSixteen && ASS_IsChannelBusy(0) == 1 && !outBytes.empty(), "the music and the effect go on");
            const int step = largestStep(samples(outBytes), 0);
            check(step <= steps,
                  "no click at the change (" + std::to_string(step) + " against " + std::to_string(steps) + ")");
        }
    }

    /* The tick's sum not yet out and the oversampling's last samples, 16 to
     * 8 and back, in the new mix's units: the 16-bit mix's 64 times the
     * 8-bit one's, within that (the halves within twice that). */
    auto sums = [] {
        std::vector<int> made;
        for (u32 k = 0; k < (u32)asylum::data.tickLeft * 2; k++) {
            made.push_back((i16)asylum::data.tables.word((u16)(asylum::data.mixPointer + (2 * k))));
        }
        return made;
    };
    auto lasts16 = [] {
        const u32 pair = asylum::data.oversample16;
        return std::vector<int>{(int)(u16)pair * 2 - 0x8000, (int)(u16)(pair >> 16) * 2 - 0x8000};
    };
    auto lastsWords = [] {
        const u32 pair = asylum::data.oversampleWords;
        return std::vector<int>{((i16)(u16)pair * 2 - 0x80) * 64, ((i16)(u16)(pair >> 16) * 2 - 0x80) * 64};
    };
    auto within = [](const std::vector<int> &a, const std::vector<int> &b, int scale, int error) {
        bool ok = !a.empty() && a.size() == b.size();
        for (std::size_t k = 0; ok && k < a.size(); k++) {
            ok = std::abs(a[k] - (b[k] * scale)) < error;
        }
        return ok;
    };
    play(0.01);
    const std::vector<int> sum16 = sums();
    const std::vector<int> last16 = lasts16();
    ASS_SetDepth(8);
    const std::vector<int> sum8 = sums();
    check(within(sum16, sum8, 64, 64) && within(last16, lastsWords(), 1, 192),
          "16 to 8: the tick's rest and the last samples in the 8-bit mix's units");
    ASS_SetDepth(16);
    check(within(sums(), sum8, 64, 1) && within(lasts16(), last16, 1, 192),
          "8 to 16: the tick's rest and the last samples in the 16-bit mix's units");

    /* A snapshot put back (snapshot.h) keeps the depth, a setting outside
     * the arena, and gives back the data: the next update makes the
     * 16-bit run's data the 8-bit depth's. */
    const std::vector<std::byte> arena = gameMemoryCopy();
    ASS_SetDepth(8);
    gameMemoryPut(arena);
    check(ASS_GetDepth() == 8 && asylum::data.sixteenBit == 1, "put back: the depth kept, the data the 16-bit run's");
    outBytes.clear();
    play(0.25);
    check(asylum::data.sixteenBit == 0 && asylum::data.eightBitWords == 1 && ASS_GetOutputBits() == 8 &&
              ASS_IsChannelBusy(0) == 1 && !outBytes.empty(),
          "the next update makes it the depth's, the music and the effect going on");
    ASS_Uninit();

    /* From the 8-bit DMA's output, whose ring takes bytes: a new start in
     * words; the song's pans, E8x's too, back. */
    start(0, 1);
    ASS_PlayMusic(&song);
    ASS_StartMusic();
    play(0.25);
    check(asylum::data.channels[2].musicPanLeft == 24 && asylum::data.channels[2].musicPanRight == 64,
          "the 8-bit mix keeps the song's pan of E83 (" + std::to_string(asylum::data.channels[2].musicPanLeft) + ", " +
              std::to_string(asylum::data.channels[2].musicPanRight) + ")");
    ASS_SetDepth(8);
    check(asylum::data.eightBitWords == 1 && audioState().sixteen && audioState().taken == 0,
          "from the 8-bit DMA's bytes to the words, a new start");
    outBytes.clear();
    play(0.25);
    check(outSixteen && !outBytes.empty(), "the music goes on in words");
    ASS_SetDepth(16);
    bool restored = true;
    for (u16 i = 0; i < asylum::MUSIC_CHANNELS; i++) {
        const asylum::Channel &c = asylum::data.channels[i];
        restored &= c.panLeft == c.musicPanLeft && c.panRight == c.musicPanRight;
    }
    check(restored && asylum::data.sixteenBit == 1, "the song's pans back at 16 bits");
    ASS_Uninit();
}

}

void hostCloseAction(void (*)()) {}
bool windowRun()
{
    return false;
}
bool windowFrame()
{
    return false;
}

bool windowFastForwarding()
{
    return false;
}
bool windowFrameUnshown()
{
    return false;
}
bool windowPicture(std::vector<u8> &, int &, int &)
{
    return false;
}
void windowAudio(const u8 *bytes, u32 size, u32 /*rate*/, bool sixteen)
{
    outBytes.insert(outBytes.end(), bytes, bytes + size);
    outSixteen = sixteen;
}
bool windowAudioState(WindowAudio &)
{
    return false;
}
void windowMessage(const char *) {}
void windowWarning(const char *) {}
void windowTitleNote(const std::string &) {}
void windowExit(int code)
{
    std::exit(code);
}

int main()
{
    const std::vector<u8> eight = startedTables(1, 0);
    const std::vector<u8> sixteen = startedTables(5, 0);
    check(eight != sixteen, "the 8-bit and the 16-bit DMA's tables");
    check(startedTables(5, 8) == eight && startedTables(1, 8) == eight, "8 bits: the 8-bit output's tables");
    check(startedTables(1, 16) == sixteen && startedTables(5, 16) == sixteen, "16 bits: the 16-bit output's tables");
    start(8);
    check(ASS_GetOutputBits() == 8 && ASS_GetDepth() == 8 && asylum::data.sixteenBit == 0 &&
              asylum::data.eightBitWords == 1 && audioState().sixteen,
          "8 bits: the 8-bit mix, the ring in words");
    ASS_Uninit();
    ASS_SetDepth(8);
    check(ASS_GetOutputBits() == 0 && ASS_GetDepth() == 8, "no output before init");

    /* An effect at the sound's volume and one at 41h (No Remorse's movies),
     * the movies' stream of No Regret (DirectPlay, past the volumes), and
     * four effects at 41h whose sum is past the 8-bit output's ends. */
    std::vector<u8> sound = squareSound(22050, 0x60);
    auto effect = [&](u16 volume) {
        return [&sound, volume] {
            ASS_PlaySFX(sound.data(), 0, volume, 0, 1);
            play(1);
        };
    };
    compare(runAt(16, effect(64)), runAt(8, effect(64)), "an effect", 1);
    compare(runAt(16, effect(0x41)), runAt(8, effect(0x41)), "an effect at 41h", 1);
    std::vector<u8> movie(22050 * 4);
    for (std::size_t k = 0; k < movie.size(); k += 4) {
        const i16 value = k % 200 < 100 ? 30000 : -30000;
        movie[k] = (u8)value;
        movie[k + 1] = (u8)((u16)value >> 8);
        movie[k + 2] = (u8)-value;
        movie[k + 3] = (u8)((u16)-value >> 8);
    }
    auto stream = [&] {
        ASS_DirectPlay(0, movie.data(), 22050, 0, 22050);
        play(0.5);
    };
    compare(runAt(16, stream), runAt(8, stream), "the movies' stream", 1);
    std::vector<u8> loud = squareSound(22050, 0x7f);
    auto four = [&] {
        for (i16 channel = 0; channel < 4; channel++) {
            ASS_PlaySFX(loud.data(), channel, 0x41, 0, 1);
        }
        play(1);
    };
    const std::vector<u8> fourEight = runAt(8, four);
    compare(runAt(16, four), fourEight, "four loud effects", 4);
    const std::vector<i16> fourSamples = samples(fourEight);
    const int peak = fourSamples.empty() ? 0 : *std::max_element(fourSamples.begin(), fourSamples.end());
    check(peak > 0x80 * 64 * 3, "four loud effects past the 8-bit output's ends (" + std::to_string(peak) + ")");

    Song song(anyCase(PORT_REMORSE_CD, "sound/m01.amf"));
    /* A song with a pan of E83 in its first row. */
    const std::size_t firstRow = song.module.header->order[0] * AMF_PATTERN_SIZE;
    song.module.patterns[firstRow + 2 * 4 + 2] = 0xe;
    song.module.patterns[firstRow + 2 * 4 + 3] = 0x83;
    switches(song.module, sound);
    double lowest = 100;
    double highest = -100;
    songs(anyCase(PORT_REMORSE_CD, "sound"), lowest, highest);
    songs(anyCase(PORT_REGRET_CD, "sound"), lowest, highest);
    std::printf("sounddepth: the songs at 8 bits against 16: %+.2f to %+.2f dB\n", lowest, highest);
    std::printf("sounddepth: %d checks, %d mismatches\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
