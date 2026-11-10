/* MLIST.C (1200:0000-085e): a list of process ids. A process waits on a list
 * of ids and operators that ends with LIST_END; its relations list keeps one
 * entry per related process, the id with relation bits. */
#ifndef PORT_MLIST_H
#define PORT_MLIST_H
#include "common.h"

class File;

/* Entries with LIST_OPERATOR set are operators, not process ids. */
constexpr int LIST_OPERATOR = 0x400;
constexpr int LIST_TRUE = 0x401;
constexpr int LIST_AND = 0x402;
constexpr int LIST_OR = 0x404;
constexpr int LIST_END = 0x408;
/* The most entries a list built from arguments may have. */
constexpr int LIST_MAX = 0x400;
constexpr int PID_MASK = 0x3ff;
/* Relation bits of the entries of Process::relations: pid listens to the
 * process, the process listens to pid, waits on pid, pid waits on it. */
constexpr int RELATION_LISTENER = 0x800;
constexpr int RELATION_LISTENING = 0x1000;
constexpr int RELATION_WAITING = 0x2000;
constexpr int RELATION_WAITER = 0x4000;

class MListEntry {
public:
    u16 value;
    MListEntry() = default;
    // NOLINTNEXTLINE(google-explicit-constructor): an entry is its word, as the original's
    MListEntry(u16 entry) { value = entry; }
    [[nodiscard]] u8 isOperator() const { return ((value & LIST_OPERATOR) != 0) ? 1 : 0; }
    [[nodiscard]] bool isPid() const { return isOperator() == 0U; }
    [[nodiscard]] u16 pid() const { return value & PID_MASK; }
    [[nodiscard]] u8 has(u16 bits) const { return ((value & bits) != 0) ? 1 : 0; }
    [[nodiscard]] bool isRelated() const
    {
        return (has(RELATION_LISTENER) != 0U) || (has(RELATION_LISTENING) != 0U) || (has(RELATION_WAITING) != 0U) ||
               (has(RELATION_WAITER) != 0U);
    }
};

class MList {
public:
    u16 count;
    /* A value the owner keeps with the list. */
    u16 tag;
    u16 *data;
    MList()
    {
        count = 0;
        data = nullptr;
        tag = 0;
    }
    /* Virtual in the port, not in the original. */
    virtual ~MList();
    [[nodiscard]] bool valid() const { return data != nullptr; }
    u16 build(u16 owner, ...);
    void clear();
    u16 add(u16 entry);
    u16 append(u16 entry);
    u16 remove(u16 entry);
    [[nodiscard]] u16 get(u16 index) const;
    void put(u16 index, u16 entry) const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u16 find(u16 entry) const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] MListEntry findPid(u16 pid) const;
    MListEntry findPid(u16 pid, u16 *index) const;
    void allocate(u16 size);
    virtual void load(File *file);
    virtual void save(File *file);
};
ASSERT_SIZE(MListEntry, 2);
ASSERT_SIZE(MList, 0xa);
#endif
