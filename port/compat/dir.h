/* Borland's dir.h for the port: the path split. The searches are the
 * platform's files (files.h). */
#ifndef PORT_DIR_H
#define PORT_DIR_H
#include <cstdint>
#ifndef _WIN32
#include <unistd.h>
#endif

constexpr int MAXPATH = 80;
constexpr int MAXDRIVE = 3;
constexpr int MAXDIR = 66;
constexpr int MAXFILE = 9;
constexpr int MAXEXT = 5;
constexpr int WILDCARDS = 0x01;
constexpr int EXTENSION = 0x02;
constexpr int FILENAME = 0x04;
constexpr int DIRECTORY = 0x08;
constexpr int DRIVE = 0x10;

int fnsplit(const char *path, char *drive, char *directory, char *name, char *extension);

#endif
