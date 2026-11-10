/* FILE\TAGFLEX.C (1250:0000-1427, No Remorse only). See tagflex.h.
 *
 * Object 0 holds the records one after another: in an archive titled
 * "LockTagFlex" each record whole (its tag and its name), in an older one
 * only the names. Record 0 is "INVALID". */
#include <cstdio>
#include <cstring>
#include "tagflex.h"
#include "clock.h"

namespace {
/* A record of bytes bytes: its tag and as much of the name as they hold
 * (TagRecord's name is as long as the longest; tagflex.h). */
TagRecord *newRecord(u32 bytes)
{
    return reinterpret_cast<TagRecord *>(new char[bytes]);
}
}

TagFlex::TagFlex(const char *path, u16 openMode, i16 objectCount) : EditableFlex(path, openMode, objectCount)
{
    records = nullptr;
    recordCount = 0;
    recordCapacity = 0x50;
    entryOrder = nullptr;
    recordBytes = nullptr;
    FlexEntry entry;

    readEntry(0, &entry);
    if ((entry.size == 0) || (entry.offset == 0)) {
        init(0xff);
    } else {
        load();
    }
    names = new char *[recordCount + 1];
    for (i16 i = 0; i < recordCount; i++) {
        names[i] = records[i]->name;
    }
    names[recordCount] = nullptr;
}

TagFlex::~TagFlex()
{
    if (mode != 0U) {
        FlexHeader header;

        readHeader(&header);
        strcpy(header.title, "LockTagFlex");
        writeHeader(&header);
        save();
    }
    release();
    /* The port's: the original leaves the names' table. */
    delete[] names;
    names = nullptr;
}

void TagFlex::load()
{
    FlexEntry entry;

    readEntry(0, &entry);
    if ((entry.size == 0) || (entry.offset == 0)) {
        return;
    }
    char *buffer = new char[static_cast<i16>(entry.size) + 1];

    readObject(0, buffer, -1L, 0L);
    buffer[entry.size] = 0;
    recordCount = count;
    records = new TagRecord *[recordCount + 1];
    entryOrder = new u16[recordCount + 1];
    recordBytes = new u16[recordCount + 1];
    for (i16 i = 0; i < static_cast<i16>(recordCount + 1); i++) {
        entryOrder[i] = i;
    }
    memset(records, 0, (recordCount + 1) * sizeof(TagRecord *));
    records[0] = newRecord(20);
    recordBytes[0] = 20;
    if (records[0] == nullptr) {
        allocationFailure();
    }
    records[0]->tag = 0;
    strcpy(records[0]->name, "INVALID");
    u32 index = 1;
    char *p = buffer;
    FlexHeader header;

    readHeader(&header);
    /* The buffer's block starts at offset 0 of its selector: p's offset is
     * how far it is into the buffer. */
    if (strcmp(header.title, "LockTagFlex") == 0) {
        for (; static_cast<s32>(p - buffer) < entry.size && index < static_cast<u32>(recordCount); index++) {
            const i16 length = strlen(p + 4) + 5;
            const i16 size = recordCapacity < length ? length : recordCapacity;

            records[index] = newRecord(size);
            recordBytes[index] = size;
            if (records[index] == nullptr) {
                allocationFailure();
            }
            memcpy(records[index], p, length);
            p += length;
        }
    } else {
        const s32 tag = clockWallTime();

        for (; static_cast<s32>(p - buffer) < entry.size && index < static_cast<u32>(recordCount); index++) {
            const i16 length = strlen(p) + 1;
            const i16 size = recordCapacity < static_cast<i16>(length + 4) ? length + 4 : recordCapacity;

            records[index] = newRecord(size);
            recordBytes[index] = size;
            if (records[index] == nullptr) {
                allocationFailure();
            }
            records[index]->tag = tag;
            memcpy(records[index]->name, p, length);
            p += length;
        }
    }
    records[recordCount] = nullptr;
    if (buffer != nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }
    sort();
}

void TagFlex::release()
{
    if (records != nullptr) {
        for (i16 i = 0; i < recordCount; i++) {
            if (records[i] != nullptr) {
                delete[] reinterpret_cast<char *>(records[i]);
                records[i] = nullptr;
            }
        }
        if (records != nullptr) {
            delete[] records;
            records = nullptr;
        }
    }
    if (entryOrder != nullptr) {
        delete[] entryOrder;
        entryOrder = nullptr;
    }
    if (recordBytes != nullptr) {
        delete[] recordBytes;
        recordBytes = nullptr;
    }
}

void TagFlex::init(i16 capacity)
{
    i16 i;

    recordCapacity = capacity;
    recordCount = count;
    release();
    records = new TagRecord *[recordCount + 1];
    entryOrder = new u16[recordCount + 1];
    recordBytes = new u16[recordCount + 1];
    for (i = 0; i < static_cast<i16>(recordCount + 1); i++) {
        entryOrder[i] = i;
    }
    const s32 tag = clockWallTime();

    for (i = 0; i < recordCount; i++) {
        records[i] = newRecord(recordCapacity);
        recordBytes[i] = recordCapacity;
        if (records[i] == nullptr) {
            allocationFailure();
        }
        records[i]->tag = tag;
        memset(records[i]->name, 0, recordCapacity - 4);
    }
    records[0]->tag = 0;
    strcpy(records[0]->name, "INVALID");
    records[recordCount] = nullptr;
}

/* UNCALLED: nothing calls it. */
void TagFlex::setEntry(i16 index, char *newName, s32 tag)
{
    if (index < 1 || index >= recordCount) {
        return;
    }
    i16 i;

    for (i = 0; entryOrder[i] != static_cast<ui16>(index); i++) {
        ;
    }
    if ((records == nullptr) || (records[i] == nullptr)) {
        init(0xff);
    }
    /* The block's limit, one less than its bytes. */
    if (static_cast<u32>(recordBytes[i] - 1) < static_cast<ui16>(strlen(newName) + 5)) {
        if (records[i] != nullptr) {
            delete[] reinterpret_cast<char *>(records[i]);
            records[i] = nullptr;
        }
        records[i] = newRecord(strlen(newName) + 5);
        recordBytes[i] = static_cast<u16>(strlen(newName) + 5);
    }
    if (tag != 0) {
        records[i]->tag = tag;
    } else {
        const s32 now = clockWallTime();

        records[i]->tag = now;
    }
    strcpy(records[i]->name, newName);
}

/* UNCALLED: nothing calls it. */
char *TagFlex::entryName(i16 index) const
{
    if (index < 0 || index > recordCount) {
        return nullptr;
    }
    i16 i;

    for (i = 0; entryOrder[i] != static_cast<ui16>(index); i++) {
        ;
    }
    return records[i]->name;
}

/* UNCALLED: nothing calls it. */
s32 TagFlex::entryTag(i16 index) const
{
    if (index < 0 || index > recordCount) {
        return 0;
    }
    i16 i;

    for (i = 0; entryOrder[i] != static_cast<ui16>(index); i++) {
        ;
    }
    return records[i]->tag;
}

/* A record without a name is its tag and a 0: never, as name is the
 * record's own array (the original's test is always true). */
void TagFlex::save()
{
    if (records != nullptr) {
        char *buffer = new char[65000];
        char *p = buffer;

        i16 number;
        i16 i;
        i16 length;

        for (number = 1; number < recordCount; number++) {
            for (i = 1; i < recordCount; i++) {
                if (entryOrder[i] == static_cast<ui16>(number)) {
                    length = strlen(records[i]->name) + 5;
                    memcpy(p, records[i], length);
                    p += length;
                }
            }
        }
        replaceObject(0, buffer, static_cast<u16>(p - buffer), 0L);
        if (buffer != nullptr) {
            delete[] buffer;
            buffer = nullptr;
        }
    }
}

/* UNCALLED: nothing calls it. */
i16 TagFlex::find(char *wanted) const
{
    for (i16 i = 0; i < recordCount; i++) {
        if (strcmp(records[i]->name, wanted) == 0) {
            return entryOrder[i];
        }
    }
    return -1;
}

/* A selection sort from record 1 on; entryOrder moves along. */
void TagFlex::sort() const
{
    i16 best;
    i16 i;
    i16 j;
    u16 order;
    TagRecord record;

    if (records != nullptr) {
        for (i = 1; i < recordCount; i++) {
            best = i;
            record.tag = records[i]->tag;
            strcpy(record.name, records[i]->name);
            for (j = i + 1; j < recordCount; j++) {
                if ((strlen(records[j]->name) != 0U) &&
                    ((record.name[0] == 0) || strcmp(records[j]->name, record.name) < 0)) {
                    record.tag = records[j]->tag;
                    strcpy(record.name, records[j]->name);
                    best = j;
                }
            }
            if (i != best) {
                records[best]->tag = records[i]->tag;
                strcpy(records[best]->name, records[i]->name);
                records[i]->tag = record.tag;
                strcpy(records[i]->name, record.name);
                order = entryOrder[best];
                entryOrder[best] = entryOrder[i];
                entryOrder[i] = order;
            }
        }
    }
}

/* The last object is left out. The original's buffer comes from farmalloc
 * and goes back through delete; the port's is new[] and delete[]. */
/* UNCALLED: nothing calls it. */
void TagFlex::merge(TagFlex *source)
{
    const i16 objects = source->count;

    if (count == objects) {
        for (i16 i = 1; i < static_cast<i16>(objects - 1); i++) {
            const s32 theirs = source->entryTag(i);
            const s32 ours = entryTag(i);

            if (theirs > ours) {
                setEntry(i, source->entryName(i), source->entryTag(i));
                FlexEntry entry;
                FlexEntry own;

                source->readEntry(i, &entry);
                if (entry.size != 0) {
                    char *buffer = new char[entry.size];

                    source->readObject(i, buffer, -1L, 0L);
                    replaceObject(i, buffer, entry.size, 0L);
                    readEntry(i, &own);
                    own.size = entry.size;
                    writeEntry(i, &own);
                    if (buffer != nullptr) {
                        delete[] buffer;
                        buffer = nullptr;
                    }
                } else {
                    readEntry(i, &own);
                    own.size = 0;
                    writeEntry(i, &own);
                }
            }
        }
    }
}

/* UNCALLED: nothing calls it. */
void TagFlex::writeDefines(char *path, char *prefix) const
{
    if (records != nullptr) {
        char *text = new char[65000];

        *text = 0;
        for (i16 i = 1; i < recordCount; i++) {
            if ((records[i] != nullptr) && (strlen(records[i]->name) != 0U)) {
                char line[80];

#pragma GCC diagnostic push
                /* The original's 80 bytes are too few for a name past 60 or so,
                 * which the size cuts short; nothing calls writeDefines, and
                 * sound.flx's names are short. */
#pragma GCC diagnostic ignored "-Wformat-truncation"
                snprintf(line, sizeof(line), "#define %s%s\t\t\t%d\n", prefix, records[i]->name, entryOrder[i]);
#pragma GCC diagnostic pop
                strcat(text, line);
            }
        }
        const File file(path, FILE_READWRITE, 1);

        file.write(text, strlen(text));
        if (text != nullptr) {
            delete[] text;
            text = nullptr;
        }
    }
}

/* UNCALLED: nothing calls it. */
void TagFlex::writeConstants(char *path, char *prefix) const
{
    if (records != nullptr) {
        char *buffer = new char[65000];

        char *text = buffer;

        *text = 0;
        for (i16 i = 1; i < recordCount; i++) {
            if ((records[i] != nullptr) && (strlen(records[i]->name) != 0U)) {
                char line[80];

#pragma GCC diagnostic push
                /* As writeDefines's. */
#pragma GCC diagnostic ignored "-Wformat-truncation"
                snprintf(line, sizeof(line), "constant %s%s\t\t = %d;\r\n", prefix, records[i]->name, entryOrder[i]);
#pragma GCC diagnostic pop
                strcat(text, line);
            }
        }
        const File file(path, FILE_READWRITE, 1);

        file.write(buffer, strlen(buffer));
        if (buffer != nullptr) {
            delete[] buffer;
            buffer = nullptr;
        }
    }
}
