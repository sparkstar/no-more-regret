/* The game's files (files.h) on the host's: its descriptors and errors,
 * the game's paths (hostpath.h), the searches of Borland's findfirst in
 * the 8.3 names DOS sees, and the free space. port/test's file_test and
 * files_test check them. */
#include "files.h"
#include "gamememory.h"
#include "filestate.h"
#include "hostpath.h"
#include "iso9660.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

fs::path writablePath(const char *dosPath, bool keep);
/* A write of dosPath at host coming: the file as it was kept when it is one
 * of the player's and they are kept (portKeepPlayersFiles). */
void beforePlayersWrite(const char *dosPath, const fs::path &host);
/* The CD's image the game's drives other than C are, none when the CD is a
 * directory; the part of dosPath in it, none when dosPath is not on it. */
IsoImage *cdImage();
std::optional<std::string> imageMember(const char *dosPath);

FileResult failed(int error)
{
    return {error, 0};
}

FileResult done(std::uint32_t value)
{
    return {0, value};
}

/* The descriptors the game has open, each plus 1 (0 is a free place).
 * Plain memory: 0 before any static constructor runs and never destroyed,
 * as global objects open and close files when they are made and at the
 * exit. */
int openFiles[FILES_OPEN];
/* What each open file is, by its handle, for a snapshot to open it again
 * (pcstate.h): its host's path and the flags without O_CREAT and O_TRUNC. */
std::map<int, std::pair<fs::path, int>> openedAs;
/* The open files of the CD's image (iso9660.h), by handle (a descriptor of
 * the image): the member, where its extent starts in the image, its size
 * and its DOS time and date. */
struct InImage {
    std::string member;
    std::int64_t base;
    std::uint32_t size;
    std::uint16_t time;
    std::uint16_t date;
};
std::map<int, InImage> inImage;
/* The files in the host's memory (fileCreateInMemory), by handle: their
 * bytes and the position. Their handles are above the host's descriptors. */
struct InMemory {
    std::vector<std::uint8_t> bytes;
    std::uint32_t at = 0;
};
std::map<int, InMemory> inMemory;
const int MEMORY_HANDLES = 0x40000000;
int nextMemoryHandle = MEMORY_HANDLES;

constexpr std::uint16_t DOS_FIRST_DATE = (1U << 5) | 1U;
constexpr std::uint16_t DOS_LAST_DATE = (127U << 9) | (12U << 5) | 31U;
constexpr std::uint16_t DOS_LAST_TIME = (23U << 11) | (59U << 5) | 29U;

/* A missing or unrepresentable date is the first DOS date. Clamp dates
 * outside 1980-2107 instead of wrapping the seven-bit year. Check a wider
 * UTC range before narrowing clock durations or calling localtime, leaving
 * room for the local time zone at both ends. */
void dosDateTime(std::chrono::sys_seconds when, std::uint16_t &time, std::uint16_t &date)
{
    time = 0;
    date = DOS_FIRST_DATE;
    if (when < std::chrono::sys_days{std::chrono::year{1979} / 1 / 1}) {
        return;
    }
    if (when > std::chrono::sys_days{std::chrono::year{2109} / 1 / 1}) {
        time = DOS_LAST_TIME;
        date = DOS_LAST_DATE;
        return;
    }
    const auto system = std::chrono::time_point_cast<std::chrono::system_clock::duration>(when);
    const std::time_t t = std::chrono::system_clock::to_time_t(system);
    const std::tm *local = std::localtime(&t);
    if (local == nullptr || local->tm_year < 80) {
        return;
    }
    if (local->tm_year > 207) {
        time = DOS_LAST_TIME;
        date = DOS_LAST_DATE;
        return;
    }
    time = static_cast<std::uint16_t>(local->tm_hour << 11 | local->tm_min << 5 | (local->tm_sec / 2));
    date = static_cast<std::uint16_t>((local->tm_year - 80) << 9 | (local->tm_mon + 1) << 5 | local->tm_mday);
}

#ifdef _WIN32
/* FILETIME's 1601 epoch and 100 ns ticks, without narrowing to file_clock's
 * nanoseconds or the CRT's restricted file timestamp conversion. */
std::chrono::sys_seconds fileTimeSeconds(const FILETIME &when)
{
    const std::uint64_t ticks = static_cast<std::uint64_t>(when.dwHighDateTime) << 32 | when.dwLowDateTime;
    const std::int64_t seconds = static_cast<std::int64_t>(ticks / 10000000U) - 11644473600LL;
    return std::chrono::sys_seconds{std::chrono::seconds{seconds}};
}
#endif

/* The place of file among them, nullptr when the game did not open it. */
int *placeOf(int file)
{
    for (int &place : openFiles) {
        if (place == file + 1) {
            return &place;
        }
    }
    return nullptr;
}

/* An error unless file is one the game opened. */
int checkFile(int file)
{
    return file >= 0 && (placeOf(file) != nullptr) ? 0 : EBADF;
}

/* The host's open; Windows takes the path in wide characters, and its
 * files are text unless they are opened binary. */
int openHost(const fs::path &host, int flags, mode_t permissions)
{
#ifdef _WIN32
    return ::_wopen(host.c_str(), flags | _O_BINARY, permissions);
#else
    return ::open(host.c_str(), flags, permissions);
#endif
}

/* A file of the CD's image: a descriptor of the image at the member's
 * extent. The image is read only. */
FileResult openInImage(const std::string &member, bool writing, int *place)
{
    IsoImage *image = cdImage();
    if (writing) {
        return failed(EACCES);
    }
    const std::optional<IsoEntry> entry = image->find(member);
    if (!entry) {
        return failed(ENOENT);
    }
    if (entry->directory) {
        return failed(EISDIR);
    }
    const int descriptor = openHost(image->path(), O_RDONLY, 0);
    if (descriptor < 0) {
        return failed(errno);
    }
    const std::int64_t base = static_cast<std::int64_t>(entry->sector) * ISO_SECTOR;
    ::lseek(descriptor, static_cast<off_t>(base), SEEK_SET);
    *place = descriptor + 1;
    openedAs[descriptor] = {image->path(), O_RDONLY};
    inImage[descriptor] = {member, base, entry->size, entry->dosTime, entry->dosDate};
    return done(static_cast<std::uint32_t>(descriptor));
}

/* Opens path with the host's flags. */
FileResult openFile(const char *path, int flags, mode_t permissions)
{
    int *place = placeOf(-1);
    if (place == nullptr) {
        return failed(EMFILE);
    }
    if (const std::optional<std::string> member = imageMember(path)) {
        return openInImage(*member, (flags & (O_WRONLY | O_RDWR)) != 0, place);
    }
    const bool writing = (flags & (O_WRONLY | O_RDWR)) != 0;
    const fs::path host = writing ? writablePath(path, (flags & O_TRUNC) == 0) : portHostPath(path);
    if (writing) {
        beforePlayersWrite(path, host);
    }
    std::error_code error;
    if (fs::is_directory(host, error)) {
        return failed(EISDIR);
    }
    const int descriptor = openHost(host, flags, permissions);
    if (descriptor < 0) {
        return failed(errno);
    }
    *place = descriptor + 1;
    openedAs[descriptor] = {host, flags & ~(O_CREAT | O_TRUNC)};
    return done(static_cast<std::uint32_t>(descriptor));
}

}

FileResult fileCreate(const char *path)
{
    const SystemSide systemSide;
    return openFile(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
}

FileResult fileCreateInMemory()
{
    const SystemSide systemSide;
    int *place = placeOf(-1);
    if (place == nullptr) {
        return failed(EMFILE);
    }
    const int file = nextMemoryHandle++;
    *place = file + 1;
    inMemory[file] = InMemory{};
    return done(static_cast<std::uint32_t>(file));
}

std::vector<std::uint8_t> fileMemoryBytes(int file)
{
    const SystemSide systemSide;
    const auto in = inMemory.find(file);
    return in != inMemory.end() ? in->second.bytes : std::vector<std::uint8_t>{};
}

FileResult fileOpen(const char *path, bool writing)
{
    const SystemSide systemSide;
    return openFile(path, writing ? O_RDWR : O_RDONLY, 0);
}

FileResult fileClose(int file)
{
    const SystemSide systemSide;
    if (const int error = checkFile(file)) {
        return failed(error);
    }
    *placeOf(file) = 0;
    if (inMemory.erase(file) != 0) {
        return done(0);
    }
    openedAs.erase(file);
    inImage.erase(file);
    ::close(file);
    return done(0);
}

FileResult fileRead(int file, void *buffer, std::uint32_t count)
{
    const SystemSide systemSide;
    if (const int error = checkFile(file)) {
        return failed(error);
    }
    if (const auto in = inMemory.find(file); in != inMemory.end()) {
        InMemory &memory = in->second;
        const std::uint32_t left =
            memory.at < memory.bytes.size() ? static_cast<std::uint32_t>(memory.bytes.size()) - memory.at : 0;
        const std::uint32_t got = std::min(count, left);
        std::copy_n(memory.bytes.begin() + memory.at, got, static_cast<std::uint8_t *>(buffer));
        memory.at += got;
        return done(got);
    }
    /* A file of the CD's image ends where its extent does. */
    if (const auto in = inImage.find(file); in != inImage.end()) {
        const std::int64_t at = ::lseek(file, 0, SEEK_CUR);
        const std::int64_t end = in->second.base + in->second.size;
        count = at >= end ? 0 : static_cast<std::uint32_t>(std::min<std::int64_t>(count, end - at));
    }
    std::uint32_t total = 0;
    while (total < count) {
        const ssize_t got = ::read(file, static_cast<std::uint8_t *>(buffer) + total, count - total);
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            return failed(errno);
        }
        if (got == 0) {
            break;
        }
        total += static_cast<std::uint32_t>(got);
    }
    return done(total);
}

FileResult fileWrite(int file, const void *buffer, std::uint32_t count)
{
    const SystemSide systemSide;
    if (const int error = checkFile(file)) {
        return failed(error);
    }
    if (const auto in = inMemory.find(file); in != inMemory.end()) {
        InMemory &memory = in->second;
        if (memory.bytes.size() < static_cast<std::size_t>(memory.at) + count) {
            memory.bytes.resize(static_cast<std::size_t>(memory.at) + count);
        }
        std::copy_n(static_cast<const std::uint8_t *>(buffer), count, memory.bytes.begin() + memory.at);
        memory.at += count;
        return done(count);
    }
    std::uint32_t total = 0;
    while (total < count) {
        const ssize_t put = ::write(file, static_cast<const std::uint8_t *>(buffer) + total, count - total);
        if (put < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOSPC) {
                break; /* what fits, as DOS writes it */
            }
            return failed(errno);
        }
        if (put == 0) {
            break;
        }
        total += static_cast<std::uint32_t>(put);
    }
    return done(total);
}

FileResult fileSeek(int file, std::int32_t position, int whence)
{
    const SystemSide systemSide;
    if (const int error = checkFile(file)) {
        return failed(error);
    }
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
        return failed(EINVAL);
    }
    if (const auto in = inMemory.find(file); in != inMemory.end()) {
        InMemory &memory = in->second;
        std::int64_t from = 0;
        if (whence == SEEK_CUR) {
            from = memory.at;
        } else if (whence == SEEK_END) {
            from = static_cast<std::int64_t>(memory.bytes.size());
        }
        const std::int64_t target = from + position;
        if (target < 0) {
            return failed(EINVAL);
        }
        memory.at = static_cast<std::uint32_t>(target);
        return done(memory.at);
    }
    /* A file of the CD's image: its places are from its extent's start. */
    if (const auto in = inImage.find(file); in != inImage.end()) {
        const std::int64_t base = in->second.base;
        std::int64_t from = base;
        if (whence == SEEK_CUR) {
            from = ::lseek(file, 0, SEEK_CUR);
        } else if (whence == SEEK_END) {
            from = base + in->second.size;
        }
        const std::int64_t target = from + position;
        if (target < base) {
            return failed(EINVAL);
        }
        if (::lseek(file, static_cast<off_t>(target), SEEK_SET) < 0) {
            return failed(errno);
        }
        return done(static_cast<std::uint32_t>(target - base));
    }
    const off_t at = ::lseek(file, position, whence);
    if (at < 0) {
        return failed(errno);
    }
    return done(static_cast<std::uint32_t>(at));
}

FileResult fileTime(int file)
{
    const SystemSide systemSide;
    if (const int error = checkFile(file)) {
        return failed(error);
    }
    if (inMemory.count(file) != 0) {
        return done(static_cast<std::uint32_t>(DOS_FIRST_DATE) << 16);
    }
    if (const auto in = inImage.find(file); in != inImage.end()) {
        return done(static_cast<std::uint32_t>(in->second.date) << 16 | in->second.time);
    }
#ifdef _WIN32
    FILETIME modified;
    const HANDLE hostFile = reinterpret_cast<HANDLE>(::_get_osfhandle(file));
    if (::GetFileTime(hostFile, nullptr, nullptr, &modified) == 0) {
        return failed(EIO);
    }
    const auto when = fileTimeSeconds(modified);
#else
    struct stat status;
    if (::fstat(file, &status) < 0) {
        return failed(errno);
    }
    const std::chrono::sys_seconds when{std::chrono::seconds{status.st_mtime}};
#endif
    std::uint16_t time;
    std::uint16_t date;
    dosDateTime(when, time, date);
    return done(static_cast<std::uint32_t>(date) << 16 | time);
}

/* The host's writes are in the file already; their going to the disk is
 * the host's business. */
FileResult fileCommit(int file)
{
    const SystemSide systemSide;
    return failed(checkFile(file));
}

/* The one program's locks on its own files never conflict. */
FileResult fileLock(int file, bool /*unused*/, std::int32_t /*unused*/, std::int32_t /*unused*/)
{
    const SystemSide systemSide;
    return failed(checkFile(file));
}

int fileSetSize(int file, std::int32_t size)
{
    const SystemSide systemSide;
    if (checkFile(file) != 0 || size < 0) {
        return -1;
    }
    if (const auto in = inMemory.find(file); in != inMemory.end()) {
        in->second.bytes.resize(static_cast<std::size_t>(size));
        return 0;
    }
    return ::ftruncate(file, size) == 0 ? 0 : -1;
}

namespace {

/* The replay's sandbox (portSandbox), none outside a replay. */
fs::path sandbox;

bool inSandbox(const fs::path &path)
{
    const std::string mine = sandbox.lexically_normal().string();
    const std::string theirs = path.lexically_normal().string();
    return theirs.compare(0, mine.size(), mine) == 0;
}

void copyKeepingTime(const fs::path &from, const fs::path &to, std::error_code &error)
{
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, error);
    if (!error) {
        fs::last_write_time(to, fs::last_write_time(from, error), error);
    }
}

/* A host file readable, and writable or not; Windows takes the path in wide
 * characters and has no others' permissions. */
int hostPermissions(const fs::path &path, bool writable)
{
#ifdef _WIN32
    return ::_wchmod(path.c_str(), writable ? _S_IREAD | _S_IWRITE : _S_IREAD);
#else
    return ::chmod(path.c_str(), writable ? S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH : S_IRUSR | S_IRGRP | S_IROTH);
#endif
}

}

int fileSetReadOnly(const char *path, bool readOnly)
{
    const SystemSide systemSide;
    if (imageMember(path)) {
        return -1;
    }
    return hostPermissions(writablePath(path, true), !readOnly) == 0 ? 0 : -1;
}

int fileRemove(const char *path)
{
    const SystemSide systemSide;
    if (imageMember(path)) {
        return -1;
    }
    std::error_code error;
    const fs::path host = writablePath(path, false);
    beforePlayersWrite(path, host);
    return fs::is_regular_file(host, error) && fs::remove(host, error) ? 0 : -1;
}

/* In a sandbox a file of the current directory is copied, not moved. */
int fileRename(const char *from, const char *to)
{
    const SystemSide systemSide;
    if (imageMember(from) || imageMember(to)) {
        return -1;
    }
    std::error_code error;
    const fs::path source = portHostPath(from);
    const fs::path target = writablePath(to, false);
    if (!fs::exists(source, error) || fs::exists(target, error)) {
        return -1;
    }
    beforePlayersWrite(to, target);
    beforePlayersWrite(from, source);
    if (!sandbox.empty() && !inSandbox(source)) {
        copyKeepingTime(source, target, error);
    } else {
        fs::rename(source, target, error);
    }
    return error ? -1 : 0;
}

std::FILE *fileStream(const char *path, const char *mode)
{
    const SystemSide systemSide;
    const bool writes = std::strpbrk(mode, "wa+") != nullptr;
    const bool keeps = std::strpbrk(mode, "ra") != nullptr;
    const fs::path host = writes ? writablePath(path, keeps) : portHostPath(path);
    if (writes) {
        beforePlayersWrite(path, host);
    }
    return portHostFopen(host, mode);
}

std::size_t fileStreamRead(std::FILE *stream, void *buffer, std::size_t size, std::size_t count)
{
    const SystemSide systemSide;
    return std::fread(buffer, size, count, stream);
}

std::size_t fileStreamWrite(std::FILE *stream, const void *buffer, std::size_t size, std::size_t count)
{
    const SystemSide systemSide;
    return std::fwrite(buffer, size, count, stream);
}

char *fileStreamLine(std::FILE *stream, char *buffer, int size)
{
    const SystemSide systemSide;
    return std::fgets(buffer, size, stream);
}

int fileStreamPrint(std::FILE *stream, const char *format, ...)
{
    const SystemSide systemSide;
    va_list arguments;
    va_start(arguments, format);
    const int length = std::vfprintf(stream, format, arguments);
    va_end(arguments);
    return length;
}

int fileStreamClose(std::FILE *stream)
{
    const SystemSide systemSide;
    return std::fclose(stream);
}

std::vector<std::uint8_t> fileContents(const char *path)
{
    const SystemSide systemSide;
    std::vector<std::uint8_t> bytes;
    std::FILE *file = portHostFopen(portHostPath(path), "rb");
    if (file == nullptr) {
        return bytes;
    }
    std::uint8_t buffer[4096];
    std::size_t count = 0;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        bytes.insert(bytes.end(), buffer, buffer + count);
    }
    std::fclose(file);
    return bytes;
}

std::FILE *portHostFopen(const fs::path &path, const char *mode)
{
#ifdef _WIN32
    std::wstring wide(mode, mode + std::strlen(mode));
    return ::_wfopen(path.c_str(), wide.c_str());
#else
    return std::fopen(path.c_str(), mode);
#endif
}

namespace {

bool same(const std::string &a, const std::string &b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t k = 0; k < a.size(); k++) {
        if (std::toupper(static_cast<unsigned char>(a[k])) != std::toupper(static_cast<unsigned char>(b[k]))) {
            return false;
        }
    }
    return true;
}

/* A name in the 11 characters of a DOS directory entry: 8 of the name and
 * 3 of the extension, space-padded, capitals; false when the name has no
 * 8.3 form (DOS does not see it). A pattern's * fills its part with ?. */
bool fcb(const std::string &name, char out[11], bool pattern)
{
    std::memset(out, ' ', 11);
    if (name == "." || name == "..") {
        std::memcpy(out, name.data(), name.size());
        return true;
    }
    const size_t dot = name.rfind('.');
    const std::string base = dot == std::string::npos ? name : name.substr(0, dot);
    const std::string ext = dot == std::string::npos ? "" : name.substr(dot + 1);
    if ((!pattern && (base.size() > 8 || ext.size() > 3)) || base.find('.') != std::string::npos) {
        return false;
    }
    auto fill = [&](const std::string &part, char *to, size_t size) {
        for (size_t k = 0, at = 0; k < part.size() && at < size; k++) {
            if (pattern && part[k] == '*') {
                while (at < size) {
                    to[at++] = '?';
                }
                return;
            }
            to[at++] = static_cast<char>(std::toupper(static_cast<unsigned char>(part[k])));
        }
    };
    fill(base, out, 8);
    fill(ext, out + 8, 3);
    return true;
}

bool matches(const char pattern[11], const char name[11])
{
    for (int k = 0; k < 11; k++) {
        if (pattern[k] != '?' && pattern[k] != name[k]) {
            return false;
        }
    }
    return true;
}

struct Found {
    std::string name;
    char attributes;
    std::uint16_t time = 0;
    std::uint16_t date = DOS_FIRST_DATE;
    std::int32_t size;
};

/* The searches under way, by the number fileFindFirst keeps in reserved. */
std::map<std::uint32_t, std::pair<std::vector<Found>, size_t>> searches;
std::uint32_t lastSearch = 0;

void fill(FileFind *block, const Found &found)
{
    block->attributes = found.attributes;
    block->time = found.time;
    block->date = found.date;
    block->size = found.size;
    std::memset(block->name, 0, sizeof(block->name));
    std::memcpy(block->name, found.name.data(), std::min<size_t>(found.name.size(), 12));
}

/* A search found its entries: the first in block, the rest kept for
 * fileFindNext; -1 when there are none. */
int startSearch(std::vector<Found> found, FileFind *block)
{
    if (found.empty()) {
        return -1;
    }
    std::uint32_t id = ++lastSearch;
    std::memcpy(block->reserved, &id, sizeof(id));
    fill(block, found[0]);
    searches[id] = {std::move(found), 1};
    return 0;
}

/* A search of a directory of the CD's image. */
int findInImage(const std::string &directory, const char wanted[11], int attributes, FileFind *block)
{
    const std::optional<std::vector<IsoEntry>> entries = cdImage()->list(directory);
    std::vector<Found> found;
    for (const IsoEntry &entry : entries.value_or(std::vector<IsoEntry>())) {
        char have[11];
        if (!fcb(entry.name, have, false) || !matches(wanted, have)) {
            continue;
        }
        if (entry.directory && ((attributes & FILE_DIRECTORY) == 0)) {
            continue;
        }
        Found f;
        f.name = entry.name;
        for (char &c : f.name) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        f.attributes = entry.directory ? FILE_DIRECTORY : FILE_ARCHIVE;
        f.size = entry.directory ? 0 : static_cast<std::int32_t>(entry.size);
        f.time = entry.dosTime;
        f.date = entry.dosDate;
        found.push_back(f);
    }
    return startSearch(std::move(found), block);
}

/* The directory of drive (a letter, 0 for the current drive). */
fs::path driveRoot(char drive)
{
    if (drive == 0 || std::toupper(static_cast<unsigned char>(drive)) == 'A' + PORT_DRIVE - 1) {
        return fs::current_path();
    }
    const char *cd = std::getenv("CRUSADER_CD");
    return (cd != nullptr) && (*cd != 0) ? fs::path(cd) : fs::current_path();
}

}

namespace {

/* A path split into its drive (0 for none) and the rest. */
struct DosPath {
    char drive;
    std::string rest;
};

DosPath splitDrive(const char *dosPath)
{
    std::string path = dosPath;
    char drive = 0;
    if (path.size() >= 2 && path[1] == ':') {
        drive = path[0];
        path.erase(0, 2);
    }
    return {drive, path};
}

bool onDriveC(char drive)
{
    return drive == 0 || std::toupper(static_cast<unsigned char>(drive)) == 'A' + PORT_DRIVE - 1;
}

/* The player's own files (portSandbox): under savegame\, crusader.cfg and
 * the port's enhanced.cfg. */
bool playersOwn(const std::string &rest)
{
    const size_t start = rest.find_first_not_of("\\/");
    if (start == std::string::npos) {
        return false;
    }
    const size_t end = rest.find_first_of("\\/", start);
    const std::string first = rest.substr(start, end == std::string::npos ? std::string::npos : end - start);
    return same(first, "savegame") ||
           (end == std::string::npos && (same(first, "crusader.cfg") || same(first, "enhanced.cfg")));
}

fs::path resolve(const fs::path &root, const std::string &path);

/* The images a game's folder may hold, in the order they are looked for:
 * GOG's, Steam's (the same image by another name). */
const char *const FOLDER_IMAGES[] = {"GAME.GOG", "GAME.BIN"};

/* The CD's image: the file CRUSADER_CD names, else the current directory's
 * GAME.GOG or GAME.BIN (the game's folder, a GOG or Steam install:
 * gamefolder.h); found again when either changes. */
IsoImage *cdImage()
{
    static std::string key;
    static std::optional<IsoImage> image;
    std::error_code error;
    const char *cd = std::getenv("CRUSADER_CD");
    const std::string asked = (cd != nullptr) ? cd : "";
    const fs::path current = fs::current_path(error);
    const std::string now = asked + '\n' + current.string();
    if (now != key) {
        key = now;
        image.reset();
        fs::path source = asked;
        if (asked.empty()) {
            for (const char *name : FOLDER_IMAGES) {
                source = resolve(current, name);
                if (fs::is_regular_file(source, error)) {
                    break;
                }
            }
        }
        if (fs::is_regular_file(source, error)) {
            image = IsoImage::open(source);
        }
    }
    return image ? &*image : nullptr;
}

std::optional<std::string> imageMember(const char *dosPath)
{
    const DosPath path = splitDrive(dosPath);
    if (onDriveC(path.drive) || cdImage() == nullptr) {
        return std::nullopt;
    }
    return path.rest;
}

/* The host path of rest under root: each part the entry of any case. */
fs::path resolve(const fs::path &root, const std::string &path)
{
    fs::path out = root;
    size_t start = 0;
    while (start <= path.size()) {
        size_t end = path.find_first_of("\\/", start);
        if (end == std::string::npos) {
            end = path.size();
        }
        const std::string part = path.substr(start, end - start);
        start = end + 1;
        if (part.empty() || part == ".") {
            continue;
        }
        if (part == "..") {
            out = out.parent_path();
            continue;
        }
        fs::path next = out / part;
        std::error_code error;
        if (!fs::exists(next, error) && fs::is_directory(out, error)) {
            for (const fs::directory_entry &entry : fs::directory_iterator(out, error)) {
                if (same(entry.path().filename().string(), part)) {
                    next = entry.path();
                    break;
                }
            }
        }
        out = next;
    }
    return out;
}

/* A path the game writes: in a sandbox there, after a copy of the current
 * directory's file when the writes keep its bytes. */
fs::path writablePath(const char *dosPath, bool keep)
{
    const DosPath path = splitDrive(dosPath);
    if (sandbox.empty() || !onDriveC(path.drive)) {
        return resolve(driveRoot(path.drive), path.rest);
    }
    fs::path mine = resolve(sandbox, path.rest);
    std::error_code error;
    if (!fs::exists(mine, error) && !playersOwn(path.rest)) {
        const fs::path theirs = resolve(fs::current_path(), path.rest);
        fs::create_directories(mine.parent_path(), error);
        if (keep && fs::is_regular_file(theirs, error)) {
            copyKeepingTime(theirs, mine, error);
        }
    }
    return mine;
}

}

namespace {

/* The player's files kept (portKeepPlayersFiles), and their writes since
 * the last take, each file as it was before. */
bool keepingPlayers = false;
std::vector<PlayerFile> playersWrites;

/* A host file's bytes, none for no file. */
std::optional<std::vector<char>> bytesOf(const fs::path &path)
{
    std::error_code error;
    if (!fs::is_regular_file(path, error)) {
        return std::nullopt;
    }
    std::ifstream in(path, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void beforePlayersWrite(const char *dosPath, const fs::path &host)
{
    const DosPath path = splitDrive(dosPath);
    if (keepingPlayers && onDriveC(path.drive) && playersOwn(path.rest)) {
        playersWrites.push_back({host, bytesOf(host)});
    }
}

/* A file made as it was: its bytes written, or it removed. */
void putPlayerFile(const PlayerFile &file)
{
    std::error_code error;
    if (!file.bytes) {
        fs::remove(file.path, error);
        return;
    }
    fs::create_directories(file.path.parent_path(), error);
    std::ofstream out(file.path, std::ios::binary | std::ios::trunc);
    out.write(file.bytes->data(), static_cast<std::streamsize>(file.bytes->size()));
}

}

void portKeepPlayersFiles(bool keep)
{
    keepingPlayers = keep;
    playersWrites.clear();
}

std::vector<PlayerFile> portPlayersWrites()
{
    std::vector<PlayerFile> taken;
    taken.swap(playersWrites);
    return taken;
}

void portPutPlayersFiles(const std::vector<PlayerFile> &files)
{
    for (auto file = files.rbegin(); file != files.rend(); ++file) {
        putPlayerFile(*file);
    }
}

std::vector<PlayerFile> portPlayersFiles()
{
    const fs::path root = sandbox.empty() ? fs::current_path() : sandbox;
    std::vector<PlayerFile> files;
    std::error_code error;
    const fs::path saves = resolve(root, "savegame");
    if (fs::is_directory(saves, error)) {
        for (const fs::directory_entry &entry : fs::recursive_directory_iterator(saves, error)) {
            if (entry.is_regular_file(error)) {
                files.push_back({entry.path(), bytesOf(entry.path())});
            }
        }
    }
    for (const char *name : {"crusader.cfg", "enhanced.cfg"}) {
        const fs::path file = resolve(root, name);
        if (fs::is_regular_file(file, error)) {
            files.push_back({file, bytesOf(file)});
        }
    }
    std::sort(files.begin(), files.end(), [](const PlayerFile &a, const PlayerFile &b) { return a.path < b.path; });
    return files;
}

void portSetPlayersFiles(const std::vector<PlayerFile> &files)
{
    for (const PlayerFile &now : portPlayersFiles()) {
        if (std::none_of(files.begin(), files.end(),
                         [&now](const PlayerFile &kept) { return kept.path == now.path; })) {
            putPlayerFile({now.path, std::nullopt});
        }
    }
    for (const PlayerFile &file : files) {
        putPlayerFile(file);
    }
}

fs::path portHostPath(const char *dosPath)
{
    const DosPath path = splitDrive(dosPath);
    if (sandbox.empty() || !onDriveC(path.drive)) {
        return resolve(driveRoot(path.drive), path.rest);
    }
    fs::path mine = resolve(sandbox, path.rest);
    std::error_code error;
    if (fs::exists(mine, error) || playersOwn(path.rest)) {
        return mine;
    }
    return resolve(fs::current_path(), path.rest);
}

void portSandbox(const fs::path &directory)
{
    sandbox = directory;
}

int fileFindFirst(const char *path, FileFind *block, int attributes)
{
    const SystemSide systemSide;
    const std::string text = path;
    const size_t cut = text.find_last_of("\\/:");
    const std::string directory = cut == std::string::npos ? "" : text.substr(0, cut + 1);
    const std::string pattern = cut == std::string::npos ? text : text.substr(cut + 1);
    char wanted[11];
    fcb(pattern, wanted, true);
    /* In a sandbox its directory and the current directory's, the one
     * name once, but the player's files only from the sandbox. */
    const DosPath place = splitDrive(directory.c_str());
    if (const std::optional<std::string> member = imageMember(directory.c_str())) {
        return findInImage(*member, wanted, attributes, block);
    }
    const bool merged = !sandbox.empty() && onDriveC(place.drive);
    std::vector<fs::path> wheres;
    if (merged) {
        wheres.push_back(resolve(sandbox, place.rest));
        wheres.push_back(resolve(fs::current_path(), place.rest));
    } else {
        wheres.push_back(portHostPath(directory.c_str()));
    }
    std::vector<Found> found;
    std::error_code error;
    for (size_t where = 0; where < wheres.size(); where++) {
        for (const fs::directory_entry &entry : fs::directory_iterator(wheres[where], error)) {
            std::string name = entry.path().filename().string();
            char have[11];
            if (!fcb(name, have, false) || !matches(wanted, have)) {
                continue;
            }
            const bool isDirectory = entry.is_directory(error);
            if (isDirectory && ((attributes & FILE_DIRECTORY) == 0)) {
                continue;
            }
            for (char &c : name) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            if (where > 0 &&
                (playersOwn(place.rest + name) ||
                 std::any_of(found.begin(), found.end(), [&](const Found &f) { return f.name == name; }))) {
                continue;
            }
            Found f;
            f.name = name;
            f.attributes = isDirectory ? FILE_DIRECTORY : FILE_ARCHIVE;
            f.size = isDirectory ? 0 : static_cast<std::int32_t>(entry.file_size(error));
            /* Read the raw timestamp as fileTime does: file_clock cannot
             * represent 1601, and the Windows CRT can lose old file dates. */
#ifdef _WIN32
            WIN32_FILE_ATTRIBUTE_DATA status;
            if (::GetFileAttributesExW(entry.path().c_str(), GetFileExInfoStandard, &status) != 0) {
                dosDateTime(fileTimeSeconds(status.ftLastWriteTime), f.time, f.date);
            }
#else
            struct stat status;
            if (::stat(entry.path().c_str(), &status) == 0) {
                dosDateTime(std::chrono::sys_seconds{std::chrono::seconds{status.st_mtime}}, f.time, f.date);
            }
#endif
            found.push_back(f);
        }
    }
    return startSearch(std::move(found), block);
}

int fileFindNext(FileFind *block)
{
    const SystemSide systemSide;
    std::uint32_t id;
    std::memcpy(&id, block->reserved, sizeof(id));
    auto search = searches.find(id);
    if (search == searches.end()) {
        return -1;
    }
    auto &[found, next] = search->second;
    if (next >= found.size()) {
        searches.erase(search);
        return -1;
    }
    fill(block, found[next++]);
    return 0;
}

char *fileCurrentDirectory(char *buffer, int length)
{
    const SystemSide systemSide;
    if (length < 4) {
        return nullptr;
    }
    std::strcpy(buffer, "C:\\");
    return buffer;
}

void fileFreeSpace(std::uint8_t drive, FileSpace *table)
{
    const SystemSide systemSide;
    std::error_code error;
    const fs::space_info space = fs::space(fs::current_path(), error);
    if ((drive != 0 && drive != PORT_DRIVE) || error) {
        table->sectorsPerCluster = 0xffff;
        return;
    }
    /* Clusters of 32K, as many as 16 bits count. */
    table->bytesPerSector = 512;
    table->sectorsPerCluster = 64;
    table->available = static_cast<std::uint16_t>(std::min<std::uintmax_t>(space.available / 0x8000, 0xffff));
    table->total = static_cast<std::uint16_t>(std::min<std::uintmax_t>(space.capacity / 0x8000, 0xffff));
}

namespace {

/* A snapshot's files that cannot be put back: the run's end (as report.h's
 * portEnd, which the tests that link this file alone do not have). */
[[noreturn]] void snapshotFailure(const char *what, const fs::path &path)
{
    std::fflush(stdout);
    std::fprintf(stderr, "snapshot: %s %s\n", what, path.string().c_str());
    std::fflush(stderr);
    std::_Exit(3);
}

}

FilesState filesState()
{
    FilesState state;
    for (const auto &[file, opened] : openedAs) {
        const std::int64_t place = ::lseek(file, 0, SEEK_CUR);
        const auto in = inImage.find(file);
        state.open[file] = {opened.first, opened.second, place, in != inImage.end() ? in->second.member : ""};
    }
    return state;
}

void putFilesState(const FilesState &state)
{
    /* The files opened since, or another file in a handle now, closed. */
    for (auto it = openedAs.begin(); it != openedAs.end();) {
        const auto kept = state.open.find(it->first);
        const auto in = inImage.find(it->first);
        const std::string member = in != inImage.end() ? in->second.member : "";
        if (kept == state.open.end() || kept->second.host != it->second.first || kept->second.member != member) {
            *placeOf(it->first) = 0;
            ::close(it->first);
            inImage.erase(it->first);
            it = openedAs.erase(it);
        } else {
            ++it;
        }
    }
    /* The files closed since opened again in their handles, and each at its
     * place. */
    for (const auto &[file, opened] : state.open) {
        if (openedAs.find(file) == openedAs.end()) {
            const int descriptor = openHost(opened.host, opened.flags, 0);
            if (descriptor < 0) {
                snapshotFailure("a file that cannot be opened again:", opened.host);
            }
            if (descriptor != file) {
                ::dup2(descriptor, file);
                ::close(descriptor);
            }
            int *place = placeOf(-1);
            if (place == nullptr) {
                snapshotFailure("no handle left for", opened.host);
            }
            *place = file + 1;
            openedAs[file] = {opened.host, opened.flags};
            if (!opened.member.empty()) {
                const std::optional<IsoEntry> entry =
                    cdImage() != nullptr ? cdImage()->find(opened.member) : std::nullopt;
                if (!entry) {
                    snapshotFailure("a file of the CD's image that is not there:", opened.member);
                }
                inImage[file] = {opened.member, static_cast<std::int64_t>(entry->sector) * ISO_SECTOR, entry->size,
                                 entry->dosTime, entry->dosDate};
            }
        }
        ::lseek(file, static_cast<off_t>(opened.place), SEEK_SET);
    }
}
