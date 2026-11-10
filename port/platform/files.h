/* The game's files: the host's files by the game's DOS paths. Drive C, and
 * a path without a drive, is the directory the game is installed in (the
 * current one, the game's folder: system/gamefolder.h); any other drive is
 * the CD (the directory CRUSADER_CD names, or the image there or in the
 * game's folder: GOG's GAME.GOG, else Steam's GAME.BIN). "\\" and "/" both
 * separate the parts, and each part finds a host entry whatever its case; a
 * part no entry matches stays as it is, for a file to be made. The system
 * layer gives it (port/system/files.cpp). */
#ifndef PORT_PLATFORM_FILES_H
#define PORT_PLATFORM_FILES_H
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

/* What a call on an open file leaves: error 0 and its value, or the
 * host's error (errno). */
struct FileResult {
    int error;
    std::uint32_t value;
};

/* The files open: the host's descriptors, as many as FILES_OPEN at once.
 * The original's DOS gives the game 65 (its handle table's 70 less the
 * five standard ones), and CRUSYS.C counts on running out of them: it
 * makes files until one fails, into a table of 100. */
const int FILES_OPEN = 65;

/* Creates path, or empties it, open for reading and writing. The file. */
FileResult fileCreate(const char *path);
/* Creates a file in the host's memory, on no disk, open for reading and
 * writing; it goes when it is closed. The pass dump writes the game's state
 * in one. The file. */
FileResult fileCreateInMemory();
/* The bytes of a file in memory, none for another file. */
std::vector<std::uint8_t> fileMemoryBytes(int file);
/* Opens path for reading, or for reading and writing. The file. */
FileResult fileOpen(const char *path, bool writing);
FileResult fileClose(int file);
/* The bytes read, fewer at the end of the file. */
FileResult fileRead(int file, void *buffer, std::uint32_t count);
/* The bytes written, fewer when the disk is full. */
FileResult fileWrite(int file, const void *buffer, std::uint32_t count);
/* whence is SEEK_SET, SEEK_CUR or SEEK_END; the new position. */
FileResult fileSeek(int file, std::int32_t position, int whence);
/* The DOS date (high word) and time (low word) of the last write, as the
 * game compares its files' times. Dates outside 1980-2107 are clamped to
 * the endpoints; an unrepresentable date becomes 1980-01-01 00:00:00. */
FileResult fileTime(int file);
/* Writes what the host holds of the file out. */
FileResult fileCommit(int file);
/* Locks or unlocks length bytes at offset: the one program's locks never
 * conflict. */
FileResult fileLock(int file, bool unlock, std::int32_t offset, std::int32_t length);
/* Borland's chsize: cuts the file to size or fills it with zeros to it; 0,
 * or -1 when it cannot. */
int fileSetSize(int file, std::int32_t size);

/* The files by their paths: Borland's _chmod (read-only or not), unlink,
 * rename (which fails when the new name is there) and fopen; 0, or -1 when
 * they cannot. */
int fileSetReadOnly(const char *path, bool readOnly);
int fileRemove(const char *path);
int fileRename(const char *from, const char *to);
std::FILE *fileStream(const char *path, const char *mode);
/* C's stdio on a stream fileStream opened: fread, fwrite, fgets, fprintf
 * and fclose, each as C's. */
std::size_t fileStreamRead(std::FILE *stream, void *buffer, std::size_t size, std::size_t count);
std::size_t fileStreamWrite(std::FILE *stream, const void *buffer, std::size_t size, std::size_t count);
char *fileStreamLine(std::FILE *stream, char *buffer, int size);
[[gnu::format(printf, 2, 3)]] int fileStreamPrint(std::FILE *stream, const char *format, ...);
int fileStreamClose(std::FILE *stream);
/* A file's bytes, none when it cannot be read: read on the system's side,
 * in the host's heap, so that a look at a file (the check of a saved game
 * before its load) takes nothing of the arena's (system/gamememory.h). */
std::vector<std::uint8_t> fileContents(const char *path);

/* A search of a directory, with the widths of Borland's ffblk (the game
 * keeps one in FileSpec): the search's own bytes, then what it found. */
struct FileFind {
    char reserved[21];
    char attributes;
    std::uint16_t time;
    std::uint16_t date;
    std::int32_t size;
    char name[13];
};
/* The attribute of a directory, which a search finds only when it asks for
 * it; the files found have FILE_ARCHIVE. */
const int FILE_DIRECTORY = 0x10;
const int FILE_ARCHIVE = 0x20;
/* The first entry that matches pattern (a path whose last part may have ?
 * and *, in the 8.3 names DOS sees, capitals), or the next of the search:
 * 0, or -1 when there is none (Borland's findfirst and findnext). Dates
 * follow fileTime's range; a failed date lookup uses the first DOS date. */
int fileFindFirst(const char *path, FileFind *block, int attributes);
int fileFindNext(FileFind *block);

/* The current directory with its drive, the root of drive C ("C:\\"), as
 * Borland's getcwd gives it. */
char *fileCurrentDirectory(char *buffer, int length);
/* The free space of drive (1 for A, 0 for the current one) as Borland's
 * getdfree gives it: clusters of 32K, as many as 16 bits count;
 * sectorsPerCluster is FFFFh for a drive there is not. */
struct FileSpace {
    std::uint16_t available;
    std::uint16_t total;
    std::uint16_t bytesPerSector;
    std::uint16_t sectorsPerCluster;
};
void fileFreeSpace(std::uint8_t drive, FileSpace *table);

/* The game of the folder the game's data are in: "regret" or "remorse"
 * when its original's program is there (No Regret's REGRET.EXE, No
 * Remorse's CRUSADER.EXE), else "". */
const char *fileFolderGame();
/* The game enhanced.cfg names (game=) in the directory the program starts
 * in: the last one played, "" for none. Read once, before main, where the
 * one program chooses its game's values (unified/src/system/values.cpp). */
const char *fileLastGame();
#endif
