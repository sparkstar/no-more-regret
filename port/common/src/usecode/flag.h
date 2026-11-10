/* FLAG.C (13e8): the usecode's global flags, a block of memory the usecode
 * reads and writes by offset (usecode\unkds.dat, or an object of a saved
 * game's FLEX), and the table of their names for the debugger
 * (usecode\unkdssym.dat). */
#ifndef PORT_FLAG_H
#define PORT_FLAG_H
#include "common.h"
#include "flex.h"

extern u8 *&flags;
/* The flags' size in bytes. */
extern i16 &flagSize;

void freeFlags(); /* 13e8:0000 */
/* Reads the flags from the file, or from object of flex; then the names
 * when their file exists. */
void loadFlags(Flex *flex, i16 object); /* 13e8:0051 */
void saveFlags(Flex *flex, i16 object); /* 13e8:02e7 */
/* The offset and size of the flag called name; 0 when there is none. */
bool findFlag(const char *name, i16 *offset, i16 *size); /* 13e8:039f */
#endif
