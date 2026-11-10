/* FILE\FILESPEC.C (1250:0000-0ca0): DOS file searches. See filespec.h. */
#include <dir.h>
#include "gamestate.h"
#include <cstdio>
#include <cstring>
#include "common.h"
#include "system.h"
#include "utility.h"
#include "file.h"
#include "filespec.h"
#include "files.h"

/* The buffer makePath used last; -1 before the buffers exist. */
/* A find result's DOS date and time as one number, the date high. */
#define FILE_STAMP(find) (((u32)(find).date << 16) | (find).time)

namespace {
i16 &slot = gameGlobal<i16>(-1);
char *(&paths)[5] = gameGlobal<char *[5]>();
}

void freePaths()
{
    if (slot != -1) {
        for (char *&path : paths) {
            delete[] path;
            path = nullptr;
        }
        slot = -1;
    }
}

char *makePath(char *drive, const char *dir, const char *name, const char *ext)
{
    i16 part;

    if (slot == -1) {
        i16 i;

        for (i = 0; i < 5; i++) {
            paths[i] = new char[80];
        }
    }
    slot++;
    if (slot == 5) {
        slot = 0;
    }
    *paths[slot] = 0;
    part = 0;
    while (part != 4) {
        switch (part) {
        case 0:
            if (drive != nullptr && *drive != 0) {
                strcat(paths[slot], drive);
            }
            part++;
            break;
        case 1:
            if (dir != nullptr && *dir != 0) {
                char *path = paths[slot];

                if (strlen(paths[slot]) != 0) {
                    if (path[static_cast<ui16>(strlen(paths[slot]) - 1)] == '\\' && *dir == '\\') {
                        dir++;
                    } else if (path[static_cast<ui16>(strlen(paths[slot]) - 1)] != '\\' && drive != nullptr &&
                               strlen(drive) > 2 && *dir != '\\') {
                        strcat(paths[slot], "\\");
                    }
                }
                strcat(paths[slot], dir);
                if (path[static_cast<ui16>(strlen(paths[slot]) - 1)] != '\\') {
                    strcat(paths[slot], "\\");
                }
            }
            part++;
            break;
        case 2:
            if (name != nullptr && *name != 0) {
                const char *found;
                const char *last;
                const char *start;

                if (*name == '\\') {
                    name++;
                }
                found = nullptr;
                last = nullptr;
                start = name;
                while ((found = strstr(name, "\\")) != nullptr) {
                    last = found;
                }
                /* The original splits a name with backslashes into dir here, writing
                 * into the caller's strings; but the loop above never ends on a
                 * backslash (it searches from name again), so this is not reached. */
                if (last != nullptr) {
                    *const_cast<char *>(last) = 0;
                    strcat(const_cast<char *>(dir), start);
                    strcat(const_cast<char *>(dir), "\\");
                    name = last + 1;
                }
                strcat(paths[slot], name);
            }
            part++;
            break;
        case 3:
            if (ext != nullptr && *ext != 0) {
                if (*ext != '.') {
                    strcat(paths[slot], ".");
                }
                strcat(paths[slot], ext);
            }
            part++;
            break;
        }
    }
    return paths[slot];
}

void FileSpec::set(const char *pattern, u16 attrib)
{
    attributes = attrib;
    if (list != nullptr) {
        delete list;
        list = nullptr;
    }
    if (pattern != nullptr) {
        if (*pattern == '@') {
            list = new File;
            pattern++;
        }
        fnsplit(pattern, drive, dir, name, ext);
        strcpy(patternName, name);
        strcpy(patternExt, ext);
        if (list != nullptr) {
            if (exists() == 0) {
                clear();
                return;
            }
            list->open(path(), FILE_READ, 0);
            list->readLine(formatBuffer, 80, -1L);
            pattern = formatBuffer;
        }
    } else {
        memset(static_cast<void *>(this), 0, sizeof(FileSpec));
    }
    done = fileFindFirst(pattern, &find, attributes);
    if (done == 0) {
        setMatch(find);
    }
    if (list != nullptr && done != 0) {
        done = 0;
        next();
    }
}

/* UNCALLED: nothing calls it. */
FileSpec::FileSpec()
{
    memset(static_cast<void *>(this), 0, sizeof(FileSpec));
    done = 1;
    attributes = 0;
    list = nullptr;
}

FileSpec::FileSpec(const char *newDrive, const char *newDir, const char *newName, const char *newExt)
{
    done = 1;
    attributes = 0;
    list = nullptr;
    if (newDrive != nullptr) {
        strcpy(this->drive, newDrive);
    } else {
        this->drive[0] = 0;
    }
    if (newDir != nullptr) {
        strcpy(this->dir, newDir);
    } else {
        this->dir[0] = 0;
    }
    if (newName != nullptr) {
        strcpy(this->name, newName);
        strcpy(patternName, newName);
    } else {
        this->name[0] = 0;
        patternName[0] = 0;
    }
    if (newExt != nullptr) {
        strcpy(this->ext, newExt);
        strcpy(patternExt, newExt);
    } else {
        this->ext[0] = 0;
        patternExt[0] = 0;
    }
}

FileSpec::~FileSpec()
{
    if (list != nullptr) {
        delete list;
        list = nullptr;
    }
}

/* UNCALLED: nothing calls it. */
void FileSpec::removeAll()
{
    char *file;

    restart();
    restart();
    do {
        file = path();
        /* Borland's unlink: DOS's, by the game's path (files.h). */
        fileRemove(file);
    } while (next() != 0);
}

i16 FileSpec::exists()
{
    return fileExists(path());
}

i16 FileSpec::next()
{
    bool found;

    if (done != 0) {
        return 0;
    }
    found = false;
    do {
        done = fileFindNext(&find);
        if (done != 0 && list != nullptr) {
            while ((list->atEnd() == 0U) && done != 0) {
                list->readLine(formatBuffer, 80, -1L);
                done = fileFindFirst(formatBuffer, &find, attributes);
                if (done == 0) {
                    setMatch(find);
                }
            }
        }
        if (done == 0 && (attributes == 0 || static_cast<ui16>(find.attributes) == attributes)) {
            found = true;
        }
    } while (!found && done == 0);
    if (!found) {
        restorePattern();
    } else {
        setMatch(find);
    }
    return static_cast<i16>(done == 0U);
}

void FileSpec::setMatch(FileFind match)
{
    char *dot;

    if (match.name[0] != '.') {
        dot = strstr(match.name, ".");
        if (dot != nullptr) {
            strcpy(ext, dot);
            *dot = 0;
        } else {
            ext[0] = 0;
        }
    } else {
        ext[0] = 0;
    }
    strcpy(name, match.name);
}

i16 FileSpec::isNewer(FileSpec *other)
{
    FileFind mine;
    FileFind theirs;
    u32 myTime;
    u32 theirTime;

    if (fileFindFirst(path(), &mine, 0) != 0) {
        return 0;
    }
    if (fileFindFirst(other->path(), &theirs, 0) != 0) {
        return 1;
    }
    myTime = FILE_STAMP(mine);
    theirTime = FILE_STAMP(theirs);
    return static_cast<i16>(myTime > theirTime);
}

/* UNCALLED: nothing calls it. */
i16 FileSpec::isNewerThan(u32 time)
{
    FileFind match;
    u32 stamp;

    if (fileFindFirst(path(), &match, 0) != 0) {
        return 0;
    }
    stamp = FILE_STAMP(match);
    return static_cast<i16>(stamp > time);
}

/* UNCALLED: nothing calls it. */
u32 FileSpec::time()
{
    FileFind match;

    if (fileFindFirst(path(), &match, 0) == 0) {
        return FILE_STAMP(match);
    }
    return 0;
}

/* UNCALLED: nothing calls it. */
void FileSpec::restart()
{
    restorePattern();
    set(path(), attributes);
}

void FileSpec::clear()
{
    memset(drive, 0, sizeof(drive));
    memset(dir, 0, sizeof(dir));
    memset(name, 0, sizeof(name));
    memset(ext, 0, sizeof(ext));
    done = 1;
}

void FileSpec::restorePattern()
{
    strcpy(name, patternName);
    strcpy(ext, patternExt);
}
