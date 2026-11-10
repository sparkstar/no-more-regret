/* FILE\FLEX.C (Code76, 1258): Origin's FLEX archive, a file of numbered
 * objects. A 128-byte header (title, object count, version, checksum) is
 * followed by an index of FlexEntry records and the object data. */
#ifndef PORT_FLEX_H
#define PORT_FLEX_H
#include "common.h"
#include "file.h"

/* Where an object lies in the file; offset 0 means there is no object. */
class FlexEntry {
public:
    s32 offset;
    s32 size;
    FlexEntry()
    {
        size = 0;
        offset = 0;
    }
    /* UNCALLED: nothing calls it. */
    bool operator==(FlexEntry &other); /* 1258:0000 */
    /* The object is missing from the archive. */
    [[nodiscard]] i16 isEmpty() const { return static_cast<i16>(size == 0); }
};
class FlexHeader {
public:
    /* Padded with ^Z. */
    char title[82];
    u16 unknown52;
    s32 count;
    u32 version;
    u32 unknown5c;
    /* Flex::checksum() of the file when it was written. */
    s32 checksum;
    /* "AVI" in the movies (flics) with sound. */
    char format[4];
    u8 reserved[24];
    FlexHeader(); /* 1258:0952 */
};
class Flex : public File {
public:
    /* BCC 3.1 made the implicit destructor of a class with virtual functions
     * virtual when it has work to do, and a delete through this class called the
     * object's own. */
    virtual ~Flex() = default;
    i16 count;
    Flex() = default;
    Flex(const char *path, u16 openMode, i16 entryCount); /* 1258:00e7 */
    /* Writes a new empty archive of count objects. */
    void create(const char *path, i16 entryCount); /* 1258:0028 */
    /* create's archive in the host's memory (File::createInMemory): the
     * port's, for the pass dump. */
    void createInMemory(i16 entryCount);
    /* Reads the object count from the header. */
    void readCount(); /* 1258:05a8 */
    /* Moves the file position to the start of an object. */
    void seekObject(i16 index); /* 1258:049e */
    /* Copies an object into target's object targetIndex (index for -1). */
    /* UNCALLED: nothing calls it. */
    void copyObject(i16 index, Flex *target, i16 targetIndex); /* 1258:0691 */
    /* 0 when an object reaches past the end of the file. */
    /* UNCALLED: nothing calls it. */
    bool isValid(); /* 1258:08df */

    /* Opens path; FILE_READ needs the file, FILE_READWRITE creates it with
     * count objects. */
    virtual void open(const char *path, u16 openMode, i16 entryCount); /* 1258:0142 */
    virtual void readEntry(i16 index, FlexEntry *entry);               /* 1258:01c8 */
    virtual void writeEntry(i16 index, FlexEntry *entry);              /* 1258:0227 */
    /* Reads length bytes (the whole object for -1) at offset in the object. */
    virtual void readObject(i16 index, void *buffer, s32 length, s32 offset); /* 1258:02a8 */
    /* Writes length bytes at offset in the object, appending a new object at
     * the end of the file; with resize the object becomes offset+length
     * long. */
    virtual void writeObject(i16 index, void *buffer, s32 length, s32 offset, u8 resize); /* 1258:0354 */
    virtual void writeTitle(char *title);                                                 /* 1258:04e3 */
    virtual void readTitle(char *title);                                                  /* 1258:0529 */
    virtual void writeHeader(FlexHeader *header);                                         /* 1258:0548 */
    virtual void readHeader(FlexHeader *header);                                          /* 1258:0578 */
    /* Moves an object's data to position (the end of the file for -1). */
    virtual void moveObject(i16 index, s32 position); /* 1258:05cb */
    /* Sets the size of the object; a larger one moves to the end of the
     * file. */
    virtual void resizeObject(i16 index, FlexEntry *entry, s32 length); /* 1258:044a */
    /* The sum of all bytes after the header, as signed chars; -1 when no
     * buffer can be had. */
    virtual s32 checksum(); /* 1258:0756 */
    /* Whether the header's checksum is right; the computed sum goes to sum. */
    virtual i16 verify(s32 *sum);      /* 1258:0872 */
    virtual s32 objectSize(i16 index); /* 1258:0252 */
};
ASSERT_SIZE(FlexEntry, 0x8);
ASSERT_SIZE(FlexHeader, 0x80);
ASSERT_SIZE(Flex, 0xc);
/* Where an object's index entry lies in the file. */
#define ENTRY_POSITION(index) ((ui16)(((index) * sizeof(FlexEntry)) + sizeof(FlexHeader)))
#endif
