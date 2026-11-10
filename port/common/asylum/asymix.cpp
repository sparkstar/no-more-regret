/* ASYLUM.DLL segment 1, the mixer: the channels' mixers into the sum of a
 * tick, the loops that make the ticks and put them out to the ring, and
 * the fills of the ring's halves. See asylib.h.
 *
 * A mixer adds count samples of its channel into the sum from pointer (an
 * offset of the tables, words: two a sample in stereo) as the hand-written
 * original does: the position in samples with a 16-bit fraction, the step
 * 16.16, added through EDI, whose low word is the pointer (its carry goes
 * into the fraction). Its count is kept in a word of its own, which the
 * sample-ended notice leaves as it was so that a sample the callback starts
 * again fills the rest; that goes on without looping whatever the new
 * sample's loop. */
#include "asylib.h"

#include <algorithm>

namespace asylum {

namespace {

/* The master volume's table: a * b / 64, as BL takes them (b added to the
 * low byte of a << 7). */
u8 scaled(u8 a, u8 b)
{
    u16 bx = static_cast<u16>(a << 7);
    bx = static_cast<u16>((bx & 0xff00) | static_cast<u8>(bx + b));
    return data.tables.byte(static_cast<u16>(bx + VOLUME_SCALE_AT));
}

/* A channel's volume: its own through the master volume, 41h unchanged. */
u8 channelVolume(const Channel &c)
{
    return c.volume == 0x41 ? 0x41 : scaled(data.masterVolume, c.volume);
}

/* The sample through the volume tables: a byte of the 8-bit output's
 * table as CBW makes it a word, or a word of the 16-bit output's. */
u16 volume8(u8 volume, u8 sample)
{
    return static_cast<u16>(
        static_cast<s16>(static_cast<s8>(data.tables.byte(static_cast<u16>(VOLUMES_AT + (volume << 8 | sample))))));
}

u16 volume16(u8 volume, u8 sample)
{
    return data.tables.word(static_cast<u16>(VOLUMES_AT + static_cast<u16>((volume << 8 | sample) << 1)));
}

void setWord(u16 at, u16 value)
{
    data.tables.setWord(at, value);
}

void addWord(u16 at, u16 value)
{
    setWord(at, static_cast<u16>(data.tables.word(at) + value));
}

/* ADD DWORD PTR [at],value: the low word's carry into the high one. */
void addDword(u16 at, u32 value)
{
    const u32 sum = (static_cast<u32>(data.tables.word(static_cast<u16>(at + 2))) << 16 | data.tables.word(at)) + value;
    setWord(at, static_cast<u16>(sum));
    setWord(static_cast<u16>(at + 2), static_cast<u16>(sum >> 16));
}

/* A stream's 16-bit sample in the 8-bit output's sum: its high byte through
 * the identity table; in the port's 8-bit precision its top ten bits, which
 * mixOut8Words puts out 64 times: the stream at the level it has in the
 * 16-bit output, which puts it out as it is. */
u16 stream8(u8 low, u8 high)
{
    if (data.eightBitWords != 0U) {
        return static_cast<u16>(static_cast<s16>(static_cast<u16>(low | high << 8)) >> 6);
    }
    return static_cast<u16>(static_cast<s16>(static_cast<s8>(data.tables.byte(static_cast<u16>(IDENTITY8_AT + high)))));
}

bool stream(Mixer kind)
{
    return kind >= MIX_STREAM16;
}

bool stereoOutput(Mixer kind)
{
    return kind == MIX_STEREO8 || kind == MIX_SURROUND8 || kind == MIX_STEREO16 || kind == MIX_SURROUND16 ||
           kind == MIX_STREAM16 || kind == MIX_STREAM8;
}

/* One sample's mix at pointer: the byte of an 8-bit sample, or the dword
 * of a stream's 16-bit pair. */
void mixOne(Mixer kind, u16 at, const u8 *sample, u8 left, u8 right)
{
    switch (kind) {
    case MIX_MONO8:
        addWord(at, volume8(left, sample[0]));
        return;
    case MIX_STEREO8:
        addWord(at, volume8(left, sample[0]));
        addWord(static_cast<u16>(at + 2), volume8(right, sample[0]));
        return;
    case MIX_SURROUND8: {
        const u16 value = volume8(left, sample[0]);
        addWord(at, value);
        addWord(static_cast<u16>(at + 2), static_cast<u16>(~value));
        return;
    }
    case MIX_MONO16:
        addWord(at, volume16(left, sample[0]));
        return;
    case MIX_STEREO16:
        addDword(at, static_cast<u32>(volume16(right, sample[0])) << 16 | volume16(left, sample[0]));
        return;
    case MIX_SURROUND16: {
        const u16 value = volume16(left, sample[0]);
        addDword(at, static_cast<u32>(value) << 16 | static_cast<u16>(~value));
        return;
    }
    case MIX_STREAM16:
        setWord(at, static_cast<u16>(sample[0] | sample[1] << 8));
        setWord(static_cast<u16>(at + 2), static_cast<u16>(sample[2] | sample[3] << 8));
        return;
    case MIX_STREAM16MONO:
        setWord(at, static_cast<u16>(sample[0] | sample[1] << 8));
        return;
    case MIX_STREAM8:
        setWord(at, stream8(sample[0], sample[1]));
        setWord(static_cast<u16>(at + 2), stream8(sample[2], sample[3]));
        return;
    case MIX_STREAM8MONO:
        /* The sum of the two, then the right one over it. */
        setWord(at, stream8(sample[0], sample[1]));
        addWord(at, stream8(sample[2], sample[3]));
        setWord(at, stream8(sample[2], sample[3]));
        return;
    case MIX_NONE:
        return;
    }
}

}

void mixChannel(Mixer kind, Channel &c, u16 pointer, u16 count)
{
    if (kind == MIX_NONE) {
        passOver(Trouble::NO_MIXER, "ASYLUM.DLL's channel without a mixer: not mixed");
        return;
    }
    u16 &saved = data.savedCounts[kind - 1];
    if ((c.active == 0U) || c.paused == 1 || c.length == 0) {
        return;
    }
    bool looping = !stream(kind) && c.loopLength > 2;
    const u16 increment = stereoOutput(kind) ? 4 : 2;
    const u32 unit = stream(kind) ? 4 : 1;
    for (;;) {
        /* LES DX,[SI]: where the samples are, whatever the mix writes over
         * the channel meanwhile. */
        const Samples samples = c.data;
        const u32 offsetOf = samples.offset;
        u32 at = (c.position * unit) + offsetOf;
        u32 end;
        if (looping) {
            data.loopLength = c.loopLength;
            end = c.loopLength + c.loopStart + offsetOf;
        } else {
            end = c.length * unit + offsetOf;
        }
        saved = count;
        /* The volumes: BH of the low word the first, of the high word the
         * second (a stereo mixer's left and right). */
        u8 left = 0;
        u8 right = 0;
        if (!stream(kind)) {
            const u8 volume = channelVolume(c);
            left = right = volume;
            if (c.volume != 0x41 && (kind == MIX_STEREO8 || kind == MIX_STEREO16)) {
                left = scaled(c.panLeft, volume);
                right = scaled(c.panRight, volume);
            }
        }
        u32 edi = static_cast<u32>(c.fraction) << 16 | pointer;
        const u32 stepLow = (c.step & 0xffff) << 16 | increment;
        const u32 stepHigh = c.step >> 16;
        bool ended = false;
        for (;;) {
            if (at >= end) {
                if (!looping) {
                    ended = true;
                    break;
                }
                at -= data.loopLength;
            }
            mixOne(kind, static_cast<u16>(edi), samples.at(at, unit), left, right);
            const std::uint64_t sum = static_cast<std::uint64_t>(edi) + stepLow;
            edi = static_cast<u32>(sum);
            at += (stepHigh + static_cast<u32>(sum >> 32)) * unit;
            if (--saved == 0U) {
                break;
            }
        }
        c.position = (at - offsetOf) / unit;
        c.fraction = static_cast<u16>(edi >> 16);
        if (!ended) {
            return;
        }
        /* The mono and 8-bit surround mixers work the position out in EBX
         * at the end (00ADh, 0437h, 05B8h) and leave it there. */
        if (kind == MIX_MONO8 || kind == MIX_SURROUND8 || kind == MIX_MONO16) {
            playerEbxHigh = static_cast<u16>(c.position >> 16);
        }
        c.active = 0;
        notifySampleEnd();
        if (c.active == 0U) {
            return;
        }
        count = saved;
        /* The 8-bit surround mixer (0437h) turns EDI only once before the
         * notice: it goes on at the old fraction as the pointer (which the
         * game's songs, without the surround's E80, never get to). */
        pointer = kind == MIX_SURROUND8 ? static_cast<u16>(edi >> 16) : static_cast<u16>(edi);
        looping = false;
    }
}

namespace {

/* The start of a tick: the sum cleared (4 bytes a sample, to 3FFFh samples),
 * the music's tick and channels at the music's volume, the sound effects'
 * starts and channels at the sound's. mixMusic mixes the music's channels. */
template <class Music> void mixTick(Music mixMusic)
{
    data.mixPointer = SUM_AT;
    u16 samples = data.tickSamples;
    if (samples == 0U) {
        /* A tick of no samples (a song's tempo below 3 or past 7Fh, a
         * divide error's 0), which the original's fills went round for
         * ever: the port's is the tick of no song, 50 a second. */
        samples = static_cast<u16>(data.tickRate / 0x32);
    }
    data.tickLeft = samples;
    const u32 bytes = static_cast<u32>(samples & 0x3fff) * 4;
    for (u32 k = 0; k < bytes; k++) {
        data.tables.byte(static_cast<u16>(SUM_AT + k)) = 0;
    }
    data.notifyChannel = 0;
    if (data.musicPlaying && data.musicPaused != 1) {
        musicTick();
        data.masterVolume = levels.musicVolume;
        for (u16 i = 0; i < MUSIC_CHANNELS; i++) {
            mixMusic(i);
            data.notifyChannel++;
        }
    }
    data.notifyChannel = 8;
    startSounds();
    data.masterVolume = levels.soundVolume;
    for (u16 i = 0; i < SFX_CHANNELS; i++) {
        Channel &c = data.channels[MUSIC_CHANNELS + i];
        mixChannel(c.mixer, c, data.mixPointer, data.tickLeft);
        data.notifyChannel++;
    }
}

}

/* 1:1EC0 count samples of 16-bit stereo to buffer, the music's channels
 * through their own mixers. The output is the sum as it is, without
 * clipping; oversampling halves each sample with the one before (their
 * offsets from 8000h). */
void mixOut16(u8 *buffer, u16 count)
{
    u16 at = 0;
    while (count != 0U) {
        if (data.tickLeft == 0U) {
            mixTick([](u16 i) {
                Channel &c = data.channels[i];
                mixChannel(c.mixer, c, data.mixPointer, data.tickLeft);
            });
        }
        const u16 n = data.tickLeft < count ? data.tickLeft : count;
        u16 from = data.mixPointer;
        data.mixPointer = static_cast<u16>(data.mixPointer + (4 * n));
        data.tickLeft = static_cast<u16>(data.tickLeft - n);
        count = static_cast<u16>(count - n);
        if (levels.oversampling != 0U) {
            u32 last = data.oversample16;
            u16 k = n;
            do {
                for (int half = 0; half < 2; half++) {
                    const u16 sample = static_cast<u16>(static_cast<u16>(data.tables.word(from) + 0x8000) >> 1);
                    from = static_cast<u16>(from + 2);
                    const u16 value = static_cast<u16>(static_cast<u16>(last) + sample + 0x8000);
                    buffer[at] = static_cast<u8>(value);
                    buffer[static_cast<u16>(at + 1)] = static_cast<u8>(value >> 8);
                    at = static_cast<u16>(at + 2);
                    last = (last & 0xffff0000) | sample;
                    last = last >> 16 | last << 16;
                }
            } while (--k != 0U);
            data.oversample16 = last;
        } else {
            u16 k = n;
            do {
                for (int b = 0; b < 4; b++) {
                    buffer[static_cast<u16>(at + b)] = data.tables.byte(static_cast<u16>(from + b));
                }
                from = static_cast<u16>(from + 4);
                at = static_cast<u16>(at + 4);
            } while (--k != 0U);
        }
    }
}

namespace {

/* The 8-bit output's music channel i: 0 and 7 through the surround mixer,
 * 1-6 through the stereo one at pans of their own (40h and 0, left and
 * right in turn). */
void mixMusic8(u16 i)
{
    static const u8 pans[8][2] = {{0, 0},    {0x40, 0x40}, {0x40, 0},    {0, 0x40},
                                  {0x40, 0}, {0, 0x40},    {0x40, 0x40}, {0, 0}};
    Channel &c = data.channels[i];
    if (i == 0 || i == 7) {
        mixChannel(MIX_SURROUND8, c, data.mixPointer, data.tickLeft);
        return;
    }
    c.panRight = pans[i][0];
    c.panLeft = pans[i][1];
    mixChannel(MIX_STEREO8, c, data.mixPointer, data.tickLeft);
}

}

/* 1:2144 count samples of 8-bit stereo to buffer: the music's channels
 * through mixMusic8; the sums clipped through the table at 40A4h, which a
 * sum past its ends reads beyond (the sum's end below, the volumes' zeros
 * above); oversampling halves each with the one before. */
void mixOut8(u8 *buffer, u16 count)
{
    u16 at = 0;
    while (count != 0U) {
        if (data.tickLeft == 0U) {
            mixTick(mixMusic8);
        }
        const u16 n = data.tickLeft < count ? data.tickLeft : count;
        u16 from = data.mixPointer;
        data.mixPointer = static_cast<u16>(data.mixPointer + (4 * n));
        data.tickLeft = static_cast<u16>(data.tickLeft - n);
        count = static_cast<u16>(count - n);
        auto clipped = [&]() {
            const u8 value = data.tables.byte(static_cast<u16>(data.tables.word(from) + CLIP8_AT));
            from = static_cast<u16>(from + 2);
            return value;
        };
        if (levels.oversampling != 0U) {
            u16 last = data.oversample8;
            u16 k = n;
            do {
                for (int half = 0; half < 2; half++) {
                    const u8 sample = static_cast<u8>(clipped() >> 1);
                    const u8 value = static_cast<u8>(static_cast<u8>(last) + sample);
                    buffer[at] = value;
                    at = static_cast<u16>(at + 1);
                    last = static_cast<u16>((last & 0xff00) | sample);
                    last = static_cast<u16>(last >> 8 | last << 8);
                }
            } while (--k != 0U);
            data.oversample8 = last;
        } else {
            u16 k = n;
            do {
                buffer[at] = clipped();
                buffer[static_cast<u16>(at + 1)] = clipped();
                at = static_cast<u16>(at + 2);
            } while (--k != 0U);
        }
    }
}

/* The port's 8-bit precision (asyapi.h's setDepth): count samples of
 * 16-bit stereo to buffer from mixOut8's mix, each sum put out 64 times as
 * a word (the 16-bit output's level, a quarter of the 8-bit one's), clipped
 * at the words' ends rather than the bytes': where mixOut8 does not clip,
 * a word is its byte's offset from 80h times 64. Oversampling halves each
 * with the one before as mixOut8 does, its last ones a word each. */
void mixOut8Words(u8 *buffer, u16 count)
{
    u16 at = 0;
    auto put = [&](s32 value) {
        const u16 word = static_cast<u16>((value - 0x80) * 64);
        buffer[at] = static_cast<u8>(word);
        buffer[static_cast<u16>(at + 1)] = static_cast<u8>(word >> 8);
        at = static_cast<u16>(at + 2);
    };
    while (count != 0U) {
        if (data.tickLeft == 0U) {
            mixTick(mixMusic8);
        }
        const u16 n = data.tickLeft < count ? data.tickLeft : count;
        u16 from = data.mixPointer;
        data.mixPointer = static_cast<u16>(data.mixPointer + (4 * n));
        data.tickLeft = static_cast<u16>(data.tickLeft - n);
        count = static_cast<u16>(count - n);
        /* The sum as mixOut8's byte, 80h its zero, past 0-255 to the
         * words' ends. */
        auto asByte = [&]() {
            const s32 sum = static_cast<s16>(data.tables.word(from));
            from = static_cast<u16>(from + 2);
            return std::clamp(sum, WORDS_SUM_LOW, WORDS_SUM_HIGH) + 0x80;
        };
        if (levels.oversampling != 0U) {
            u32 last = data.oversampleWords;
            u16 k = n;
            do {
                for (int half = 0; half < 2; half++) {
                    const s32 sample = asByte() >> 1;
                    put(static_cast<s16>(static_cast<u16>(last)) + sample);
                    last = (last & 0xffff0000) | static_cast<u16>(sample);
                    last = last >> 16 | last << 16;
                }
            } while (--k != 0U);
            data.oversampleWords = last;
        } else {
            u16 k = n;
            do {
                put(asByte());
                put(asByte());
            } while (--k != 0U);
        }
    }
}

namespace {

/* The fill of a ring of 16-bit stereo through mixOut, as 1:0ECA. */
void fillWords(void (*mixOut)(u8 *, u16))
{
    const u16 left = dmaCount();
    playerEbxHigh = 0;
    if (data.fillHalf == 0) {
        if (left >= 0x800) {
            return;
        }
        mixOut(data.ring, 0x400);
        data.fillHalf++;
    } else {
        if (left < 0x800) {
            return;
        }
        mixOut(data.ring + 0x1000, 0x400);
        data.fillHalf--;
    }
}

}

/* 1:0ECA the fill of 16-bit stereo: the DMA's count of words left says
 * which half plays; the other half, 400h samples, is mixed when the half
 * to fill next is not the one that plays. The fill takes EBX's high word
 * from its caller: 0 (see updateDac). */
void fill16()
{
    fillWords(mixOut16);
}

/* The port's 8-bit precision's fill: 16-bit stereo, as fill16. */
void fill8Words()
{
    fillWords(mixOut8Words);
}

/* 1:0E73 the fill of 8-bit stereo: the count is of bytes, 2000h of them,
 * and the halves change at 800h, a quarter from the end. */
void fill8()
{
    const u16 left = dmaCount();
    playerEbxHigh = 0;
    if (data.fillHalf == 0) {
        if (left >= 0x800) {
            return;
        }
        mixOut8(data.ring, 0x800);
        data.fillHalf++;
    } else {
        if (left < 0x800) {
            return;
        }
        mixOut8(data.ring + 0x1000, 0x800);
        data.fillHalf--;
    }
}

}
