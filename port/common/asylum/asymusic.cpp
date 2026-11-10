/* ASYLUM.DLL segment 2: the music, which segment 1's player plays from the
 * tables here. See asylib.h. */
#include "asylib.h"

#include <algorithm>

namespace asylum {

namespace {

/* The samples of a tick at rate for the tempo: rate / (tempo * 24 / 60),
 * the tempo a signed byte as CBW makes it. */
u16 tickSamplesOf(u16 rate, u8 tempo)
{
    const s16 ticks = static_cast<s16>(static_cast<s16>(static_cast<s8>(tempo)) * 0x18 / 0x3c);
    return divide16(rate, static_cast<u16>(ticks));
}

/* The music's eight channels at full volume, centred, with the mixer of
 * the output. */
void centreChannels()
{
    for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
        Channel &channel = data.channels[i];
        channel.volume = 0x40;
        channel.panRight = 0x40;
        channel.panLeft = 0x40;
        channel.musicPanRight = channel.panRight;
        channel.musicPanLeft = channel.panLeft;
        channel.mixer = sampleMixer();
    }
}

u16 wordOf(const u8 *at)
{
    return static_cast<u16>(at[0] | at[1] << 8);
}

}

/* 2:0025 */
void startMusicSystem()
{
    if (data.musicStarted != 0U) {
        return;
    }
    data.musicPlaying = 1;
    data.musicStarted = 1;
    data.unknown94 = 0;
    centreChannels();
}

/* 2:0080 */
void stopMusicSystem()
{
    if (data.musicStarted != 0U) {
        data.musicStarted = 0;
    }
}

/* 2:008E: the song (AMFLOAD.C's module: its header, its patterns and the
 * samples): the speed, the tempo, the orders and 31 instruments of the
 * header's 64, their lengths' low words, their C2 speeds by their
 * finetunes (0-15 in the game's songs). It clears the song's tables first
 * (2F0h-623h). */
void loadSong(const Song &song)
{
    data.musicPlaying = 0;
    data.songLength = 0;
    data.restart = 0;
    std::memset(data.orders, 0, sizeof(data.orders));
    data.patterns = nullptr;
    data.patternsSize = 0;
    for (u16 i = 0; i < INSTRUMENTS; i++) {
        data.sampleData[i] = nullptr;
        data.sampleSizes[i] = 0;
        data.sampleLengths[i] = data.loopStarts[i] = data.loopLengths[i] = data.volumes[i] = data.transposes[i] = 0;
        data.instrumentC2spds[i] = 0;
    }
    const u8 *header = song.header;
    data.module = header;
    data.speed = header[0x20];
    data.tempo = header[0x21];
    data.tickSamples = tickSamplesOf(data.tickRate, data.tempo);
    data.songLength = header[0x24];
    const u8 restart = header[0x25];
    data.restart = restart >= header[0x24] ? 0xff : restart;
    data.patterns = song.patterns;
    data.patternsSize = song.patternsSize;
    std::memcpy(data.orders, header + 0x26, 0x100);
    for (u16 i = 0; i < INSTRUMENTS; i++) {
        const u8 *at = header + 0x13c + (0x25 * i);
        data.sampleLengths[i] = wordOf(at + 3);
        data.volumes[i] = at[1];
        data.transposes[i] = static_cast<u16>(static_cast<s16>(static_cast<s8>(at[2])));
        data.loopStarts[i] = wordOf(at + 7);
        data.loopLengths[i] = wordOf(at + 0xb);
        /* A finetune past 15 reads past the C2 speeds in the original,
         * which the game's songs never make it do: the instrument's is
         * then finetune 0's, untuned. */
        u8 finetune = at[0];
        if (finetune >= 16) {
            passOver(Trouble::FINETUNE, "ASYLUM.DLL's finetune past 15, past its C2 speeds: untuned");
            finetune = 0;
        }
        data.instrumentC2spds[i] = data.c2spds[finetune];
        if (data.sampleLengths[i] != 0U) {
            data.sampleData[i] = song.samples[i];
            data.sampleSizes[i] = song.sizes[i];
        }
    }
}

/* 2:01EB: the song from its start, at the header's speed and tempo, the
 * tick from the mix rate itself (not the tick rate as the others). */
void startSong()
{
    if (data.musicStarted == 0U) {
        return;
    }
    data.order = 0;
    data.speed = data.module[0x20];
    data.speedCount = data.speed;
    data.tempo = data.module[0x21];
    data.row = 0x40;
    data.breakRow = 0;
    data.tickSamples = tickSamplesOf(data.mixRate, data.tempo);
    data.musicPlaying = 1;
    centreChannels();
}

/* 2:0276: no song, the defaults, the music's channels cleared. (It clears
 * 334h bytes at 18ECh as well, where playMusic clears them at 2F0h: bytes
 * nothing reads.) */
void stopSong()
{
    data.unknown94 = 0;
    data.module = nullptr;
    data.musicPlaying = 0;
    data.order = 0;
    data.patternOffset = 0;
    data.patternBlock = nullptr;
    data.speed = 6;
    data.speedCount = 6;
    data.tempo = 0x7d;
    data.row = 0x40;
    data.breakRow = 0;
    data.tickSamples = static_cast<u16>(data.tickRate / 0x32);
    for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
        data.channels[i] = Channel{};
    }
}

/* 2:02E8 */
void setMusicVolume(u16 volume)
{
    const s16 v = static_cast<s16>(volume);
    levels.musicVolume = static_cast<u8>(std::clamp<s16>(v, 0, 0x40));
}

/* 2:0306 */
s16 getMusicVolume()
{
    return static_cast<s8>(levels.musicVolume);
}

}
