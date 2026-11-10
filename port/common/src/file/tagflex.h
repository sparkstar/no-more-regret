/* FILE\TAGFLEX.C (No Remorse only, 1250:0000-1427): an editable flex whose
 * objects carry a name and a tag, a time stamp, kept in object 0. DSFXMAN.C
 * opens sound.flx as one. The names are kept sorted; entryOrder maps each
 * record back to its object. */
#ifndef PORT_TAGFLEX_H
#define PORT_TAGFLEX_H
#include "common.h"
#include "flexedit.h"

/* The name and tag of an object. A record is only as long as its name
 * needs, recordCapacity at least; object 0's is 20 bytes, "INVALID" with tag
 * 0. sort keeps a whole one. */
struct TagRecord {
    s32 tag;
    char name[1020];
};
ASSERT_SIZE(TagRecord, 0x400);

class TagFlex : public EditableFlex {
public:
    /* One record an object, the last 0. */
    TagRecord **records;
    i16 recordCount;
    /* The smallest size of a record. */
    i16 recordCapacity;
    /* The object of each record. */
    u16 *entryOrder;
    /* Each record's name, the last 0. */
    char **names;
    /* The port's: the bytes of each record's block, which the original read
     * from the block's selector (its limit, one less). */
    u16 *recordBytes;

    /* Reads the records of object 0, or makes empty ones when it has none;
     * out of line, it allocates the 20h bytes itself (1250:0000). */
    TagFlex(const char *path, u16 openMode, i16 objectCount);
    /* An archive opened for writing gets the title "LockTagFlex" and its
     * records in object 0 (1250:01ba). */
    ~TagFlex() override;
    /* Reads the records from object 0; one titled "LockTagFlex" holds the
     * records, an older one only the names, which get the time as tag
     * (1250:026c). */
    void load();
    /* Frees the records (1250:067b). */
    void release();
    /* Makes a record of capacity bytes for each object, tagged with the time
     * (1250:0749). */
    void init(i16 capacity);
    /* Names object index; tag 0 takes the time (1250:0910). */
    /* UNCALLED: nothing calls it. */
    void setEntry(i16 index, char *newName, s32 tag);
    /* The name (1250:0ac9) and the tag (0b22) of object index, 0 when it has
     * none. */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] char *entryName(i16 index) const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] s32 entryTag(i16 index) const;
    /* Writes the records in the objects' order to object 0 (1250:0b7c). */
    void save();
    /* The object named name, -1 for none (1250:0ce5). */
    /* UNCALLED: nothing calls it. */
    i16 find(char *wanted) const;
    /* Sorts the records by name, empty names last (1250:0d45). */
    void sort() const;
    /* Nothing calls these, in the original either. */
    /* Takes the objects source has with a newer tag, when both have as many
     * objects (1250:0f7c). */
    /* UNCALLED: nothing calls it. */
    void merge(TagFlex *source);
    /* Writes each name with prefix and its object's number to path, as C
     * #defines (1250:1170) or as "constant" lines (12c8). */
    /* UNCALLED: nothing calls it. */
    void writeDefines(char *path, char *prefix) const;
    /* UNCALLED: nothing calls it. */
    void writeConstants(char *path, char *prefix) const;
};
ASSERT_SIZE(TagFlex, 0x20);
#endif
