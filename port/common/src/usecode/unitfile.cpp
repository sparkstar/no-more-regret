/* ..\\UNK\\UNITFILE.C (1408:0000-0298): the unit file. See unitfile.h. */
#include "unitfile.h"

UnitFile::UnitFile(const char *path) : FastFlex(path, FILE_READ, -1)
{
    FlexEntry entry;

    readEntry(1, &entry);
    header = std::make_unique_for_overwrite<u8[]>(entry.size);
    if (header == nullptr) {
        allocationFailure();
    }
    readObject(1, header.get(), -1L, 0L);
}

/* header frees itself. */
UnitFile::~UnitFile() = default;

void UnitFile::readUnit(i16 unit, void *buffer, s32 length, s32 offset)
{
    readObject(unit + 2, buffer, length, offset);
    if (*static_cast<u8 *>(buffer) == 0xff) {
        halt();
    }
}

void UnitFile::readUnitEntry(i16 unit, FlexEntry *entry)
{
    readEntry(unit + 2, entry);
}

/* UNCALLED: nothing calls it. */
void *UnitFile::loadUnit(i16 unit, s32 length, s32 offset)
{
    if (static_cast<i16>(count - 2) < unit) {
        return nullptr;
    }
    FlexEntry entry;
    readEntry(unit + 2, &entry);
    if (entry.size <= 20) {
        halt();
    }
    if (length == -1) {
        length = entry.size;
    }
    void *buffer = new u8[length];
    readObject(unit + 2, buffer, length, offset);
    return buffer;
}
