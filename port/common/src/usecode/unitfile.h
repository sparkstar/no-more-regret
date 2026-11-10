/* ..\\UNK\\UNITFILE.C (1408:0000-0298): the unit file, a FLEX whose object
 * 1 is read whole when it is opened and whose units are the objects from 2
 * on. */
#ifndef PORT_UNITFILE_H
#define PORT_UNITFILE_H
#include "common.h"
#include "fastflex.h"

#include <memory>

class UnitFile : public FastFlex {
public:
    /* Object 1. */
    /* Its own. */
    std::unique_ptr<u8[]> header;

    explicit UnitFile(const char *path);
    ~UnitFile() override;
    /* Reads length bytes at offset of a unit; halts on a unit that starts
     * with FFh. */
    void readUnit(i16 unit, void *buffer, s32 length, s32 offset);
    void readUnitEntry(i16 unit, FlexEntry *entry);
    /* Length bytes (the whole unit for -1) at offset of a unit in new
     * memory; 0 past the last unit. */
    /* UNCALLED: nothing calls it. */
    void *loadUnit(i16 unit, s32 length, s32 offset);
};
ASSERT_SIZE(UnitFile, 0x14);
#endif
