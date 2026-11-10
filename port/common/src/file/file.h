/* FILE\FILE.C (1240:0000-0f23): files, DOS's through INT 21h in the
 * original and the platform's in the port (files.h), with the file name kept
 * for error messages, and record locks. */
#ifndef PORT_FILE_H
#define PORT_FILE_H
#include "common.h"

/* File::open modes (INT 21h AH=3Dh AL): read only, read and write, and
 * the sharing bits that keep other handles from writing or let them do
 * anything. */
constexpr int FILE_READ = 0;
constexpr int FILE_READWRITE = 2;
constexpr int FILE_DENY_WRITE = 0x20;
constexpr int FILE_SHARED = 0x40;

class File {
public:
    char *name;
    /* The platform's file (the original's DOS handle), -1 when closed. */
    int handle;
    /* The INT 21h AH=3Dh access and sharing mode it was opened with. */
    u16 mode;

    File(); /* 1240:0000 */
    /* Opens path (1240:003a). */
    File(const char *path, u16 openMode, u8 create);
    ~File() { close(); }
    [[nodiscard]] u8 isOpen() const { return handle != -1 ? 1 : 0; }
    /* Keeps a copy of path, or none for 0 (1240:008f). */
    void setName(const char *path);
    /* Creates path empty and keeps the handle (1240:011b). */
    void create(const char *path);
    /* Opens a new empty file in the host's memory (files.h's
     * fileCreateInMemory), on no disk: the port's, for the pass dump. */
    void createInMemory();
    /* Closes, then opens path; with create, or a missing file when mode is not
     * 0, creates it first (1240:016a). */
    void open(const char *path, u16 openMode, u8 create);
    void close(); /* 1240:0261 */
    /* whence is SEEK_SET, SEEK_CUR or SEEK_END (1240:02cc). */
    void seek(s32 position, u16 whence) const;
    /* Reads count bytes at position, or at the current position for -1, in
     * pieces of up to FFFAh bytes, moving the buffer's selector base along
     * (1240:0312). */
    s32 read(void *buffer, s32 count, s32 position = -1L) const;
    s32 write(void *buffer, s32 count, s32 position = -1L) const; /* 1240:0434 */
    [[nodiscard]] s32 size() const;                               /* 1240:0556 */
    /* The DOS time of the last write (1240:05d4). */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u16 time() const;
    /* INT 21h AH=68h: writes the buffers out (1240:0614). */
    void commit() const;
    /* UNCALLED: nothing calls it. */
    void makeWritable() const; /* 1240:0652 */
    /* UNCALLED: nothing calls it. */
    void makeReadOnly() const;      /* 1240:0685 */
    [[nodiscard]] s32 tell() const; /* 1240:06b8 */
    /* Copies count bytes from position from of source (this when 0) to
     * position to of this file, backwards when the ranges overlap; -1 is the
     * current position (1240:06fc). */
    void copy(s32 from, s32 to, s32 count, File *source);
    /* Copies all of source to the start of this file. */
    /* UNCALLED: nothing calls it. */
    void copyAll(File *source) { copy(0L, 0L, source->size(), source); }
    /* Cuts or extends the file to size, keeping the position if it is still
     * inside (1240:0909). */
    void setSize(s32 size) const;
    /* Ends the file at the current position (1240:0962). */
    void commitSize() const;
    [[nodiscard]] u8 atEnd() const; /* 1240:0983 */
    /* INT 21h AH=5Ch: 1 when the region could be locked or unlocked
     * (1240:09ea, 0a4d). */
    [[nodiscard]] bool lock(s32 offset, s32 length) const;
    [[nodiscard]] bool unlock(s32 offset, s32 length) const;
    /* Renames the closed file (1240:0ab0). */
    void rename(const char *newName);
    /* Writes a copy of the open file to path (1240:0b17). */
    /* UNCALLED: nothing calls it. */
    void copyTo(const char *path);
    /* Reads a line of up to size bytes at position (-1: here), ending it at
     * CR or ^Z, and leaves the position after it (1240:0bc4). */
    void readLine(char *buffer, s32 size, s32 position) const;
};
ASSERT_SIZE(File, 0x8);

/* Locks a region of a file for its lifetime, trying retries times before it
 * halts. */
class FileLock {
public:
    File *file;
    s32 length;
    s32 offset;
    u16 retries;

    /* UNCALLED: nothing calls it. */
    FileLock(File *lockedFile, s32 lockLength, s32 lockOffset, u16 retryCount); /* 1240:0ce4 */
    /* UNCALLED: nothing calls it. */
    ~FileLock(); /* 1240:0db7 */
};
ASSERT_SIZE(FileLock, 0xe);

/* A file opened shared for reading and writing that holds a lock on its
 * first byte while it is open (1240:0e36). */
class LockedFile : public File {
public:
    /* UNCALLED: nothing calls it. */
    explicit LockedFile(const char *path);
};
ASSERT_SIZE(LockedFile, 0x8);
#endif
