/* Code18 (1088, CACHE\cacheblk.asm): the blocks of the cache's memory.
 * Every block starts with a BlockHeader; its data follows. Used blocks are
 * also in the LRU list (the block used last right after usedList), free
 * blocks in the free list after freeList. The members of CacheBlock are in
 * assembly. */
#ifndef PORT_CACHEBLK_H
#define PORT_CACHEBLK_H
#include "common.h"

/* BlockHeader::handle: a free block, one of the two heads (usedList and
 * freeList), and a used block without a slot in the handle table. The used
 * blocks have handles below FREE_HANDLE. */
constexpr int NO_HANDLE = 0xfffd;
constexpr int FREE_HANDLE = 0xfffe;
constexpr int HEAD_HANDLE = 0xffff;
/* BlockHeader::handler of the free blocks and the heads. */
constexpr int FREE_HANDLER = 0xff;

/* The header of a block, at its offset in the cache's memory. The list
 * fields are the offsets of other headers. */
struct __attribute__((packed)) BlockHeader {
    i16 handler;   /* its number in registerCacheHandler */
    u16 handle;    /* the slot in cacheHandles */
    s32 size;      /* the data after the header */
    u8 slack;      /* the bytes it holds beyond what was asked for */
    u32 unknown09; /* no references */
    u32 nextInMemory;
    u32 previousInMemory;
    u32 next; /* in the LRU list or the free list */
    u32 previous;
};
ASSERT_FILE_SIZE(BlockHeader, 0x1d);

/* A block: the offset of its header in the cache's memory. */
class CacheBlock {
public:
    u32 address;

    CacheBlock() = default;
    // NOLINTNEXTLINE(google-explicit-constructor): a block is its offset, as the original's
    CacheBlock(u32 at) { address = at; }
    /* Puts the block at the front of the LRU list (1088:0000). */
    void linkUsed() const;
    /* Moves a used block to the front of the LRU list (1088:0046). */
    void touch() const;
    /* Takes the block out of the LRU list or the free list (1088:0098). */
    void unlink() const;
    /* Takes the block out of the list in memory; no references (1088:00cf). */
    void unlinkMemory() const;
    /* Puts the block after previous in memory (1088:0106). */
    void insertAfter(CacheBlock previous) const;
    /* Takes the place of old in the LRU list or the free list (1088:013c). */
    void replace(CacheBlock old) const;
    /* Puts the block at the front of the free list (1088:0181). */
    void linkFree() const;
    /* Takes next, the block after this one, out of the list in memory
     * (1088:01c8). */
    void absorb(CacheBlock next) const;
    /* Points the block's neighbours in both lists at moved (1088:01f9). */
    void relink(CacheBlock moved) const;
    /* Moves the block back over the free block before it, to moved: the free
     * block leaves both of its lists (1088:023e). */
    void slideBack(CacheBlock moved) const;
    /* Makes the two heads and the first free block (1088:02af). */
    static void initLists();
    [[nodiscard]] u32 unknown09() const;              /* 1088:037e */
    [[nodiscard]] s32 size() const;                   /* 1088:03a0 */
    [[nodiscard]] u32 next() const;                   /* 1088:03c2 */
    [[nodiscard]] u32 previous() const;               /* 1088:03e4 */
    [[nodiscard]] u32 nextInMemory() const;           /* 1088:0406 */
    [[nodiscard]] u32 previousInMemory() const;       /* 1088:0428 */
    [[nodiscard]] u16 handle() const;                 /* 1088:044a */
    [[nodiscard]] u16 slack() const;                  /* 1088:0467 */
    [[nodiscard]] i16 handler() const;                /* 1088:0485 */
    void setSize(s32 size) const;                     /* 1088:04a1 */
    void setUnknown09(u32 value) const;               /* 1088:04c3 */
    void setNextInMemory(CacheBlock block) const;     /* 1088:04e5 */
    void setPreviousInMemory(CacheBlock block) const; /* 1088:0507 */
    void setHandler(i16 handler) const;               /* 1088:0529 */
    void setSlack(u8 slack) const;                    /* 1088:0548 */
    void setHandle(u16 handle) const;                 /* 1088:0568 */
    /* Moves the block distance bytes down; no references (1088:0588). */
    [[nodiscard]] i16 moveDown(s32 distance) const;
    /* Moves only the header distance bytes down: the block grows at its
     * front (1088:0617). */
    void slideHeader(s32 distance) const;
    /* Frees the block (freeBlock; inline in cache.h). */
    void release();
    [[nodiscard]] bool isUsed() const { return handle() < FREE_HANDLE; }
    [[nodiscard]] bool isFree() const { return handle() == FREE_HANDLE; }
    [[nodiscard]] bool isHead() const { return handle() == HEAD_HANDLE; }
};
ASSERT_SIZE(CacheBlock, 4);
#endif
