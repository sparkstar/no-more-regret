/* SNDMGR.C (12e8:0000-0605): the sound effect channels of ASYLUM.DLL. A
 * played sound is known by a handle; channels 0-7 take sounds that play once,
 * 8-11 the looping ones, and when all are busy the oldest is cut off. */
#ifndef PORT_SNDMGR_H
#define PORT_SNDMGR_H
#include "common.h"

constexpr int CHANNELS = 12;
/* The first channel of the looping sounds. */
constexpr int LOOP_CHANNELS = 8;

/* SoundChannel::flags */
constexpr int CHANNEL_LOOP = 1;
constexpr int CHANNEL_DONE = 2; /* the sample ended or was stopped */

struct SoundChannel {
    u32 handle; /* -1 when free */
    i16 sound;
    u16 priority; /* kept, never read */
    u16 volume;
    u16 flags;
};
ASSERT_SIZE(SoundChannel, 0xc);

/* The sound card setup the configuration sets. */
extern u16 &soundPort;
extern u16 &soundIrq;
extern u16 &soundDma;
extern u16 &soundCard;

/* Starts ASYLUM.DLL and the channels; closeSoundManager stops them. */
void initSoundManager();
void closeSoundManager();
/* Stops every channel and frees it. */
void resetChannels();
/* Plays sound on a free or the oldest channel and returns its handle, or -1. */
u32 playSound(i16 sound, u16 volume, u16 priority, u8 loop);
/* Plays sample data on channel 0. */
/* UNCALLED: nothing calls it. */
void playSample(void *sample);
/* Plays sound again on handle's channel, or anew when it has ended. */
u32 restartSound(u32 handle, i16 sound, u16 volume, u16 priority);
void stopSound(u32 handle);
void stopAllSounds();
/* Frees the channels whose sample ended and keeps the looping sounds in the
 * cache. */
void updateChannels();
/* ASYLUM.DLL calls it when channel's sample ends. */
void channelDone(i16 channel);
/* Stops the channels that play sound. */
void stopSoundNumber(i16 sound);
void setSoundVolume(u32 handle, u16 volume);
/* The sound handle plays, or -1. */
i16 soundOf(u32 handle);
/* The low word of the handle of a channel that plays sound, or -1. */
i16 handleOf(i16 sound);

/* Nothing calls these, in the original either. */
bool isSoundPlaying(u32 handle);
#endif
