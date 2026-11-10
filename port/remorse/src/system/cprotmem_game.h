/* No Remorse's values of the memory's sharing (common/src/system/cprotmem.cpp). */
#ifndef PORT_CPROTMEM_GAME_H
#define PORT_CPROTMEM_GAME_H
#include "common.h"

/* The sound's fragment's bytes. */
inline constexpr u32 SOUND_FRAGMENT_SIZE = 300000;
/* The share of the memory fragment 0 gets at first (firstFragmentPercent). */
inline constexpr u16 FIRST_FRAGMENT_PERCENT = 57;
/* The share goes by the memory's size, counted in kilobytes (startMemory): No Regret's. */
inline constexpr bool SHARE_BY_MEMORY_SIZE = false;
/* The smallest fragment 0 the original takes. */
inline constexpr u32 FIRST_MINIMUM = 0x180000L;
#endif
