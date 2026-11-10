/* FILE\FLEX.C (1258:0000-09a9): FLEX archives. See flex.h. */
#include <cstddef>
#include <cstdio>
#include <cstring>
#include "common.h"
#include "utility.h"
#include "file.h"
#include "flex.h"

/* UNCALLED: nothing calls it. */
bool FlexEntry::operator==(FlexEntry &other)
{
    return memcmp(this, &other, sizeof(FlexEntry)) == 0;
}

void Flex::create(const char *path, i16 entryCount)
{
    if (entryCount == -1) {
        halt();
    }
    this->count = entryCount;
    File::open(path, FILE_READWRITE, 1);
    FlexHeader header;

    header.count = entryCount;
    write(&header, sizeof(header), 0L);
    FlexEntry entry;
    i16 i;

    entry.size = 0;
    entry.offset = 0;
    for (i = 0; i < this->count; i++) {
        write(&entry, sizeof(entry), -1L);
    }
}

void Flex::createInMemory(i16 entryCount)
{
    if (entryCount == -1) {
        halt();
    }
    this->count = entryCount;
    File::createInMemory();
    FlexHeader header;

    header.count = entryCount;
    write(&header, sizeof(header), 0L);
    FlexEntry entry;

    entry.size = 0;
    entry.offset = 0;
    for (i16 i = 0; i < this->count; i++) {
        write(&entry, sizeof(entry), -1L);
    }
}

Flex::Flex(const char *path, u16 openMode, i16 entryCount)
{
    open(path, openMode, entryCount);
}

void Flex::open(const char *path, u16 openMode, i16 entryCount)
{
    const u8 exists = fileExists(path);

    if (openMode == FILE_READ && (exists == 0U)) {
        halt();
    } else if (openMode == FILE_READWRITE && (exists == 0U)) {
        create(path, entryCount);
        return;
    } else {
        File::open(path, openMode, 0);
    }
    readCount();
}

void Flex::readEntry(i16 index, FlexEntry *entry)
{
    if (count <= index) {
        halt();
    } else if (read(entry, sizeof(FlexEntry), ENTRY_POSITION(index)) != static_cast<ui16>(sizeof(FlexEntry))) {
        halt();
    }
}

void Flex::writeEntry(i16 index, FlexEntry *entry)
{
    write(entry, sizeof(FlexEntry), ENTRY_POSITION(index));
}

s32 Flex::objectSize(i16 index)
{
    if (count < index) {
        halt();
    }
    FlexEntry entry;

    readEntry(index, &entry);
    return entry.size;
}

void Flex::readObject(i16 index, void *buffer, s32 length, s32 offset)
{
    FlexEntry entry;
    char fileName[80];

    readEntry(index, &entry);
    if (length == -1) {
        length = entry.size;
    }
    if (entry.size != 0) {
        if (read(buffer, length, entry.offset + offset) != length) {
            strcpy(fileName, name);
            haltMessage(haltFormat("Flex Read Error: %s\n\rR:%d L:%d O:%d", fileName, index, length, offset));
        }
    }
}

void Flex::writeObject(i16 index, void *buffer, s32 length, s32 offset, u8 resize)
{
    FlexEntry entry;

    readEntry(index, &entry);
    if (entry.offset == 0) {
        if (offset != 0) {
            halt();
        }
        entry.offset = size();
        entry.size = length;
        writeEntry(index, &entry);
    }
    if ((resize != 0U) && length + offset != entry.size) {
        resizeObject(index, &entry, length + offset);
    }
    if (length != 0) {
        write(buffer, length, entry.offset + offset);
    }
    commit();
}

void Flex::resizeObject(i16 index, FlexEntry *entry, s32 length)
{
    if (entry->size < length) {
        entry->offset = size();
    }
    entry->size = length;
    writeEntry(index, entry);
}

void Flex::seekObject(i16 index)
{
    FlexEntry entry;

    readEntry(index, &entry);
    seek(entry.offset, SEEK_SET);
}

void Flex::writeTitle(char *title)
{
    char text[82];

    memset(text, 0x1a, sizeof(text));
    strncpy(text, title, 79);
    write(title, sizeof(text), 0L);
}

void Flex::readTitle(char *title)
{
    read(title, 80L, 0L);
}

void Flex::writeHeader(FlexHeader *header)
{
    write(header, sizeof(FlexHeader), 0L);
    count = header->count;
}

void Flex::readHeader(FlexHeader *header)
{
    read(header, sizeof(FlexHeader), 0L);
    count = header->count;
}

void Flex::readCount()
{
    read(&count, sizeof(count), offsetof(FlexHeader, count));
}

void Flex::moveObject(i16 index, s32 position)
{
    const s32 end = size();

    if (position == -1) {
        position = end;
    }
    FlexEntry entry;

    readEntry(index, &entry);
    if (entry.size != 0) {
        if (position == end && entry.offset + entry.size == end) {
            return;
        }
        copy(entry.offset, position, entry.size, nullptr);
    }
    entry.offset = position;
    writeEntry(index, &entry);
    commit();
}

/* UNCALLED: nothing calls it. */
void Flex::copyObject(i16 index, Flex *target, i16 targetIndex)
{
    if (targetIndex == -1) {
        targetIndex = index;
    }
    FlexEntry from;
    FlexEntry to;
    s32 fromOffset;
    s32 toOffset;

    readEntry(index, &from);
    target->readEntry(targetIndex, &to);
    target->resizeObject(targetIndex, &to, from.size);
    fromOffset = from.offset;
    toOffset = to.offset;
    target->copy(fromOffset, toOffset, from.size, this);
    target->commit();
}

s32 Flex::checksum()
{
    const u16 chunk = 65000U;
    char *buffer = new char[chunk];

    const s32 end = size() - static_cast<ui16>(sizeof(FlexHeader));
    s32 position = sizeof(FlexHeader);
    s32 sum = 0;
    s32 length;
    u16 i;

    seek(position, SEEK_SET);
    while (position < end) {
        length = chunk > end - position ? end - position : chunk;
        read(buffer, length, -1L);
        for (i = 0; i < length; i++) {
            sum += buffer[i];
        }
        position += length;
    }
    delete[] buffer;
    return sum;
}

i16 Flex::verify(s32 *sum)
{
    const s32 computed = checksum();
    FlexHeader header;

    readHeader(&header);
    if (sum != nullptr) {
        *sum = computed;
    }
    return static_cast<i16>(header.checksum == computed);
}

/* UNCALLED: nothing calls it. */
bool Flex::isValid()
{
    const s32 end = size();
    i16 i;

    for (i = 0; i < count; i++) {
        FlexEntry entry;

        readEntry(i, &entry);
        if (entry.size + entry.offset > end) {
            return false;
        }
    }
    return true;
}

FlexHeader::FlexHeader()
{
    memset(this, 0, sizeof(FlexHeader));
    memset(title, 0x1a, sizeof(title));
    version = 1;
}
