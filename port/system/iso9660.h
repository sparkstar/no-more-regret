/* A CD's image read as the CD: ISO 9660 in sectors of 2048 bytes, as
 * GOG's GAME.GOG and Steam's GAME.BIN are, its files found by their paths
 * and read straight from the image, with no mount (files.cpp gives the
 * game's other drives from one). The level of the games' CDs: names of 8.3
 * capitals with a version (";1"), one extent a file. */
#ifndef PORT_ISO9660_H
#define PORT_ISO9660_H
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/* The bytes of a sector. */
const std::uint32_t ISO_SECTOR = 2048;
/* The sector of the primary volume's descriptor. */
const std::uint32_t ISO_DESCRIPTOR = 16;

/* A file or directory of the image: its name without the version, its
 * extent and the date recorded, as DOS gives a file's (MSCDEX's: the
 * fields as they are, without the time zone). */
struct IsoEntry {
    std::string name;
    bool directory = false;
    std::uint32_t sector = 0;
    std::uint32_t size = 0;
    std::uint16_t dosTime = 0;
    std::uint16_t dosDate = 0;
    bool operator==(const IsoEntry &) const = default;
};
/* The root directory of the primary volume's descriptor (sector 16's
 * bytes), none when they are not one of sectors of 2048 bytes. */
[[nodiscard]] std::optional<IsoEntry> isoRoot(std::string_view descriptor);
/* The entries of a directory's extent, without its own and its parent's;
 * a record stops at the end of its sector. */
[[nodiscard]] std::vector<IsoEntry> isoEntries(std::string_view extent);
/* The DOS time and date of a record's seven bytes of date (years from 1900,
 * month, day, hour, minute, second, the zone), the years before 1980 and
 * after 2107 at the ends, as fileTime's (files.h). */
struct DosStamp {
    std::uint16_t time;
    std::uint16_t date;
};
[[nodiscard]] DosStamp isoDosStamp(std::string_view recorded);

/* An image's directories, read as they are asked for. */
class IsoImage {
public:
    /* The image at path, none when it is not one. */
    [[nodiscard]] static std::optional<IsoImage> open(const std::filesystem::path &path);
    [[nodiscard]] const std::filesystem::path &path() const { return path_; }
    /* The entry at path (its parts separated by \\ or /, any case), none
     * when there is none. */
    [[nodiscard]] std::optional<IsoEntry> find(const std::string &path);
    /* The entries of the directory at path, none when it is none. */
    [[nodiscard]] std::optional<std::vector<IsoEntry>> list(const std::string &path);

private:
    IsoImage(std::filesystem::path path, IsoEntry root) : path_(std::move(path)), root_(std::move(root)) {}
    [[nodiscard]] std::string read(std::uint32_t sector, std::uint32_t size) const;
    [[nodiscard]] const std::vector<IsoEntry> &entriesOf(const IsoEntry &directory);
    std::filesystem::path path_;
    IsoEntry root_;
    /* The directories read, by their extents' sectors. */
    std::map<std::uint32_t, std::vector<IsoEntry>> directories_;
};
#endif
