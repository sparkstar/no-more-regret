/* The run's replay: a run with a window recorded as what its frames gave the
 * game, and a run without one that gives the game the same at the same frames,
 * as fast as the host goes, its picture and sound captured if asked
 * (capture.h). The game's time is the virtual time (clock.h), and its input
 * comes only at the window's frames, a sixtieth of a second of it apart
 * (timer.cpp, window.h): the same frames' input gives the same run. What else a
 * run with a window takes from the host is recorded too: the passes the wall
 * clock made longer (clockPass on a host that fell behind) and the day
 * (clockDate). The player's files, which the game reads and writes (savegame\,
 * crusader.cfg, enhanced.cfg), come with the recording as they were at its
 * start; a replay writes into a copy of them, and its other writes go there too
 * (hostpath.h's portSandbox), so the player's are not touched.
 *
 * Where the input comes from and whether it is recorded are apart: the window's
 * recorded (--record), a recording's (--replay), a recording's recorded
 * (--replay with --record: the recording is what the replay gave, as the
 * window's is what the window gave). The recorder keeps its lines and writes
 * them on; a rewind or a state's load while it records cuts them (recordPlace)
 * or puts back a state's copy of them (recordCopy), and counts a rerecord in
 * run.tsv.
 *
 * A recording is a directory (commandline.h's --record, --replay):
 *   inputs.tsv  what the frames gave, a line each in the frames' order,
 *               the words tab-separated, a # to the line's end a comment:
 *       <frame> key <byte>         a byte the keyboard sends, in hex (E0h
 *                                  and E1h before the extended keys)
 *       <frame> release <byte>     the key's break code (byte | 80h)
 *       <frame> motion <x> <y> <buttons>
 *                                  the mouse's motion in mickeys and its
 *                                  buttons (bits 0-2: left, right, middle)
 *       <frame> pad|axis|padgone|mousebutton|wheel ...
 *                                  the other devices' events, as a script
 *                                  has them (script.h)
 *       <frame> close              the window closed (the run ends)
 *       <frame> date <yyyy-mm-dd>  the day from that frame on
 *       <frame> pass <pass> <ticks>
 *                                  the pass ended at that virtual time (in
 *                                  the interval timer's ticks), later than
 *                                  its sixtieth of a second
 *       <frame> save <slot>        the game saved in savegame\savegame.<slot+1>
 *       <frame> load <slot>        or loaded from it, at the end of that
 *                                  frame's pass, as a script's (script.h; a recording that starts from a save)
 *       <frame> end                the run's last frame
 *   checks.tsv  the game's state each second of passes (statelog.cpp's
 *               values): a replay that comes to another says so, once
 *   run.tsv     the game, the port's version (portversion.h) and when the
 *               recording began
 *   files/      the player's files at the start
 * A replay without an end line ends at its last line's frame. */
#ifndef PORT_REPLAY_H
#define PORT_REPLAY_H
#include "common.h"
#include "clock.h"
#include "input.h"

#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/* commandline.cpp's: the recording into directory, which it has made, or
 * the replay of one, to its frame last at most. After startReplay,
 * startRecording records the replay: the recording starts with the
 * replay's rerecords (its files are commandline.cpp's to copy). The
 * game's arguments (the original's own command line: -warp, -egg, -skill
 * and the like, without the port's options) go into run.tsv's arguments,
 * a word each, which the replay gives the game again. */
void startRecording(const std::string &directory, const std::vector<std::string> &arguments);
void startReplay(const std::string &directory, std::uint64_t last);
/* The game a recording is of (its run.tsv's game), "" for none. */
[[nodiscard]] std::string replayGame(const std::string &directory);
/* The game's arguments a recording was made with (run.tsv's arguments),
 * none for none. */
[[nodiscard]] std::vector<std::string> replayArguments(const std::string &directory);
/* The recorder is on; the input comes from a recording. */
bool recording();
bool replaying();
/* The replay in a window (--watch, --continue), shown from the frame from
 * (at most its last). Before the lead to that frame (about what the
 * rewind's ring holds) it goes as fast as the host goes (replayFast): the
 * window open but given neither its picture nor its sound, the game
 * drawing nothing (host.h's hostDraws), the rewind's ring not kept. In the
 * lead the game draws again, the whole screen anew first, and the ring is
 * kept. From the frame on (watching) it goes at the wall clock's pace, the
 * window's input going nowhere but its host actions; at the replay's end
 * (replayEnded), or when the player asks (the host action TAKE_OVER),
 * rewind.cpp's hostFrame takes it over: the input is the window's from
 * that frame on, recorded if the replay is (BizHawk's movie end "record").
 * takeOver gives the keys the replay has given down, for the window to
 * bring to those down now. */
void watchReplay(std::uint64_t from);
/* The run watches a replay; it is shown now; it goes fast now. */
bool replayWatched();
bool watching();
bool replayFast();
[[nodiscard]] bool replayEnded();
[[nodiscard]] std::bitset<256> takeOver();
/* The recording ends without the window's close: the game's own end (its
 * menu's QUIT), or a replay recorded to its end. */
[[nodiscard]] bool replayEndsItself();
/* Whether the run follows a recording, recording or replaying: the
 * player's settings and files go back with the game's state then. */
bool followingRecording();

/* timer.cpp's. A frame of the window comes (recordFrame before
 * window.h's windowFrame); a replay's frame instead: its input, its
 * capture, and at the replay's end the run's end. */
void recordFrame(std::uint64_t frame);
void replayFrame(std::uint64_t frame);
/* A pass that the wall clock let end at until; a replay's end of a pass,
 * 0 when it ended at its sixtieth. Passes count from 1. */
void recordPass(std::uint64_t pass, std::uint64_t until);
[[nodiscard]] std::uint64_t replayPass(std::uint64_t pass);
/* The day a run with a window had; a replay's, none before its first. */
void recordDate(const ClockDate &date);
[[nodiscard]] std::optional<ClockDate> replayDate();

/* window.cpp's: what a frame gives the keyboard and the mouse's driver
 * (pc.h, mousedrv.h), and the window closed. */
void recordScan(u8 scan);
void recordMotion(i16 x, i16 y, u16 buttons);
void recordDevice(const DeviceEvent &device);
void recordClose();

/* statelog.cpp's: whether the state after the pass is checked, and the
 * state then (its values space-separated). */
[[nodiscard]] bool replayChecks(std::uint64_t pass);
void replayCheck(std::uint64_t pass, const std::string &state);

/* Where a replay is: its frame, its next input and its day, which a
 * snapshot's test puts back with the snapshot. */
struct ReplayPlace {
    std::uint64_t frame = 0;
    std::size_t input = 0;
    std::optional<ClockDate> day;
    /* The keys the replay has given down (window.h's KeysHeld), and an
     * E0h before the next. */
    std::bitset<256> keys;
    bool afterE0 = false;
};
[[nodiscard]] ReplayPlace replayPlace();
void putReplayPlace(const ReplayPlace &place);

/* Where the recorder is: its lines of inputs.tsv and of checks.tsv, and
 * the day it recorded last. Put back, the lines after it are cut (and the
 * files written again). */
struct RecordPlace {
    std::size_t inputs = 0;
    std::size_t checks = 0;
    std::optional<ClockDate> day;
};
[[nodiscard]] RecordPlace recordPlace();
void putRecordPlace(const RecordPlace &place);
/* A rerecord: run.tsv's count one more. */
void countRerecord();

/* The pure parts: a line of inputs.tsv and back. */
struct ReplayInput {
    enum Kind { KEY, MOTION, DEVICE, CLOSE, DATE, PASS, SAVE, LOAD, END };
    std::uint64_t frame = 0;
    Kind kind = KEY;
    /* KEY's byte; MOTION's mickeys and buttons; DEVICE's event; DATE's
     * day; PASS's pass and its end; SAVE's and LOAD's slot (in x). */
    u8 scan = 0;
    i16 x = 0;
    i16 y = 0;
    u16 buttons = 0;
    DeviceEvent device{};
    ClockDate date{};
    std::uint64_t pass = 0;
    std::uint64_t until = 0;
};
[[nodiscard]] std::string replayLine(const ReplayInput &input);
/* A line read: an input, nothing (a blank line, a comment), or what is
 * wrong with it. */
struct ReplayLine {
    enum What { INPUT, NOTHING, MISTAKE } what = NOTHING;
    ReplayInput input;
    std::string mistake;
};
[[nodiscard]] ReplayLine readReplayLine(const std::string &text);

/* The recorder's lines (a state's, rewind.h): inputs.tsv's in the order
 * they came, checks.tsv's by pass, and the day it recorded last. Put back,
 * they are the recording's (and the files written again). */
struct RecordCopy {
    std::vector<ReplayInput> inputs;
    std::vector<std::pair<std::uint64_t, std::string>> checks;
    std::optional<ClockDate> day;
};
[[nodiscard]] RecordCopy recordCopy();
void putRecordCopy(const RecordCopy &copy);
/* A time's frame: a frame, a sixtieth of a second, or [h:]m:ss[.ss]
 * (commandline.h's --from and --to); none for words that are no time. And
 * a frame's time so, which replayFrameOf takes back to the frame (the
 * window's state over its picture). */
[[nodiscard]] std::optional<std::uint64_t> replayFrameOf(const std::string &text);
[[nodiscard]] std::string replayTimeText(std::uint64_t frame);
/* A day of the Gregorian calendar with its weekday (0 Sunday); none for a
 * day that is not one. */
[[nodiscard]] std::optional<ClockDate> clockDateOf(int year, int month, int day);
#endif
