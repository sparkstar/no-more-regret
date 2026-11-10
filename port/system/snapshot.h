/* A memory snapshot of the run, taken and put back between two of the
 * game's steps (end.h's playGame), when none of the game's calls is
 * running: the game's arena (gamememory.h), its globals among the rest
 * (platform/gamestate.h), and the virtual PC (pcstate.h). Put back, the run
 * goes on from that step as it went then; on every host, the same way.
 *
 * Within one run of the program: the snapshot holds the program's
 * addresses (the vtables, the functions, the arena), which another run,
 * loaded elsewhere, does not have.
 *
 * While the run follows a recording (replay.h's followingRecording) a
 * snapshot holds also what the recording's input made outside the game's
 * state: the player's settings (gamememory.h's gameSettings) and files
 * (hostpath.h), and where the run is in the recordings: the replay's place,
 * the recorder's lines, the state log's passes. Put back, the recorder has
 * the snapshot's lines: a rerecord. */
#ifndef PORT_SNAPSHOT_H
#define PORT_SNAPSHOT_H
#include "hostpath.h"
#include "pcstate.h"
#include "replay.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct Snapshot {
    std::vector<std::byte> arena;
    PcState pc;
    bool following = false;
    std::vector<std::byte> settings;
    std::vector<PlayerFile> files;
    ReplayPlace replay;
    RecordCopy record;
    std::uint64_t logged = 0;
};

/* Whether a snapshot can be taken now: the arena there, between two
 * steps. */
[[nodiscard]] bool snapshotReady();
[[nodiscard]] Snapshot takeSnapshot();
void putSnapshot(const Snapshot &snapshot);

/* hostFrame's, after a step in which a frame passed. With PORT_SNAPSHOT_TEST=<frame>:<frames> in
 * a replay, the snapshots' test: a snapshot at that frame, the frames more,
 * then the snapshot put back with the replay's place, so that the replay
 * goes on as recorded only if the snapshot held all of the run. */
void snapshotFrame();
/* The player's settings and files now, hashed (the tests' check that they
 * went back with a rewind or a state's load). */
[[nodiscard]] std::uint64_t playersHash();
#endif
