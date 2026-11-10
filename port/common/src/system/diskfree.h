/* Code11 (1050:039e-0494, file name unknown): a file's directory and the free
 * space of the current drive. It follows MissileTracker's module in the
 * segment; the order of the segment's NE relocations shows that it was a
 * module of its own. */
#ifndef PORT_DISKFREE_H
#define PORT_DISKFREE_H
#include "common.h"

/* The free bytes of the current drive; 0 when it cannot be read
 * (1050:0429). */
u32 diskFree();

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void findFile(char *directory, char *name);
#endif
