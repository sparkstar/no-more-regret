/* DSFXMAN.C (12d8:0386-05f7, both games'): the game's sound effects. See
 * dsfxman.h. */
#include "dsfxman.h"
#include "gamestate.h"
#include "assproc.h"
#include "filespec.h"
#include "gamefile.h"
#include "sndcache.h"
#include "sndmgr.h"
#include "sndsrc.h"
#include "dsfxman_game.h"
#include "fastflex.h"
#include "tagflex.h"

/* The number of cache slots and of sound sources. */
constexpr int SOUND_SLOTS = 32;

u8 &effectsStarted = gameGlobal<u8>(0);
/* The game's kind of archive (dsfxman_game.h): No Regret's a FastFlex, No
 * Remorse's a TagFlex, which is one too. */
FastFlex *&soundFlex = gameGlobal<FastFlex *>(nullptr);
/* Nothing uses it. */
void *&unusedEffects = gameGlobal<void *>(nullptr);

namespace {
/* Opens the archive and starts the cache, the channels and the sound
 * sources; closeSoundEffects stops them. */
void initSoundEffects()
{
    char *path;

    if (effectsStarted == 0U) {
        path = makePath(nullptr, soundDirectory, "sound", "flx");
        if (soundFlex == nullptr) {
            if (SOUND_FLEX_TAGGED) {
                soundFlex = new TagFlex(path, SOUND_FLEX_MODE, -1);
            } else {
                soundFlex = new FastFlex(path, SOUND_FLEX_MODE, -1);
            }
        }
        setSoundFlex(soundFlex);
        initSoundCache(SOUND_SLOTS);
        initSoundManager();
        initSoundSources(SOUND_SLOTS);
        effectsStarted = 1;
    }
}
}

void closeSoundEffects()
{
    if (effectsStarted != 0U) {
        closeSoundManager();
        closeSoundSources();
        if (soundFlex != nullptr) {
            delete soundFlex;
            soundFlex = nullptr;
        }
        closeSoundCache();
        effectsStarted = 0;
    }
}

void playEffect(i16 sound)
{
    if (effectsStarted != 0U) {
        playSound(sound, 64, 255, 0);
    }
}

/* UNCALLED: nothing calls it. */
void playEffect(i16 sound, u8 volume)
{
    if (effectsStarted != 0U) {
        playSound(sound, volume, 255, 0);
    }
}

/* UNCALLED: nothing calls it. */
void playEffectData(void *sample)
{
    if (effectsStarted != 0U) {
        playSample(sample);
    }
}

/* UNCALLED: nothing calls it. */
void loopEffect(i16 sound)
{
    if (effectsStarted != 0U) {
        playSound(sound, 64, 255, 1);
    }
}

/* UNCALLED: nothing calls it. */
void loopEffect(i16 sound, u16 volume)
{
    if (effectsStarted != 0U) {
        playSound(sound, volume, 255, 1);
    }
}

void stopEffect(i16 sound)
{
    i16 handle;

    if (effectsStarted != 0U) {
        handle = handleOf(sound);
        if (handle != -1) {
            stopSound(handle);
        }
    }
}

void stopAllEffects()
{
    if (effectsStarted != 0U) {
        stopAllSounds();
    }
}

void pauseEffects()
{
    if (effectsStarted != 0U) {
        stopAllSounds();
    }
}

void resumeEffects() {}

bool isEffectPlaying(i16 sound)
{
    if (effectsStarted != 0U) {
        updateSoundSources();
        return handleOf(sound) != -1;
    }
    return false;
}

void startSoundEffects()
{
    if (soundEnabled != 0U) {
        initSoundEffects();
    }
}
