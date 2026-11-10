/* FLAG.C (13e8:0000-044c): the usecode's global flags. See flag.h. */
#include <cstdio>
#include "gamestate.h"
#include <cstring>
#include "flag.h"
#include "usemem.h"
#include "filespec.h"
#include "gamefile.h"
#include "utility.h"
#include "files.h"

/* The flags and the names, in the usecode directory. */
const char *(&flagFiles)[2] = gameGlobal<const char *[2]>({"unkds.dat", "unkdssym.dat"});
/* Entries of a name, its terminating zero, then the flag's offset and size
 * (two u16). */
char *&flagNames = gameGlobal<char *>(nullptr);
i16 &flagSize = gameGlobal<i16>(0);
i16 &flagNamesSize = gameGlobal<i16>(0);
u8 *&flags = gameGlobal<u8 *>();

void freeFlags()
{
    if (flags != nullptr) {
        usecodeMemory.remove(flags);
        delete[] flags;
        flags = nullptr;
    }
    if (flagNames != nullptr) {
        delete[] flagNames;
        flagNames = nullptr;
    }
    flagSize = 0;
}

void loadFlags(Flex *flex, i16 object)
{
    if (flags != nullptr) {
        usecodeMemory.remove(flags);
        delete[] flags;
        flags = nullptr;
    }
    if (flagNames != nullptr) {
        delete[] flagNames;
        flagNames = nullptr;
    }
    if (flex == nullptr) {
        char *path = makePath(nullptr, usecodeDirectory, flagFiles[0], nullptr);

        if (fileExists(path) != 0) {
            const File file(path, FILE_READ, 0);

            flagSize = file.size();
            if (flags == nullptr) {
                flags = new u8[flagSize];
            }
            usecodeMemory.setFlags(flags, flagSize);
            file.read(flags, flagSize);
        }
    } else {
        flagSize = flex->objectSize(object);
        if (flags == nullptr) {
            flags = new u8[flagSize];
        }
        usecodeMemory.setFlags(flags, flagSize);
        flex->readObject(object, flags, -1L, 0L);
    }
    FileSpec spec(makePath(nullptr, usecodeDirectory, flagFiles[1], nullptr), 0);

    if (spec.exists() != 0) {
        const File file(spec.path(), FILE_READ, 0);

        flagNamesSize = file.size();
        if (flagNames == nullptr) {
            flagNames = new char[flagNamesSize];
            file.read(flagNames, flagNamesSize);
        }
    }
}

void saveFlags(Flex *flex, i16 object)
{
    if (flex == nullptr) {
        char *path = makePath(nullptr, usecodeDirectory, flagFiles[0], nullptr);

        fileRemove(path);
        File file;
        file.open(path, FILE_READWRITE, 1);
        file.write(flags, flagSize);
    } else {
        flex->writeObject(object, flags, flagSize, 0L, 1);
    }
}

bool findFlag(const char *name, i16 *offset, i16 *size)
{
    bool found = false;

    if (name != nullptr) {
        char *entry = flagNames;
        i16 length;

        for (u16 i = 0; i < static_cast<ui16>(flagNamesSize); i += length + 4) {
            length = strlen(entry) + 1;
            char *symbol = entry;
            entry += length;
            if (strcmp(symbol, name) == 0) {
                *offset = (reinterpret_cast<i16 *>(entry))[0];
                *size = (reinterpret_cast<i16 *>(entry))[1];
                found = true;
                break;
            }
            entry += 4;
        }
    }
    return found;
}
