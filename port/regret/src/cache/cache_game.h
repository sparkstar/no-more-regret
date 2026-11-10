/* No Regret's values of the cache (common/src/cache/cache.cpp). */
#ifndef PORT_CACHE_GAME_H
#define PORT_CACHE_GAME_H

/* The memory profiler's process runs (initCache); the MEMORY cheat turns
 * its drawing on and off (memoryProfiler). */
inline constexpr bool MEMORY_PROFILER = true;
/* The cache keeps a tenth of itself free (minimumFree, allocateCacheMemory):
 * findFree makes room for that much more, and the preloading stops at half
 * of it. */
inline constexpr bool CACHE_KEEPS_FREE = true;
/* ShapeHandler::closeSwap halts when keepSwap is off instead of making the
 * shape file anew: No Regret's; the port's keepSwap is always 1. */
inline constexpr bool SWAP_REBUILD_HALTS = true;
#endif
