/* The machine's memory: how much the game has, which the original measures
 * at its start (CPROTMEM.C asks the DOS extender for all it gives) and
 * shares out in fragments, so that the caches' sizes and the preloading of
 * a map's shapes come from it. The game allocates from the host's heap;
 * the number only decides the sharing. */
#ifndef PORT_PLATFORM_MEMORY_H
#define PORT_PLATFORM_MEMORY_H
#include <cstdint>

/* The bytes the machine gives the game (CPROTMEM.C's systemMemory). */
std::uint32_t memoryTotal();
#endif
