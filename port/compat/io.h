/* Borland's io.h for the port: the handle-level file functions that POSIX does not have. On Windows the C
 * library's io.h has them, which this one stands in front of: its own headers include io.h too. */
#ifndef PORT_IO_H
#define PORT_IO_H
#ifdef _WIN32
#include_next <io.h>
#else
#include <unistd.h>

int chsize(int handle, long size);
long tell(int handle);
#endif

#endif
