/* No Remorse's CPROTMEM.C sharing of the machine's memory (No Regret's is
 * port/regret/test/unit/cprotmem_test.cpp): the fragments the original makes on
 * its machine of 16 MB (DOSBox, memsize=16, CRUSADER -x43: "System memory:
 * 15000576 bytes", fragment 0 8550328 bytes, 1 300000, the largest block
 * 8550286), and the room the caches' blocks take as the original's heap gives
 * it. */
#include "cprotmem.h"
#include "gamestate.h"
#include "system.h"
#include "wezzy.h"

#include <cstdio>
#include <cstdlib>
#include <string>

/* What cprotmem.cpp and the game's values call of the rest of the game,
 * which the test does not make: the compaction's purge (never, the blocks
 * fit) and the weasel's visit, which No Remorse's saved objects name
 * (savegame_game.h). */
System *&currentSystem = gameGlobal<System *>(nullptr);
u8 &weaselVisited = gameGlobal<u8>(0);

void halt(std::source_location place)
{
    std::printf("halt %s %u\n", place.file_name(), (unsigned)place.line());
    std::exit(2);
}

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH remorse_cprotmem: %s\n", what.c_str());
    }
}

u32 largestOf(u16 fragment)
{
    u32 total, largest;
    fragmentFreeSpace(fragment, &total, &largest);
    return largest;
}

}

int main()
{
    check(systemMemory == 15000576, "the machine's memory");
    check(firstFragmentPercent == 57, "fragment 0's share");
    check(fragments[0].size == 8550328, "fragment 0, 57% of the bytes, a multiple of 4");
    check(fragments[SOUND_FRAGMENT].size == 300000, "the sound fragment");
    check(fragmentCount == 3 && fragments[2].size == 15000576 - 8550328 - 300000, "the rest in one fragment");
    u32 total, largest;
    u16 selectors;
    memoryStatus(&total, &largest, &selectors);
    check(largest == 8550286, "the largest block as the original reports it");
    /* The sound cache: all of the sound fragment but 4096 bytes (No
     * Regret's leaves 19096), then the rest of its heap. */
    u32 sound = largestOf(SOUND_FRAGMENT);
    check(sound == 299958, "the sound fragment's free space");
    void *soundCache = allocateMemory(sound - 4096, 0, SOUND_FRAGMENT);
    u32 soundLeft = largestOf(SOUND_FRAGMENT);
    check(soundCache && soundLeft == 4086, "the sound fragment after the sound cache: " + std::to_string(soundLeft));
    /* The cache: all of fragment 0 but 4K, rounded to pages past 1M:
     * 8546190 bytes take 8548352 and the header's 10. */
    void *cache = allocateMemory(largestOf(0) - 4096, 0, 0);
    u32 left = largestOf(0);
    check(cache && left == 1924, "fragment 0 after the cache: " + std::to_string(left));
    void *other = allocateMemory(1000, 0, ANY_FRAGMENT);
    check(other && largestOf(0) == left && largestOf(SOUND_FRAGMENT) == soundLeft, "another block, of the host's heap");
    check(!allocateMemory(0, 0, ANY_FRAGMENT), "0 bytes fail");
    releaseMemory(other, 0);
    releaseMemory(cache, 0);
    releaseMemory(soundCache, 0);
    check(largestOf(0) == 8550286 && largestOf(SOUND_FRAGMENT) == 299958, "the blocks given back");
    std::printf("remorse_cprotmem: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
