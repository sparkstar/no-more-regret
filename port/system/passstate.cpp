/* The pass dump's game state (passdump.h's O and C records): saveGame's
 * objects without its work (writeGameState) in a FLEX in memory, and the
 * loaded functions' code, at each pass's end once the game's tables are
 * made. */
#include "passdump.h"
#include "fastflex.h"
#include "files.h"
#include "flag.h"
#include "itemdata.h"
#include "kernel.h"
#include "npc.h"
#include "report.h"
#include "savegame.h"
#include "tracker.h"
#include "uprocess.h"
#include "usemem.h"
#include "yamm.h"

#include <vector>

/* kernel.cpp's table of processes. */
extern Process **&processes;
extern u16 &processLimit;

namespace {

/* saveGame's FLEX: the count of its objects at 54h and their entries from
 * 80h, an offset and a size each; the kernel's objects from 100, two for
 * each process from 115 on. */
const std::size_t FLEX_COUNT = 0x54, FLEX_ENTRIES = 0x80;
const int KERNEL_OBJECTS = 100, PROCESS_OBJECTS = 15;

void add(std::vector<u8> &payload, u32 value, int bytes)
{
    for (int k = 0; k < bytes; k++) {
        payload.push_back(static_cast<u8>(value >> (8 * k)));
    }
}

/* The game's tables, which writeGameState reads, are made. */
bool stateMade()
{
    return (processes != nullptr) && (itemX != nullptr) && (npcs != nullptr) && (yammHeap != nullptr) &&
           (flags != nullptr) && (targets != nullptr);
}

u32 wordAt(const std::vector<u8> &bytes, std::size_t at)
{
    return static_cast<u32>(bytes[at]) | static_cast<u32>(bytes[at + 1]) << 8 | static_cast<u32>(bytes[at + 2]) << 16 |
           static_cast<u32>(bytes[at + 3]) << 24;
}

/* writeGameState's objects in a FLEX in memory, each an O record. */
void putObjects()
{
    FastFlex flex;
    flex.createInMemory(static_cast<i16>(KERNEL_OBJECTS + PROCESS_OBJECTS + (2 * processLimit)));
    writeGameState(&flex);
    const std::vector<u8> bytes = fileMemoryBytes(flex.handle);
    flex.close();
    const u32 count = wordAt(bytes, FLEX_COUNT);
    for (u32 object = 0; object < count; object++) {
        const u32 offset = wordAt(bytes, FLEX_ENTRIES + (8 * object));
        const u32 size = wordAt(bytes, FLEX_ENTRIES + (8 * object) + 4);
        if (size == 0) {
            continue;
        }
        if (static_cast<std::size_t>(offset) + size > bytes.size()) {
            portRefuse("REGRET_PASS_DUMP: object %u lies past the archive's end", object);
        }
        std::vector<u8> payload;
        add(payload, object, 2);
        payload.insert(payload.end(), bytes.begin() + offset, bytes.begin() + offset + size);
        passDumpRecord('O', payload);
    }
}

/* The loaded functions' code, each a C record. */
void putCode()
{
    if (classes == nullptr) {
        return;
    }
    for (GumpLink *link = classes->blocks.first; link != nullptr; link = link->next) {
        const CodeBlock *block = static_cast<const CodeBlock *>(link);
        std::vector<u8> payload;
        add(payload, usecodeMemory.regionOf(block->code.get()), 2);
        add(payload, block->classId, 2);
        add(payload, block->function, 2);
        passDumpRecord('C', payload);
    }
}

[[maybe_unused]] const bool STARTED = [] {
    passDumpStateHook = []() {
        if (!stateMade()) {
            return false;
        }
        putObjects();
        putCode();
        return true;
    };
    return true;
}();

}
