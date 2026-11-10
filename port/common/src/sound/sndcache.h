/* SNDCACHE.C: the sound effects in memory. One block of
 * protected memory holds them, cut into pieces on a list; each of a number
 * of slots maps one sound (the original through a selector of its own).
 * The least recently used slot gives way, and the pieces are packed
 * together when no free one is large enough. */
#ifndef PORT_SNDCACHE_H
#define PORT_SNDCACHE_H
#include "common.h"
#include "flex.h"
#include "gumplist.h"
#include "linklist.h"

/* A piece of the cache memory: a slot's sound, or free (slot -1). */
class SoundBlock : public GumpLink {
public:
    i16 slot;
    u32 size;
    u32 offset; /* from the start of the memory */
};
ASSERT_SIZE(SoundBlock, 0x14);
/* A slot on the list of use, the least recently used first. */
class SlotLink : public ListLink {
public:
    i16 slot;
    explicit SlotLink(i16 number) { slot = number; }
};
ASSERT_SIZE(SlotLink, 0x8);

/* The head of a sound effect in SOUND.FLX; the samples follow it. */
struct SoundEffect {
    char magic[4]; /* "ASFX" */
    u32 length;    /* of the samples */
    u32 loopStart;
    u32 loopLength; /* 0: the sound plays once */
    u16 unknown10;
    u16 rate; /* samples a second */
};
ASSERT_SIZE(SoundEffect, 0x14);

/* Allocates count slots and the memory the other programs leave. */
void initSoundCache(i16 count);
void closeSoundCache();
/* The archive the sounds are read from. */
void setSoundFlex(Flex *flex);
/* A pointer to sound, read in when it is not in memory; 0 for an empty
 * sound. */
void *getSound(i16 sound);
/* The port's: the slot of the sound getSound found last and its size,
 * which a channel that plays the sound follows (ASS_PlaySlotSFX), as the
 * original's pointer was the slot's selector. */
void *const *lastSoundSlot();
const u32 *lastSoundSize();
/* Empties every slot (leaveMap). */
void resetSoundCache();
/* Moves the sounds down over the free pieces, with interrupts off. */
void compactBlocks();
#endif
