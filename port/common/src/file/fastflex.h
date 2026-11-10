/* FILE\FASTFLEX.C (Code72, 1238): a FLEX archive whose whole index is kept
 * in memory. */
#ifndef PORT_FASTFLEX_H
#define PORT_FASTFLEX_H
#include "common.h"
#include "flex.h"

#include <memory>

class FastFlex : public Flex {
public:
    /* The whole index, its own. */
    std::unique_ptr<FlexEntry[]> entries;
    FastFlex() = default;
    FastFlex(const char *path, u16 openMode, i16 entryCount) { open(path, openMode, entryCount); }
    /* Reads the index from the file. */
    void loadIndex();                                                   /* 1238:022b */
    void open(const char *path, u16 openMode, i16 entryCount) override; /* 1238:0070 */
    /* open's new archive in the host's memory (Flex::createInMemory): the
     * port's, for the pass dump. */
    void createInMemory(i16 entryCount);
    void readEntry(i16 index, FlexEntry *entry) override;  /* 1238:01b7 */
    void writeEntry(i16 index, FlexEntry *entry) override; /* 1238:01de */
    ~FastFlex() override;                                  /* 1238:0000 */
};
ASSERT_SIZE(FastFlex, 0x10);
#endif
