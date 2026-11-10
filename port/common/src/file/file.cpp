/* FILE\FILE.C (1240:0000-0f23): File over the platform's files (files.h),
 * in the original's order, with its halts and their lines. The original's handles and error codes are DOS's, the
 * port's the host's. The original moves a buffer's selector base along to
 * read or write more than 64K; the port moves the pointer. See file.h. */
#include "file.h"
#include "files.h"
#include "utility.h"

#include <cstdio>
#include <cstring>

namespace {
/* The error of the last failed call. */
int fileError;
}
/* Halts with the error when the call failed. */
#define CHECK_ERROR(result)                                                                                            \
    if ((result).error) {                                                                                              \
        fileError = (result).error;                                                                                    \
        haltMessage(haltFormat("File Error %s, code = %04X", name, fileError));                                        \
    }

namespace {
/* Borland's tell(handle): the position, -1 when there is none. */
s32 handlePosition(int handle)
{
    const FileResult result = fileSeek(handle, 0L, SEEK_CUR);
    return (result.error != 0) ? -1L : static_cast<s32>(result.value);
}
}

File::File()
{
    name = nullptr;
    handle = -1;
}

File::File(const char *path, u16 openMode, u8 create)
{
    name = nullptr;
    handle = -1;
    open(path, openMode, create);
}

void File::setName(const char *path)
{
    u16 length;

    if (name != nullptr) {
        delete[] name;
        name = nullptr;
    }
    if (path != nullptr) {
        length = static_cast<u16>(std::strlen(path) + 1);
        name = new char[length];
        std::memcpy(name, path, length);
    }
}

void File::create(const char *path)
{
    const FileResult result = fileCreate(path);
    if (result.error != 0) {
        fileError = result.error;
        haltMessage(haltFormat("Unable to create file.  Error #%d", fileError));
    }
    handle = static_cast<int>(result.value);
    if (handle != -1) {
        setName(path);
    }
}

void File::createInMemory()
{
    close();
    const FileResult result = fileCreateInMemory();
    if (result.error != 0) {
        fileError = result.error;
        haltMessage(haltFormat("Unable to create file.  Error #%d", fileError));
    }
    handle = static_cast<int>(result.value);
    mode = FILE_READWRITE;
    setName("memory");
}

void File::open(const char *path, u16 openMode, u8 create)
{
    if (path == nullptr) {
        haltMessage("Error: Attempting to open null file.");
    }
    if (openMode != FILE_READ && (fileExists(path) == 0)) {
        create = 1;
    }
    if (create != 0U) {
        fileRemove(path);
        this->create(path);
    }
    close();
    /* The access in the low bits of the mode (INT 21h AH=3Dh AL): the port's
     * files have no sharing. */
    const FileResult result = fileOpen(path, (openMode & 7) != FILE_READ);
    if (result.error != 0) {
        fileError = result.error;
        openFailure(path);
    }
    handle = static_cast<int>(result.value);
    this->mode = openMode;
    if (handle == -1 && (create != 0U) && (fileExists(path) == 0)) {
        this->create(path);
    }
    if (handle != -1) {
        setName(path);
    }
}

void File::close()
{
    const int closingHandle = this->handle;

    if (isOpen() != 0U) {
        const FileResult result = fileClose(closingHandle);
        CHECK_ERROR(result);
        setName(nullptr);
        this->handle = -1;
    }
}

void File::seek(s32 position, u16 whence) const
{
    const FileResult result = fileSeek(handle, position, whence);
    CHECK_ERROR(result);
}

s32 File::read(void *buffer, s32 count, s32 position) const
{
    s32 total = 0;
    u8 *window = static_cast<u8 *>(buffer);
    u16 chunk;

    if (position != -1) {
        const FileResult result = fileSeek(handle, position, SEEK_SET);
        CHECK_ERROR(result);
    }
    while (count > 0) {
        chunk = count > 0xfffaL ? 0xfffa : static_cast<u16>(count);
        const FileResult result = fileRead(handle, window, chunk);
        CHECK_ERROR(result);
        total += static_cast<u16>(result.value);
        count -= chunk;
        window += chunk;
    }
    return total;
}

s32 File::write(void *buffer, s32 count, s32 position) const
{
    s32 total = 0;
    const u8 *window = static_cast<const u8 *>(buffer);
    u16 chunk;

    if (position != -1) {
        const FileResult result = fileSeek(handle, position, SEEK_SET);
        CHECK_ERROR(result);
    }
    while (count > 0) {
        chunk = count > 0xfffaL ? 0xfffa : static_cast<u16>(count);
        const FileResult result = fileWrite(handle, window, chunk);
        CHECK_ERROR(result);
        total += static_cast<u16>(result.value);
        count -= chunk;
        window += chunk;
    }
    return total;
}

s32 File::size() const
{
    const s32 position = tell();
    s32 length;

    const FileResult result = fileSeek(handle, 0L, SEEK_END);
    CHECK_ERROR(result);
    length = static_cast<s32>(result.value);
    seek(position, SEEK_SET);
    return length;
}

/* UNCALLED: nothing calls it. */
u16 File::time() const
{
    const FileResult result = fileTime(handle);
    CHECK_ERROR(result);
    return static_cast<u16>(result.value);
}

void File::commit() const
{
    const FileResult result = fileCommit(handle);
    CHECK_ERROR(result);
}

/* Borland's _chmod */
/* UNCALLED: nothing calls it. */
void File::makeWritable() const
{
    if (fileSetReadOnly(name, false) != 0) {
        halt();
    }
}

/* UNCALLED: nothing calls it. */
void File::makeReadOnly() const
{
    if (fileSetReadOnly(name, true) != 0) {
        halt();
    }
}

s32 File::tell() const
{
    const FileResult result = fileSeek(handle, 0L, SEEK_CUR);
    CHECK_ERROR(result);
    return static_cast<s32>(result.value);
}

void File::copy(s32 from, s32 to, s32 count, File *source)
{
    s32 distance;
    bool backwards;
    u16 chunk;
    char *buffer;

    if (count == 0) {
        return;
    }
    if (source == nullptr) {
        source = this;
    }
    if (to == -1) {
        to = tell();
    }
    if (from == -1) {
        from = source->tell();
    }
    distance = to - from;
    if (distance == 0 && source == this) {
        return;
    }
    if (distance < 0) {
        distance = -distance;
    }
    backwards = false;
    chunk = count < 0xfffaL ? static_cast<u16>(count) : 0xfffa;
    if (distance < count && from < to && source == this) {
        from = from + count - chunk;
        to = to + count - chunk;
        backwards = true;
    }
    buffer = nullptr;
    while (chunk != 0) {
        buffer = new char[chunk];
        if (buffer != nullptr) {
            break;
        }
        chunk >>= 1;
    }
    if (chunk == 0) {
        allocationFailure();
    }
    while (count != 0) {
        source->seek(from, SEEK_SET);
        source->read(buffer, chunk, -1L);
        seek(to, SEEK_SET);
        write(buffer, chunk, -1L);
        count -= chunk;
        if (!backwards) {
            from += chunk;
            to += chunk;
            chunk = chunk > count ? static_cast<u16>(count) : chunk;
        } else {
            chunk = chunk > count ? static_cast<u16>(count) : chunk;
            from -= chunk;
            to -= chunk;
        }
    }
    delete[] buffer;
}

/* Borland's chsize, which fills with zeros past the end, and its tell. */
void File::setSize(s32 size) const
{
    s32 position = handlePosition(handle);

    /* chsize's result, which the original does not look at */
    fileSetSize(handle, size);
    position = position > size ? size : position;
    seek(position, SEEK_SET);
}

void File::commitSize() const
{
    setSize(handlePosition(handle));
}

u8 File::atEnd() const
{
    const s32 position = tell();
    s32 end;

    seek(0L, SEEK_END);
    end = tell();
    seek(position, SEEK_SET);
    return position >= end ? 1 : 0;
}

bool File::lock(s32 offset, s32 length) const
{
    return fileLock(handle, false, offset, length).error == 0;
}

bool File::unlock(s32 offset, s32 length) const
{
    return fileLock(handle, true, offset, length).error == 0;
}

/* The original copies the new name over the old one's allocation, past its
 * end when the new name is longer; the port makes a new one. */
void File::rename(const char *newName)
{
    if (handle == -1) {
        if (fileRename(this->name, newName) != 0) {
            haltMessage("Cannot rename file.");
        }
        setName(newName);
    } else {
        halt();
    }
}

/* UNCALLED: nothing calls it. */
void File::copyTo(const char *path)
{
    if (handle != -1) {
        if (fileExists(path) != 0) {
            fileRemove(path);
        }
        File copy;

        copy.open(path, FILE_READWRITE, 0);
        copy.copyAll(this);
        copy.commitSize();
        return;
    }
    halt();
}

void File::readLine(char *buffer, s32 size, s32 position) const
{
    s32 here = tell();
    s16 index;

    if (position != here && position != -1) {
        seek(position, SEEK_SET);
        here = position;
    }
    if ((isOpen() != 0U) && size != 0 && (read(buffer, size, -1L) != 0)) {
        for (index = 0; index < size; index++) {
            if (buffer[index] == 0x1a) {
                buffer[index] = 0;
                index++;
                break;
            }
            if (buffer[index] == '\r') {
                buffer[index] = 0;
                index += 2;
                break;
            }
        }
        if (index < size) {
            seek(here + index, SEEK_SET);
        } else {
            buffer[index - 1] = 0;
        }
    } else {
        halt();
    }
}

/* UNCALLED: nothing calls it. */
FileLock::FileLock(File *lockedFile, s32 lockLength, s32 lockOffset, u16 retryCount)
{
    this->file = lockedFile;
    this->offset = lockOffset;
    this->length = lockLength;
    this->retries = retryCount;
    u16 attempt = 0;

    if (this->file != nullptr) {
        while (!this->file->lock(this->offset, this->length)) {
            if (this->retries == attempt) {
                haltMessage(haltFormat("Access to file %s denied.", this->file->name));
            }
            attempt++;
        }
    } else {
        haltMessage("Record lock attempted on a null file.");
    }
}

/* UNCALLED: nothing calls it. */
FileLock::~FileLock()
{
    u16 attempt = 0;

    while (!file->unlock(offset, length)) {
        if (retries == attempt) {
            haltMessage(haltFormat("Could not unlock file %s.", file->name));
        }
        attempt++;
    }
}

/* UNCALLED: nothing calls it. */
LockedFile::LockedFile(const char *path)
{
    open(path, FILE_READWRITE | FILE_SHARED, 0);
    const File other(path, FILE_READWRITE | FILE_SHARED, 0);

    while (!lock(0L, 1L)) {
        ;
    }
    while (!other.lock(0L, 2L)) {
        ;
    }
    close();
    open(path, FILE_READWRITE | FILE_SHARED, 0);
    static_cast<void>(lock(0L, 1L));
    static_cast<void>(other.unlock(0L, 2L));
}
