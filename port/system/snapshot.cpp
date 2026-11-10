/* See snapshot.h. */
#include "snapshot.h"
#include "end.h"
#include "gamememory.h"
#include "pc.h"
#include "replay.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace {

[[noreturn]] void snapshotFailure(const char *what)
{
    std::fflush(stdout);
    std::fprintf(stderr, "snapshot: %s\n", what);
    std::fflush(stderr);
    std::_Exit(3);
}

/* The test's (snapshotFrame): when, how far, what it holds and the
 * player's settings and files then. */
struct Test {
    std::uint64_t at = 0;
    std::uint64_t frames = 0;
    std::unique_ptr<Snapshot> snapshot;
    std::uint64_t players = 0;
    bool done = false;
};

void hashBytes(std::uint64_t &hash, const void *bytes, std::size_t size)
{
    const auto *byte = static_cast<const unsigned char *>(bytes);
    for (std::size_t k = 0; k < size; k++) {
        hash ^= byte[k];
        hash *= 0x100000001b3ULL;
    }
}

std::optional<Test> testAsked()
{
    const char *asked = std::getenv("PORT_SNAPSHOT_TEST");
    if (asked == nullptr) {
        return std::nullopt;
    }
    Test test;
    unsigned long long at = 0;
    unsigned long long frames = 0;
    if (std::sscanf(asked, "%llu:%llu", &at, &frames) != 2 || frames == 0) {
        snapshotFailure("PORT_SNAPSHOT_TEST is <frame>:<frames>");
    }
    test.at = at;
    test.frames = frames;
    return test;
}

}

bool snapshotReady()
{
    return betweenSteps() && gameMemory().start != nullptr;
}

Snapshot takeSnapshot()
{
    if (!snapshotReady()) {
        snapshotFailure("none to take: no arena yet or the game's step running");
    }
    Snapshot snapshot;
    snapshot.arena = gameMemoryCopy();
    snapshot.pc = pcState();
    snapshot.following = followingRecording();
    if (snapshot.following) {
        const GameMemory settings = gameSettings();
        snapshot.settings.resize(settings.used);
        copyUnchecked(snapshot.settings.data(), settings.start, settings.used);
        snapshot.files = portPlayersFiles();
        snapshot.replay = replayPlace();
        snapshot.record = recordCopy();
        snapshot.logged = stateLogPasses();
    }
    return snapshot;
}

void putSnapshot(const Snapshot &snapshot)
{
    if (!snapshotReady()) {
        snapshotFailure("none to put back: no arena yet or the game's step running");
    }
    gameMemoryPut(snapshot.arena);
    putPcState(snapshot.pc);
    if (snapshot.following != followingRecording()) {
        snapshotFailure("a snapshot of a run that followed a recording otherwise");
    }
    if (snapshot.following) {
        const GameMemory settings = gameSettings();
        if (settings.used != snapshot.settings.size()) {
            snapshotFailure("the settings' bytes of another size");
        }
        copyUnchecked(settings.start, snapshot.settings.data(), settings.used);
        portSetPlayersFiles(snapshot.files);
        putReplayPlace(snapshot.replay);
        putRecordCopy(snapshot.record);
        putStateLogPasses(snapshot.logged);
        countRerecord();
    }
}

std::uint64_t playersHash()
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    const GameMemory settings = gameSettings();
    std::vector<std::byte> bytes(settings.used);
    copyUnchecked(bytes.data(), settings.start, settings.used);
    hashBytes(hash, bytes.data(), bytes.size());
    for (const PlayerFile &file : portPlayersFiles()) {
        const std::string path = file.path.string();
        hashBytes(hash, path.data(), path.size());
        if (file.bytes) {
            hashBytes(hash, file.bytes->data(), file.bytes->size());
        }
    }
    return hash;
}

void snapshotFrame()
{
    static std::optional<Test> test = testAsked();
    if (!test || test->done) {
        return;
    }
    const std::uint64_t frame = timerState().windowFrames;
    if (!test->snapshot && frame >= test->at) {
        test->snapshot = std::make_unique<Snapshot>(takeSnapshot());
        test->players = playersHash();
        std::fprintf(stderr, "snapshot: taken at frame %llu (%llu bytes of the arena)\n",
                     static_cast<unsigned long long>(frame),
                     static_cast<unsigned long long>(test->snapshot->arena.size()));
    } else if (test->snapshot && frame >= test->at + test->frames) {
        const std::uint64_t players = playersHash();
        putSnapshot(*test->snapshot);
        test->snapshot.reset();
        test->done = true;
        const std::uint64_t now = playersHash();
        std::fprintf(stderr, "snapshot: put back at frame %llu, the player's settings and files %s%s\n",
                     static_cast<unsigned long long>(frame), now == test->players ? "as they were" : "NOT as they were",
                     now != players ? ", changed since" : "");
    }
}
