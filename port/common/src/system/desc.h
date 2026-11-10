/* MEMORY\DESC.C (12a0:0000-0721) and DESCINIT.C: 80286/386 segment
 * descriptors in the GDT and the LDT, a pool of free LDT descriptors, and
 * 32 selectors that window linear memory. The port has no descriptors;
 * what is left is the pool's count. */
#ifndef PORT_DESC_H
#define PORT_DESC_H
#include "common.h"

/* The pool's free descriptors, which the memory status shows (memoryStatus,
 * the world's status text). The port's pool has 400h, less one for each of
 * the sound cache's slots, which had a selector each (cprotmem.cpp). */
extern u16 &poolFree;
#endif
