/* 1428:0000-0983 (No Regret's), SNDSRC.C: the sounds items make. See sndsrc.h. */
#include "gamestate.h"
#include "sndsrc.h"
#include "sndsrc_game.h"
#include "sndcache.h"
#include "sndmgr.h"
#include "item.h"
#include "npc.h"
#include "distance.h"

u8 isSoundFromItem(i16 sound);
void updateLoopVolumes();

/* The volume of a sound from no item or from an item on the screen. */
constexpr int FULL_VOLUME = 64;

namespace {
SoundSource *&sources = gameGlobal<SoundSource *>(nullptr);
i16 &sourceCount = gameGlobal<i16>(0);
u8 &sourcesOpen = gameGlobal<u8>(0);

/* The volume of a sound from item (64 on the screen, less far away). */
u16 itemVolume(u16 item)
{
    i16 range;

    if (item == 0U) {
        return FULL_VOLUME;
    }
    const Item source(item);

    if (source.isOnScreen()) {
        return FULL_VOLUME;
    }
    range = mapDistance(itemX[controlledNpc.id], itemY[controlledNpc.id], itemX[source.id], itemY[source.id]);
    if (range > 960) {
        return 8;
    }
    if (range > 640) {
        return 16;
    }
    if (range > 380) {
        return 32;
    }
    return 50;
}
}

void initSoundSources(i16 count)
{
    if (sourcesOpen == 0U) {
        sourceCount = count;
        sources = new SoundSource[sourceCount];
        sourcesOpen = 1;
        resetSoundSources();
    }
}

void closeSoundSources()
{
    if (sourcesOpen != 0U) {
        if (sources != nullptr) {
            delete[] sources;
            sources = nullptr;
        }
        sourcesOpen = 0;
    }
}

void resetSoundSources()
{
    if (sourcesOpen != 0U) {
        memset(sources, 0, sourceCount * sizeof(SoundSource));
        for (i16 i = 0; i < sourceCount; i++) {
            sources[i].handle = -1;
        }
    }
}

namespace {
/* A free slot, -1 when there is none. */
i16 freeSource()
{
    i16 i;

    for (i = 0; i < static_cast<i16>(sourceCount - LOOP_SOURCES) && sources[i].handle != -1; i++) {
        ;
    }
    if (i < static_cast<i16>(sourceCount - LOOP_SOURCES)) {
        return i;
    }
    return -1;
}

i16 freeLoopSource()
{
    i16 i;

    for (i = sourceCount - LOOP_SOURCES; i < sourceCount && sources[i].handle != -1; i++) {
        ;
    }
    if (i < sourceCount) {
        return i;
    }
    return -1;
}
}

void updateSoundSources()
{
    i16 i;
    i16 handle;

    updateChannels();
    for (i = 0; i < sourceCount; i++) {
        handle = sources[i].handle;
        if (handle == -1 || soundOf(handle) == -1) {
            sources[i].handle = -1;
        }
    }
}

void playItemSound(i16 sound, u16 item)
{
    i16 slot;
    u16 volume;

    updateSoundSources();
    slot = freeSource();
    if (slot == -1 || sound < 1 || sound > LAST_SOUND) {
        return;
    }
    volume = itemVolume(item);
    sources[slot].item = item;
    sources[slot].volume = volume;
    sources[slot].priority = volume;
    sources[slot].sound = sound;
    sources[slot].looping = 0;
    sources[slot].handle = playSound(sound, volume, 0, 0);
}

void playItemLoop(i16 sound, u16 item)
{
    SoundEffect *effect;
    i16 slot;
    u16 volume;

    updateSoundSources();
    effect = static_cast<SoundEffect *>(getSound(sound));
    if (effect == nullptr) {
        return;
    }
    if (effect->loopLength == 0U) {
        playItemSound(sound, item);
        return;
    }
    if (isSoundFromItem(sound) != 0U) {
        return;
    }
    slot = freeLoopSource();
    if (slot == -1 || sound < 1 || sound > LAST_SOUND) {
        return;
    }
    volume = itemVolume(item);
    sources[slot].item = item;
    sources[slot].volume = volume;
    sources[slot].priority = volume;
    sources[slot].sound = sound;
    sources[slot].looping = 1;
    sources[slot].handle = playSound(sound, volume, 0, 1);
}

void restartItemSound(i16 sound, u16 item)
{
    i16 i = 0;
    i16 handle = -1;

    if (sound < 1 || sound > LAST_SOUND) {
        return;
    }
    for (; i < static_cast<i16>(sourceCount - LOOP_SOURCES); i++) {
        if (sources[i].item == item && sources[i].sound == sound) {
            handle = restartSound(sources[i].handle, sound, sources[i].volume, sources[i].priority);
            if (handle == -1) {
                sources[i].item = 0;
                sources[i].handle = -1;
            } else {
                break;
            }
        }
    }
    if (i == static_cast<i16>(sourceCount - LOOP_SOURCES) || handle == -1) {
        playItemSound(sound, item);
    }
}

void stopItemSound(i16 sound, u16 item)
{
    i16 i;

    if (sound < 1 || sound > LAST_SOUND) {
        return;
    }
    for (i = 0; i < sourceCount; i++) {
        if (sources[i].sound == sound && sources[i].item == item) {
            stopSound(sources[i].handle);
            sources[i].handle = -1;
            sources[i].item = 0;
            break;
        }
    }
}

void forgetItemSounds(u16 item)
{
    i16 i;

    for (i = 0; i < sourceCount; i++) {
        if (sources[i].item == item) {
            sources[i].item = 0;
            sources[i].item = 0;
        }
    }
}

void stopItemSounds(u16 item)
{
    i16 i;

    for (i = 0; i < sourceCount; i++) {
        if (sources[i].item == item) {
            stopSound(sources[i].handle);
            sources[i].handle = -1;
            sources[i].item = 0;
        }
    }
}

void stopItemLoops(u16 item)
{
    i16 i;

    for (i = 0; i < sourceCount; i++) {
        if (sources[i].item == item && (sources[i].looping != 0U)) {
            stopSound(sources[i].handle);
            sources[i].handle = -1;
            sources[i].looping = 0;
            sources[i].item = 0;
        }
    }
}

u8 isItemSoundPlaying(i16 sound, u16 item)
{
    i16 i;

    if (sound < 1 || sound > LAST_SOUND) {
        return 0;
    }
    u8 playing = 0;

    updateSoundSources();
    for (i = 0; i < sourceCount; i++) {
        if (sources[i].item == item && sources[i].sound == sound) {
            playing |= static_cast<int>(soundOf(sources[i].handle) == sources[i].sound);
        }
    }
    return playing;
}

/* The sound plays from some item. */
u8 isSoundFromItem(i16 sound)
{
    i16 i;

    if (sound < 1 || sound > LAST_SOUND) {
        return 0;
    }
    u8 playing = 0;

    updateSoundSources();
    for (i = 0; i < sourceCount; i++) {
        if ((sources[i].item != 0U) && sources[i].sound == sound) {
            playing |= static_cast<int>(soundOf(sources[i].handle) == sources[i].sound);
        }
    }
    return playing;
}

void unusedSoundSource() {}

void setItemSoundVolume(i16 sound, u16 item, char volume)
{
    i16 i;

    if (sound < 1 || sound > LAST_SOUND) {
        return;
    }
    for (i = 0; i < sourceCount; i++) {
        if (sources[i].sound == sound && sources[i].item == item) {
            setSoundVolume(sources[i].handle, volume);
            break;
        }
    }
}

void forgetSoundSource(u32 handle)
{
    i16 i;

    for (i = 0; i < sourceCount && static_cast<u32>(sources[i].handle) != handle; i++) {
        ;
    }
    if (i < sourceCount) {
        sources[i].handle = -1;
        sources[i].item = 0;
    }
}

void updateItemSounds()
{
    updateLoopVolumes();
}

void updateLoopVolumes()
{
    i16 i;
    u16 range;

    for (i = sourceCount - LOOP_SOURCES; i < sourceCount; i++) {
        if (sources[i].handle == -1) {
            continue;
        }
        if (sources[i].item == 0U) {
            continue;
        }
        const Item source(sources[i].item);
        const u16 volume = sources[i].volume;

        if (source.isOnScreen()) {
            sources[i].volume = FULL_VOLUME;
        } else {
            range = mapDistance(itemX[controlledNpc.id], itemY[controlledNpc.id], itemX[source.id], itemY[source.id]);
            if (range > 960) {
                sources[i].volume = 8;
            } else if (range > 640) {
                sources[i].volume = 16;
            } else if (range > 380) {
                sources[i].volume = 32;
            } else {
                sources[i].volume = 50;
            }
        }
        if (sources[i].volume != volume) {
            setSoundVolume(sources[i].handle, sources[i].volume);
        }
    }
}
