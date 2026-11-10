/* The platform's files by their paths (files.h, port/system/files.cpp) and
 * compat's path split, which have no oracle: the original's call INT 21h. In
 * a directory made for the test, with names in both cases, a long name and a
 * subdirectory: the host paths, the searches, the path split, the directory
 * the game sees and the free space; and the CD as an image (iso9660.h, GOG's
 * GAME.GOG, Steam's GAME.BIN): one made for the test, named by CRUSADER_CD
 * and as the current directory's GAME.GOG or GAME.BIN. */
#include "dir.h"
#include "files.h"
#include "filestate.h"
#include "hostpath.h"
#include "iso9660.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iterator>
#include <random>
#include <set>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH files: %s\n", what.c_str());
    }
}

std::set<std::string> search(const char *pattern, int attributes);

/* The bytes of a directory's record (iso9660.h): name, extent and size
 * both ways round, the date 1995-11-28 12:34:56. */
std::string isoRecord(const std::string &name, std::uint32_t sector, std::uint32_t size, bool directory)
{
    std::string record(33 + name.size() + ((33 + name.size()) % 2), '\0');
    record[0] = (char)record.size();
    for (int k = 0; k < 4; k++) {
        record[2 + k] = (char)(sector >> (8 * k));
        record[9 - k] = (char)(sector >> (8 * k));
        record[10 + k] = (char)(size >> (8 * k));
        record[17 - k] = (char)(size >> (8 * k));
    }
    const char date[7] = {95, 11, 28, 12, 34, 56, 0};
    std::memcpy(&record[18], date, sizeof date);
    record[25] = directory ? 2 : 0;
    record[32] = (char)name.size();
    std::memcpy(&record[33], name.data(), name.size());
    return record;
}

/* A CD's image: the primary volume's descriptor at sector 16, the root at
 * 18 (README.TXT at 20, STATIC at 19), STATIC's FIXED.DAT at 21 and NOEXT
 * at 22; each file is followed by bytes of none, which a read must not
 * reach. */
std::string isoImage()
{
    std::string image(23 * ISO_SECTOR, '\0');
    char *descriptor = &image[16 * ISO_SECTOR];
    descriptor[0] = 1;
    std::memcpy(descriptor + 1, "CD001", 5);
    descriptor[6] = 1;
    descriptor[128] = 0;
    descriptor[129] = 8;
    descriptor[130] = 8;
    const std::string root = isoRecord(std::string(1, '\0'), 18, ISO_SECTOR, true);
    std::memcpy(descriptor + 156, root.data(), root.size());
    const std::string rootRecords = root + isoRecord(std::string(1, '\1'), 18, ISO_SECTOR, true) +
                                    isoRecord("README.TXT;1", 20, 6, false) + isoRecord("STATIC", 19, ISO_SECTOR, true);
    image.replace(18 * ISO_SECTOR, rootRecords.size(), rootRecords);
    const std::string staticRecords = isoRecord(std::string(1, '\0'), 19, ISO_SECTOR, true) +
                                      isoRecord(std::string(1, '\1'), 18, ISO_SECTOR, true) +
                                      isoRecord("FIXED.DAT;1", 21, 5, false) + isoRecord("NOEXT.;1", 22, 3, false);
    image.replace(19 * ISO_SECTOR, staticRecords.size(), staticRecords);
    image.replace(20 * ISO_SECTOR, 12, "readmeSPILLS");
    image.replace(21 * ISO_SECTOR, 10, "fixedSPILL");
    image.replace(22 * ISO_SECTOR, 3, "abc");
    return image;
}

void setCd(const std::string &value)
{
#ifdef _WIN32
    ::_putenv_s("CRUSADER_CD", value.c_str());
#else
    if (value.empty()) {
        ::unsetenv("CRUSADER_CD");
    } else {
        ::setenv("CRUSADER_CD", value.c_str(), 1);
    }
#endif
}

std::string readAll(int file, std::uint32_t count)
{
    std::string bytes(count, '\0');
    const FileResult got = fileRead(file, bytes.data(), count);
    bytes.resize(got.error != 0 ? 0 : got.value);
    return bytes;
}

/* The CD as an image, CRUSADER_CD's and then the current directory's
 * GAME.GOG or GAME.BIN, in that order. */
void imageChecks(const fs::path &root)
{
    constexpr std::uint16_t TIME = (12U << 11) | (34U << 5) | 28U;
    constexpr std::uint16_t DATE = (15U << 9) | (11U << 5) | 28U;
    const char recorded[7] = {95, 11, 28, 12, 34, 56, 0};
    const DosStamp stamp = isoDosStamp(std::string_view(recorded, 7));
    check(stamp.time == TIME && stamp.date == DATE, "image: a record's date as DOS's");
    const char early[7] = {75, 1, 1, 0, 0, 0, 0};
    check(isoDosStamp(std::string_view(early, 7)).date == ((1U << 5) | 1U), "image: a date before 1980 is 1980's");
    const char late[7] = {(char)208, 1, 1, 0, 0, 0, 0};
    check(isoDosStamp(std::string_view(late, 7)).date == ((127U << 9) | (12U << 5) | 31U),
          "image: a date after 2107 is 2107's last");

    const fs::path image = root / "cd.iso";
    std::ofstream(image, std::ios::binary) << isoImage();
    setCd(image.string());
    FileResult opened = fileOpen("D:\\STATIC\\FIXED.DAT", false);
    check(opened.error == 0, "image: a file opened");
    if (opened.error == 0) {
        const int file = (int)opened.value;
        check(readAll(file, 100) == "fixed", "image: a read ends with the file");
        check(fileSeek(file, 0, SEEK_END).value == 5, "image: the end is the file's");
        check(fileSeek(file, 1, SEEK_SET).value == 1 && readAll(file, 2) == "ix", "image: a place from the start");
        check(fileSeek(file, -10, SEEK_SET).error == EINVAL, "image: no place before the start");
        const FileResult time = fileTime(file);
        check(time.error == 0 && time.value == ((std::uint32_t)DATE << 16 | TIME), "image: the file's date");
        check(fileSeek(file, 2, SEEK_SET).value == 2, "image: a place for the snapshot");
        const FilesState state = filesState();
        check(fileClose(file).error == 0, "image: closed");
        putFilesState(state);
        check(readAll(file, 10) == "xed", "image: opened again by the snapshot at its place");
        check(fileClose(file).error == 0, "image: closed again");
    }
    opened = fileOpen("d:static\\fixed.dat", false);
    check(opened.error == 0 && fileClose((int)opened.value).error == 0, "image: any case, no root");
    check(fileOpen("D:\\STATIC\\FIXED.DAT", true).error == EACCES, "image: not written");
    check(fileOpen("D:\\NONE.DAT", false).error == ENOENT, "image: no such file");
    check(fileOpen("D:\\STATIC", false).error == EISDIR, "image: a directory");
    check(search("D:\\STATIC\\*.*", 0) == std::set<std::string>{"FIXED.DAT", "NOEXT"}, "image: a search");
    check(search("D:\\*.*", 0) == std::set<std::string>{"README.TXT"}, "image: no directory without FILE_DIRECTORY");
    check(search("D:\\*.*", FILE_DIRECTORY) == std::set<std::string>{"README.TXT", "STATIC"},
          "image: a directory with FILE_DIRECTORY");
    FileFind found{};
    check(fileFindFirst("D:\\STATIC\\FIXED.DAT", &found, 0) == 0 && found.size == 5 && found.date == DATE &&
              found.time == TIME,
          "image: a search's size and date");
    check(fileRemove("D:\\README.TXT") == -1 && fileRename("D:\\README.TXT", "MOVED") == -1,
          "image: nothing removed or renamed");
    check(fs::exists(image), "image: the image left alone");

    /* Without CRUSADER_CD, the current directory's GAME.GOG (a GOG install). */
    setCd("");
    fs::rename(image, root / "GAME.GOG");
    opened = fileOpen("D:\\README.TXT", false);
    check(opened.error == 0 && readAll((int)opened.value, 100) == "readme" && fileClose((int)opened.value).error == 0,
          "image: GAME.GOG of the current directory");
    opened = fileOpen("README.TXT", false);
    check(opened.error == 0 && readAll((int)opened.value, 100) == "readme.txt" &&
              fileClose((int)opened.value).error == 0,
          "image: drive C still the current directory");

    /* A Steam install: GAME.BIN, the same image by another name. The image
     * is found again in another directory. */
    fs::create_directory(root / "steam");
    fs::copy_file(root / "GAME.GOG", root / "steam" / "GAME.BIN");
    fs::current_path(root / "steam");
    opened = fileOpen("D:\\README.TXT", false);
    check(opened.error == 0 && readAll((int)opened.value, 100) == "readme" && fileClose((int)opened.value).error == 0,
          "image: GAME.BIN of the current directory");
    /* Both: GAME.GOG first (here GAME.BIN is no image). */
    fs::create_directory(root / "both");
    fs::copy_file(root / "GAME.GOG", root / "both" / "GAME.GOG");
    std::ofstream(root / "both" / "GAME.BIN") << "not an image";
    fs::current_path(root / "both");
    opened = fileOpen("D:\\README.TXT", false);
    check(opened.error == 0 && readAll((int)opened.value, 100) == "readme" && fileClose((int)opened.value).error == 0,
          "image: GAME.GOG before GAME.BIN");
    fs::current_path(root);
    fs::remove_all(root / "steam");
    fs::remove_all(root / "both");
    fs::remove(root / "GAME.GOG");
}

std::set<std::string> search(const char *pattern, int attributes)
{
    std::set<std::string> names;
    FileFind block;
    for (int result = fileFindFirst(pattern, &block, attributes); result == 0; result = fileFindNext(&block)) {
        names.insert(block.name);
    }
    return names;
}

/* Set the host's timestamp directly, without narrowing it through
 * file_clock or the Windows CRT before the range checks run. */
bool setTimestamp(const fs::path &path, std::chrono::sys_seconds when)
{
#ifdef _WIN32
    const HANDLE file =
        ::CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    constexpr std::int64_t FILETIME_EPOCH = 11644473600;
    const std::uint64_t ticks =
        static_cast<std::uint64_t>(when.time_since_epoch().count() + FILETIME_EPOCH) * 10000000U;
    const FILETIME modified{static_cast<DWORD>(ticks), static_cast<DWORD>(ticks >> 32)};
    const bool changed = ::SetFileTime(file, nullptr, nullptr, &modified) != 0;
    ::CloseHandle(file);
    return changed;
#else
    const timespec times[2]{{0, UTIME_OMIT}, {static_cast<std::time_t>(when.time_since_epoch().count()), 0}};
    return ::utimensat(AT_FDCWD, path.c_str(), times, 0) == 0;
#endif
}

std::uint32_t checkTimestamp(const fs::path &root, const char *name, std::chrono::sys_seconds when, std::uint16_t date,
                             std::uint16_t time)
{
    const fs::path path = root / "DATES" / name;
    std::ofstream(path) << "subtitle";
    check(setTimestamp(path, when), std::string(name) + ": set timestamp");
    const std::string dosPath = std::string("DATES\\") + name;
    FileFind found{};
    const int result = fileFindFirst(dosPath.c_str(), &found, 0);
    check(result == 0 && found.size == 8, std::string(name) + ": found with its size");
    if (result == 0 && (found.date != date || found.time != time)) {
        std::printf("%s: search date/time %04x/%04x, expected %04x/%04x\n", name, found.date, found.time, date, time);
    }
    check(result == 0 && found.date == date && found.time == time, std::string(name) + ": DOS search timestamp");
    const std::uint32_t stamp = static_cast<std::uint32_t>(date) << 16 | time;
    const FileResult opened = fileOpen(dosPath.c_str(), false);
    check(opened.error == 0, std::string(name) + ": opened");
    if (opened.error == 0) {
        const int file = static_cast<int>(opened.value);
        const FileResult modified = fileTime(file);
        if (modified.error != 0 || modified.value != stamp) {
            std::printf("%s: handle error %d, date/time %08x, expected %08x\n", name, modified.error, modified.value,
                        stamp);
        }
        check(modified.error == 0 && modified.value == stamp, std::string(name) + ": DOS handle timestamp");
        char text[8];
        const FileResult read = fileRead(file, text, sizeof(text));
        check(read.error == 0 && read.value == sizeof(text) && std::memcmp(text, "subtitle", sizeof(text)) == 0,
              std::string(name) + ": contents still readable");
        check(fileClose(file).error == 0, std::string(name) + ": closed");
    }
    return static_cast<std::uint32_t>(found.date) << 16 | found.time;
}

}

int main()
{
    fs::path home = fs::current_path();
    fs::path root = fs::temp_directory_path() / ("port-files-" + std::to_string(std::random_device()()));
    fs::create_directories(root / "STATIC");
    /* By its real path, which the working directory is: macOS's temporary
     * folder is under a symbolic link (/var is /private/var). */
    root = fs::canonical(root);
    for (const char *name : {"SAVE1.SAV", "save2.sav", "readme.txt", "LongFileName.dat", "NOEXT", "STATIC/fixed.dat"}) {
        std::ofstream(root / name) << name;
    }
    fs::current_path(root);

    /* The same file: Windows's names are of any case, and the path keeps
     * the game's. */
    auto same = [](const fs::path &a, const fs::path &b) {
        std::error_code error;
        return fs::equivalent(a, b, error);
    };
    check(same(portHostPath("STATIC\\FIXED.DAT"), root / "STATIC" / "fixed.dat"), "host path of STATIC\\FIXED.DAT");
    check(same(portHostPath("C:\\static\\Fixed.Dat"), root / "STATIC" / "fixed.dat"), "host path with the drive");
    check(portHostPath("NEW.SAV") == root / "NEW.SAV", "a new file keeps its name");
    check(search("*.SAV", 0) == std::set<std::string>{"SAVE1.SAV", "SAVE2.SAV"}, "*.SAV");
    check(search("*.*", 0) == std::set<std::string>{"SAVE1.SAV", "SAVE2.SAV", "README.TXT", "NOEXT"},
          "*.* (the long name has no 8.3 form)");
    check(search("*", 0) == std::set<std::string>{"NOEXT"}, "* is *. in DOS");
    check(search("SAVE?.SAV", 0).size() == 2, "SAVE?.SAV");
    check(search("STATIC", 0).empty(), "a directory without FILE_DIRECTORY");
    check(search("STATIC", FILE_DIRECTORY) == std::set<std::string>{"STATIC"}, "a directory with FILE_DIRECTORY");
    check(search("static\\*.DAT", 0) == std::set<std::string>{"FIXED.DAT"}, "a search in a subdirectory");
    FileFind block;
    check(fileFindFirst("readme.txt", &block, 0) == 0 && block.size == 10, "the size");
    check(fileFindFirst("NONE.SAV", &block, 0) == -1, "no file");

    /* UTC makes the DOS calendar fields exact at the range boundaries. */
#ifdef _WIN32
    ::_putenv_s("TZ", "UTC");
    ::_tzset();
#else
    ::setenv("TZ", "UTC", 1);
    ::tzset();
#endif
    fs::create_directories(root / "DATES");
    using namespace std::chrono;
    checkTimestamp(root, "T02.TXT", sys_days{year{1601} / 1 / 1} + hours{19} + minutes{47}, 0x0021, 0);
    checkTimestamp(root, "EARLY.DAT", sys_days{year{1975} / 1 / 1}, 0x0021, 0);
    checkTimestamp(root, "FIRST.DAT", sys_days{year{1980} / 1 / 1}, 0x0021, 0);
    const std::uint32_t older = checkTimestamp(
        root, "NORMAL.DAT", sys_days{year{2000} / 5 / 6} + hours{12} + minutes{34} + seconds{57}, 0x28a6, 0x645c);
    const std::uint32_t newer = checkTimestamp(root, "NEWER.DAT", sys_days{year{2001} / 5 / 6}, 0x2aa6, 0);
    check(newer > older, "normal timestamps retain the ordering used by FileSpec::isNewer");
    checkTimestamp(root, "LAST.DAT", sys_days{year{2107} / 12 / 31} + hours{23} + minutes{59} + seconds{59}, 0xff9f,
                   0xbf7d);
    checkTimestamp(root, "FUTURE.DAT", sys_days{year{2108} / 1 / 1}, 0xff9f, 0xbf7d);
#ifndef _WIN32
    /* A dangling link has a directory entry but no readable target date. */
    fs::create_symlink("NOTFOUND", root / "DATES" / "BAD.DAT");
    FileFind bad{};
    check(fileFindFirst("DATES\\BAD.DAT", &bad, 0) == 0 && bad.date == 0x0021 && bad.time == 0,
          "a failed date lookup retains the first DOS date");
#endif

    char drive[MAXDRIVE], dir[MAXDIR], name[MAXFILE], ext[MAXEXT];
    int flags = fnsplit("C:\\GAMES\\CRUSADER\\SAVE1.SAV", drive, dir, name, ext);
    check(flags == (DRIVE | DIRECTORY | FILENAME | EXTENSION) && !std::strcmp(drive, "C:") &&
              !std::strcmp(dir, "\\GAMES\\CRUSADER\\") && !std::strcmp(name, "SAVE1") && !std::strcmp(ext, ".SAV"),
          "fnsplit of a full path");
    flags = fnsplit("*.unk", drive, dir, name, ext);
    check(flags == (WILDCARDS | FILENAME | EXTENSION) && !*drive && !*dir && !std::strcmp(name, "*"),
          "fnsplit of *.unk");
    flags = fnsplit("..", drive, dir, name, ext);
    check(flags == DIRECTORY && !std::strcmp(dir, "..") && !*name, "fnsplit of ..");

    char cwd[16];
    check(fileCurrentDirectory(cwd, sizeof(cwd)) && !std::strcmp(cwd, "C:\\"), "the current directory");
    FileSpace space{};
    fileFreeSpace(0, &space);
    check(space.sectorsPerCluster == 64 && space.bytesPerSector == 512 && space.total > 0, "the free space");
    fileFreeSpace(1, &space);
    check(space.sectorsPerCluster == 0xffff, "no drive A");

    /* A replay's sandbox (hostpath.h's portSandbox): the writes there, a
     * file the game keeps writing in copied there first, the reads there
     * before the current directory's; the player's files (savegame\,
     * crusader.cfg) only there; a search sees both, the player's only from
     * the sandbox, a name once. */
    fs::path sandbox = fs::temp_directory_path() / ("port-sandbox-" + std::to_string(std::random_device()()));
    fs::create_directories(root / "SAVEGAME");
    std::ofstream(root / "SAVEGAME" / "SAVEGAME.1") << "the player's";
    std::ofstream(root / "CRUSADER.CFG") << "the player's";
    fs::create_directories(sandbox / "savegame");
    std::ofstream(sandbox / "savegame" / "savegame.2") << "the recording's";
    portSandbox(sandbox);
    auto text = [](const fs::path &path) {
        std::ifstream in(path);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    };
    check(same(portHostPath("STATIC\\FIXED.DAT"), root / "STATIC" / "fixed.dat"), "sandbox: a read of the game's");
    check(!fs::exists(portHostPath("SAVEGAME\\SAVEGAME.1")), "sandbox: the player's save hidden");
    check(same(portHostPath("SAVEGAME\\SAVEGAME.2"), sandbox / "savegame" / "savegame.2"),
          "sandbox: the recording's save");
    check(!fs::exists(portHostPath("CRUSADER.CFG")), "sandbox: the player's crusader.cfg hidden");
    FileResult made = fileCreate("SAVEGAME\\SAVEGAME.3");
    check(!made.error && !fileWrite((int)made.value, "new", 3).error && !fileClose((int)made.value).error,
          "sandbox: a save made");
    check(text(sandbox / "savegame" / "SAVEGAME.3") == "new" && !fs::exists(root / "SAVEGAME" / "SAVEGAME.3"),
          "sandbox: the save in the sandbox");
    FileResult kept = fileOpen("README.TXT", true);
    check(!kept.error && !fileWrite((int)kept.value, "X", 1).error && !fileClose((int)kept.value).error,
          "sandbox: a game's file written");
    check(text(root / "readme.txt") == "readme.txt" && text(portHostPath("README.TXT")) == "Xeadme.txt",
          "sandbox: the file copied, then written");
    if (std::FILE *cfg = fileStream("crusader.cfg", "wb")) {
        std::fputs("the replay's", cfg);
        std::fclose(cfg);
    }
    check(text(root / "CRUSADER.CFG") == "the player's" && text(portHostPath("CRUSADER.CFG")) == "the replay's",
          "sandbox: crusader.cfg written there");
    check(fileRename("NOEXT", "MOVED") == 0 && fs::exists(root / "NOEXT") &&
              same(portHostPath("MOVED"), sandbox / "MOVED"),
          "sandbox: a game's file renamed is copied");
    check(fileRemove("MOVED") == 0 && !fs::exists(sandbox / "MOVED"), "sandbox: a file removed there");
    check(search("SAVEGAME\\*.*", 0) == std::set<std::string>{"SAVEGAME.2", "SAVEGAME.3"},
          "sandbox: the saves searched, the player's not");
    check(search("*.TXT", 0) == std::set<std::string>{"README.TXT"} && search("*.CFG", 0).size() == 1,
          "sandbox: a name found once");
    portSandbox({});
    fs::remove_all(sandbox);

    imageChecks(root);

    fs::current_path(home);
    fs::remove_all(root);
    std::printf("files: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
