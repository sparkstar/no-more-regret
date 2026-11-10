/* The game's folder: where the game's data are. A player copies the port's
 * program into a GOG or Steam install of one of the games, so that the
 * program's own folder holds the original's program (REGRET.EXE, No
 * Remorse's CRUSADER.EXE), its data and the CD's image (GAME.GOG, Steam's
 * GAME.BIN); a developer's run and the tests start the program in an
 * install made for it (tools/install.py), which holds no original's
 * program: the directory the program starts in. The original's program
 * there picks the game (files.h's fileFolderGame,
 * unified/src/system/values.cpp). */
#ifndef PORT_GAMEFOLDER_H
#define PORT_GAMEFOLDER_H
#include <cstdint>
#include <filesystem>
#include <string>

/* An original's program the port knows: its game (regret, remorse, or ""
 * for a name of none) and whether it is the version the port was made
 * from (No Regret 1.06's REGRET.EXE, No Remorse 1.21's CRUSADER.EXE, as
 * GOG sells them), by its name in any case, its size and the FNV-1a 64-bit
 * hash of its bytes. */
struct Original {
    std::string game;
    bool known = false;
};
[[nodiscard]] Original originalOf(const std::string &name, std::uintmax_t size, std::uint64_t hash);
[[nodiscard]] std::uint64_t fnv1a64(const std::string &bytes);

struct GameFolder {
    /* The program's own folder when it holds an original's program and the
     * game's data (STATIC), else the directory the program started in when
     * that does, else the directory the program started in. */
    std::filesystem::path path;
    /* The original's program there, empty for none. */
    std::filesystem::path program;
    Original original;
};
/* The program's own file, empty when the host does not tell it. */
[[nodiscard]] std::filesystem::path programFile();
/* Found once (values.cpp's static object asks for it before main). */
[[nodiscard]] const GameFolder &gameFolder();
/* The directory the program started in, which the command line's relative
 * paths are in. */
[[nodiscard]] const std::filesystem::path &startDirectory();
/* main's first: into the game's folder, which drive C is (files.h), with a
 * warning on the standard error when its original's program is another
 * version than the port was made from; a GOG install's SAVEGAME made when
 * it is not there. */
void enterGameFolder();
/* Back to the directory the program started in, before the program runs
 * again for the other game (restart.h), which finds its folder anew. */
void leaveGameFolder();
#endif
