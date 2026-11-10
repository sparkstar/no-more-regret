/* No Remorse's values of the cache (common/src/cache/cache.cpp). */
#ifndef PORT_CACHE_GAME_H
#define PORT_CACHE_GAME_H

/* The memory profiler's process runs (initCache): not in No Remorse, which
 * has no MEMORY cheat either (memoryProfiler stays 0). */
inline constexpr bool MEMORY_PROFILER = false;
/* The cache keeps a tenth of itself free (minimumFree, allocateCacheMemory):
 * not in No Remorse, whose findFree makes room for size alone. */
inline constexpr bool CACHE_KEEPS_FREE = false;
/* ShapeHandler::closeSwap halts when keepSwap is off instead of making the
 * shape file anew (No Regret's); the port's keepSwap is always 1. */
inline constexpr bool SWAP_REBUILD_HALTS = false;
#endif
