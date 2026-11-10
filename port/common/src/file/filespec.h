/* FILE\FILESPEC.C (Code75, 1250): a DOS file search over a pattern such as
 * "C:\GAMES\*.SAV". A pattern "@name" takes the patterns one per line from
 * the file name. */
#ifndef PORT_FILESPEC_H
#define PORT_FILESPEC_H
#include <dir.h>
#include "common.h"
#include "file.h"
#include "files.h"

/* Merges a path into one of five rotating buffers. */
char *makePath(char *drive, const char *dir, const char *name, const char *ext); /* 1250:0000 */
/* Deletes makePath's buffers; the next call makes them again (the port's, for gameRelease). */
void freePaths();

class FileSpec {
public:
    /* The name and extension of the pattern given to set(). */
    char patternName[9];
    char patternExt[5];
    FileFind find;
    u16 attributes;
    /* The list file of an "@name" pattern. */
    File *list;
    /* The last fileFindFirst/fileFindNext result; nonzero when there is no
     * match. */
    u16 done;
    /* The pattern split by fnsplit; name and ext hold the current match. */
    char drive[3];
    char dir[66];
    char name[9];
    char ext[5];

    /* UNCALLED: nothing calls it. */
    FileSpec();                                                                                  /* 1250:056a */
    FileSpec(const char *newDrive, const char *newDir, const char *newName, const char *newExt); /* 1250:05c2 */
    FileSpec(const char *pattern, u16 attrib)
    {
        list = nullptr;
        set(pattern, attrib);
    }
    ~FileSpec();                               /* 1250:06eb */
    void set(const char *pattern, u16 attrib); /* 1250:032e */
    /* Deletes every matching file. */
    /* UNCALLED: nothing calls it. */
    void removeAll(); /* 1250:0756 */
    i16 exists();     /* 1250:07cd */
    /* Moves to the next match; 0 when there is none. */
    i16 next(); /* 1250:080d */
    /* Takes name and ext from a find result. */
    void setMatch(FileFind match); /* 1250:097a */
    /* Whether the current file is newer than other's (than time, in DOS
     * date:time form). */
    i16 isNewer(FileSpec *other); /* 1250:09f0 */
    /* UNCALLED: nothing calls it. */
    i16 isNewerThan(u32 time); /* 1250:0acb */
    /* The current file's date:time; 0 when it does not exist. */
    /* UNCALLED: nothing calls it. */
    u32 time(); /* 1250:0b41 */
    /* Searches the pattern again from its first match. */
    /* UNCALLED: nothing calls it. */
    void restart(); /* 1250:0baa */
    /* Empties the pattern. */
    void clear(); /* 1250:0c02 */
    /* Puts the pattern's name and extension back into name and ext. */
    void restorePattern(); /* 1250:0c68 */
    /* The current match (or the pattern) as a path. */
    char *path() { return makePath(drive, dir, name, ext); }
};
ASSERT_SIZE(FileSpec, 0x94);
#endif
