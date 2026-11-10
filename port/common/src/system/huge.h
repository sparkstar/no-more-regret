/* MEMORY\HUGE.C (No Remorse only, 1298:0000-1477): fills of memory past 64K.
 * The original moves its pointer's selector base 64K at a time; the port's
 * pointers reach the whole block. Its cursors over linear memory (Huge,
 * HugeByte, HugeWord, HugeLong, a 64K window of a selector of their own) are
 * not ported: nothing calls them, in the original either, and they are made
 * of selectors. */
#ifndef PORT_HUGE_H
#define PORT_HUGE_H
#include "common.h"

/* Fills count bytes from pointer with value, 64K at a time (1298:0000):
 * only PLAYER.C calls it. */
/* UNCALLED: nothing calls it. */
void hugeMemset(void *pointer, char value, u32 count);

/* Nothing calls these, in the original either. */
/* hugeMemset with the arguments in setmem's order (1298:008c). */
/* UNCALLED: nothing calls it. */
void hugeSetmem(void *pointer, u32 count, char value);
#endif
