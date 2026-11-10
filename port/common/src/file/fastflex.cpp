/* FILE\FASTFLEX.C: FLEX archives with the index in memory. See fastflex.h. */
#include "common.h"
#include "utility.h"
#include "fastflex.h"
#include "flex.h"

FastFlex::~FastFlex()
{
    close();
}

void FastFlex::open(const char *path, u16 openMode, i16 entryCount)
{
    entries.reset();
    if (fileExists(path) == 0) {
        if (openMode == FILE_READ) {
            /* The path, in both games: No Remorse's original named the
             * file's name, which is 0 until the file opens. */
            openFailure(path);
        } else {
            create(path, entryCount);
            entries = std::make_unique_for_overwrite<FlexEntry[]>(this->count);
            if (entries == nullptr) {
                allocationFailure();
            }
            memset(static_cast<void *>(entries.get()), 0, entryCount * sizeof(FlexEntry));
        }
    } else {
        Flex::open(path, openMode, entryCount);
        entries = std::make_unique_for_overwrite<FlexEntry[]>(this->count);
        if (entries == nullptr) {
            allocationFailure();
        }
        loadIndex();
    }
}

void FastFlex::createInMemory(i16 entryCount)
{
    entries.reset();
    Flex::createInMemory(entryCount);
    entries = std::make_unique_for_overwrite<FlexEntry[]>(this->count);
    if (entries == nullptr) {
        allocationFailure();
    }
    memset(static_cast<void *>(entries.get()), 0, entryCount * sizeof(FlexEntry));
}

void FastFlex::readEntry(i16 index, FlexEntry *entry)
{
    *entry = entries[index];
}

void FastFlex::writeEntry(i16 index, FlexEntry *entry)
{
    entries[index] = *entry;
    write(entry, sizeof(FlexEntry), ENTRY_POSITION(index));
}

void FastFlex::loadIndex()
{
    read(entries.get(), static_cast<ui16>(count * sizeof(FlexEntry)), sizeof(FlexHeader));
}
