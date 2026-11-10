/* The game's DOS paths on the host (files.h): drive C, and a path without a
 * drive, is the current directory, the directory the game is installed in;
 * any other drive is the CD, the directory CRUSADER_CD names (the current
 * directory without it). "\\" and "/" both separate the parts, and each
 * part finds a host entry whatever the case of either (the data's names are
 * capitals on the CD). A part no entry matches stays as it is, for a file
 * to be made. files.cpp gives it. */
#ifndef PORT_HOSTPATH_H
#define PORT_HOSTPATH_H
#include <cstdio>
#include <filesystem>
#include <optional>
#include <vector>

std::filesystem::path portHostPath(const char *dosPath);
/* A replay's files (replay.h): drive C's writes go to directory instead,
 * a file the game keeps writing in copied there first, and its reads find
 * a file there before the current directory's; the player's files
 * (savegame\, crusader.cfg and enhanced.cfg) are only there. */
void portSandbox(const std::filesystem::path &directory);
/* The player's files while the run follows a recording (replay.h): they
 * go back with the game's state, as the recording's input that wrote
 * them goes. A file as it was: its path on the host and its bytes, none
 * for no file. */
struct PlayerFile {
    std::filesystem::path path;
    std::optional<std::vector<char>> bytes;
};
/* Whether the player's files are kept: each the game writes (opens to
 * write, makes, removes, renames to) as it was before, in the order of the
 * writes, until they are taken. */
void portKeepPlayersFiles(bool keep);
[[nodiscard]] std::vector<PlayerFile> portPlayersWrites();
/* The files made as they were, the last first (a rewind's). */
void portPutPlayersFiles(const std::vector<PlayerFile> &files);
/* The player's files now, and the player's files made those, the others
 * removed (a state's). */
[[nodiscard]] std::vector<PlayerFile> portPlayersFiles();
void portSetPlayersFiles(const std::vector<PlayerFile> &files);
/* The drive the game sees: C (3, A being 1). */
const int PORT_DRIVE = 3;
/* fopen of a host path, which Windows takes in wide characters. */
std::FILE *portHostFopen(const std::filesystem::path &path, const char *mode);

#endif
