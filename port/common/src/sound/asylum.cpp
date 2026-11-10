/* ASYLUM.DLL's exports (asylum.h) over the port's sound module
 * (port/common/asylum, asyapi.h): the game's structures as the module's,
 * the pointers as they are. Each call goes to the pass dump (passdump.h's A
 * records, by REGRET.EXE's import numbers), and the module's calls of the
 * game's callback (B), for the pass oracle's original, whose ASYLUM.DLL
 * gives the port's answers; the port's own calls (the depth's) are not the
 * original's and go to none. */
#include "asylum.h"
#include "gamestate.h"
#include "amfload.h"
#include "asyapi.h"
#include "passdump.h"

#include <algorithm>
#include <cstring>

/* The game's callback, which the module calls through soundEnded. */
using SoundCallback = void (*)(i16 channel);
namespace {
SoundCallback &gameCallback = gameGlobal<SoundCallback>(nullptr);

void soundEnded(i16 channel)
{
    passDumpSoundCallback(channel);
    gameCallback(channel);
}
}

extern "C" {

u16 ASS_GetDmaBytesLeft(void)
{
    const u16 left = asylum::getDmaBytesLeft();
    passDumpSound(18, {}, left);
    return left;
}

/* A stream's length dwords at samples: the reads stay within them. */
void ASS_DirectPlay(i16 channel, void *samples, u32 length, i16 unknown, u16 rate)
{
    asylum::directPlay(channel, (const u8 *)samples, length, unknown, rate);
    passDumpSound(19, {(u16)channel, (u16)length, (u16)(length >> 16), (u16)unknown, rate}, 0);
}

void ASS_SetSFXVolume(i16 channel, u16 volume)
{
    asylum::setSfxVolume(channel, volume);
    passDumpSound(26, {(u16)channel, volume}, 0);
}

void ASS_StopAllSFX(void)
{
    asylum::stopAllSfx();
    passDumpSound(27, {}, 0);
}

void ASS_StopSFX(i16 channel)
{
    asylum::stopSfx(channel);
    passDumpSound(28, {(u16)channel}, 0);
}

/* A block the game gives by its pointer: the original's selector had it
 * whole, the module its sound as its head gives it. */
void ASS_PlaySFX(void *sample, i16 channel, u16 volume, i16 unknown, i16 loop)
{
    const u8 *head = (const u8 *)sample;
    auto dword = [&](u32 at) { return (head[at] | head[at + 1] << 8 | head[at + 2] << 16 | (u32)head[at + 3] << 24); };
    const u32 size = 0x14 + std::max(dword(4), dword(8) + dword(0xc));
    asylum::playSfx(head, size, channel, volume, unknown, loop);
    passDumpSound(30, {(u16)channel, volume, (u16)unknown, (u16)loop}, 0);
}

void ASS_PlaySlotSFX(void *const *slot, const u32 *size, i16 channel, u16 volume, i16 unknown, i16 loop)
{
    asylum::playSfx(slot, size, channel, volume, unknown, loop);
    /* The original's ASS_PlaySFX. */
    passDumpSound(30, {(u16)channel, volume, (u16)unknown, (u16)loop}, 0);
}

/* Not the original's: nothing for the pass dump. */
void ASS_StopSlotSFX(void *const *slot)
{
    asylum::stopSlotSfx(slot);
}

i16 ASS_GetSoundVolume(void)
{
    const i16 volume = asylum::getSoundVolume();
    passDumpSound(31, {}, (u16)volume);
    return volume;
}

void ASS_SetSoundVolume(u16 volume)
{
    asylum::setSoundVolume(volume);
    passDumpSound(32, {volume}, 0);
}

void ASS_ResumeMusic(void)
{
    asylum::resumeMusic();
    passDumpSound(33, {}, 0);
}

void ASS_PauseMusic(void)
{
    asylum::pauseMusic();
    passDumpSound(34, {}, 0);
}

void ASS_StopMusic(void)
{
    asylum::stopMusic();
    passDumpSound(35, {}, 0);
}

void ASS_StartMusic(void)
{
    asylum::startMusic();
    passDumpSound(36, {}, 0);
}

/* AMFLOAD.C's module: its header, patterns and samples, and the sizes of
 * their blocks. */
void ASS_PlayMusic(AmfModule *module)
{
    asylum::Song song{};
    const AmfHeader *header = module->header;
    song.header = (const u8 *)header;
    song.patterns = module->patterns;
    song.patternsSize = (header != nullptr) ? header->patterns * AMF_PATTERN_SIZE : 0;
    for (u16 i = 0; i < AMF_SAMPLES; i++) {
        song.samples[i] = module->samples[i];
        song.sizes[i] = (header != nullptr) ? header->sample[i].length : 0;
    }
    asylum::playMusic(song);
    passDumpSound(37, {}, 0);
}

i16 ASS_GetMusicVolume(void)
{
    const i16 volume = asylum::getMusicVolume();
    passDumpSound(39, {}, (u16)volume);
    return volume;
}

void ASS_SetMusicVolume(u16 volume)
{
    asylum::setMusicVolume(volume);
    passDumpSound(40, {volume}, 0);
}

u16 ASS_GetOversampling(void)
{
    const u16 oversampling = asylum::getOversampling();
    passDumpSound(48, {}, oversampling);
    return oversampling;
}

void ASS_ClrOversampling(void)
{
    asylum::clrOversampling();
    passDumpSound(49, {}, 0);
}

void ASS_SetOversampling(void)
{
    asylum::setOversampling();
    passDumpSound(50, {}, 0);
}

void ASS_RegisterCallback(void (*callback)(i16))
{
    gameCallback = callback;
    asylum::registerCallback(callback != nullptr ? soundEnded : nullptr);
    passDumpSound(52, {}, 0);
}

void ASS_GetCardName(i16 card, char *name)
{
    asylum::getCardName(card, name);
    passDumpSound(53, {(u16)card}, 0, (const u8 *)name, (u16)(std::strlen(name) + 1));
}

void ASS_Init(AssConfig *config)
{
    asylum::Config c;
    c.card = config->card;
    c.port = config->port;
    c.irq = config->irq;
    c.dma = config->dma;
    c.rate = config->rate;
    c.music = config->unknown0a;
    c.sound = config->unknown0c;
    asylum::init(c);
    passDumpSound(61, {}, 0, (const u8 *)config, sizeof(AssConfig));
}

void ASS_UpdateDAC(void)
{
    asylum::updateDac();
    passDumpSound(59, {}, 0);
}

void ASS_Uninit(void)
{
    asylum::uninit();
    passDumpSound(60, {}, 0);
}

void ASS_NotifySampleChange(i16 channel, s32 offset, u32 length)
{
    asylum::notifySampleChange(channel, offset, length);
    passDumpSound(21, {(u16)channel, (u16)offset, (u16)((u32)offset >> 16), (u16)length, (u16)(length >> 16)}, 0);
}

i16 ASS_IsChannelBusy(i16 channel)
{
    const i16 busy = asylum::isChannelBusy(channel);
    passDumpSound(22, {(u16)channel}, (u16)busy);
    return busy;
}

void ASS_SetDepth(u16 bits)
{
    asylum::setDepth(bits);
}

u16 ASS_GetDepth(void)
{
    return asylum::getDepth();
}

u16 ASS_GetOutputBits(void)
{
    return asylum::outputBits();
}
}
