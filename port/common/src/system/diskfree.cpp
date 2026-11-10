/* Code11 (1050:039e-0494, file name unknown): see diskfree.h. */
#include <cstring>
#include "diskfree.h"
#include "filespec.h"
#include "files.h"
#include "utility.h"

/* Puts directory before name when that file is there (1050:039e). Nothing
 * calls it. */
/* UNCALLED: nothing calls it. */
void findFile(char *directory, char *name)
{
    char path[80];

    strcpy(path, name);
    if (fileExists(makePath(nullptr, directory, path, nullptr)) != 0) {
        strcpy(name, path);
    }
}

u32 diskFree()
{
    u32 bytes = 0;
    FileSpace space;
    u8 drive;
    char path[80];

    fileCurrentDirectory(path, 80);
    drive = path[0] & 0x1f;
    fileFreeSpace(drive, &space);
    if (space.sectorsPerCluster == 0xffff) {
        bytes = 0;
    } else {
        bytes = static_cast<u32>(space.available) * space.bytesPerSector * space.sectorsPerCluster;
    }
    return bytes;
}
