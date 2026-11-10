/* The port's own options on the command line (commandline.h). */
#include "commandline.h"
#include "capture.h"
#include "files.h"
#include "gamefolder.h"
#include "gamememory.h"
#include "host.h"
#include "hostpath.h"
#include "portversion.h"
#include "profile.h"
#include "replay.h"
#include "restart.h"
#include "report.h"
#include "video.h"
#include "window.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

/* The replay's sandbox (hostpath.h), which goes at the run's end, copied
 * to keptFiles first when it is set (--keep-files). */
fs::path sandbox;
fs::path keptFiles;

/* The debug features, for the whole run (hostDebugFeatures). */
bool debugFeatures = false;

/* The text without the spaces, tabs and returns at its ends. */
std::string trimmed(const std::string &text)
{
    const std::size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

std::string lowered(std::string text)
{
    for (char &c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

/* Whether the game folder's enhanced.cfg turns the debug features on. */
bool debugFeaturesConfigured()
{
    std::ifstream file(portHostPath("enhanced.cfg"), std::ios::binary);
    if (!file) {
        return false;
    }
    return debugFeatureLine(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
}

void copyTree(const fs::path &from, const fs::path &to);

void removeSandbox()
{
    std::error_code error;
    if (!keptFiles.empty()) {
        fs::create_directories(keptFiles, error);
        copyTree(sandbox, keptFiles);
    }
    fs::remove_all(sandbox, error);
}

/* A copy of a file or a directory's tree, the files' times kept. */
void copyTree(const fs::path &from, const fs::path &to)
{
    std::error_code error;
    if (fs::is_directory(from, error)) {
        fs::create_directories(to, error);
        for (const fs::directory_entry &entry : fs::directory_iterator(from, error)) {
            copyTree(entry.path(), to / entry.path().filename());
        }
        return;
    }
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, error);
    if (error) {
        portEnd(3, "%s cannot be copied to %s", from.string().c_str(), to.string().c_str());
    }
    fs::last_write_time(to, fs::last_write_time(from, error), error);
}

/* The player's files a recording starts with, as the game finds them:
 * savegame\, crusader.cfg and the port's enhanced.cfg. */
void keepPlayersFiles(const fs::path &files)
{
    std::error_code error;
    fs::create_directories(files, error);
    for (const char *name : {"savegame", "crusader.cfg", "enhanced.cfg"}) {
        const fs::path theirs = portHostPath(name);
        if (fs::exists(theirs, error)) {
            copyTree(theirs, files / theirs.filename());
        }
    }
}

/* A sandbox in the host's temporary directory with the recording's files. */
fs::path makeSandbox(const fs::path &files)
{
    std::random_device random;
    fs::path made = fs::temp_directory_path() / (std::string(PROFILE_NAME) + "-replay-" + std::to_string(random()));
    std::error_code error;
    fs::create_directories(made, error);
    if (error) {
        portEnd(3, "no sandbox for the replay at %s", made.string().c_str());
    }
    if (fs::is_directory(files, error)) {
        copyTree(files, made);
    }
    return made;
}

/* A path the command line gives, in the directory the program started in
 * (the program has moved into the game's folder: gamefolder.h). */
std::string argumentPath(const std::string &value)
{
    const fs::path path(value);
    return path.is_absolute() ? value : (startDirectory() / path).string();
}

/* The game --game asked for, the next run's when it is not told one: the
 * game= line of the port's enhanced.cfg (unified/src/system/values.cpp),
 * the file's other lines as they are. */
void rememberGame(const std::string &game)
{
    const fs::path path = portHostPath("enhanced.cfg");
    std::string text;
    if (std::ifstream file(path, std::ios::binary); file) {
        text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
    std::string kept;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        end = end == std::string::npos ? text.size() : end + 1;
        const std::string line = text.substr(start, end - start);
        if (line.rfind("game=", 0) != 0) {
            kept += line;
        }
        start = end;
    }
    std::ofstream file(path, std::ios::binary);
    file << "game=" << game << "\r\n" << kept;
}

/* The game's arguments a replay gives it (its recording's), which argv
 * points into for the rest of the run. */
std::vector<std::string> replayedArguments;
std::vector<char *> replayedArgv;

std::string spaced(const std::vector<std::string> &words)
{
    std::string text;
    for (const std::string &word : words) {
        text += (text.empty() ? "" : " ") + word;
    }
    return text;
}

}

bool debugFeatureLine(const std::string &text)
{
    std::size_t start = 0;
    bool on = false;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        const std::string line = trimmed(text.substr(start, end - start));
        start = end + 1;
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos || line[0] == '#' || line[0] == '/') {
            continue;
        }
        if (lowered(trimmed(line.substr(0, equals))) == "debug_feature") {
            const std::string value = lowered(trimmed(line.substr(equals + 1)));
            on = value == "true" || value == "1";
        }
    }
    return on;
}

std::string debugOption(const std::vector<std::string> &words)
{
    for (const std::string &word : words) {
        if (word == "--record" || word == "--replay" || word == "--continue" || word == "--watch" ||
            word == "--capture" || word == "--from" || word == "--to" || word == "--keep-files") {
            return word;
        }
    }
    return "";
}

bool hostDebugFeatures()
{
    const SystemSide systemSide;
    return debugFeatures;
}

const char *hostEnvironment(const char *name)
{
    const SystemSide systemSide;
    return std::getenv(name);
}

bool portVersionAsked(int argc, char **argv)
{
    for (int k = 1; k < argc; k++) {
        if (std::string(argv[k]) == "--version") {
            std::printf("%s %s\n", PORT_NAME, portVersion());
            return true;
        }
    }
    return false;
}

void portCommandLine(int &argc, char **&argv)
{
    /* The command line as it came, for a run of the other game (restart.h). */
    const std::vector<char *> given(argv, argv + argc + 1);
    std::string game;
    std::string record;
    std::string replay;
    std::string capture;
    std::optional<std::uint64_t> from;
    std::optional<std::uint64_t> to;
    bool watch = false;
    /* --continue: the replay watched from its end. */
    bool continued = false;
    /* The debug features: --debug-feature's, else enhanced.cfg's; the
     * options that are debug features end the run without them. */
    debugFeatures = debugFeaturesConfigured();
    for (int k = 1; k < argc; k++) {
        if (std::string(argv[k]) == "--debug-feature") {
            debugFeatures = true;
        }
    }
    if (!debugFeatures) {
        const std::string option = debugOption(std::vector<std::string>(argv + 1, argv + argc));
        if (!option.empty()) {
            portRefuse("%s is a debug feature: --debug-feature on the command line, or debug_feature=true in %s",
                       option.c_str(), portHostPath("enhanced.cfg").string().c_str());
        }
    }
    int kept = 1;
    for (int k = 1; k < argc; k++) {
        const std::string word = argv[k];
        if (word == "--debug-feature") {
            continue;
        }
        if (word == "--watch") {
            watch = true;
            continue;
        }
        if (word != "--record" && word != "--replay" && word != "--continue" && word != "--capture" &&
            word != "--from" && word != "--to" && word != "--game" && word != "--keep-files") {
            argv[kept++] = argv[k];
            continue;
        }
        if (k + 1 >= argc) {
            portRefuse("%s needs what follows it", word.c_str());
        }
        const std::string value = argv[++k];
        if (word == "--game") {
            if (value != "regret" && value != "remorse") {
                portRefuse("--game %s: regret or remorse", value.c_str());
            }
            game = value;
        } else if (word == "--record") {
            record = argumentPath(value);
        } else if (word == "--replay") {
            if (continued) {
                portRefuse("--continue replays its recording itself, without --replay");
            }
            replay = argumentPath(value);
        } else if (word == "--continue") {
            if (!replay.empty()) {
                portRefuse("--continue replays its recording itself, without --replay");
            }
            replay = argumentPath(value);
            continued = true;
        } else if (word == "--capture") {
            capture = value == "-" ? value : argumentPath(value);
        } else if (word == "--keep-files") {
            keptFiles = argumentPath(value);
        } else {
            const std::optional<std::uint64_t> frame = replayFrameOf(value);
            if (!frame) {
                portRefuse("%s %s: a frame or [h:]m:ss[.ss]", word.c_str(), value.c_str());
            }
            (word == "--from" ? from : to) = frame;
        }
    }
    argv[kept] = nullptr;
    argc = kept;
    /* The game's own arguments, which a recording keeps. */
    std::vector<std::string> arguments(argv + 1, argv + argc);
    if (!record.empty() && !replay.empty() && !capture.empty()) {
        portRefuse("--capture is a replay's that is not recorded");
    }
    if (watch && (replay.empty() || continued || !capture.empty())) {
        portRefuse("--watch watches a replay (--replay) in a window, without --capture (--continue watches its own)");
    }
    if (continued && (from || !capture.empty())) {
        portRefuse("--continue watches its recording from its end (--to's), without --from and --capture");
    }
    if ((!capture.empty() || from || to || !keptFiles.empty()) && replay.empty()) {
        portRefuse("--capture, --from, --to and --keep-files are a replay's (--replay, --continue)");
    }
    if (to && (*to == 0 || (from && *to <= *from))) {
        portRefuse("--to comes after the start and after --from");
    }
    /* The game the run plays: --game's, else the recording's. Another than
     * the one this run started with is a run of its own (restart.h). The
     * player's --game is the next run's game too. */
    const bool asked = !game.empty();
    if (game.empty() && !replay.empty()) {
        game = replayGame(replay);
    }
    /* A folder of one game's data plays that game (gamefolder.h). */
    const std::string folder = fileFolderGame();
    if (!folder.empty() && !game.empty() && game != folder) {
        portRefuse("%s has %s's data, not %s's", gameFolder().path.string().c_str(), folder.c_str(), game.c_str());
    }
    if (!game.empty() && game != PROFILE_NAME) {
        restartAs(game, given.data());
    }
    if (asked && replay.empty()) {
        rememberGame(game);
    }
    if (!replay.empty()) {
        startReplay(replay, to ? *to - 1 : ~static_cast<std::uint64_t>(0));
        /* The game's arguments the recording was made with. */
        const std::vector<std::string> recorded = replayArguments(replay);
        if (!arguments.empty() && arguments != recorded) {
            portRefuse("the replay gives the game its recording's arguments (\"%s\"), not \"%s\"",
                       spaced(recorded).c_str(), spaced(arguments).c_str());
        }
        arguments = recorded;
        replayedArguments = recorded;
        replayedArgv.assign(1, argv[0]);
        for (std::string &argument : replayedArguments) {
            replayedArgv.push_back(argument.data());
        }
        replayedArgv.push_back(nullptr);
        argv = replayedArgv.data();
        argc = static_cast<int>(replayedArgv.size()) - 1;
        /* Watched from --from (the start without), --continue from the end:
         * as fast as it can go before (replay.h). A recording that ends
         * with the game's own end goes on from before it only. */
        if (continued && !to && replayEndsItself()) {
            portRefuse("%s ends without the window's close (the game's own end, its menu's QUIT, or a replay recorded "
                       "to its end): --to an earlier time goes on from before it",
                       replay.c_str());
        }
        if (watch || continued) {
            watchReplay(continued ? ~static_cast<std::uint64_t>(0) : from.value_or(0));
            if (!windowRun()) {
                portRefuse("--watch and --continue need a window (a build with one, no REGRET_SCRIPT)");
            }
        }
        sandbox = makeSandbox(fs::path(replay) / "files");
        portSandbox(sandbox);
        std::atexit(removeSandbox);
        if (!capture.empty()) {
            /* The wide screen in the widest display's shape (capture.h); without
             * a display wider than 4:3 (wine's own is 4:3) video.h's
             * WIDE_DEFAULT. */
            if (const u16 width = displayWideWidth(); width > 640U) {
                setWideWidth(width);
            }
            startCapture(capture, from.value_or(0));
        }
    }
    if (!record.empty()) {
        if (!windowRun() && replay.empty()) {
            portRefuse("--record records a run with a window (REGRET_SCRIPT has none)");
        }
        std::error_code error;
        fs::create_directories(record, error);
        if (fs::exists(fs::path(record) / "inputs.tsv", error)) {
            portRefuse("%s has a recording already", record.c_str());
        }
        /* A replay recorded starts as the replay did: with its files. */
        if (replay.empty()) {
            keepPlayersFiles(fs::path(record) / "files");
        } else if (fs::is_directory(fs::path(replay) / "files", error)) {
            copyTree(fs::path(replay) / "files", fs::path(record) / "files");
        }
        startRecording(record, arguments);
    }
}
