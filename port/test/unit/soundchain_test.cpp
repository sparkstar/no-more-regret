/* The chain of longplay-1013 through SNDMGR.C and SNDCACHE.C (both games')
 * and the sound module, without the games' data: a sound ended, the game
 * plays it again on its handle (restartSound) before updateChannels, which
 * then frees the channel the mixer plays it on; the cache gives the
 * sound's slot to a shorter one. The original read the shorter sound past
 * its end there and faulted; the port's cache stops the slot's effects
 * (ASS_StopSlotSFX) and the lost channel stops, while the game's books
 * stay the original's: the handle not playing, the channel free. */
#include "asylib.h"
#include "asylum.h"
#include "assproc.h"
#include "flex.h"
#include "kernel.h"
#include "pc.h"
#include "sndcache.h"
#include "sndmgr.h"
#include "system.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int checks = 0;
int failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH soundchain: %s\n", what.c_str());
    }
}

/* A sound in SOUND.FLX's form (signed bytes at 14h), length samples: a
 * square wave between size and -size, a half period every 25 samples (0:
 * silence). */
std::vector<u8> squareSound(u32 length, u8 size)
{
    std::vector<u8> sound(0x14 + length);
    auto put32 = [&](u32 at, u32 value) {
        for (u32 k = 0; k < 4; k++) {
            sound[at + k] = (u8)(value >> (8 * k));
        }
    };
    put32(4, length);
    put32(8, 0);
    put32(0xc, length);
    sound[0x12] = (u8)22050;
    sound[0x13] = (u8)(22050 >> 8);
    for (u32 k = 0; k < length; k++) {
        sound[0x14 + k] = (u8)((k / 25) % 2 == 0 ? size : -size);
    }
    return sound;
}

/* The game's time: the timer's ticks run the sound's process
 * (assproc.cpp's ASSProcess), whose update mixes. */
void play(double seconds)
{
    passTime(pcTime() + (std::uint64_t)(seconds * PIT_HZ));
}

/* The cache's slots, as DSFXMAN.C makes them. */
constexpr i16 SLOTS = 32;

/* SOUND.FLX in memory: sound 1 of 2,000 samples, the others of 100. */
class MemoryFlex : public Flex {
public:
    std::vector<std::vector<u8>> sounds;

    MemoryFlex()
    {
        sounds.emplace_back();
        sounds.push_back(squareSound(2000, 0x60));
        for (int i = 2; i < 40; i++) {
            sounds.push_back(squareSound(100, 0x60));
        }
    }
    void readObject(i16 index, void *buffer, s32 /*length*/, s32 /*offset*/) override
    {
        std::memcpy(buffer, sounds[index].data(), sounds[index].size());
    }
    s32 objectSize(i16 index) override { return (s32)sounds[index].size(); }
};

/* The chain of longplay-1013 (SNDMGR.C and SNDCACHE.C, both games'): a
 * sound ended, the game plays it again on its handle (restartSound) before
 * updateChannels, which then frees the channel the mixer plays it on; the
 * cache gives the slot to a shorter sound. The cache's stop of the slot's
 * effects stops that channel, and the game's books stay the original's:
 * the handle not playing, the channel free. */
void chain()
{
    MemoryFlex flex;
    /* What the game's System makes before the sound (debugPrintf's). */
    std::vector<char> text(FORMAT_BUFFER_SIZE);
    formatBuffer = text.data();
    initKernel();
    asylum::loadData(asylum::data);
    ASS_SetDepth(16);
    sampleRate = 22050;
    soundDma = 5;
    soundCard = 1;
    setSoundFlex(&flex);
    initSoundCache(SLOTS);
    initSoundManager();
    /* The first free channel of the sounds that play once. */
    const u32 handle = playSound(1, 64, 0, 0);
    const i16 channel = 0;
    const asylum::Channel &mixed = asylum::data.channels[asylum::MUSIC_CHANNELS + channel];
    for (int tick = 0; tick < 60 && isSoundPlaying(handle); tick++) {
        play(1.0 / 60);
    }
    check(!isSoundPlaying(handle) && soundOf(handle) == -1, "sound 1 ended: its handle done");
    check(restartSound(handle, 1, 64, 0) == handle, "played again on its handle");
    play(1.0 / 60);
    check(ASS_IsChannelBusy(channel) == 1 && mixed.length == 2000 && !isSoundPlaying(handle),
          "the mixer plays it, the game's books say it does not");
    updateChannels();
    check(handleOf(1) == -1 && soundOf(handle) == -1, "updateChannels frees its channel");
    for (i16 sound = 2; sound < 2 + SLOTS; sound++) {
        getSound(sound);
    }
    check(ASS_IsChannelBusy(channel) == 0, "the slot given to a shorter sound: the lost channel's effect stopped");
    play(1.0 / 60);
    check(mixed.length == 0, "the lost channel not mixed past it");
    check(!isSoundPlaying(handle) && handleOf(1) == -1, "the game's books as the original's");
    closeSoundManager();
    closeSoundCache();
    exitKernel();
    formatBuffer = nullptr;
}

}

int main()
{
    chain();
    std::printf("soundchain: %d checks, %d mismatches\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
