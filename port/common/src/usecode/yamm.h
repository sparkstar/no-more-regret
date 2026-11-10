/* YAMM.C (1400): the usecode's memory. One block of YAMM_SIZE bytes holds a
 * ring of chunks, each an 8-byte header and its data; everything refers to a
 * chunk by the offset of its data. A chunk belongs to a process (its owner),
 * so the chunks of a process can be freed when it ends. The usecode lists
 * are chains of such chunks. */
#ifndef PORT_YAMM_H
#define PORT_YAMM_H
#include "common.h"
#include "flex.h"

constexpr u32 YAMM_SIZE = 64000U;
/* The size bit of a free chunk. */
constexpr int YAMM_FREE = 0x8000;

struct YammChunk {
    /* Offsets of the neighbouring chunks in the ring; the first chunk (at 1)
     * is empty and closes it. */
    u16 next;
    u16 prev;
    u16 owner;
    /* The data size, with YAMM_FREE. */
    u16 size;

    [[nodiscard]] u16 length() const { return size & ~YAMM_FREE; }
    [[nodiscard]] u8 isFree() const { return ((size & YAMM_FREE) != 0) ? 1 : 0; }
    void markFree()
    {
        size |= YAMM_FREE;
        owner = 0;
    }
    void markUsed() { size &= ~YAMM_FREE; }
    void grow(u16 amount) { size += amount; }
};
ASSERT_SIZE(YammChunk, 8);

/* The chunk at offset, and the data of a list node. */
#define YAMM_CHUNK(offset) ((YammChunk *)(yammHeap + (offset)))
#define NODE_DATA(node) (yammHeap + (ui16)((node) + 2))

/* A usecode list: nodes of a next offset and an element of size bytes, or of
 * a string's offset when strings is set. */
class YammList {
public:
    u16 head;
    u16 size;
    u8 strings;

    YammList() { head = 0; }
    YammList(u16 first, u16 elementSize, i16 stringList)
    {
        head = first;
        size = elementSize;
        strings = stringList;
    }
    /* Makes this the list at first (INTER.C). */
    void attach(u16 first, u16 elementSize, i16 stringList)
    {
        head = first;
        size = elementSize;
        strings = stringList;
    }
    /* Frees the nodes (and their strings). */
    void clear() const; /* 1400:0602 */
    /* Moves node to the next node (the first for 0); 0 at the end. */
    u8 next(u16 *node) const; /* 1400:0651 */
    /* Makes this a copy of from and returns its head. */
    u16 copy(YammList *from); /* 1400:0694 */
    /* The element at index, or 0. */
    [[nodiscard]] void *at(i16 index) const; /* 1400:07f2 */
    char *string(u16 node);                  /* 1400:0860 */
    void append(u16 node);                   /* 1400:087b */
    /* Appends copies of the elements of the list at first that it lacks. */
    void merge(u16 first); /* 1400:08c3 */
    /* Removes the elements of the list at first. */
    void remove(u16 first);                 /* 1400:0a9a */
    i16 contains(void *value);              /* 1400:0c31 */
    void set(i16 index, void *value) const; /* 1400:0cf6 */
    void setOwner(u16 owner) const;         /* 1400:0d7a */
};
ASSERT_SIZE(YammList, 5);

extern u8 *&yammHeap;
/* The owner of new chunks: the process being run. */
extern u16 &yammOwner;

/* Gives the chunk of the data at offset to owner. */
inline void setChunkOwner(u16 offset, u16 owner)
{
    (reinterpret_cast<YammChunk *>(yammHeap + offset) - 1)->owner = owner;
}

void initYamm(); /* 1400:0000 */
void freeYamm(); /* 1400:00bf */
/* A copy of text; its offset, or 0. */
u16 yammString(const char *text); /* 1400:00e5 */
/* The offset of size new bytes; halts when there is no room. */
u16 yammAlloc(u16 size);       /* 1400:013e */
void yammFreeOwner(u16 owner); /* 1400:0251 */
void yammFree(u16 offset);     /* 1400:02ac */
/* UNCALLED: nothing calls it. */
void yammDump(); /* 1400:03a3 */
/* Object holds the chunk count, object+1 the memory. */
void loadYamm(Flex *flex, i16 object); /* 1400:0501 */
void saveYamm(Flex *flex, i16 object); /* 1400:05ac */

void yammFailure(i16 check, std::source_location place);

/* Nothing calls these, in the original either. */
void yammCheck(std::source_location place = CALL_PLACE);
/* UNCALLED: nothing calls it. */
void printLongs(u16 head);
/* UNCALLED: nothing calls it. */
void printInts(u16 head);
/* UNCALLED: nothing calls it. */
void printStrings(u16 head);
/* UNCALLED: nothing calls it. */
void showYamm();
/* UNCALLED: nothing calls it. */
void showYammWait();
#endif
