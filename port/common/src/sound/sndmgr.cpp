/* SNDMGR.C: the sound effect channels. See sndmgr.h. */
#include "gamestate.h"
#include "sndmgr.h"
#include "assproc.h"
#include "asylum.h"
#include "sndcache.h"
#include "sndsrc.h"

i16 allocateChannel(u16 priority, u8 loop);

u8 &soundManagerStarted = gameGlobal<u8>(0);
SoundChannel *&channels = gameGlobal<SoundChannel *>(nullptr);
/* The channels to cut off next: short sounds and looping ones. */
i16 &nextShort = gameGlobal<i16>(0);
i16 &nextLoop = gameGlobal<i16>(LOOP_CHANNELS);
u32 &nextHandle = gameGlobal<u32>(0);
u16 &soundPort = gameGlobal<u16>();
u16 &soundIrq = gameGlobal<u16>();
u16 &soundDma = gameGlobal<u16>();
u16 &soundCard = gameGlobal<u16>();

void initSoundManager()
{
    if (soundManagerStarted == 0U) {
        channels = new SoundChannel[CHANNELS];
        initAsylum(soundPort, soundIrq, soundDma, soundCard, 22050);
        /* After the flag, so that resetChannels clears the new channels: No
         * Remorse's original reset them before it and, closing, after clearing
         * it, when resetChannels does nothing. */
        soundManagerStarted = 1;
        resetChannels();
    }
}

void closeSoundManager()
{
    if (soundManagerStarted != 0U) {
        resetChannels();
        soundManagerStarted = 0;
        closeAsylum();
        if (channels != nullptr) {
            delete[] channels;
            channels = nullptr;
        }
    }
}

void resetChannels()
{
    i16 i;

    if (soundManagerStarted != 0U) {
        nextHandle = 0;
        nextShort = 0;
        nextLoop = LOOP_CHANNELS;
        memset(channels, 0, CHANNELS * sizeof(SoundChannel));
        for (i = 0; i < CHANNELS; i++) {
            ASS_StopSFX(i);
            channels[i].handle = -1;
            channels[i].flags = 0;
        }
    }
}

namespace {
/* The channel that plays handle, or -1. */
i16 channelOf(u32 handle)
{
    i16 i;

    if (handle != static_cast<u32>(-1)) {
        for (i = 0; i < CHANNELS; i++) {
            if (channels[i].handle == handle) {
                return i;
            }
        }
    }
    return -1;
}
}

u32 playSound(i16 sound, u16 volume, u16 priority, u8 loop)
{
    i16 channel;
    void *sample;

    updateChannels();
    channel = allocateChannel(priority, loop);
    if (channel != -1) {
        sample = getSound(sound);
        channels[channel].sound = sound;
        channels[channel].priority = priority;
        channels[channel].flags = 0;
        channels[channel].volume = volume;
        if (loop != 0U) {
            channels[channel].flags |= CHANNEL_LOOP;
        }
        if (sample != nullptr) {
            ASS_PlaySlotSFX(lastSoundSlot(), lastSoundSize(), channel, volume, 0, loop);
        }
        return channels[channel].handle;
    }
    return -1;
}

/* UNCALLED: nothing calls it. */
void playSample(void *sample)
{
    if (sample == nullptr) {
        return;
    }
    ASS_PlaySFX(sample, 0, 64, 0, 0);
}

bool isSoundPlaying(u32 handle)
{
    i16 channel;

    if (handle != static_cast<u32>(-1)) {
        channel = channelOf(handle);
        if (channel == -1) {
            return false;
        }
        if ((channels[channel].flags & CHANNEL_DONE) != 0) {
            return false;
        }
        return true;
    }
    return false;
}

u32 restartSound(u32 handle, i16 sound, u16 volume, u16 priority)
{
    i16 channel;
    void *sample;

    if (handle != static_cast<u32>(-1)) {
        channel = channelOf(handle);
        if (channel == -1) {
            return playSound(sound, volume, priority, 0);
        }
        sample = getSound(sound);
        if (sample != nullptr) {
            ASS_PlaySlotSFX(lastSoundSlot(), lastSoundSize(), channel, channels[channel].volume, 0, 0);
        }
        return handle;
    }
    return -1;
}

void stopSound(u32 handle)
{
    const i16 channel = channelOf(handle);

    if (channel != -1) {
        channels[channel].flags |= CHANNEL_DONE;
        ASS_StopSFX(channel);
        channels[channel].handle = -1;
    }
}

void stopAllSounds()
{
    i16 i;

    for (i = 0; i < CHANNELS; i++) {
        if (channels[i].handle != static_cast<u32>(-1)) {
            stopSound(channels[i].handle);
        }
    }
}

/* A channel for a new sound with a new handle; priority is not used. */
i16 allocateChannel(u16 /*unused*/, u8 loop)
{
    i16 channel = -1;
    i16 i;
    i16 first = 0;
    i16 end = LOOP_CHANNELS;

    if (loop != 0U) {
        first = LOOP_CHANNELS;
        end = CHANNELS;
    }
    for (i = first; i < end; i++) {
        if (channels[i].handle == static_cast<u32>(-1)) {
            channel = i;
            break;
        }
    }
    if (channel == -1) {
        if (loop == 0U) {
            channel = nextShort++;
            if (nextShort == LOOP_CHANNELS) {
                nextShort = 0;
            }
        } else {
            channel = nextLoop++;
            if (nextLoop == CHANNELS) {
                nextLoop = LOOP_CHANNELS;
            }
        }
        forgetSoundSource(channels[channel].handle);
        ASS_StopSFX(channel);
    }
    if (channel != -1) {
        channels[channel].handle = nextHandle;
        nextHandle++;
    }
    return channel;
}

void updateChannels()
{
    i16 i;

    for (i = 0; i < CHANNELS; i++) {
        if ((channels[i].flags & CHANNEL_DONE) != 0) {
            channels[i].flags &= ~CHANNEL_DONE;
            channels[i].handle = -1;
            channels[i].sound = 0;
        } else if (channels[i].handle != static_cast<u32>(-1) && i >= LOOP_CHANNELS) {
            getSound(channels[i].sound);
        }
    }
}

void channelDone(i16 channel)
{
    if (channels[channel].handle != static_cast<u32>(-1)) {
        channels[channel].flags |= CHANNEL_DONE;
    }
}

void stopSoundNumber(i16 sound)
{
    i16 i;

    for (i = 0; i < CHANNELS; i++) {
        if (channels[i].sound == sound) {
            ASS_StopSFX(i);
            if (channels[i].handle != static_cast<u32>(-1)) {
                forgetSoundSource(channels[i].handle);
                stopSound(channels[i].handle);
            }
        }
    }
}

void setSoundVolume(u32 handle, u16 volume)
{
    const i16 channel = channelOf(handle);

    if (channel != -1) {
        channels[channel].volume = volume;
        ASS_SetSFXVolume(channel, volume);
    }
}

i16 soundOf(u32 handle)
{
    const i16 channel = channelOf(handle);

    if (channel != -1 && ((channels[channel].flags & CHANNEL_DONE) == 0)) {
        return channels[channel].sound;
    }
    return -1;
}

i16 handleOf(i16 sound)
{
    i16 i;

    for (i = 0; i < CHANNELS; i++) {
        if (channels[i].handle != static_cast<u32>(-1) && channels[i].sound == sound) {
            return channels[i].handle;
        }
    }
    return -1;
}
