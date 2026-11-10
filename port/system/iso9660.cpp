/* See iso9660.h. */
#include "iso9660.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace {

std::uint32_t word32(std::string_view bytes, std::size_t at)
{
    return static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[at])) |
           static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[at + 1])) << 8 |
           static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[at + 2])) << 16 |
           static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[at + 3])) << 24;
}

std::uint16_t word16(std::string_view bytes, std::size_t at)
{
    return static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[at]) | static_cast<unsigned char>(bytes[at + 1])
                                                                                  << 8);
}

/* A directory record's entry (record: its bytes, its length first). */
IsoEntry entryOf(std::string_view record)
{
    IsoEntry entry;
    entry.sector = word32(record, 2);
    entry.size = word32(record, 10);
    entry.directory = (static_cast<unsigned char>(record[25]) & 2) != 0;
    const DosStamp stamp = isoDosStamp(record.substr(18, 7));
    entry.dosTime = stamp.time;
    entry.dosDate = stamp.date;
    const std::size_t length = static_cast<unsigned char>(record[32]);
    std::string name(record.substr(33, std::min(length, record.size() - 33)));
    if (const std::size_t version = name.find(';'); version != std::string::npos) {
        name.erase(version);
    }
    /* A file without an extension is recorded with its dot. */
    if (!name.empty() && name.back() == '.') {
        name.pop_back();
    }
    entry.name = name;
    return entry;
}

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

/* The parts of a path (\\ or / between them; . is none, .. the one before
 * goes), none when .. goes above the root. */
std::optional<std::vector<std::string>> partsOf(const std::string &path)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= path.size()) {
        std::size_t end = path.find_first_of("\\/", start);
        if (end == std::string::npos) {
            end = path.size();
        }
        const std::string part = path.substr(start, end - start);
        start = end + 1;
        if (part.empty() || part == ".") {
            continue;
        }
        if (part == "..") {
            if (parts.empty()) {
                return std::nullopt;
            }
            parts.pop_back();
            continue;
        }
        parts.push_back(part);
    }
    return parts;
}

}

DosStamp isoDosStamp(std::string_view recorded)
{
    const int year = 1900 + static_cast<unsigned char>(recorded[0]);
    if (year < 1980) {
        return {0, (1U << 5) | 1U};
    }
    if (year > 2107) {
        return {(23U << 11) | (59U << 5) | 29U, (127U << 9) | (12U << 5) | 31U};
    }
    const unsigned month = static_cast<unsigned char>(recorded[1]);
    const unsigned day = static_cast<unsigned char>(recorded[2]);
    const unsigned hour = static_cast<unsigned char>(recorded[3]);
    const unsigned minute = static_cast<unsigned char>(recorded[4]);
    const unsigned second = static_cast<unsigned char>(recorded[5]);
    return {static_cast<std::uint16_t>(hour << 11 | minute << 5 | (second / 2)),
            static_cast<std::uint16_t>(static_cast<unsigned>(year - 1980) << 9 | month << 5 | day)};
}

std::optional<IsoEntry> isoRoot(std::string_view descriptor)
{
    if (descriptor.size() < ISO_SECTOR || descriptor[0] != 1 || descriptor.substr(1, 5) != "CD001" ||
        word16(descriptor, 128) != ISO_SECTOR) {
        return std::nullopt;
    }
    IsoEntry root = entryOf(descriptor.substr(156, 34));
    root.name.clear();
    return root;
}

std::vector<IsoEntry> isoEntries(std::string_view extent)
{
    std::vector<IsoEntry> entries;
    std::size_t at = 0;
    while (at < extent.size()) {
        const std::size_t length = static_cast<unsigned char>(extent[at]);
        if (length == 0) {
            /* The rest of the sector is empty: the next record starts the next. */
            at = (at / ISO_SECTOR + 1) * ISO_SECTOR;
            continue;
        }
        if (length < 34 || at + length > extent.size()) {
            break;
        }
        const std::string_view record = extent.substr(at, length);
        at += length;
        /* Its own (0) and its parent (1). */
        if (static_cast<unsigned char>(record[32]) == 1 &&
            (static_cast<unsigned char>(record[33]) == 0 || static_cast<unsigned char>(record[33]) == 1)) {
            continue;
        }
        entries.push_back(entryOf(record));
    }
    return entries;
}

std::optional<IsoImage> IsoImage::open(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::string descriptor(ISO_SECTOR, '\0');
    file.seekg(static_cast<std::streamoff>(ISO_DESCRIPTOR) * ISO_SECTOR);
    if (!file.read(descriptor.data(), ISO_SECTOR)) {
        return std::nullopt;
    }
    std::optional<IsoEntry> root = isoRoot(descriptor);
    if (!root) {
        return std::nullopt;
    }
    return IsoImage(path, *root);
}

std::string IsoImage::read(std::uint32_t sector, std::uint32_t size) const
{
    std::ifstream file(path_, std::ios::binary);
    std::string bytes(size, '\0');
    file.seekg(static_cast<std::streamoff>(sector) * ISO_SECTOR);
    file.read(bytes.data(), size);
    bytes.resize(static_cast<std::size_t>(file.gcount()));
    return bytes;
}

const std::vector<IsoEntry> &IsoImage::entriesOf(const IsoEntry &directory)
{
    auto found = directories_.find(directory.sector);
    if (found == directories_.end()) {
        found = directories_.emplace(directory.sector, isoEntries(read(directory.sector, directory.size))).first;
    }
    return found->second;
}

std::optional<IsoEntry> IsoImage::find(const std::string &path)
{
    const std::optional<std::vector<std::string>> parts = partsOf(path);
    if (!parts) {
        return std::nullopt;
    }
    IsoEntry at = root_;
    for (const std::string &part : *parts) {
        if (!at.directory) {
            return std::nullopt;
        }
        const std::vector<IsoEntry> &entries = entriesOf(at);
        const auto next = std::find_if(entries.begin(), entries.end(),
                                       [&](const IsoEntry &entry) { return sameName(entry.name, part); });
        if (next == entries.end()) {
            return std::nullopt;
        }
        at = *next;
    }
    return at;
}

std::optional<std::vector<IsoEntry>> IsoImage::list(const std::string &path)
{
    const std::optional<IsoEntry> directory = find(path);
    if (!directory || !directory->directory) {
        return std::nullopt;
    }
    return entriesOf(*directory);
}
