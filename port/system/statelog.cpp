/* The game's state at each pass: with REGRET_STATE set to a file, a line
 * for each pass of the game's loop (clockPass), so that two runs of one
 * input, or a run and the original's (the pass dump's S records), can be
 * compared line by line. A line has the pass, the virtual time in ticks
 * of the interval timer, the random seed, the processes alive, the
 * Silencer's item, place, direction and hit points, the hit points of all
 * the NPCs and how many are dead, and the alert. A recording keeps the
 * line of each 60th pass and a replay compares its own with it (replay.h). */
#include "alert.h"
#include "flag.h"
#include "itemdata.h"
#include "kernel.h"
#include "npc.h"
#include "passdump.h"
#include "pc.h"
#include "random.h"
#include "replay.h"
#include "report.h"
#include "trace.h"

#include <cstdio>
#include <cstdlib>

/* kernel.cpp's table of processes. */
extern Process **&processes;
extern u16 &processLimit;

namespace {

std::FILE *file = nullptr;
std::uint64_t passes = 0;

void writeState()
{
    /* The usecode's globals the pass changed, in the trace as of the pass
     * (before it is counted). */
    traceGlobals(flags, static_cast<std::uint16_t>(flagSize));
    passes++;
    const bool checked = replayChecks(passes);
    if ((file == nullptr) && !checked && !passDumping()) {
        return;
    }
    unsigned alive = 0;
    for (u16 pid = 0; (processes != nullptr) && pid < processLimit; pid++) {
        if (processes[pid] != nullptr) {
            alive++;
        }
    }
    unsigned long hitPoints = 0;
    unsigned dead = 0;
    for (u16 npc = 0; (npcs != nullptr) && npc < NPCS; npc++) {
        hitPoints += npcs[npc].hp;
        dead += npcs[npc].dead;
    }
    const u16 self = controlledNpc.id;
    const bool placed = (npcs != nullptr) && (itemX != nullptr) && (self != 0U) && self < NPCS;
    char line[256];
    std::snprintf(line, sizeof line, "%llu %llu %08X %u %u %u %u %u %u %u %lu %u %u",
                  static_cast<unsigned long long>(passes), static_cast<unsigned long long>(pcTime()), getRandomSeed(),
                  alive, self, placed ? itemX[self] : 0, placed ? itemY[self] : 0, placed ? itemZ[self] : 0,
                  placed ? npcs[self].direction : 0, placed ? npcs[self].hp : 0, hitPoints, dead, alertOn);
    if (file != nullptr) {
        std::fprintf(file, "%s\n", line);
    }
    if (checked) {
        replayCheck(passes, line);
    }
    passDumpPass(passes, line);
}

[[maybe_unused]] const bool STARTED = [] {
    if (const char *path = std::getenv("REGRET_STATE")) {
        file = std::fopen(path, "w");
        if (!file) {
            portRefuse("REGRET_STATE %s cannot be written", path);
        }
    }
    portPassHook = writeState;
    return true;
}();

}

std::uint64_t stateLogPasses()
{
    return passes;
}

void putStateLogPasses(std::uint64_t count)
{
    passes = count;
}
