/* SNDSRC.C: the sounds items make. Each source ties a playing sound to
 * the item it comes from; its volume falls with the item's distance from the
 * NPC the player controls, and is full while the item is on the screen. The
 * last four slots hold the looping sounds, whose volumes follow the NPC. */
#ifndef PORT_SNDSRC_H
#define PORT_SNDSRC_H
#include "common.h"

/* The looping sounds' slots at the end. */
constexpr int LOOP_SOURCES = 4;

struct SoundSource {
    i16 handle; /* the sound manager's; -1 when free */
    u16 item;
    i16 sound;
    u16 volume;
    u16 priority; /* the first volume */
    u8 looping;
};
ASSERT_SIZE(SoundSource, 0xb);

void initSoundSources(i16 count);
void closeSoundSources();
void resetSoundSources();
/* Frees the slots whose sound has ended. */
void updateSoundSources();
/* Sounds 1-1024 in No Regret, 1-512 in No Remorse (LAST_SOUND). playItemLoop
 * plays a looping sound (once when the effect does not loop) unless the sound
 * already plays from an item. */
void playItemSound(i16 sound, u16 item);
void playItemLoop(i16 sound, u16 item);
/* Plays the sound of item again, or anew when it has ended. */
void restartItemSound(i16 sound, u16 item);
void stopItemSound(i16 sound, u16 item);
void forgetItemSounds(u16 item);
void stopItemSounds(u16 item);
void stopItemLoops(u16 item);
u8 isItemSoundPlaying(i16 sound, u16 item);
void setItemSoundVolume(i16 sound, u16 item, char volume);
/* Drops the source of the sound handle. */
void forgetSoundSource(u32 handle);
/* Sets the looping sounds' volumes for where the controlled NPC is. */
void updateItemSounds();

/* Nothing calls these, in the original either. */
void unusedSoundSource();
#endif
