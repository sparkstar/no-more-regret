/* The run's replay (replay.h). */
#include "replay.h"
#include "host.h"
#include "capture.h"
#include "gamememory.h"
#include "mousedrv.h"
#include "pc.h"
#include "portversion.h"
#include "profile.h"
#include "report.h"
#include "script.h"
#include "window.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <map>
#include <sstream>

namespace {

using Clock = std::chrono::steady_clock;

/* Where the input comes from (a recording, startReplay; else the window)
 * and whether it is recorded (startRecording). */
bool fromRecording = false;
bool recorder = false;
/* The replay in a window (watchReplay): the frame it is shown from, and
 * whether it has come to it (it stays shown when rewound). */
bool watched = false;
std::uint64_t watchFrom = 0;
bool shown = false;

/* The frame the input comes at, the window's or the replay's. */
std::uint64_t frameNow = 0;

/* The recorder: its directory and files, its lines (inputs.tsv's in the
 * order they came, checks.tsv's), how many of them the files have (all of
 * them again when they were cut), the day last recorded, the rerecords and
 * the run's lines of run.tsv. */
std::string recordDirectory;
std::FILE *inputs = nullptr;
std::FILE *checks = nullptr;
std::vector<ReplayInput> recordedInputs;
std::vector<std::pair<std::uint64_t, std::string>> recordedChecks;
std::size_t writtenInputs = 0;
std::size_t writtenChecks = 0;
bool rewrite = false;
std::optional<ClockDate> recordedDate;
std::uint64_t rerecords = 0;
std::string runLines;

/* The replay: its inputs in the frames' order and the next one, the passes'
 * ends, the states to check by pass, its last frame, the day, whether the
 * run has come to another state; the wall clock at its start and at the
 * last report of how far it is. */
std::vector<ReplayInput> replayInputs;
std::size_t nextInput = 0;
std::map<std::uint64_t, std::uint64_t> passEnds;
/* The keys the replay has given down, and an E0h before the next. */
std::bitset<256> replayKeys;
bool replayAfterE0 = false;
std::map<std::uint64_t, std::string> states;
std::uint64_t lastFrame = 0;
/* The recording ends with the game's own end, not the window's close; the
 * replay goes to the window's close. */
bool endsItself = false;
bool toClose = false;
std::optional<ClockDate> day;
bool differed = false;
Clock::time_point replayStart, lastReport;
/* Seconds of the wall clock between the reports. */
const int REPORT_SECONDS = 5;

/* A watched replay's lead (watchReplay): about what the rewind's ring
 * holds (256 MB, which No Remorse's play filled in 3.6 minutes); the
 * tests' PORT_WATCH_LEAD gives it in frames. */
std::uint64_t watchLead()
{
    static const std::uint64_t lead = [] {
        const char *frames = std::getenv("PORT_WATCH_LEAD");
        return frames != nullptr ? std::strtoull(frames, nullptr, 10) : std::uint64_t{4 * 60 * 60};
    }();
    return lead;
}

std::string pathIn(const std::string &directory, const char *name)
{
    return directory + "/" + name;
}

/* The words of a line, whitespace between them (a tab, or the spaces an
 * editor made of one), to a # or the end. */
std::vector<std::string> wordsOf(const std::string &text)
{
    std::istringstream line(text.substr(0, text.find('#')));
    std::vector<std::string> words;
    std::string word;
    while (line >> word) {
        words.push_back(word);
    }
    return words;
}

std::string joined(const std::vector<std::string> &words, std::size_t from, const char *between)
{
    std::string text;
    for (std::size_t k = from; k < words.size(); k++) {
        if (k > from) {
            text += between;
        }
        text += words[k];
    }
    return text;
}

std::optional<std::uint64_t> unsignedOf(const std::string &word, int base)
{
    if (word.empty() || word[0] == '-' || word[0] == '+') {
        return std::nullopt;
    }
    char *end = nullptr;
    unsigned long long value = std::strtoull(word.c_str(), &end, base);
    if (*end != 0) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(value);
}

std::optional<i16> shortOf(const std::string &word)
{
    char *end = nullptr;
    long value = std::strtol(word.c_str(), &end, 10);
    if (word.empty() || (*end != 0) || value < -32768 || value > 32767) {
        return std::nullopt;
    }
    return static_cast<i16>(value);
}

ReplayLine mistake(const std::string &what)
{
    ReplayLine line;
    line.what = ReplayLine::MISTAKE;
    line.mistake = what;
    return line;
}

std::string timeOf(std::uint64_t frame)
{
    char text[32];
    std::snprintf(text, sizeof text, "%llu:%02llu", static_cast<unsigned long long>(frame / 3600),
                  static_cast<unsigned long long>(frame / 60 % 60));
    return text;
}

/* An input to the recorder, at the frame now. */
void writeInput(ReplayInput input)
{
    input.frame = frameNow;
    recordedInputs.push_back(input);
}

const char *const INPUTS_HEADER = "# frame\tinput\tvalues (port/system/replay.h)\n";
const char *const CHECKS_HEADER = "# pass\ttime\tseed\tprocesses\tnpc\tx\ty\tz\tdirection\thp\tnpcs' hp\tdead\talert\n";

std::FILE *openRecorded(const char *name)
{
    const std::string path = pathIn(recordDirectory, name);
    std::FILE *file = std::fopen(path.c_str(), "w");
    if (file == nullptr) {
        portEnd(3, "the recording cannot be written to %s", recordDirectory.c_str());
    }
    return file;
}

/* run.tsv: the game, the port's version and when it was recorded, the
 * rerecords. */
void writeRun()
{
    std::FILE *run = openRecorded("run.tsv");
    std::fprintf(run, "%srerecords\t%llu\n", runLines.c_str(), static_cast<unsigned long long>(rerecords));
    std::fclose(run);
}

/* The recorder's lines not in its files yet to them; all of them again
 * after a cut. */
void flushRecording()
{
    if (inputs == nullptr) {
        return;
    }
    if (rewrite) {
        std::fclose(inputs);
        std::fclose(checks);
        inputs = openRecorded("inputs.tsv");
        checks = openRecorded("checks.tsv");
        std::fputs(INPUTS_HEADER, inputs);
        std::fputs(CHECKS_HEADER, checks);
        writtenInputs = 0;
        writtenChecks = 0;
        rewrite = false;
    }
    for (; writtenInputs < recordedInputs.size(); writtenInputs++) {
        const std::string line = replayLine(recordedInputs[writtenInputs]) + "\n";
        std::fputs(line.c_str(), inputs);
    }
    for (; writtenChecks < recordedChecks.size(); writtenChecks++) {
        std::fprintf(checks, "%s\n", recordedChecks[writtenChecks].second.c_str());
    }
    std::fflush(inputs);
    std::fflush(checks);
}

void endRecording()
{
    if (inputs != nullptr) {
        ReplayInput end;
        end.kind = ReplayInput::END;
        writeInput(end);
        flushRecording();
        std::fclose(inputs);
        inputs = nullptr;
        std::fclose(checks);
        checks = nullptr;
    }
}

/* The replay's last frame done: the run's end. */
[[noreturn]] void endReplay(std::uint64_t frame)
{
    const double seconds = std::chrono::duration<double>(Clock::now() - replayStart).count();
    std::fflush(stdout);
    std::fprintf(stderr, "replay: %s (%llu frames) in %.1f s, %llu inside the game's waits%s\n",
                 timeOf(frame + 1).c_str(), static_cast<unsigned long long>(frame + 1), seconds,
                 static_cast<unsigned long long>(framesInWaits()), differed ? ", not as recorded" : "");
    hostEnd(0);
}

void reportProgress(std::uint64_t frame)
{
    const Clock::time_point now = Clock::now();
    if (now - lastReport < std::chrono::seconds(REPORT_SECONDS)) {
        return;
    }
    lastReport = now;
    std::fflush(stdout);
    std::fprintf(stderr, "replay: %s of %s\n", timeOf(frame).c_str(), timeOf(lastFrame).c_str());
    /* A watched one's window, not shown yet, in its title too. */
    if (watched) {
        windowTitleNote(" - replaying " + timeOf(frame) + " of " + timeOf(watchFrom));
    }
}

/* A watched replay's run that ends before it is taken over: where, and
 * why when that is the recording's own end. */
void endWatching()
{
    if (!watched || !fromRecording) {
        return;
    }
    std::fflush(stdout);
    std::fprintf(stderr, "replay: the run ended at %s (frame %llu), before it was taken over at %s%s\n",
                 timeOf(frameNow).c_str(), static_cast<unsigned long long>(frameNow), timeOf(lastFrame).c_str(),
                 (endsItself && frameNow >= lastFrame)
                     ? ": the recording ends with the game's own end, --to an earlier time goes on before it"
                     : "");
}

/* A file of the recording, its lines; none when it is not there. */
std::vector<std::string> linesOf(const std::string &path, bool needed)
{
    std::ifstream in(path);
    if (!in && needed) {
        portRefuse("%s cannot be read", path.c_str());
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

}

std::optional<std::uint64_t> replayFrameOf(const std::string &text)
{
    if (text.empty()) {
        return std::nullopt;
    }
    if (text.find(':') == std::string::npos) {
        if (text.find_first_not_of("0123456789") != std::string::npos) {
            return std::nullopt;
        }
        return std::strtoull(text.c_str(), nullptr, 10);
    }
    std::vector<std::string> parts;
    std::size_t start = 0;
    for (;;) {
        const std::size_t colon = text.find(':', start);
        parts.push_back(text.substr(start, colon == std::string::npos ? std::string::npos : colon - start));
        if (colon == std::string::npos) {
            break;
        }
        start = colon + 1;
    }
    if (parts.size() > 3) {
        return std::nullopt;
    }
    std::uint64_t whole = 0;
    for (std::size_t k = 0; k + 1 < parts.size(); k++) {
        if (parts[k].empty() || parts[k].find_first_not_of("0123456789") != std::string::npos) {
            return std::nullopt;
        }
        whole = whole * 60 + std::strtoull(parts[k].c_str(), nullptr, 10);
    }
    const std::string &last = parts.back();
    char *end = nullptr;
    const double seconds = std::strtod(last.c_str(), &end);
    if (last.empty() || (*end != 0) || !(seconds >= 0) || seconds >= 60 ||
        last.find_first_of("eExXnN") != std::string::npos) {
        return std::nullopt;
    }
    return (whole * 3600) + static_cast<std::uint64_t>(std::llround(seconds * 60));
}

std::string replayTimeText(std::uint64_t frame)
{
    const auto hours = static_cast<unsigned long long>(frame / 216000);
    const auto minutes = static_cast<unsigned long long>(frame / 3600 % 60);
    const double seconds = static_cast<double>(frame % 3600) / 60;
    char text[48];
    if (hours != 0) {
        std::snprintf(text, sizeof text, "%llu:%02llu:%05.2f", hours, minutes, seconds);
    } else {
        std::snprintf(text, sizeof text, "%llu:%05.2f", minutes, seconds);
    }
    return text;
}

std::optional<ClockDate> clockDateOf(int year, int month, int dayOfMonth)
{
    using namespace std::chrono;
    const year_month_day date{std::chrono::year{year}, std::chrono::month{static_cast<unsigned>(month)},
                              std::chrono::day{static_cast<unsigned>(dayOfMonth)}};
    if (month < 1 || dayOfMonth < 1 || year < 1980 || year > 9999 || !date.ok()) {
        return std::nullopt;
    }
    return ClockDate{static_cast<std::uint16_t>(year), static_cast<std::uint8_t>(month),
                     static_cast<std::uint8_t>(dayOfMonth),
                     static_cast<std::uint8_t>(weekday{sys_days{date}}.c_encoding())};
}

std::string replayLine(const ReplayInput &input)
{
    const std::string line = std::to_string(input.frame) + "\t";
    char text[64];
    switch (input.kind) {
    case ReplayInput::KEY:
        if (input.scan >= 0x80 && input.scan != 0xe0 && input.scan != 0xe1) {
            std::snprintf(text, sizeof text, "release\t%02x", static_cast<unsigned>(input.scan & 0x7f));
        } else {
            std::snprintf(text, sizeof text, "key\t%02x", static_cast<unsigned>(input.scan));
        }
        return line + text;
    case ReplayInput::MOTION:
        std::snprintf(text, sizeof text, "motion\t%d\t%d\t%u", input.x, input.y, static_cast<unsigned>(input.buttons));
        return line + text;
    case ReplayInput::DEVICE:
        return line + joined(deviceWords(input.device), 0, "\t");
    case ReplayInput::CLOSE:
        return line + "close";
    case ReplayInput::DATE:
        std::snprintf(text, sizeof text, "date\t%04u-%02u-%02u", static_cast<unsigned>(input.date.year),
                      static_cast<unsigned>(input.date.month), static_cast<unsigned>(input.date.day));
        return line + text;
    case ReplayInput::PASS:
        return line + "pass\t" + std::to_string(input.pass) + "\t" + std::to_string(input.until);
    case ReplayInput::SAVE:
    case ReplayInput::LOAD:
        return line + (input.kind == ReplayInput::SAVE ? "save\t" : "load\t") + std::to_string(input.x);
    case ReplayInput::END:
        break;
    }
    return line + "end";
}

ReplayLine readReplayLine(const std::string &text)
{
    std::vector<std::string> words = wordsOf(text);
    if (words.empty()) {
        return {};
    }
    std::optional<std::uint64_t> frame = unsignedOf(words[0], 10);
    if (!frame) {
        return mistake("a line starts with its frame");
    }
    if (words.size() < 2) {
        return mistake("no input");
    }
    const std::string &verb = words[1];
    const std::size_t count = words.size() - 2;
    ReplayLine line;
    line.what = ReplayLine::INPUT;
    ReplayInput &input = line.input;
    input.frame = *frame;
    if (verb == "key" || verb == "release") {
        std::optional<std::uint64_t> scan = count == 1 ? unsignedOf(words[2], 16) : std::nullopt;
        if (!scan || *scan > 0xff || (verb == "release" && *scan >= 0x80)) {
            return mistake(verb + " <byte>, in hex" + (verb == "release" ? ", below 80" : ""));
        }
        input.kind = ReplayInput::KEY;
        input.scan = static_cast<u8>(verb == "release" ? *scan | 0x80 : *scan);
    } else if (verb == "motion") {
        std::optional<i16> x = count == 3 ? shortOf(words[2]) : std::nullopt;
        std::optional<i16> y = count == 3 ? shortOf(words[3]) : std::nullopt;
        std::optional<std::uint64_t> buttons = count == 3 ? unsignedOf(words[4], 10) : std::nullopt;
        if (!x || !y || !buttons || *buttons > 7) {
            return mistake("motion <x> <y> <buttons>: mickeys, and the buttons' bits 0-2");
        }
        input.kind = ReplayInput::MOTION;
        input.x = *x;
        input.y = *y;
        input.buttons = static_cast<u16>(*buttons);
    } else if (verb == "close" || verb == "end") {
        if (count != 0U) {
            return mistake(verb + " has nothing after it");
        }
        input.kind = verb == "close" ? ReplayInput::CLOSE : ReplayInput::END;
    } else if (verb == "date") {
        int year = 0;
        int month = 0;
        int dayOfMonth = 0;
        char rest = 0;
        std::optional<ClockDate> date;
        if (count == 1 && std::sscanf(words[2].c_str(), "%d-%d-%d%c", &year, &month, &dayOfMonth, &rest) == 3) {
            date = clockDateOf(year, month, dayOfMonth);
        }
        if (!date) {
            return mistake("date <yyyy-mm-dd>");
        }
        input.kind = ReplayInput::DATE;
        input.date = *date;
    } else if (verb == "save" || verb == "load") {
        std::optional<std::uint64_t> slot = count == 1 ? unsignedOf(words[2], 10) : std::nullopt;
        if (!slot || *slot > 12) {
            return mistake(verb + " <slot>, 0-12");
        }
        input.kind = verb == "save" ? ReplayInput::SAVE : ReplayInput::LOAD;
        input.x = static_cast<i16>(*slot);
    } else if (verb == "pass") {
        std::optional<std::uint64_t> pass = count == 2 ? unsignedOf(words[2], 10) : std::nullopt;
        std::optional<std::uint64_t> until = count == 2 ? unsignedOf(words[3], 10) : std::nullopt;
        if (!pass || !until || (*pass == 0U)) {
            return mistake("pass <pass> <ticks>");
        }
        input.kind = ReplayInput::PASS;
        input.pass = *pass;
        input.until = *until;
    } else if (readDeviceWords(verb, joined(words, 2, " "), &input.device)) {
        input.kind = ReplayInput::DEVICE;
    } else if (verb == "pad" || verb == "axis" || verb == "padgone" || verb == "mousebutton" || verb == "wheel") {
        return mistake(verb + ": pad <button> <1|0>, axis <axis> <value>, padgone, mousebutton <middle|x1|x2> <1|0>, "
                              "wheel <notches>");
    } else {
        return mistake("unknown input " + verb);
    }
    return line;
}

namespace {

/* A value of a recording's run.tsv, "" for none. */
std::string runValue(const std::string &directory, const char *key)
{
    std::string value;
    for (const std::string &line : linesOf(pathIn(directory, "run.tsv"), true)) {
        std::vector<std::string> words = wordsOf(line);
        if (words.size() == 2 && words[0] == key) {
            value = words[1];
        }
    }
    return value;
}

}

std::vector<std::string> replayArguments(const std::string &directory)
{
    std::vector<std::string> arguments;
    for (const std::string &line : linesOf(pathIn(directory, "run.tsv"), true)) {
        std::vector<std::string> words = wordsOf(line);
        if (!words.empty() && words[0] == "arguments") {
            arguments.assign(words.begin() + 1, words.end());
        }
    }
    return arguments;
}

void startRecording(const std::string &directory, const std::vector<std::string> &arguments)
{
    recordDirectory = directory;
    inputs = openRecorded("inputs.tsv");
    checks = openRecorded("checks.tsv");
    std::fputs(INPUTS_HEADER, inputs);
    std::fputs(CHECKS_HEADER, checks);
    const std::time_t now = std::time(nullptr);
    char when[32];
    std::strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    runLines = std::string("game\t") + PROFILE_NAME + "\nversion\t" + portVersion() + "\nrecorded\t" + when + "\n";
    if (!arguments.empty()) {
        for (const std::string &argument : arguments) {
            if (argument.find_first_of(" \t#") != std::string::npos) {
                portRefuse("the game's argument \"%s\" cannot be recorded (run.tsv's words have no spaces or #)",
                           argument.c_str());
            }
        }
        runLines += "arguments\t" + joined(arguments, 0, "\t") + "\n";
    }
    writeRun();
    recorder = true;
    std::atexit(endRecording);
}

std::string replayGame(const std::string &directory)
{
    return runValue(directory, "game");
}

void startReplay(const std::string &directory, std::uint64_t last)
{
    /* A replay first: a mistake in the recording ends the run without a
     * window's message box (report.h). */
    fromRecording = true;
    const std::string counted = runValue(directory, "rerecords");
    rerecords = unsignedOf(counted, 10).value_or(0);
    const std::string game = replayGame(directory);
    if (game != PROFILE_NAME) {
        portRefuse("%s is a recording of %s, not of %s", directory.c_str(), game.empty() ? "no game" : game.c_str(),
                   PROFILE_NAME);
    }
    const std::string path = pathIn(directory, "inputs.tsv");
    std::vector<std::string> lines = linesOf(path, true);
    std::optional<std::uint64_t> end;
    for (std::size_t number = 0; number < lines.size(); number++) {
        const ReplayLine line = readReplayLine(lines[number]);
        if (line.what == ReplayLine::MISTAKE) {
            portRefuse("%s line %llu: %s", path.c_str(), static_cast<unsigned long long>(number + 1),
                       line.mistake.c_str());
        }
        if (line.what == ReplayLine::NOTHING) {
            continue;
        }
        const ReplayInput &input = line.input;
        if (input.kind == ReplayInput::PASS) {
            passEnds[input.pass] = input.until;
            continue;
        }
        if ((input.kind == ReplayInput::END || input.kind == ReplayInput::CLOSE) && (!end || input.frame < *end)) {
            end = input.frame;
            endsItself = input.kind == ReplayInput::END;
        }
        if (input.kind == ReplayInput::DATE && !day) {
            day = input.date;
        }
        replayInputs.push_back(input);
    }
    std::stable_sort(replayInputs.begin(), replayInputs.end(),
                     [](const ReplayInput &a, const ReplayInput &b) { return a.frame < b.frame; });
    if (end) {
        lastFrame = *end;
        toClose = !endsItself && *end <= last;
    } else if (replayInputs.empty()) {
        lastFrame = 0;
    } else {
        lastFrame = replayInputs.back().frame;
    }
    lastFrame = std::min(lastFrame, last);
    for (const std::string &line : linesOf(pathIn(directory, "checks.tsv"), false)) {
        std::vector<std::string> words = wordsOf(line);
        std::optional<std::uint64_t> pass = words.empty() ? std::nullopt : unsignedOf(words[0], 10);
        if (pass) {
            states[*pass] = joined(words, 0, "\t");
        }
    }
    replayStart = lastReport = Clock::now();
}

bool recording()
{
    return recorder;
}

bool replaying()
{
    return fromRecording;
}

bool followingRecording()
{
    return recorder || fromRecording;
}

void watchReplay(std::uint64_t from)
{
    watched = true;
    /* Taken over at the window's close: from the frame before, without the
     * input that came with the close (Alt+F4's keys). */
    if (toClose && lastFrame > 0) {
        lastFrame--;
    }
    watchFrom = std::min(from, lastFrame);
    std::atexit(endWatching);
}

bool replayWatched()
{
    return watched;
}

bool watching()
{
    return watched && shown;
}

bool replayFast()
{
    return fromRecording && watched && !shown && frameNow + watchLead() < watchFrom;
}

bool replayEndsItself()
{
    return endsItself;
}

bool hostDraws()
{
    const SystemSide systemSide;
    return !replayFast();
}

bool replayEnded()
{
    return fromRecording && frameNow >= lastFrame;
}

std::bitset<256> takeOver()
{
    fromRecording = false;
    std::fflush(stdout);
    std::fprintf(stderr, "replay: taken over at %s (frame %llu): the window's input from now on, %s\n",
                 timeOf(frameNow).c_str(), static_cast<unsigned long long>(frameNow),
                 recorder ? "recorded" : "not recorded");
    /* The player goes on from where the replay came: said when that is not
     * where the recording went (replayCheck). */
    if (differed) {
        windowWarning("The replay did not go as recorded (the terminal says from where): the game goes on from where "
                      "the replay came, not from where the recording was.");
    }
    return replayKeys;
}

void recordFrame(std::uint64_t frame)
{
    if (!recorder) {
        return;
    }
    flushRecording();
    frameNow = frame;
}

void replayFrame(std::uint64_t frame)
{
    flushRecording();
    /* Watched past its end: nothing more until it is taken over. */
    if (watched && frame > lastFrame) {
        return;
    }
    frameNow = frame;
    if (watched && !shown && frame >= watchFrom) {
        shown = true;
        windowTitleNote("");
        if (watchFrom != 0) {
            const double seconds = std::chrono::duration<double>(Clock::now() - replayStart).count();
            std::fflush(stdout);
            std::fprintf(stderr, "replay: shown from %s (frame %llu), %.1f s after the start\n", timeOf(frame).c_str(),
                         static_cast<unsigned long long>(frame), seconds);
        }
    }
    bool ending = frame >= lastFrame;
    while (nextInput < replayInputs.size() && replayInputs[nextInput].frame <= frame) {
        const ReplayInput &input = replayInputs[nextInput++];
        switch (input.kind) {
        case ReplayInput::KEY:
            keyboardScan(input.scan);
            if (input.scan == 0xe0) {
                replayAfterE0 = true;
            } else {
                replayKeys[(input.scan & 0x7f) + (replayAfterE0 ? 128 : 0)] = (input.scan & 0x80) == 0;
                replayAfterE0 = false;
            }
            break;
        case ReplayInput::MOTION:
            mouseMotion(input.x, input.y, input.buttons);
            break;
        case ReplayInput::DEVICE:
            keyboardDevice(input.device);
            break;
        case ReplayInput::DATE:
            day = input.date;
            recordedDate = input.date;
            break;
        case ReplayInput::CLOSE:
        case ReplayInput::END:
            ending = true;
            break;
        case ReplayInput::PASS:
            break;
        case ReplayInput::SAVE:
        case ReplayInput::LOAD: {
            /* As a script's, at the end of this frame's pass. */
            InputEvent action{};
            action.time = pcTime();
            action.kind = input.kind == ReplayInput::SAVE ? InputEvent::SAVE : InputEvent::LOAD;
            action.slot = input.x;
            addInput(action);
            break;
        }
        }
        /* What the replay gives, recorded as it is given. */
        if (recorder && input.kind != ReplayInput::CLOSE && input.kind != ReplayInput::END) {
            writeInput(input);
        }
    }
    captureFrame(frame);
    if (ending && !watched) {
        endReplay(frame);
    }
    if (!watching()) {
        reportProgress(frame);
    }
}

void recordPass(std::uint64_t pass, std::uint64_t until)
{
    if (!recorder) {
        return;
    }
    ReplayInput input;
    input.kind = ReplayInput::PASS;
    input.pass = pass;
    input.until = until;
    writeInput(input);
}

std::uint64_t replayPass(std::uint64_t pass)
{
    auto end = passEnds.find(pass);
    if (end == passEnds.end()) {
        return 0;
    }
    recordPass(pass, end->second);
    return end->second;
}

void recordDate(const ClockDate &date)
{
    if (!recorder) {
        return;
    }
    if (recordedDate && recordedDate->year == date.year && recordedDate->month == date.month &&
        recordedDate->day == date.day) {
        return;
    }
    recordedDate = date;
    ReplayInput input;
    input.kind = ReplayInput::DATE;
    input.date = date;
    writeInput(input);
}

std::optional<ClockDate> replayDate()
{
    return day;
}

void recordScan(u8 scan)
{
    if (!recorder || fromRecording) {
        return;
    }
    ReplayInput input;
    input.kind = ReplayInput::KEY;
    input.scan = scan;
    writeInput(input);
}

void recordMotion(i16 x, i16 y, u16 buttons)
{
    if (!recorder || fromRecording) {
        return;
    }
    ReplayInput input;
    input.kind = ReplayInput::MOTION;
    input.x = x;
    input.y = y;
    input.buttons = buttons;
    writeInput(input);
}

void recordDevice(const DeviceEvent &device)
{
    if (!recorder || fromRecording) {
        return;
    }
    ReplayInput input;
    input.kind = ReplayInput::DEVICE;
    input.device = device;
    writeInput(input);
}

void recordClose()
{
    /* Recorded while a replay gives the input too: the window's close ends
     * the run there, which a continued replay goes on from (--continue). */
    if (!recorder) {
        return;
    }
    ReplayInput input;
    input.kind = ReplayInput::CLOSE;
    writeInput(input);
}

bool replayChecks(std::uint64_t pass)
{
    if ((pass % 60) != 0U) {
        return false;
    }
    return recorder || (fromRecording && !differed && (states.count(pass) != 0U));
}

void replayCheck(std::uint64_t pass, const std::string &state)
{
    const std::string values = joined(wordsOf(state), 0, "\t");
    if (recorder) {
        recordedChecks.emplace_back(pass, values);
    }
    auto recorded = states.find(pass);
    if (!fromRecording || differed || recorded == states.end() || recorded->second == values) {
        return;
    }
    differed = true;
    std::fflush(stdout);
    std::fprintf(stderr,
                 "replay: the run is not as recorded after pass %llu (about %s): the recording has\n"
                 "  %s\nand the replay\n  %s\n",
                 static_cast<unsigned long long>(pass), timeOf(frameNow).c_str(), recorded->second.c_str(),
                 values.c_str());
}

ReplayPlace replayPlace()
{
    return {frameNow, nextInput, day, replayKeys, replayAfterE0};
}

void putReplayPlace(const ReplayPlace &place)
{
    frameNow = place.frame;
    nextInput = place.input;
    day = place.day;
    replayKeys = place.keys;
    replayAfterE0 = place.afterE0;
}

RecordPlace recordPlace()
{
    return {recordedInputs.size(), recordedChecks.size(), recordedDate};
}

void putRecordPlace(const RecordPlace &place)
{
    if (place.inputs < recordedInputs.size() || place.checks < recordedChecks.size()) {
        recordedInputs.resize(std::min(place.inputs, recordedInputs.size()));
        recordedChecks.resize(std::min(place.checks, recordedChecks.size()));
        rewrite = true;
    }
    recordedDate = place.day;
}

RecordCopy recordCopy()
{
    return {recordedInputs, recordedChecks, recordedDate};
}

void putRecordCopy(const RecordCopy &copy)
{
    recordedInputs = copy.inputs;
    recordedChecks = copy.checks;
    recordedDate = copy.day;
    rewrite = true;
}

void countRerecord()
{
    if (!recorder) {
        return;
    }
    rerecords++;
    writeRun();
}
