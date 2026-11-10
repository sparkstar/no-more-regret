/* No Regret's values of sndcache.cpp. */
#ifndef PORT_SNDCACHE_GAME_H
#define PORT_SNDCACHE_GAME_H

/* What the sound cache leaves of the sound fragment's largest free block:
 * 15,000 bytes more than No Remorse's. */
inline constexpr int SOUND_CACHE_SPARE = (15000 + 4096);
#endif
