/* The machine's memory (platform/memory.h): the port's machine is the
 * original's with 16 MB, as DOSBox (memsize=16) runs it, where the
 * original's CPROTMEM.C measures what the game's profile gives (profile.h;
 * No Regret's). The same each run, as the virtual time is, so that each
 * run has the same caches. */
#include "memory.h"
#include "gamememory.h"
#include "profile.h"

std::uint32_t memoryTotal()
{
    const SystemSide systemSide;
    return PROFILE_MEMORY;
}
