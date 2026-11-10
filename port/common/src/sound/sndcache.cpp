/* SNDCACHE.C: the sound effects in memory. See sndcache.h. */
#include "gamestate.h"
#include "sndcache.h"
#include "sndcache_game.h"
#include "asylum.h"
#include "cprotmem.h"
#include "desc.h"
#include "sndmgr.h"
#include "clock.h"

u32 allocateBlock(u32 size, i16 slot);
void freeBlocks(i16 slot);

GumpList *&blocks = gameGlobal<GumpList *>(nullptr);
Flex *&cacheFlex = gameGlobal<Flex *>(nullptr);
/* Cleared with each new archive; nothing reads it. */
u8 &cacheFlexSet = gameGlobal<u8>(0);
void *&cacheMemory = gameGlobal<void *>(nullptr);
/* The slots' pointers and the sound each one holds (-1: none). */
void **&slots = gameGlobal<void **>(nullptr);
i16 *&slotSounds = gameGlobal<i16 *>(nullptr);
i16 &blockCount = gameGlobal<i16>(0);
/* Nothing uses it. */
i16 &unusedCache = gameGlobal<i16>(0);
u32 &cacheSize = gameGlobal<u32>(0);
u32 &cacheUsed = gameGlobal<u32>(0);
i16 &slotCount = gameGlobal<i16>(0);
/* The last sound getSound found and its slot. */
i16 &lastSound = gameGlobal<i16>(0);
i16 &lastSlot = gameGlobal<i16>(0);
SlotLink **&slotLinks = gameGlobal<SlotLink **>(nullptr);
LinkList &slotOrder = gameGlobal<LinkList>();

namespace {
/* The port: a slot points at its sound in the memory, offset bytes on,
 * and has its size. The original's pointer was a selector of its own,
 * whose base (a linear address, cacheBase and on) moved with the sound
 * while the pointer stayed, and whose limit was the sound's; the port's
 * pointer moves, and the channels that play the sound follow the slot
 * (lastSoundSlot). */
u32 *&slotSizes = gameGlobal<u32 *>(nullptr);

void setSlotOffset(i16 slot, u32 offset)
{
    slots[slot] = static_cast<u8 *>(cacheMemory) + offset;
}

void setSlotLimit(i16 slot, u32 limit)
{
    slotSizes[slot] = limit + 1;
}
}

void initSoundCache(i16 count)
{
    i16 i;
    [[maybe_unused]] void *slot;
    u32 total;
    u32 largest;

    slotCount = count;
    slots = new void *[slotCount];
    slotSizes = new u32[slotCount];
    for (i = 0; i < slotCount; i++) {
        /* The slot's selector, of the pool of descriptors (DESC.C), whose
         * free count the memory status shows. */
        poolFree--;
        setSlotOffset(i, 0);
        slot = slots[i];
        setSlotLimit(i, 0);
    }
    /* All of the sound fragment's largest free block but the game's
     * SOUND_CACHE_SPARE bytes. */
    fragments[SOUND_FRAGMENT].freeSpace(&total, &largest);
    cacheSize = largest - SOUND_CACHE_SPARE;
    cacheMemory = allocateMemory(cacheSize, 0, SOUND_FRAGMENT);
    if (cacheMemory == nullptr) {
        halt();
    }
    slotOrder.clear();
    slotLinks = new SlotLink *[slotCount];
    slotSounds = new i16[slotCount];
    blocks = new GumpList;
    resetSoundCache();
}

void closeSoundCache()
{
    resetSoundCache();
    if (slots != nullptr) {
        delete[] slots;
        slots = nullptr;
        delete[] slotSizes;
        slotSizes = nullptr;
    }
    if (slotSounds != nullptr) {
        delete[] slotSounds;
        slotSounds = nullptr;
    }
    if (blocks != nullptr) {
        delete blocks;
        blocks = nullptr;
    }
    if (slotLinks != nullptr) {
        delete[] slotLinks;
        slotLinks = nullptr;
    }
    /* The original's farfree, which is CPROTMEM.C's releaseMemory. */
    if (cacheMemory != nullptr) {
        releaseMemory(cacheMemory, 0);
        cacheMemory = nullptr;
    }
}

void setSoundFlex(Flex *flex)
{
    cacheFlex = flex;
    cacheFlexSet = 0;
}

/* Empties every slot. */
void resetSoundCache()
{
    i16 i;

    if (cacheMemory == nullptr) {
        return;
    }
    SlotLink *link = nullptr;
    slotOrder.clear();
    for (i = 0; i < slotCount; i++) {
        link = new SlotLink(i);
        slotOrder.append(link);
        slotLinks[i] = link;
    }
    /* The original tests each slot's pointer, a selector, never 0. */
    for (i = 0; i < slotCount; i++) {
        setSlotOffset(i, 0);
        setSlotLimit(i, 0);
        slotSounds[i] = -1;
    }
    blocks->clear();
    blockCount = 1;
    SoundBlock *block = new SoundBlock;
    block->slot = -1;
    block->size = cacheSize;
    block->offset = 0;
    blocks->prepend(block);
    cacheUsed = 0;
    lastSound = -1;
}

namespace {
/* Stops the channels that play the slot's sound and frees the slot. */
void releaseSlot(i16 slot)
{
    if (slotSounds[slot] == -1) {
        return;
    }
    stopSoundNumber(slotSounds[slot]);
    /* The port's: also a channel that SNDMGR.C has lost while it plays the
     * sound (its restartSound of a sound whose end has come, which
     * updateChannels then forgets), which would go on in the slot's next
     * sound: past its end the original faulted. */
    ASS_StopSlotSFX(&slots[slot]);
    freeBlocks(slot);
    setSlotOffset(slot, 0);
    setSlotLimit(slot, 0);
    slotSounds[slot] = -1;
}
}

void *getSound(i16 sound)
{
    if (lastSound == sound) {
        return slots[lastSlot];
    }
    i16 slot = -1;
    SlotLink *link = nullptr;
    i16 i;

    for (i = 0; i < slotCount; i++) {
        if (slotSounds[i] == sound) {
            slot = i;
            link = slotLinks[slot];
            break;
        }
    }
    i = 0;
    if (slot == -1) {
        link = static_cast<SlotLink *>(slotOrder.first);
        slot = link->slot;
        releaseSlot(slot);
        const u32 size = cacheFlex->objectSize(sound);
        if (size == 0) {
            return nullptr;
        }
        /* Frees the least recently used slots until the sound fits. */
        while (cacheUsed + size > cacheSize) {
            slotOrder.raise(link);
            if (i == slotCount) {
                halt();
            }
            i++;
            link = static_cast<SlotLink *>(slotOrder.first);
            slot = link->slot;
            releaseSlot(slot);
        }
        setSlotLimit(slot, size - 1);
        setSlotOffset(slot, allocateBlock(size, slot));
        slotSounds[slot] = sound;
        cacheFlex->readObject(sound, slots[slot], -1, 0);
    }
    slotOrder.raise(link);
    lastSound = sound;
    lastSlot = slot;
    return slots[slot];
}

void *const *lastSoundSlot()
{
    return &slots[lastSlot];
}

const u32 *lastSoundSize()
{
    return &slotSizes[lastSlot];
}

/* size bytes for slot: their offset in the memory. */
u32 allocateBlock(u32 size, i16 slot)
{
    if (cacheUsed + size > cacheSize) {
        halt();
    }
    SoundBlock *block = nullptr;
    SoundBlock *rest = nullptr;

    while (blocks->forward(reinterpret_cast<GumpLink **>(&block))) {
        if (block->slot != -1) {
            continue;
        }
        if (block->size < size) {
            continue;
        }
        if (block->size > size) {
            rest = new SoundBlock;
            rest->size = block->size - size;
            rest->slot = -1;
            rest->offset = block->offset + size;
            blocks->insertAfter(block, rest);
        }
        blockCount++;
        block->slot = slot;
        block->size = size;
        cacheUsed += size;
        return block->offset;
    }
    compactBlocks();
    block = static_cast<SoundBlock *>(blocks->last);
    if (block->slot == -1 && block->size >= size) {
        if (block->size > size) {
            rest = new SoundBlock;
            rest->size = block->size - size;
            rest->slot = -1;
            rest->offset = block->offset + size;
            blockCount++;
            blocks->append(rest);
        }
        block->slot = slot;
        block->size = size;
        cacheUsed += size;
        return block->offset;
    }
    halt();
    return 0;
}

void freeBlocks(i16 slot)
{
    SoundBlock *block = nullptr;
    SoundBlock *next;

    while (blocks->forward(reinterpret_cast<GumpLink **>(&block))) {
        if (block->slot != slot) {
            continue;
        }
        block->slot = -1;
        cacheUsed -= block->size;
        next = static_cast<SoundBlock *>(block->next);
        if (next != nullptr && next->slot == -1) {
            block->size += next->size;
            blocks->destroy(next);
            blockCount--;
        }
        next = static_cast<SoundBlock *>(block->previous);
        if (next != nullptr && next->slot == -1) {
            next->size += block->size;
            blocks->destroy(block);
            blockCount--;
        }
        return;
    }
    halt();
}

void compactBlocks()
{
    /* No Remorse's original goes on without an archive. */
    if (cacheFlex == nullptr) {
        return;
    }
    u32 end;
    SoundBlock *block = nullptr;
    SoundBlock *free = nullptr;
    SoundBlock *next = nullptr;

    while (blocks->forward(reinterpret_cast<GumpLink **>(&block))) {
        if (block->previous == nullptr) {
            continue;
        }
        if (block->slot == -1) {
            continue;
        }
        free = static_cast<SoundBlock *>(block->previous);
        if (free->slot != -1) {
            continue;
        }
        /* Moves the sound down into the free piece before it. */
        clockPushHold();
        clockHold();
        memmove(static_cast<u8 *>(cacheMemory) + free->offset, static_cast<u8 *>(cacheMemory) + block->offset,
                block->size);
        block->offset = free->offset;
        setSlotOffset(block->slot, block->offset);
        clockPopHold();
        blocks->detach(free);
        end = block->offset + block->size;
        free->offset = end;
        blocks->insertAfter(block, free);
        while (free->next != nullptr && (static_cast<SoundBlock *>(free->next))->slot == -1) {
            next = static_cast<SoundBlock *>(free->next);
            free->size += next->size;
            blocks->destroy(next);
            blockCount--;
        }
    }
}
