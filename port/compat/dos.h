/* Borland's dos.h for the port: far pointers and the machine. A far
 * pointer is a selector and an offset in the original; the port has plain
 * pointers, and nothing builds or takes apart a far pointer. The rest is
 * the system layer's. */
#ifndef PORT_DOS_H
#define PORT_DOS_H
#include <cstdint>
#ifndef _WIN32
#include <unistd.h>
#endif

#endif
