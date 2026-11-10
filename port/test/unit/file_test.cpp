/* File on the host's files: the game's file.cpp over the platform's files
 * (files.h; port/system/files.cpp), which has no oracle (the original calls INT
 * 21h). In a directory made for the test, which is drive C, with the
 * repository's cd/noregret/ as the CD (CRUSADER_CD): reads and writes past 64K,
 * the size and the position, copies that overlap, the size cut and grown,
 * lines, names in any case, the halts of a missing file and of the 65 files
 * open, rename, and the game's data files by the paths the game gives. The
 * halts are the test's: they throw. */
#include "file.h"
#include "files.h"
#include "hostpath.h"
#include "testenv.h"
#include "utility.h"

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH file: %s\n", what.c_str());
    }
}

/* A halt of the game: the function it was called from and its message. */
struct Halted {
    std::string function;
    std::string message;
};

std::vector<u8> hostBytes(const fs::path &path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<u8>(std::istreambuf_iterator<char>(in), {});
}

/* The halt that f ends in, or no function when it returns. */
template <class F> Halted haltOf(F f)
{
    try {
        f();
    } catch (const Halted &halted) {
        return halted;
    }
    return {"", ""};
}

}

/* The game's halts (common.h) and fileExists (utility.cpp), which file.cpp
 * calls. */
void halt(std::source_location place)
{
    throw Halted{place.function_name(), ""};
}

void haltMessage(const char *message, std::source_location place)
{
    throw Halted{place.function_name(), message};
}

const char *haltFormat(const char *format, ...)
{
    static char text[256];
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    return text;
}

void allocationFailure(std::source_location place)
{
    throw Halted{place.function_name(), "out of memory"};
}

void openFailure(const char *name, std::source_location place)
{
    throw Halted{place.function_name(), name};
}

i16 fileExists(const char *path)
{
    FileFind find;
    return !fileFindFirst(path, &find, FILE_DIRECTORY);
}

int main()
try {
    const char *origin = std::getenv("CRUSADER_ORIGIN");
    fs::path cd = origin ? fs::path(origin) : fs::path(PORT_ORIGIN);
    fs::path home = fs::current_path();
    fs::path root = fs::temp_directory_path() / ("port-file-" + std::to_string(std::random_device()()));
    fs::create_directories(root);
    fs::current_path(root);
    setenv("CRUSADER_CD", cd.string().c_str(), 1);

    /* Past 64K, in the pieces of FFFAh bytes the original reads and writes. */
    std::vector<u8> data(100000);
    std::mt19937 random(1);
    for (u8 &b : data) {
        b = (u8)random();
    }
    {
        File file("TEST.DAT", FILE_READWRITE, 1);
        check(file.isOpen() && file.write(data.data(), (s32)data.size(), -1L) == (s32)data.size(), "a write of 100000");
        check(file.size() == 100000 && file.tell() == 100000, "size and position after it");
        std::vector<u8> back(data.size());
        check(file.read(back.data(), (s32)back.size(), 0L) == (s32)back.size() && back == data, "read back");
        check(file.atEnd(), "at the end");
        u8 some[10];
        check(file.read(some, 10, 70000L) == 10 && !std::memcmp(some, data.data() + 70000, 10), "a read at a position");
        check(file.read(some, 10, -1L) == 10 && !std::memcmp(some, data.data() + 70010, 10) && file.tell() == 70020,
              "a read where it is");
        check(file.read(some, 10, 99995L) == 5, "a read past the end counts what it read");
        check(!std::strcmp(file.name, "TEST.DAT"), "the name kept");
        check(file.lock(0, 1) && file.unlock(0, 1), "locks");
        FileResult time = fileTime(file.handle);
        u16 date = (u16)(time.value >> 16);
        check(!time.error && (date >> 9) + 1980 >= 2024, "the date of the last write");

        /* An overlapping copy within the file goes backwards. */
        file.copy(0, 1000, 70000, nullptr);
        std::vector<u8> moved(data);
        std::memmove(moved.data() + 1000, data.data(), 70000);
        check(file.read(back.data(), (s32)back.size(), 0L) == (s32)back.size() && back == moved,
              "a copy forward over itself");
        file.copy(1000, 0, 70000, nullptr);
        std::memmove(moved.data(), moved.data() + 1000, 70000);
        check(file.read(back.data(), (s32)back.size(), 0L) == (s32)back.size() && back == moved,
              "a copy backward over itself");

        /* The size cut and grown; the position stays when it can. */
        file.seek(500, SEEK_SET);
        file.setSize(300);
        check(file.size() == 300 && file.tell() == 300, "cut below the position");
        file.seek(100, SEEK_SET);
        file.setSize(400);
        check(file.size() == 400 && file.tell() == 100, "grown");
        check(file.read(back.data(), 400, 0L) == 400 && !std::memcmp(back.data(), moved.data(), 300) &&
                  std::all_of(back.begin() + 300, back.begin() + 400, [](u8 b) { return b == 0; }),
              "grown with zeros");
        file.seek(250, SEEK_SET);
        file.commitSize();
        check(file.size() == 250, "commitSize");
    }
    check(fs::file_size(root / "TEST.DAT") == 250, "closed at the end of its life");

    /* copyAll into another file. */
    {
        File from("TEST.DAT", FILE_READ, 0), to("COPY.DAT", FILE_READWRITE, 0);
        to.copyAll(&from);
        to.commitSize();
    }
    check(hostBytes(root / "COPY.DAT") == hostBytes(root / "TEST.DAT"), "copyAll");

    /* Lines end at CR (skipping LF) or ^Z. */
    {
        std::ofstream(root / "lines.txt", std::ios::binary) << "first\r\nsecond\r\n\x1alast";
        File file("LINES.TXT", FILE_READ, 0);
        char line[20];
        file.readLine(line, sizeof(line), 0L);
        check(!std::strcmp(line, "first") && file.tell() == 7, "a line");
        file.readLine(line, sizeof(line), -1L);
        check(!std::strcmp(line, "second") && file.tell() == 15, "the next line");
        file.readLine(line, sizeof(line), -1L);
        check(!*line && file.tell() == 16, "^Z");
    }

    /* Names in any case; a new file keeps the case given. */
    {
        std::ofstream(root / "Mixed.Dat") << "mixed";
        File file("c:\\MIXED.DAT", FILE_READ, 0);
        char text[5];
        check(file.read(text, 5, -1L) == 5 && !std::memcmp(text, "mixed", 5), "a name in another case");
    }

    /* The halts: a missing file to read, too many files, rename onto a file. */
    Halted halted = haltOf([] { File file("NONE.DAT", FILE_READ, 0); });
    check(halted.function.find("File::open") != std::string::npos && halted.message == "NONE.DAT",
          "a missing file halts in File::open");
    {
        File file("NEW.DAT", FILE_READWRITE, 0);
        check(file.isOpen() && fs::exists(root / "NEW.DAT"), "a missing file to write is made");
    }
    {
        std::vector<std::unique_ptr<File>> files;
        halted = haltOf([&] {
            for (;;) {
                files.push_back(std::make_unique<File>("TEST.DAT", FILE_READ, 0));
            }
        });
        check(files.size() == FILES_OPEN && halted.function.find("File::open") != std::string::npos,
              "65 files open, then a halt");
    }
    std::vector<int> created;
    FileResult result;
    for (;;) {
        char name[16];
        std::snprintf(name, sizeof(name), "FHACK%d", (int)created.size());
        result = fileCreate(name);
        if (result.error) {
            break;
        }
        created.push_back((int)result.value);
    }
    check(created.size() == FILES_OPEN && result.error == EMFILE, "CRUSYS.C's check makes 65, then too many files");
    for (int file : created) {
        check(!fileClose(file).error, "a file closed");
    }
    check(fileClose(created[0]).error == EBADF, "a file closed twice");
    for (u16 i = 0; i < created.size(); i++) {
        char name[16];
        std::snprintf(name, sizeof(name), "FHACK%d", i);
        check(fileRemove(name) == 0, std::string("remove ") + name);
    }
    {
        File file;
        file.setName("COPY.DAT");
        file.rename("MOVED.DAT");
        check(fs::exists(root / "MOVED.DAT") && !fs::exists(root / "COPY.DAT") && !std::strcmp(file.name, "MOVED.DAT"),
              "rename");
        halted = haltOf([&] { file.rename("TEST.DAT"); });
        check(halted.function.find("File::rename") != std::string::npos, "rename onto a file halts");
        file.setName(nullptr);
    }

    /* The game's data on the CD, by the game's paths: gameDrive "d:", the
     * static directory and the names in lower case. */
    for (const char *name : {"fixed.dat", "typeflag.dat", "shapes.flx", "fonts.flx"}) {
        std::string path = std::string("d:\\static\\") + name;
        File file(path.c_str(), FILE_READ, 0);
        fs::path host = portHostPath(path.c_str());
        std::vector<u8> bytes = hostBytes(host);
        check(!bytes.empty() && file.size() == (s32)bytes.size(), path + ": its size");
        std::vector<u8> head(0x80);
        check(file.read(head.data(), 0x80, 0L) == 0x80 && std::equal(head.begin(), head.end(), bytes.begin()),
              path + ": its first bytes");
        if (std::string(name).ends_with(".flx")) {
            u32 count;
            std::memcpy(&count, head.data() + 0x54, 4);
            check(count > 0 && count < 0x10000, path + ": a FLEX's object count");
        }
    }

    fs::current_path(home);
    fs::remove_all(root);
    std::printf("file: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
} catch (const Halted &halted) {
    std::printf("MISMATCH file: a halt no check expects, in %s: %s\n", halted.function.c_str(), halted.message.c_str());
    return 1;
}
