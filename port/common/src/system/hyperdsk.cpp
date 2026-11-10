/* Code11, hyperdsk.asm (1050:0000-0072) in C++. The original finds
 * HyperDisk, a DOS disk cache, on INT 2Fh's multiplex numbers from DFh up
 * ('DH' in BX, 'YH' back in CX) and turns its delayed writing off and on
 * (bit 1 of its state); where it finds none it does nothing. The port's
 * machine has no resident programs: it finds none. */
#include "hyperdsk.h"

/* UNCALLED: nothing calls it. */
void stopHyperDisk() {}

void startHyperDisk() {}
