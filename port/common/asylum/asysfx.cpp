/* ASYLUM.DLL segment 3: the sound effects' twelve channels, whose requests
 * (SfxRecord) segment 1 takes at its next tick. See asylib.h. */
#include "asylib.h"

#include <algorithm>

namespace asylum {

namespace {

u16 wordOf(const u8 *at)
{
    return static_cast<u16>(at[0] | at[1] << 8);
}

u32 dwordOf(const u8 *at)
{
    return at[0] | at[1] << 8 | at[2] << 16 | static_cast<u32>(at[3]) << 24;
}

}

/* 3:0000 */
void startSfxSystem()
{
    for (SfxRecord &record : data.sfx) {
        record = SfxRecord{};
    }
    for (u16 i = 0; i < SFX_CHANNELS; i++) {
        data.channels[MUSIC_CHANNELS + i].mixer = sampleMixer();
    }
}

/* 3:0042 */
void stopSfxSystem()
{
    for (SfxRecord &record : data.sfx) {
        record = SfxRecord{};
    }
}

namespace {

/* 3:0054: the sound at samples (a FLEX object of SOUND.FLX: its length,
 * loop, rate at 12h and the bytes at 14h) on channel at volume, pitch added
 * to its step. The step's division is by EBX (MOVZX), whose high word is 0
 * after it. */
void playSound(Samples samples, s16 channel, u16 volume, s16 pitch, s16 loop)
{
    if (channel < 0 || channel >= static_cast<s16>(SFX_CHANNELS)) {
        return;
    }
    SfxRecord &record = data.sfx[channel];
    record.state = 0;
    if (pitch > 0x358) {
        pitch = 0x358;
    }
    const u32 rate = wordOf(samples.at(0x12, 2));
    record.step = divide32(rate << 16, data.dspRate);
    playerEbxHigh = 0;
    record.step += static_cast<u32>(static_cast<s32>(pitch));
    record.length = dwordOf(samples.at(4, 4));
    if (loop != 0) {
        record.loopStart = dwordOf(samples.at(8, 4));
        record.loopLength = dwordOf(samples.at(0xc, 4));
    } else {
        record.loopStart = 0;
        record.loopLength = 0;
    }
    samples.offset = 0x14;
    record.data = samples;
    record.volume = static_cast<u8>(volume);
    record.state = 1;
    data.channels[MUSIC_CHANNELS + channel].mixer = sampleMixer();
}

}

void playSfx(void *const *slot, const u32 *size, s16 channel, u16 volume, s16 pitch, s16 loop)
{
    playSound(Samples{slot, size, nullptr, 0, 0}, channel, volume, pitch, loop);
}

void playSfx(const u8 *sample, u32 size, s16 channel, u16 volume, s16 pitch, s16 loop)
{
    playSound(Samples{nullptr, nullptr, sample, size, 0}, channel, volume, pitch, loop);
}

/* 3:0139: channels 0-12; 12 is past the records: the word at 738h, the
 * first music channel's samples' offset. */
void stopSfx(s16 channel)
{
    if (channel >= 0 && channel < static_cast<s16>(SFX_CHANNELS)) {
        data.sfx[channel].state = 0;
    } else if (channel == static_cast<s16>(SFX_CHANNELS)) {
        data.channels[0].data.offset &= 0xffff0000;
    }
}

void stopSlotSfx(void *const *slot)
{
    for (SfxRecord &record : data.sfx) {
        if (record.state != 0 && record.data.slot == slot) {
            record.state = 0;
        }
    }
}

/* 3:015B: channels 0-12 as stopSfx: 12 the word at 738h. */
s16 isChannelBusy(s16 channel)
{
    if (channel >= 0 && channel < static_cast<s16>(SFX_CHANNELS)) {
        return static_cast<s16>(data.sfx[channel].state != 0);
    }
    if (channel == static_cast<s16>(SFX_CHANNELS)) {
        return static_cast<s16>(static_cast<u16>(data.channels[0].data.offset) != 0);
    }
    return 0;
}

/* 3:01C9 */
void setSoundVolume(u16 volume)
{
    const s16 v = static_cast<s16>(volume);
    levels.soundVolume = static_cast<u8>(std::clamp<s16>(v, 0, 0x40));
}

/* 3:01E7 */
s16 getSoundVolume()
{
    return static_cast<s16>(levels.soundVolume);
}

/* 3:01ED: channels 0-12 as stopSfx: 12 the byte at 74Ah, the third of the
 * first music channel's loop's length. */
void setSfxVolume(s16 channel, u16 volume)
{
    if (channel >= 0 && channel < static_cast<s16>(SFX_CHANNELS)) {
        data.sfx[channel].volume = static_cast<u8>(volume);
    } else if (channel == static_cast<s16>(SFX_CHANNELS)) {
        data.channels[0].loopLength =
            (data.channels[0].loopLength & 0xff00ffff) | static_cast<u32>(static_cast<u8>(volume)) << 16;
    }
}

/* 3:02B7 (_ASYLUMSFX_DirectPlay): length dwords of 16-bit stereo samples
 * at samples on channel at rate, through the stream mixers; EBX's high
 * word 0 after the step, as PlaySFX's: from AUDSTRM.C's callback the mix
 * goes on with it. */
void directPlay(s16 channel, const u8 *samples, u32 length, s16 /*unused*/, u16 rate)
{
    if (channel < 0 || channel >= static_cast<s16>(SFX_CHANNELS)) {
        return;
    }
    SfxRecord &record = data.sfx[channel];
    record.state = 0;
    record.step = divide32(static_cast<u32>(rate) << 16, data.dspRate);
    playerEbxHigh = 0;
    record.length = length;
    record.loopStart = 0;
    record.loopLength = 0;
    record.data = Samples{nullptr, nullptr, samples, length * 4, 0};
    record.volume = 0x40;
    record.state = 1;
    data.channels[MUSIC_CHANNELS + channel].mixer = streamMixer();
}

}
