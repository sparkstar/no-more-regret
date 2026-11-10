/* ASYLUM.DLL segment 4: the exports the game calls (asyapi.h), the tables
 * ASS_Init makes, the driver it picks, the sample-ended notice and the
 * mixers a channel gets. See asylib.h. */
#include "asylib.h"
#include "gamestate.h"
#include "host.h"

#include <algorithm>
#include <cstdio>

namespace asylum {

Data &data = gameGlobal<Data>();
Levels &levels = gameSetting<Levels>({0x20, 0x40, 1});

namespace {

/* The data as the file has it, when the game starts. */
[[maybe_unused]] const bool LOADED = [] {
    loadData(data);
    return true;
}();

void setDword(u16 at, u32 value)
{
    data.tables.setWord(at, static_cast<u16>(value));
    data.tables.setWord(static_cast<u16>(at + 2), static_cast<u16>(value >> 16));
}

/* The port's depth of the output (setDepth): 8 or 16 bits, or 0 for the DMA
 * channel's. A setting outside the arena, which a snapshot put back keeps:
 * the data's tables and mixers may then be the other depth's, which
 * updateDac makes the depth's. */
u16 &depth = gameSetting<u16>(0);

/* The trouble passOver told, once a run each: the run's and not the
 * game's, outside the arena (a rewind past the notice keeps it told). */
bool (&told)[static_cast<u8>(Trouble::COUNT)] = gameSetting<bool[static_cast<u8>(Trouble::COUNT)]>();
/* The two bytes outsideTables gives, 0 each time, which nothing else reads:
 * outside the arena too. */
u8 (&nothing)[2] = gameSetting<u8[2]>();

/* Whether the mix is the 16-bit output's: the depth's, else the DMA
 * channel's. */
u16 depthSixteenBit()
{
    if (depth != 0U) {
        return depth == 16 ? 1 : 0;
    }
    return static_cast<s16>(data.config.dma) >= 4 ? 1 : 0;
}

/* Whether the 8-bit mix goes out as words: the port's 8-bit precision. */
u16 depthEightBitWords()
{
    return depth == 8 ? 1 : 0;
}

/* The mixer of the same kind for the 16-bit output or the 8-bit, by the
 * mixer (Mixer's order). */
const Mixer SIXTEEN_BIT_MIXERS[] = {MIX_NONE,         MIX_MONO16,   MIX_STEREO16,    MIX_SURROUND16,
                                    MIX_MONO16,       MIX_STEREO16, MIX_SURROUND16,  MIX_STREAM16,
                                    MIX_STREAM16MONO, MIX_STREAM16, MIX_STREAM16MONO};
const Mixer EIGHT_BIT_MIXERS[] = {MIX_NONE,        MIX_MONO8,   MIX_STEREO8,    MIX_SURROUND8,
                                  MIX_MONO8,       MIX_STEREO8, MIX_SURROUND8,  MIX_STREAM8,
                                  MIX_STREAM8MONO, MIX_STREAM8, MIX_STREAM8MONO};
static_assert(sizeof(SIXTEEN_BIT_MIXERS) == MIX_STREAM8MONO + 1 && sizeof(EIGHT_BIT_MIXERS) == MIX_STREAM8MONO + 1);

/* The oversampling's last sample, halved, of the 16-bit output (an offset
 * from 8000h) and of the port's 8-bit words (the 8-bit output's byte, past
 * 0-255), one in the other's. */
u16 halfWordsOfSixteen(u16 half)
{
    const s32 word = static_cast<s32>(static_cast<u16>(half << 1)) - 0x8000;
    return static_cast<u16>(static_cast<s16>(((word >> 6) + 0x80) >> 1));
}

u16 halfSixteenOfWords(u16 half)
{
    const s32 word = ((static_cast<s32>(static_cast<s16>(half)) << 1) - 0x80) * 64;
    return static_cast<u16>(static_cast<u16>(word + 0x8000) >> 1);
}

u32 eachHalf(u32 pair, u16 (*convert)(u16))
{
    return static_cast<u32>(convert(static_cast<u16>(pair >> 16))) << 16 | convert(static_cast<u16>(pair));
}

/* The port's change between the 16-bit mix and the 8-bit one in words
 * (setDepth), the ring going on: the tick's sum not yet out and the
 * oversampling's last samples in the new mix's units, the 16-bit one's 64
 * times the 8-bit one's. */
void convertMix(bool toSixteen)
{
    Tables &t = data.tables;
    const u32 words = static_cast<u32>(data.tickLeft & 0x3fff) * 2;
    for (u32 k = 0; k < words; k++) {
        const u16 at = static_cast<u16>(data.mixPointer + (2 * k));
        const s32 sum = static_cast<s16>(t.word(at));
        if (toSixteen) {
            t.setWord(at, static_cast<u16>(std::clamp(sum, WORDS_SUM_LOW, WORDS_SUM_HIGH) * 64));
        } else {
            t.setWord(at, static_cast<u16>(sum >> 6));
        }
    }
    if (toSixteen) {
        data.oversample16 = eachHalf(data.oversampleWords, halfSixteenOfWords);
    } else {
        data.oversampleWords = eachHalf(data.oversample16, halfWordsOfSixteen);
    }
}

}

void passOver(Trouble trouble, const char *text)
{
    bool &once = told[static_cast<u8>(trouble)];
    if (!once) {
        once = true;
        hostNotice(text);
    }
}

u8 *outsideTables(u16 at)
{
    char text[120];
    std::snprintf(text, sizeof(text), "ASYLUM.DLL's data at %04Xh, past its tables: read as 0, written nowhere", at);
    passOver(Trouble::OUTSIDE_TABLES, text);
    nothing[0] = 0;
    nothing[1] = 0;
    return nothing;
}

const u8 *Samples::at(u32 position, u32 bytes) const
{
    /* What a read past the block gives (bytes are 4 at most): silence in
     * every mixer, volume8's and volume16's rows at 0 and stream8's
     * identity; the 8-bit surround's other side, ~0, is 1 below it. */
    static const u8 silence[4] = {};
    const u8 *block = (slot != nullptr) ? static_cast<const u8 *>(*slot) : data;
    const u32 limit = (slot != nullptr) ? *slotSize : size;
    if (position > limit || limit - position < bytes) {
        passOver(Trouble::READ_PAST_BLOCK,
                 "ASYLUM.DLL's read of samples past their block, where the original faults: silence");
        return silence;
    }
    return block + position;
}

void divideError()
{
    passOver(Trouble::DIVIDE_ERROR, "ASYLUM.DLL's divide error, where the original faults: a quotient of 0");
}

/* 4:036D: the volume tables at 4524h, 65 rows of 256 by the sample's byte
 * (its low half positive, its high half the negatives from -1 down), and
 * the master volume's table at C926h. 16-bit: words of sample * row / 2,
 * a row 41h of sample * 64; 8-bit: bytes of sample * row / 128 and the
 * identity at 8624h. The negatives are the positives' negations, one
 * place over. */
void buildVolumes()
{
    Tables &t = data.tables;
    if (data.sixteenBit != 0U) {
        for (u32 i = 0; i < 0x41; i++) {
            const u32 step = (0x40 - i) << 16;
            u32 sum = 0;
            const u16 row = static_cast<u16>((0x40 - i) << 8);
            for (u16 k = 0; k < 0x80; k++) {
                const u16 value = static_cast<u16>(static_cast<s32>(sum) >> 17);
                t.setWord(static_cast<u16>(VOLUMES_AT + static_cast<u16>(static_cast<u16>(row + k) << 1)), value);
                t.setWord(static_cast<u16>(VOLUMES_AT + static_cast<u16>(static_cast<u16>(row + 0xff - k) << 1)),
                          static_cast<u16>(-value));
                sum += step;
            }
        }
        u32 sum = 0;
        for (u16 k = 0; k < 0x80; k++) {
            const u16 value = static_cast<u16>(static_cast<s32>(sum) >> 16);
            t.setWord(static_cast<u16>(VOLUMES_AT + static_cast<u16>(static_cast<u16>(0x4100 + k) << 1)), value);
            t.setWord(static_cast<u16>(VOLUMES_AT + static_cast<u16>(static_cast<u16>(0x4100 + 0xff - k) << 1)),
                      static_cast<u16>(-value));
            sum += 0x400000;
        }
    } else {
        for (u32 i = 0; i < 0x41; i++) {
            const u32 step = ((0x40 - i) << 16) >> 7;
            u32 sum = 0;
            u16 at = static_cast<u16>(((0x40 - i) << 8) + VOLUMES_AT);
            u16 back = static_cast<u16>(at + 0xff);
            for (u16 k = 0; k < 0x80; k++) {
                const u8 value = static_cast<u8>(static_cast<s32>(sum) >> 16);
                t.byte(at++) = value;
                t.byte(back--) = static_cast<u8>(-value);
                sum += step;
            }
        }
        for (u16 k = 0; k < 0x100; k++) {
            t.byte(static_cast<u16>(IDENTITY8_AT + k)) = static_cast<u8>(k);
        }
    }
    for (u32 i = 0; i < 0x41; i++) {
        const u32 step = (i != 0U) ? (i << 16) >> 6 : 0;
        u32 sum = 0;
        u16 at = static_cast<u16>((i << 7) + VOLUME_SCALE_AT);
        for (u16 k = 0; k < 0x41; k++) {
            t.byte(at++) = static_cast<u8>(sum >> 16);
            sum += step;
        }
    }
}

/* 4:0532: the clipping of the 8-bit output, whose sums index 40A4h: 400h
 * zeros, the bytes 0-255, 400h of FFh. The 16-bit output does not clip; its
 * table is of the signed bytes at 4424h. */
void buildClip()
{
    Tables &t = data.tables;
    if (data.sixteenBit != 0U) {
        u16 back = 0x4523;
        for (u16 k = 0; k < 0x80; k++) {
            const u8 value = static_cast<u8>(k > 0x7f ? 0x7f : k);
            t.byte(static_cast<u16>(0x4424 + k)) = value;
            t.byte(back--) = static_cast<u8>(-value);
        }
        return;
    }
    for (u16 k = 0; k < 0x400; k++) {
        t.byte(static_cast<u16>(CLIP_AT + k)) = 0;
        t.byte(static_cast<u16>(0x4124 + k)) = 0xff;
    }
    for (u16 k = 0; k < 0x100; k++) {
        t.byte(static_cast<u16>(CLIP_AT + 0x400 + k)) = static_cast<u8>(k);
    }
}

/* 4:058F: the settings of the config and the card's driver: the port's
 * output is the Sound Blaster 16's (card 3) for every card. The original
 * had the drivers of cards 1 (Sound Blaster), 2 (Pro) and 4 (Ensoniq
 * Soundscape) as well, and for 0 (none) and the others it left the
 * driver's pointers as they were; crusader.cfg's soundcard stays the
 * player's, which an original sharing the folder (GOG's DOSBox) reads. A
 * 16-bit DMA channel makes the output 16-bit, whose ticks halve the tick
 * rate (until the driver's start sets it again). The driver's far
 * pointers (EB38h), which a note past the periods reads, are the
 * original's in the oracle machine. */
void pickDriver()
{
    data.unknown107 = 0;
    data.port = data.config.port;
    data.irq = static_cast<u8>(data.config.irq);
    data.dma = static_cast<u8>(data.config.dma);
    data.music = data.config.music;
    data.sound = data.config.sound;
    data.mixRate = data.config.rate;
    data.rate = data.config.rate;
    data.tickRate = data.config.rate;
    const u16 driver = 0x1038;
    setDword(DRIVER_AT, static_cast<u32>(driver) << 16 | 0x0050);
    setDword(static_cast<u16>(DRIVER_AT + 4), static_cast<u32>(driver) << 16 | 0x0178);
    setDword(static_cast<u16>(DRIVER_AT + 8), static_cast<u32>(driver) << 16 | 0x01af);
    /* The DMA channel's, or the port's depth in its place (setDepth). */
    data.sixteenBit = depthSixteenBit();
    data.eightBitWords = depthEightBitWords();
    if (data.sixteenBit != 0U) {
        data.tickRate >>= 1;
    }
}

/* 4:093D: 96 periods, eight octaves of the twelve at 129h, each times 16
 * times 8363 shifted down by its octave. */
void buildPeriods()
{
    for (u16 i = 0; i < 96; i++) {
        setDword(static_cast<u16>(PERIODS_AT + (4 * i)),
                 static_cast<u32>(static_cast<s32>(PERIOD_BASE[i % 12]) * 16 * 0x20ab) >> static_cast<u8>(i / 12));
    }
}

/* 4:07F6: the mixer's channel notifyChannel has ended its sample. A sound
 * effect's channel tells the callback, whose sample the callback may have
 * started again (the record's state 1): then the channel takes it at
 * once. */
void notifySampleEnd()
{
    if (data.callback == nullptr) {
        return;
    }
    s16 channel = static_cast<s8>(data.notifyChannel);
    if (channel < 8 || channel >= 20) {
        return;
    }
    channel -= 8;
    SfxRecord &record = data.sfx[channel];
    record.state = 0;
    data.callback(channel);
    if (data.sfx[channel].state != 1) {
        return;
    }
    data.sfx[channel].state = 2;
    Channel &mixed = data.channels[MUSIC_CHANNELS + channel];
    mixed.active = 1;
    mixed.data = data.sfx[channel].data;
    mixed.length = data.sfx[channel].length;
    mixed.loopStart = data.sfx[channel].loopStart;
    mixed.loopLength = data.sfx[channel].loopLength;
    mixed.position = 0;
    mixed.fraction = 0;
}

/* 4:0DE4: the mixer of a channel of 8-bit samples. */
Mixer sampleMixer()
{
    if (data.sixteenBit != 0U) {
        return (data.stereo != 0U) ? MIX_STEREO16 : MIX_MONO16;
    }
    return (data.stereo != 0U) ? MIX_STEREO8 : MIX_MONO8;
}

/* 4:0E09: the mixer of a stream of 16-bit stereo samples. */
Mixer streamMixer()
{
    if (data.sixteenBit != 0U) {
        return (data.stereo != 0U) ? MIX_STREAM16 : MIX_STREAM16MONO;
    }
    return (data.stereo != 0U) ? MIX_STREAM8 : MIX_STREAM8MONO;
}

/* 4:0E2E: the mixer for the pan of E8x: 0 is the surround. */
Mixer panMixer(u16 pan)
{
    if (data.sixteenBit != 0U) {
        if (data.stereo == 0U) {
            return MIX_MONO16;
        }
        return (pan != 0U) ? MIX_STEREO16 : MIX_SURROUND16;
    }
    if (data.stereo == 0U) {
        return MIX_MONO8;
    }
    return (pan != 0U) ? MIX_STEREO8 : MIX_SURROUND8;
}

/* 4:0E65: the pans of E8x, from the tables at 141h and 161h. */
void setPan(u16 pan)
{
    if ((data.stereo != 0U) && (pan != 0U)) {
        data.panRight = PAN_RIGHTS[static_cast<u16>(pan << 1) & 0x1f];
        data.panLeft = PAN_LEFTS[static_cast<u16>(pan << 1) & 0x1f];
        return;
    }
    data.panLeft = 0x40;
    data.panRight = 0x40;
}

/* The exports. */

/* 4:0000. The config goes after the periods as well (EB26h), where a note
 * past them reads it; the original's hook of the card's interrupt after
 * it, which the module has not, is 0. It clears the first channel only;
 * a start after an end keeps the sound effects' channels as they were. */
void init(const Config &config)
{
    if (data.initialized != 0U) {
        return;
    }
    data.config = config;
    const u16 fields[7] = {config.card, config.port, config.irq, config.dma, config.rate, config.music, config.sound};
    for (int k = 0; k < 7; k++) {
        data.tables.setWord(static_cast<u16>(CONFIG_AT + (2 * k)), fields[k]);
    }
    setDword(static_cast<u16>(CONFIG_AT + 14), 0);
    data.channels[0] = Channel{};
    data.mixPointer = 0;
    data.tickLeft = 0;
    data.order = 0;
    data.speed = 6;
    data.speedCount = 6;
    data.tempo = 0x7d;
    data.row = 0x40;
    data.breakRow = 0;
    data.tickSamples = static_cast<u16>(data.mixRate / 50);
    pickDriver();
    buildPeriods();
    buildVolumes();
    buildClip();
    /* 4:0ACE: the DMA buffer, 4000h bytes of DOS memory, silent. */
    std::memset(data.ring, wordOutput() ? 0 : 0x80, sizeof(data.ring));
    if (data.music != 0U) {
        startMusicSystem();
    }
    startSfxSystem();
    startOutput();
    data.halfBuffer = wordOutput() ? 0x2000 : 0x1000;
    data.initialized = 1;
}

void uninit()
{
    if (data.initialized == 0U) {
        return;
    }
    stopOutput();
    if (data.music != 0U) {
        stopMusicSystem();
    }
    if (data.sound != 0U) {
        stopSfxSystem();
    }
    data.initialized = 0;
}

void setDepth(u16 bits)
{
    depth = bits == 8 || bits == 16 ? bits : 0;
    const u16 sixteen = depthSixteenBit();
    const u16 words = depthEightBitWords();
    if (data.initialized == 0U || (sixteen == data.sixteenBit && words == data.eightBitWords)) {
        return;
    }
    /* Between the port's two (or the 16-bit DMA's mix and the port's 8-bit)
     * the ring takes words in both: it goes on as it plays, the mixed
     * samples in it at the same level. */
    const bool sameRing = wordOutput() == (sixteen != 0U || words != 0U);
    if (sameRing) {
        convertMix(sixteen != 0U);
    } else {
        stopOutput();
    }
    data.sixteenBit = sixteen;
    data.eightBitWords = words;
    /* The bytes the depths' tables take, 0 as the file has them, and the
     * depth's tables as init builds them. */
    for (u32 at = CLIP_AT; at < VOLUME_SCALE_AT; at++) {
        data.tables.byte(static_cast<u16>(at)) = 0;
    }
    buildVolumes();
    buildClip();
    for (Channel &channel : data.channels) {
        channel.mixer = (sixteen != 0U) ? SIXTEEN_BIT_MIXERS[channel.mixer] : EIGHT_BIT_MIXERS[channel.mixer];
    }
    if (sixteen != 0U) {
        /* The song's pans, which the 8-bit mix wrote its own over. */
        for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
            data.channels[i].panLeft = data.channels[i].musicPanLeft;
            data.channels[i].panRight = data.channels[i].musicPanRight;
        }
    }
    if (sameRing) {
        return;
    }
    /* A tick of its own from the next sample on; the oversampling's sample
     * before the first as the file has it; the ring silent, its first half
     * the next to fill, as init leaves them. */
    data.tickLeft = 0;
    data.oversample8 = 0x50;
    data.oversample16 = 0;
    data.oversampleWords = 0x50;
    std::memset(data.ring, wordOutput() ? 0 : 0x80, sizeof(data.ring));
    data.fillHalf = 0;
    data.halfBuffer = wordOutput() ? 0x2000 : 0x1000;
    restartOutput();
}

u16 getDepth()
{
    return depth;
}

u16 outputBits()
{
    if (data.initialized == 0U) {
        return 0;
    }
    if (depth != 0U) {
        return depth;
    }
    return (data.sixteenBit != 0U) ? 16 : 8;
}

/* The driver's update, which the driver's pointer names once a card was
 * picked: before, none (the original called through the pointer of 0).
 * The fills take EBX's high word from their caller: 0 here, where the
 * original has what the code the timer interrupted left there. */
void updateDac()
{
    if (data.tables.dword(static_cast<u16>(DRIVER_AT + 8)) == 0U) {
        passOver(Trouble::UPDATE_BEFORE_CARD, "ASYLUM.DLL's update before a card was picked: none");
        return;
    }
    /* A snapshot put back keeps the depth and gives back the data: the
     * other depth's tables and mixers made the depth's, as its change makes
     * them. */
    if (depthSixteenBit() != data.sixteenBit || depthEightBitWords() != data.eightBitWords) {
        setDepth(depth);
    }
    if (data.sixteenBit != 0U) {
        fill16();
    } else if (data.eightBitWords != 0U) {
        fill8Words();
    } else {
        fill8();
    }
}

void startMusic()
{
    for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
        data.channels[i].active = 0;
    }
    startSong();
}

void stopMusic()
{
    stopSong();
    for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
        data.channels[i].active = 0;
    }
}

void pauseMusic()
{
    data.musicPaused = 1;
}

void resumeMusic()
{
    data.musicPaused = 0;
}

/* 4:02EB: cards 1-4 by their names, "N/A" otherwise, into name. */
void getCardName(s16 card, char *name)
{
    const char *text = "N/A";
    if (static_cast<u16>(card - 1) <= 3) {
        text = CARD_NAMES[card];
    }
    std::strcpy(name, text);
}

void stopAllSfx()
{
    for (s16 i = 0; i < static_cast<s16>(SFX_CHANNELS); i++) {
        stopSfx(i);
    }
}

void registerCallback(Callback callback)
{
    data.callback = callback;
}

/* 4:09BB, in V1.06 too (4:0968). */
void notifySampleChange(s16 /*unused*/, s32 /*unused*/, u32 /*unused*/) {}

void playMusic(const Song &song)
{
    loadSong(song);
}

void setOversampling()
{
    levels.oversampling = 1;
}

void clrOversampling()
{
    levels.oversampling = 0;
}

u16 getOversampling()
{
    return levels.oversampling;
}

/* 4:0E9F: the bytes left of the DMA transfer, from the DMA's count (words
 * of 16-bit output), less the half the fill waits for, as bytes of the
 * mix. */
u16 getDmaBytesLeft()
{
    const u16 half = data.halfBuffer >> 1;
    u16 left = dmaCount();
    if (wordOutput()) {
        left = static_cast<u16>(left * 2);
    }
    if (data.fillHalf == 1) {
        if (left >= half) {
            left = static_cast<u16>(left - half);
        } else {
            left = static_cast<u16>(half + left);
        }
    }
    if (!wordOutput()) {
        left = static_cast<u16>(left * 2);
    } else if (data.stereo == 1) {
        left = static_cast<u16>(left * 2);
    }
    return left;
}

}
