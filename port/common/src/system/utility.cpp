/* 12d8:0000-0385, file name unknown: memory, string, file and number
 * helpers. See utility.h. */
#include <cstdlib>
#include <cstring>
#include <cctype>
#include "utility.h"
#include "files.h"
#include "random.h"

/* memcpy moves at most this many bytes at a time. */
constexpr int COPY_CHUNK = 0xfffa;

/* Copies count bytes, over 64K too. The original moves the bases of the two
 * pointers' selectors along after each chunk and restores them; the port's
 * pointers move along. */
/* UNCALLED: nothing calls it. */
void hugeCopy(void *destination, void *source, u32 count)
{
    u8 *to = static_cast<u8 *>(destination);
    u8 *from = static_cast<u8 *>(source);
    u16 chunk;

    while (count > 0) {
        chunk = count > COPY_CHUNK ? COPY_CHUNK : static_cast<u16>(count);
        memcpy(to, from, chunk);
        count -= chunk;
        to += chunk;
        from += chunk;
    }
}

/* A copy of text in memory from new, or 0. */
char *copyString(const char *text)
{
    const u16 length = strlen(text) + 1;
    char *copy = new char[length];

    if (copy != nullptr) {
        memcpy(copy, text, length);
    }
    return copy;
}

/* Nonzero when path was written after other; a missing file counts as the
 * oldest. */
/* UNCALLED: nothing calls it. */
i16 isNewer(const char *path, const char *other)
{
    u32 time;
    u32 otherTime;
    FileFind find;

    if (fileFindFirst(path, &find, 0) != 0) {
        time = 0;
    } else {
        time = (static_cast<u32>(find.date) << 16) | find.time;
    }
    if (fileFindFirst(other, &find, 0) != 0) {
        otherTime = 0;
    } else {
        otherTime = (static_cast<u32>(find.date) << 16) | find.time;
    }
    return static_cast<i16>(time > otherTime);
}

/* Converts text to upper case in place and returns it. */
char *upperCase(char *text)
{
    char *letter;

    if (text != nullptr) {
        for (letter = text; *letter != 0; letter++) {
            *letter = toupper(*letter);
        }
    }
    return text;
}

i16 fileExists(const char *path)
{
    FileFind find;

    return static_cast<i16>(fileFindFirst(path, &find, FILE_DIRECTORY) == 0);
}

/* UNCALLED: nothing calls it. */
i16 pathExists(const char *path)
{
    FileFind find;

    return static_cast<i16>(fileFindFirst(path, &find, FILE_DIRECTORY) == 0);
}

/* The free bytes of drive (0 is A:); 1 when the drive cannot be read. */
/* UNCALLED: nothing calls it. */
u32 diskFree(u8 drive)
{
    FileSpace space;

    fileFreeSpace(drive + 1, &space);
    if (space.sectorsPerCluster == 0xffff) {
        return 1;
    }
    return static_cast<u32>(space.available) * space.bytesPerSector * space.sectorsPerCluster;
}

i16 randomBetween(i16 low, i16 high)
{
    if (low <= high) {
        return randomNumber(high - low + 1) + low;
    }
    return randomNumber(low - high + 1) + high;
}

/* The largest of the distances along the three axes. */
i16 distance3(i16 x1, i16 y1, u8 z1, i16 x2, i16 y2, u8 z2)
{
    i16 dx;
    i16 dy;
    i16 dz;
    i16 most;

    dx = abs(static_cast<i16>(x1 - x2));
    dy = abs(static_cast<i16>(y1 - y2));
    dz = abs(z1 - z2);
    most = dx > dy ? dx : dy;
    most = most > dz ? most : dz;
    return most;
}

i16 distance(i16 x1, i16 y1, i16 x2, i16 y2)
{
    i16 dx;
    i16 dy;

    dx = abs(static_cast<i16>(x1 - x2));
    dy = abs(static_cast<i16>(y1 - y2));
    return dx > dy ? dx : dy;
}
