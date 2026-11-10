/* Borland's functions of dir.h that the game calls and the port keeps:
 * fnsplit, the path split (as stricmp). FLICPLAY.C's sleep is the clock's.
 * The files' own went to the platform's files (files.h,
 * port/system/files.cpp); port/test's files_test checks these. */
#include "dir.h"
#include "dos.h"

#include <cstring>
#include <string>

/* FNSPLIT.C: the drive with its colon, the directory with its last
 * backslash, the name and the extension with its dot, each cut to its
 * size; the flags of the parts there are and of wildcards. "." and ".." at
 * the end are directories. */
int fnsplit(const char *path, char *drive, char *directory, char *name, char *extension)
{
    std::string text = path;
    int flags = 0;
    std::string d;
    std::string dir;
    std::string n;
    std::string e;
    if (text.size() >= 2 && text[1] == ':') {
        d = text.substr(0, 2);
        text.erase(0, 2);
        flags |= DRIVE;
    }
    const size_t cut = text.find_last_of("\\/");
    std::string last = cut == std::string::npos ? text : text.substr(cut + 1);
    dir = cut == std::string::npos ? "" : text.substr(0, cut + 1);
    if (last == "." || last == "..") {
        dir += last;
        last.clear();
    }
    const size_t dot = last.rfind('.');
    n = dot == std::string::npos ? last : last.substr(0, dot);
    e = dot == std::string::npos ? "" : last.substr(dot);
    if (!dir.empty()) {
        flags |= DIRECTORY;
    }
    if (!n.empty()) {
        flags |= FILENAME;
    }
    if (!e.empty()) {
        flags |= EXTENSION;
    }
    if ((n + e).find_first_of("*?") != std::string::npos) {
        flags |= WILDCARDS;
    }
    auto put = [](char *to, const std::string &part, size_t size) {
        if (to) {
            const std::string truncated = part.substr(0, size - 1);
            std::memcpy(to, truncated.c_str(), truncated.size() + 1);
        }
    };
    put(drive, d, MAXDRIVE);
    put(directory, dir, MAXDIR);
    put(name, n, MAXFILE);
    put(extension, e, MAXEXT);
    return flags;
}
