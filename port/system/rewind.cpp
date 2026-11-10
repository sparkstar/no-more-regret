/* See rewind.h. */
#include "rewind.h"
#include "gamememory.h"
#include "host.h"
#include "hostpath.h"
#include "pc.h"
#include "pcstate.h"
#include "profile.h"
#include "replay.h"
#include "snapshot.h"
#include "window.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <memory>
#include <optional>
#include <thread>

namespace {

/* The ring's bytes at most. */
constexpr std::size_t RING_BYTES = std::size_t{256} << 20;
/* Bytes alike between two that differ which a run of them takes in. */
constexpr std::size_t GAP = 32;
/* The sound of a frame put back: a quarter of its level. */
constexpr int QUIET = 4;
/* What a seldom changed part of the PC's state takes on the host's heap,
 * about (its queues' and its map's blocks). */
constexpr std::size_t HEAVY = 2048;

/* The places compared in place: the arena (its bytes in use), the SVGA's
 * and the VGA's memory and the colours; the player's settings while the run
 * follows a recording (keptPlayers). */
enum Place : std::uint8_t { ARENA, SVGA, VGA, COLOURS, SETTINGS, PLACES };
/* The bytes compared at once before the bytes one by one. */
constexpr std::size_t PAGE = 4096;

/* What a frame keeps whole: the PC's state; on the ring the PC's parts that
 * seldom change (the interrupts' saved flags, the keyboard's and the
 * mouse's queues, the open files) only when they differ from the frame
 * after, the frame after's else. */
struct Small {
    TimerState timer{};
    AudioState audio{};
    VideoState video{};
    KeysHeld held;
    std::optional<InterruptState> interrupts;
    std::optional<KeyboardState> keyboard;
    std::optional<MouseState> mouse;
    std::optional<FilesState> files;
    /* Where the run is in the recordings it follows (replay.h): the
     * replay's place, the recorder's, the state log's passes; the test's
     * hash of the player's settings and files. */
    ReplayPlace place;
    RecordPlace record;
    std::uint64_t logged = 0;
    std::uint64_t players = 0;
};

/* A frame on the ring: what differed from the frame after it, as it was
 * (a run each: its place, offset and length, then its bytes), what it keeps
 * whole, and the sound that came between it and the frame after. */
struct Entry {
    std::vector<std::byte> runs;
    Small small;
    std::vector<u8> sound;
    /* The player's files the frame wrote, as they were before. */
    std::vector<PlayerFile> files;
    std::size_t bytes = 0;
};
struct RunHeader {
    std::uint8_t place;
    std::uint8_t unused[3];
    std::uint32_t offset;
    std::uint32_t length;
};

std::deque<Entry> ring;
std::size_t ringBytes = 0;
/* Whether the ring keeps the player's settings and files: while the run
 * follows a recording (replay.h's followingRecording), as the input that
 * changed them goes back. */
bool keptPlayers = false;
/* The copy of the places at the last frame, and what it kept whole. */
bool started = false;
std::vector<std::byte> previous[PLACES];
Small previousSmall;
/* The state saved, and the keys down then. */
std::unique_ptr<Snapshot> slot;
KeysHeld slotHeld;

/* The test's (rewind.h): at which frame, how many frames back. */
struct Test {
    std::uint64_t at = 0;
    std::uint64_t frames = 0;
    bool done = false;
};
std::optional<Test> testAsked()
{
    const char *asked = std::getenv("PORT_REWIND_TEST");
    if (asked == nullptr) {
        return std::nullopt;
    }
    unsigned long long at = 0;
    unsigned long long frames = 0;
    if (std::sscanf(asked, "%llu:%llu", &at, &frames) != 2 || frames == 0) {
        std::fprintf(stderr, "rewind: PORT_REWIND_TEST is <frame>:<frames>\n");
        std::_Exit(3);
    }
    return Test{at, frames, false};
}
std::optional<Test> test = testAsked();

/* The game's repaint (host.h), and whether the frame before went fast (a
 * watched replay's, replay.h): the game draws again, the whole screen anew
 * first. */
void (*repaintAction)() = nullptr;
bool wentFast = false;

struct Live {
    std::byte *start;
    std::size_t size;
};

Live live(Place place)
{
    const VideoPlaces video = videoPlaces();
    switch (place) {
    case ARENA:
        return {gameMemory().start, gameMemory().used};
    case SVGA:
        return {reinterpret_cast<std::byte *>(video.svga), video.svgaBytes};
    case VGA:
        return {reinterpret_cast<std::byte *>(video.vga), video.vgaBytes};
    case COLOURS:
        return {reinterpret_cast<std::byte *>(video.colours), video.colourBytes};
    case SETTINGS:
        if (keptPlayers) {
            return {gameSettings().start, gameSettings().used};
        }
        break;
    case PLACES:
        break;
    }
    return {nullptr, 0};
}

Small smallNow()
{
    Small small;
    small.timer = timerState();
    small.audio = audioState();
    small.video = videoState();
    small.held = windowKeysTaken();
    small.interrupts = interruptState();
    small.keyboard = keyboardState();
    small.mouse = mouseState();
    small.files = filesState();
    small.place = replayPlace();
    small.record = recordPlace();
    small.logged = stateLogPasses();
    if (test && keptPlayers) {
        small.players = playersHash();
    }
    return small;
}

void putSmall(const Small &small)
{
    putTimerState(small.timer);
    putAudioState(small.audio);
    putVideoState(small.video);
    if (small.interrupts) {
        putInterruptState(*small.interrupts);
    }
    if (small.keyboard) {
        putKeyboardState(*small.keyboard);
    }
    if (small.mouse) {
        putMouseState(*small.mouse);
    }
    if (small.files) {
        putFilesState(*small.files);
    }
    if (keptPlayers) {
        putReplayPlace(small.place);
        putRecordPlace(small.record);
        putStateLogPasses(small.logged);
    }
}

/* The copies made anew from the places as they are. */
void startRing()
{
    ring.clear();
    ringBytes = 0;
    keptPlayers = followingRecording();
    portKeepPlayersFiles(keptPlayers);
    for (int place = 0; place < PLACES; place++) {
        const Live bytes = live(static_cast<Place>(place));
        previous[place].resize(bytes.size);
        if (bytes.size != 0) {
            copyUnchecked(previous[place].data(), bytes.start, bytes.size);
        }
    }
    previousSmall = smallNow();
    started = true;
}

[[gnu::no_sanitize_address]] std::uint64_t word(const std::byte *at)
{
    std::uint64_t value = 0;
    __builtin_memcpy(&value, at, sizeof value);
    return value;
}

/* The runs where live differs from copy, as copy has them, onto runs; the
 * copy then as live. A page at a time, the pages alike passed over at
 * once, the runs within a page. The bytes are read unchecked (gamememory.h's
 * copyUnchecked: the address sanitizer's poison). */
[[gnu::no_sanitize_address]] void compare(Place place, const std::byte *now, std::byte *copy, std::size_t size,
                                          std::vector<std::byte> &runs)
{
    for (std::size_t page = 0; page < size; page += PAGE) {
        const std::size_t last = std::min(page + PAGE, size);
        if (sameUnchecked(now + page, copy + page, last - page)) {
            continue;
        }
        std::size_t at = page;
        while (at < last) {
            while (at + 8 <= last && word(now + at) == word(copy + at)) {
                at += 8;
            }
            while (at < last && now[at] == copy[at]) {
                at++;
            }
            if (at >= last) {
                break;
            }
            std::size_t end = at + 1;
            std::size_t alike = 0;
            while (end < last && alike < GAP) {
                alike = (now[end] == copy[end]) ? alike + 1 : 0;
                end++;
            }
            end -= alike;
            const RunHeader header{place, {}, static_cast<std::uint32_t>(at), static_cast<std::uint32_t>(end - at)};
            const std::size_t written = runs.size();
            runs.resize(written + sizeof header + (end - at));
            __builtin_memcpy(runs.data() + written, &header, sizeof header);
            copyUnchecked(runs.data() + written + sizeof header, copy + at, end - at);
            copyUnchecked(copy + at, now + at, end - at);
            at = end;
        }
    }
}

/* A frame onto the ring: what differs from the copies, as the copies had
 * it, and what the frame before kept whole. */
void capture()
{
    Entry entry;
    for (int place = 0; place < PLACES; place++) {
        const Live bytes = live(static_cast<Place>(place));
        std::vector<std::byte> &copy = previous[place];
        if (copy.size() < bytes.size) {
            /* The arena grew: the bytes above its old top were free. */
            const std::size_t old = copy.size();
            copy.resize(bytes.size);
            copyUnchecked(copy.data() + old, bytes.start + old, bytes.size - old);
        }
        compare(static_cast<Place>(place), bytes.start, copy.data(), copy.size(), entry.runs);
    }
    entry.small = std::move(previousSmall);
    entry.sound = takeAudioTap();
    entry.files = portPlayersWrites();
    previousSmall = smallNow();
    /* The seldom changed parts only when they changed. */
    if (entry.small.interrupts == previousSmall.interrupts) {
        entry.small.interrupts.reset();
    }
    if (entry.small.keyboard == previousSmall.keyboard) {
        entry.small.keyboard.reset();
    }
    if (entry.small.mouse == previousSmall.mouse) {
        entry.small.mouse.reset();
    }
    if (entry.small.files == previousSmall.files) {
        entry.small.files.reset();
    }
    entry.runs.shrink_to_fit();
    std::size_t files = 0;
    for (const PlayerFile &file : entry.files) {
        files += file.bytes ? file.bytes->size() : 0;
    }
    entry.bytes = entry.runs.capacity() + entry.sound.capacity() + files + sizeof entry +
                  (entry.small.interrupts ? HEAVY : 0) + (entry.small.keyboard ? HEAVY : 0) +
                  (entry.small.mouse ? HEAVY : 0) + (entry.small.files ? HEAVY : 0);
    ringBytes += entry.bytes;
    ring.push_back(std::move(entry));
    while (ringBytes > RING_BYTES && ring.size() > 1) {
        ringBytes -= ring.front().bytes;
        ring.pop_front();
    }
}

/* The frames a rewind puts back each sixtieth of a second: four times as
 * fast as the game went. */
constexpr int REWIND_SPEED = 4;

/* The sound of a frame backwards and quiet, after the sound before. */
void addBackwards(std::vector<u8> &out, const std::vector<u8> &sound, const AudioState &audio)
{
    const std::size_t frame = audio.sixteen ? 4 : 2;
    std::vector<u8> back(sound.size());
    for (std::size_t at = 0; at + frame <= sound.size(); at += frame) {
        const std::size_t to = sound.size() - frame - at;
        if (audio.sixteen) {
            for (std::size_t channel = 0; channel < 4; channel += 2) {
                auto sample = static_cast<std::int16_t>(sound[at + channel] | (sound[at + channel + 1] << 8));
                sample = static_cast<std::int16_t>(sample / QUIET);
                back[to + channel] = static_cast<u8>(sample & 0xff);
                back[to + channel + 1] = static_cast<u8>((sample >> 8) & 0xff);
            }
        } else {
            for (std::size_t channel = 0; channel < 2; channel++) {
                back[to + channel] = static_cast<u8>(128 + ((sound[at + channel] - 128) / QUIET));
            }
        }
    }
    out.insert(out.end(), back.begin(), back.end());
}

/* The sound of the frames a sixtieth of a second put back, one sample of
 * REWIND_SPEED, to the window: its time's length. */
void playBackwards(const std::vector<u8> &back, const AudioState &audio)
{
    const std::size_t frame = audio.sixteen ? 4 : 2;
    std::vector<u8> thinned;
    for (std::size_t at = 0; at + frame <= back.size(); at += frame * REWIND_SPEED) {
        thinned.insert(thinned.end(), back.begin() + static_cast<std::ptrdiff_t>(at),
                       back.begin() + static_cast<std::ptrdiff_t>(at + frame));
    }
    windowAudio(thinned.data(), static_cast<u32>(thinned.size()), audio.rate, audio.sixteen);
}

/* A frame back from the ring: its runs written to the places and the
 * copies, what it kept whole put back; its sound backwards after sound's. */
void step(std::vector<u8> &sound)
{
    Entry entry = std::move(ring.back());
    ring.pop_back();
    ringBytes -= entry.bytes;
    std::size_t at = 0;
    while (at + sizeof(RunHeader) <= entry.runs.size()) {
        RunHeader header{};
        __builtin_memcpy(&header, entry.runs.data() + at, sizeof header);
        at += sizeof header;
        const Live bytes = live(static_cast<Place>(header.place));
        copyUnchecked(bytes.start + header.offset, entry.runs.data() + at, header.length);
        copyUnchecked(previous[header.place].data() + header.offset, entry.runs.data() + at, header.length);
        at += header.length;
    }
    portPutPlayersFiles(entry.files);
    gameMemoryWritten();
    putSmall(entry.small);
    /* The frame put back is the frame before's: its parts not kept are the
     * frame after's. */
    if (!entry.small.interrupts) {
        entry.small.interrupts = std::move(previousSmall.interrupts);
    }
    if (!entry.small.keyboard) {
        entry.small.keyboard = std::move(previousSmall.keyboard);
    }
    if (!entry.small.mouse) {
        entry.small.mouse = std::move(previousSmall.mouse);
    }
    if (!entry.small.files) {
        entry.small.files = std::move(previousSmall.files);
    }
    previousSmall = std::move(entry.small);
    addBackwards(sound, entry.sound, previousSmall.audio);
}

/* While the rewind's key is held: REWIND_SPEED frames back each sixtieth
 * of a second, the last shown, their sound backwards in a sixtieth's
 * length; then the input brought to now. */
void rewind()
{
    using Clock = std::chrono::steady_clock;
    const auto sixtieth = std::chrono::nanoseconds(1000000000 / 60);
    auto next = Clock::now();
    std::uint64_t back = 0;
    std::vector<u8> sound;
    while (windowRewinding()) {
        sound.clear();
        for (int k = 0; k < REWIND_SPEED && !ring.empty(); k++) {
            step(sound);
            back++;
        }
        playBackwards(sound, previousSmall.audio);
        if (!windowRewindFrame()) {
            break;
        }
        next += sixtieth;
        const auto now = Clock::now();
        if (next < now) {
            next = now;
        }
        std::this_thread::sleep_until(next);
    }
    windowRewindEnd(previousSmall.held);
    if (back != 0) {
        countRerecord();
    }
    std::fflush(stdout);
    std::fprintf(stderr, "%s: %llu frames rewound, %llu left on the ring\n", PROFILE_NAME,
                 static_cast<unsigned long long>(back), static_cast<unsigned long long>(ring.size()));
}

/* While the pause is on (the host action PAUSE): the game's time stands,
 * between two steps, the screen shown with its state each sixtieth of a
 * second; the rewind and the state's save and load go as at a frame. At
 * its end the wall clock goes on from now, and the input that came in it
 * is dropped, the keys brought to those down now (as at a rewind's end). */
void pauseWhileAsked();

void message(const char *text)
{
    std::fflush(stdout);
    std::fprintf(stderr, "%s: %s\n", PROFILE_NAME, text);
}

/* The state's save or load, when asked. */
void stateAsked()
{
    const int asked = windowStateAsked();
    if (asked == 1) {
        slot = std::make_unique<Snapshot>(takeSnapshot());
        slotHeld = windowKeysTaken();
        message("the state saved");
    } else if (asked == 2) {
        if (!slot) {
            message("no state saved to load");
            return;
        }
        putSnapshot(*slot);
        windowRewindEnd(slotHeld);
        startRing();
        message("the state loaded");
    }
}

void pauseWhileAsked()
{
    if (!windowPaused()) {
        return;
    }
    using Clock = std::chrono::steady_clock;
    const auto sixtieth = std::chrono::nanoseconds(1000000000 / 60);
    auto next = Clock::now();
    while (windowPaused()) {
        if (windowRewinding()) {
            rewind();
            continue;
        }
        stateAsked();
        if (!windowPauseFrame()) {
            break;
        }
        next += sixtieth;
        const auto now = Clock::now();
        if (next < now) {
            next = now;
        }
        std::this_thread::sleep_until(next);
    }
    windowRewindEnd(windowKeysTaken());
    restartWallClock();
}

/* The test's rewind: the frames back at its frame, no window. */
bool testRewind()
{
    if (!test || test->done || timerState().windowFrames < test->at) {
        return false;
    }
    capture();
    const std::uint64_t players = keptPlayers ? playersHash() : 0;
    std::uint64_t back = 0;
    std::vector<u8> sound;
    for (; back < test->frames && !ring.empty(); back++) {
        step(sound);
    }
    if (back != 0) {
        countRerecord();
    }
    test->done = true;
    std::fprintf(stderr, "rewind: %llu frames back at frame %llu, %llu on the ring (%llu bytes)\n",
                 static_cast<unsigned long long>(back), static_cast<unsigned long long>(test->at),
                 static_cast<unsigned long long>(ring.size()), static_cast<unsigned long long>(ringBytes));
    if (keptPlayers) {
        const std::uint64_t now = playersHash();
        std::fprintf(stderr, "rewind: the player's settings and files %s%s\n",
                     now == previousSmall.players ? "as they were" : "NOT as they were",
                     now != players ? ", changed since" : "");
    }
    return true;
}

bool enabled()
{
    if (!snapshotReady()) {
        return false;
    }
    if (test) {
        return replaying();
    }
    /* Not while a watched replay goes fast: it keeps the ring from the
     * lead to the part it shows (replay.h); none without the debug
     * features (host.h), whose rewind and states it is. */
    return hostDebugFeatures() && windowRun() && !replayFast();
}

}

void hostRepaintAction(void (*repaint)())
{
    const SystemSide systemSide;
    repaintAction = repaint;
}

void hostFrame()
{
    /* A watched replay that went fast draws again (replay.h): the game draws
     * again only what changes, so the whole screen first. */
    const bool fast = replayFast();
    if (wentFast && !fast && repaintAction != nullptr) {
        const GameSide gameSide;
        repaintAction();
    }
    wentFast = fast;
    /* A watched replay taken over at its end or when asked (replay.h). */
    if (replaying() && watching() && (windowTakeOverAsked() || replayEnded())) {
        windowRewindEnd(takeOver());
    }
    snapshotFrame();
    if (!enabled()) {
        return;
    }
    if (!started) {
        audioTap(true);
        startRing();
        return;
    }
    /* The run follows a recording no more (a replay watched to its end)
     * or does now: the ring and the state of the other. */
    if (keptPlayers != followingRecording()) {
        startRing();
        slot.reset();
        return;
    }
    if (test) {
        if (!testRewind()) {
            capture();
        }
        return;
    }
    stateAsked();
    /* This frame onto the ring first: a rewind starts from it. */
    capture();
    if (windowRewinding()) {
        rewind();
    }
    pauseWhileAsked();
}
