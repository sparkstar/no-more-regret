/* files.cpp's part of the virtual PC's state (pcstate.h), in a header of
 * its own, which needs none of the game's (files_test compiles files.cpp
 * without them). */
#ifndef PORT_FILESTATE_H
#define PORT_FILESTATE_H
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

/* files.cpp's: each open file of the game by its handle (the host's
 * descriptor), what it is and where in it (the place in the host's file);
 * a file of the CD's image is the image's, its member named. */
struct OpenFile {
    std::filesystem::path host;
    int flags;
    std::int64_t place;
    std::string member;
    bool operator==(const OpenFile &) const = default;
};
struct FilesState {
    std::map<int, OpenFile> open;
    bool operator==(const FilesState &) const = default;
};
[[nodiscard]] FilesState filesState();
void putFilesState(const FilesState &state);
#endif
