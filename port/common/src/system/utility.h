/* 12d8:0000-0385, file name unknown: the SYSTEM library's module after
 * SYSTEM\SYSTEM.C (GOODS\MOD2AMF.EXE and CRUSND.EXE link it right after it).
 * Memory, string, file and number helpers. TLINK packed it into the segment
 * before DSFXMAN.C. */
#ifndef PORT_UTILITY_H
#define PORT_UTILITY_H
#include "common.h"

/* Nonzero when a file or directory matches path. The two are the same. */
i16 fileExists(const char *path);
/* A random number from low to high, both included, in either order. */
i16 randomBetween(i16 low, i16 high);
/* The larger of the distances along the axes. */
i16 distance(i16 x1, i16 y1, i16 x2, i16 y2);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void hugeCopy(void *destination, void *source, u32 count);
char *copyString(const char *text);
/* UNCALLED: nothing calls it. */
i16 isNewer(const char *path, const char *other);
char *upperCase(char *text);
/* UNCALLED: nothing calls it. */
i16 pathExists(const char *path);
/* UNCALLED: nothing calls it. */
u32 diskFree(u8 drive);
i16 distance3(i16 x1, i16 y1, u8 z1, i16 x2, i16 y2, u8 z2);
#endif
