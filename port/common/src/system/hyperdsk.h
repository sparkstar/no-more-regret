/* Code11 (1050:0000-0072, file name unknown): HyperDisk, the disk cache,
 * called through INT 2Fh ('DH'); a TASM module (hyperdsk.cpp). */
#ifndef PORT_HYPERDSK_H
#define PORT_HYPERDSK_H
#include "common.h"

/* Stop and restart HyperDisk's delayed writes (1050:0041, 005a). */
/* UNCALLED: nothing calls it. */
void stopHyperDisk();
void startHyperDisk();
#endif
