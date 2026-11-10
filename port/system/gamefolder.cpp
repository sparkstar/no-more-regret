/* See gamefolder.h. */
#include "gamefolder.h"
#include "files.h"
#include "gamememory.h"
#include "profile.h"

#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

namespace {

/* The originals' programs, the versions the port was made from. */
struct Known {
    const char *name;
    const char *game;
    std::uintmax_t size;
    std::uint64_t hash;
};
constexpr std::array<Known, 2> KNOWN = {{
    {"REGRET.EXE", "regret", 1005584, 0x68c1cf409b99112dULL},
    {"CRUSADER.EXE", "remorse", 991878, 0xb7e3bd8ac253f32bULL},
}};

bool sameName(const std::string &a, const std::string &b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); i++) {
        if (std::toupper(static_cast<unsigned char>(a[i])) != std::toupper(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

/* The entry of folder named name in any case, empty for none. */
fs::path entryNamed(const fs::path &folder, const std::string &name)
{
    std::error_code error;
    for (const fs::directory_entry &entry : fs::directory_iterator(folder, error)) {
        if (sameName(entry.path().filename().string(), name)) {
            return entry.path();
        }
    }
    return {};
}

/* The original's program in folder (the first the port knows by name),
 * empty for none or for a folder without the game's data (STATIC): the
 * port's own build of one game is regret.exe on Windows. */
fs::path originalIn(const fs::path &folder)
{
    std::error_code error;
    if (!fs::is_directory(entryNamed(folder, "STATIC"), error)) {
        return {};
    }
    for (const fs::directory_entry &entry : fs::directory_iterator(folder, error)) {
        const std::string name = entry.path().filename().string();
        if (entry.is_regular_file(error) && !originalOf(name, 0, 0).game.empty()) {
            return entry.path();
        }
    }
    return {};
}

Original originalAt(const fs::path &program)
{
    std::ifstream file(program, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return originalOf(program.filename().string(), bytes.size(), fnv1a64(bytes));
}

GameFolder findFolder()
{
    GameFolder folder;
    for (const fs::path &place : {programFile().parent_path(), startDirectory()}) {
        if (place.empty()) {
            continue;
        }
        const fs::path program = originalIn(place);
        if (!program.empty()) {
            folder.path = place;
            folder.program = program;
            folder.original = originalAt(program);
            return folder;
        }
    }
    folder.path = startDirectory();
    return folder;
}

}

fs::path programFile()
{
#ifdef _WIN32
    std::vector<wchar_t> path(MAX_PATH);
    for (;;) {
        const DWORD got = GetModuleFileNameW(nullptr, path.data(), (DWORD)path.size());
        if (got == 0) {
            return {};
        }
        if (got < path.size()) {
            return fs::path(path.data());
        }
        path.resize(path.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> path(size);
    if (_NSGetExecutablePath(path.data(), &size) != 0) {
        return {};
    }
    std::error_code error;
    const fs::path program = fs::canonical(path.data(), error);
    return error ? fs::path() : program;
#else
    std::error_code error;
    const fs::path program = fs::read_symlink("/proc/self/exe", error);
    return error ? fs::path() : program;
#endif
}

Original originalOf(const std::string &name, std::uintmax_t size, std::uint64_t hash)
{
    for (const Known &known : KNOWN) {
        if (sameName(name, known.name)) {
            return {known.game, size == known.size && hash == known.hash};
        }
    }
    return {};
}

std::uint64_t fnv1a64(const std::string &bytes)
{
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char value : bytes) {
        hash = (hash ^ static_cast<unsigned char>(value)) * 0x100000001b3ULL;
    }
    return hash;
}

const fs::path &startDirectory()
{
    static const fs::path start = [] {
        std::error_code error;
        return fs::current_path(error);
    }();
    return start;
}

const GameFolder &gameFolder()
{
    static const GameFolder folder = findFolder();
    return folder;
}

const char *fileFolderGame()
{
    const SystemSide systemSide;
    return gameFolder().original.game.c_str();
}

const char *fileLastGame()
{
    const SystemSide systemSide;
    static const std::string last = [] {
        for (const char *name : {"enhanced.cfg", "ENHANCED.CFG"}) {
            std::ifstream file(name);
            std::string line;
            while (std::getline(file, line)) {
                if (line.rfind("game=", 0) == 0) {
                    std::string game = line.substr(5);
                    while (!game.empty() && (game.back() == '\r' || game.back() == ' ')) {
                        game.pop_back();
                    }
                    return game;
                }
            }
        }
        return std::string();
    }();
    return last.c_str();
}

void enterGameFolder()
{
    const GameFolder &folder = gameFolder();
    std::error_code error;
    if (folder.path != fs::current_path(error)) {
        fs::current_path(folder.path, error);
    }
    /* The game's saves go in SAVEGAME, which GOG's installer makes and the
     * game does not; innoextract writes the installer's empty folders under
     * app. */
    if (!folder.program.empty() && entryNamed(folder.path, "SAVEGAME").empty()) {
        fs::create_directory(folder.path / "SAVEGAME", error);
    }
    if (!folder.program.empty() && !folder.original.known) {
        std::fprintf(stderr,
                     "%s: %s is not the version the port was made from (No Regret 1.06's REGRET.EXE, No Remorse "
                     "1.21's CRUSADER.EXE, as GOG sells them); the game may not run as it should\n",
                     PROFILE_NAME, folder.program.string().c_str());
    }
}

void leaveGameFolder()
{
    std::error_code error;
    fs::current_path(startDirectory(), error);
}
