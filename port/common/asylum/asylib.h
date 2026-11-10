/* ASYLUM.DLL, the Asylum Sound System V1.12 by Jason Ely: the game's sound,
 * its mixer, the sound effects' twelve channels, the AMF music's player and
 * the output's loops, as the original computes them, on the port's pointers
 * and the platform's output: no data segment, far pointers, ports, DMA or
 * card interrupt. The game calls asyapi.h's functions
 * (common/src/sound/asylum.cpp gives the game's ASS_* over them).
 *
 * What the original's calculation leans on stays: the tables as its data
 * segment lays them out one after the other (Tables: a sum past the 8-bit
 * output's clipping table reads its neighbours, a tick longer than the sum
 * clears into them, a note past the periods reads the config after them);
 * the high word of EBX that the code before leaves and the portamentos
 * divide by (playerEbxHigh); the mixers' counts, which the sample-ended
 * notice leaves for the sample the callback starts again; the channels a
 * new start does not clear. What only a random data segment reaches (a
 * volume past 41h, a sum at the other end of the segment, the 8-bit
 * surround mixer's restart at its old fraction) it has not: the game's
 * sound never gets there (the comparison with the original's DLL).
 *
 * The files follow the original's segments: asyapi.cpp 4 (the exports, the
 * tables, the notice), asymusic.cpp 2, asysfx.cpp 3, asymix.cpp and
 * asyplay.cpp 1, asyout.cpp 8 (the Sound Blaster 16's driver, in the port
 * the platform's output), asydata.cpp the data the file has. */
#ifndef PORT_ASYLIB_H
#define PORT_ASYLIB_H
#include "asyapi.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace asylum {

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;

const u16 SFX_CHANNELS = 12;
const u16 MUSIC_CHANNELS = 8;
const u16 INSTRUMENTS = 31;

/* The mixers a channel has (the original's are routines of segment 1 by
 * their offsets): 8-bit samples into a mono, stereo or surround sum of
 * 8-bit or 16-bit output, and 16-bit stereo streams. */
enum Mixer : u8 {
    MIX_NONE,
    MIX_MONO8,
    MIX_STEREO8,
    MIX_SURROUND8,
    MIX_MONO16,
    MIX_STEREO16,
    MIX_SURROUND16,
    MIX_STREAM16,
    MIX_STREAM16MONO,
    MIX_STREAM8,
    MIX_STREAM8MONO
};

/* Where a channel's samples are, as the original's far pointer had them:
 * a block (its selector) and the offset in it. The block is at data, size
 * bytes, or it is a slot of the sound cache (at *slot, *slotSize bytes),
 * which moves with its sound and takes the next sound the slot gets, as
 * the slot's selector did. A read past the block's end faulted in the
 * original; the port reads silence there (at). */
struct Samples {
    void *const *slot = nullptr;
    const u32 *slotSize = nullptr;
    const u8 *data = nullptr;
    u32 size = 0;
    u32 offset = 0;

    /* The bytes bytes at position in the block. */
    [[nodiscard]] const u8 *at(u32 position, u32 bytes) const;
};

/* A channel of the mixer (the original's 738h + 46h * n: 0-7 the music's,
 * 8-19 the sound effects' channels 0-11). */
struct Channel {
    Samples data;
    u32 position; /* in samples (4-byte units for a stream) */
    u32 length;   /* 0: silent */
    u32 loopStart;
    u32 loopLength; /* more than 2: it loops */
    u8 volume;      /* 0-64, 65 mixes without the master volume or the pans */
    u8 panRight;    /* the second word of a stereo pair */
    u8 panLeft;     /* the first */
    /* The song's pans, which the 8-bit output's mix writes its own over
     * (mixOut8); back when the port's setDepth changes to the 16-bit one. */
    u8 musicPanRight;
    u8 musicPanLeft;
    u16 fraction; /* of the position */
    u16 period;   /* the music's */
    u32 step;     /* 16.16 samples a sample of the mix */
    u16 effect;   /* the music's: command in the high byte */
    u16 portaTarget;
    u8 portaSpeed;
    u8 vibratoPosition;
    u8 vibrato;
    u8 sampleOffset;
    u32 arpeggio[3];   /* steps */
    u32 arpeggioIndex; /* bytes into arpeggio */
    u16 active;
    u16 paused; /* 1 skips the channel */
    u16 transpose;
    u8 counter; /* retrigger or cut */
    u8 counterReload;
    u32 c2spd;
    Mixer mixer;
};

/* A sound effect channel's request (624h + 17h * n), which the mixer takes
 * at its next tick. */
struct SfxRecord {
    u16 state; /* 0 idle, 1 to start, 2 playing */
    Samples data;
    u32 length;
    u32 loopStart;
    u32 loopLength;
    u8 volume;
    u32 step;
};

/* The tables as the original's data segment has them, from the sum of a
 * tick at 1C24h to the driver's pointers that end at EB44h, read and
 * written at the segment's offsets. */
const u16 TABLES_AT = 0x1c24;
const u16 SUM_AT = 0x1c24;          /* 1000h words, a stereo sample two */
const u16 CLIP_AT = 0x3c24;         /* 8-bit: 400h zeros, 0-255, 400h FFh */
const u16 CLIP8_AT = 0x40a4;        /* where a sum of 0 reads it */
const u16 VOLUMES_AT = 0x4524;      /* 41h rows of 256, words in 16-bit */
const u16 IDENTITY8_AT = 0x8624;    /* 8-bit: the bytes 0-255 */
const u16 VOLUME_SCALE_AT = 0xc926; /* 65 rows of 128 */
const u16 PERIODS_AT = 0xe9a6;      /* 96 dwords */
const u16 CONFIG_AT = 0xeb26;       /* ASS_Init's config, 12h bytes */
const u16 DRIVER_AT = 0xeb38;       /* the driver's three far pointers */
const u16 TABLES_END = 0xeb44;
/* The sound's trouble that ended the original, or that reads what is no
 * sound, which the port passes over and goes on from: the sound's trouble
 * never ends the game. Each is told once a run (host.h's hostNotice). */
enum class Trouble : u8 {
    READ_PAST_BLOCK,
    OUTSIDE_TABLES,
    DIVIDE_ERROR,
    NO_MIXER,
    UPDATE_BEFORE_CARD,
    INSTRUMENT,
    FINETUNE,
    COUNT
};
void passOver(Trouble trouble, const char *text);
/* A reach of the data outside the tables, which the game's sound never
 * makes: two bytes of 0 for it to read and write, which nothing else
 * reads (asyapi.cpp). */
u8 *outsideTables(u16 at);

struct Tables {
    u8 bytes[TABLES_END - TABLES_AT];

    /* The byte and the word at the offset; one outside the tables is a
     * reach the game's sound never makes, which reads 0 and writes
     * nothing (outsideTables). Inline: the mixer reads and writes them for
     * each sample (the tests' times). */
    u8 &byte(u16 at)
    {
        if (at < TABLES_AT || at >= TABLES_END) {
            return *outsideTables(at);
        }
        return bytes[at - TABLES_AT];
    }
    /* A word's two bytes checked at once (the second's offset wraps as the
     * byte's did): a word with a byte outside is all outside. */
    u8 *wordAt(u16 at)
    {
        if (at < TABLES_AT || at + 1 >= TABLES_END) {
            return outsideTables(at < TABLES_AT || at >= TABLES_END ? at : static_cast<u16>(at + 1));
        }
        return &bytes[at - TABLES_AT];
    }
    u16 word(u16 at)
    {
        const u8 *p = wordAt(at);
        return static_cast<u16>(p[0] | p[1] << 8);
    }
    void setWord(u16 at, u16 value)
    {
        u8 *p = wordAt(at);
        p[0] = static_cast<u8>(value);
        p[1] = static_cast<u8>(value >> 8);
    }
    u32 dword(u16 at) { return word(at) | static_cast<u32>(word(static_cast<u16>(at + 2))) << 16; }
};

/* The module's data: the original's data segment by its fields. */
struct Data {
    u16 musicStarted;
    u16 unknown94;
    u16 musicPlaying;
    u16 musicPaused;
    u8 order;
    u8 speed;
    u8 speedCount;
    u8 tempo;
    u8 row; /* 40h: the next pattern */
    u8 breakRow;
    u16 tickSamples;
    u32 patternOffset;      /* the row's in the song's patterns */
    const u8 *patternBlock; /* the original's selector of them */
    u16 c2spds[16];         /* by finetune */
    u16 initialized;
    u16 stereo;
    u16 highSpeed;
    u16 mixRate;
    u16 tickRate;
    u16 dspRate;
    u16 rate;
    u16 port;
    u8 irq;
    u8 dma;
    u16 fillHalf;   /* the half to fill next */
    u16 mixPointer; /* into the sum */
    u16 tickLeft;   /* samples of the tick not yet out */
    u32 loopLength; /* the looping mixers' */
    u16 volume;
    u16 music; /* the config's */
    u16 sound;
    u8 notifyChannel; /* the channel the mixer mixes */
    u16 sixteenBit;   /* the 16-bit output's tables and mixers */
    /* The port's 8-bit precision (asyapi.h's setDepth): the 8-bit output's
     * tables and mixers (sixteenBit 0), their sums put out as the 16-bit
     * output's words at its level (mixOut8Words). */
    u16 eightBitWords;
    u16 unknown107;
    u8 panRight; /* the pan E8x picks */
    u8 panLeft;
    u16 halfBuffer;
    Callback callback;
    u16 savedCounts[10]; /* the mixers' counts, by their Mixer less 1 */
    u8 songLength;
    u8 restart; /* FFh: none */
    u8 orders[256];
    const u8 *patterns;
    u32 patternsSize; /* their block's */
    const u8 *sampleData[INSTRUMENTS];
    u32 sampleSizes[INSTRUMENTS]; /* their blocks' */
    u16 sampleLengths[INSTRUMENTS];
    u16 loopStarts[INSTRUMENTS];
    u16 loopLengths[INSTRUMENTS];
    u16 volumes[INSTRUMENTS];
    u16 transposes[INSTRUMENTS];
    u32 instrumentC2spds[INSTRUMENTS];
    SfxRecord sfx[SFX_CHANNELS];
    Channel channels[MUSIC_CHANNELS + SFX_CHANNELS];
    u8 masterVolume;
    u16 oversample8; /* the last sample */
    u32 oversample16;
    u32 oversampleWords; /* mixOut8Words's last samples, halved, a word each */
    const u8 *module;    /* the song's header */
    Config config;
    Tables tables;
    /* The output: the DMA buffer, 2000h bytes the output goes round. */
    u8 ring[0x4000];
};

extern Data &data;
/* The data's fields the player sets (the original's at 96h, CCh, 103h):
 * the volumes and the oversampling, settings outside the arena, which a
 * snapshot put back keeps; their initial values the file's. */
struct Levels {
    u8 musicVolume; /* 0-64 */
    u8 soundVolume; /* 0-64 */
    u16 oversampling;
};
extern Levels &levels;
/* EBX's high word as the code runs (asyplay.cpp). */
extern u16 &playerEbxHigh;

/* The bounds of the port's 8-bit sums put out as words (mixOut8Words): 64
 * times them are the words' ends. */
const s32 WORDS_SUM_LOW = -0x200;
const s32 WORDS_SUM_HIGH = 0x1ff;

/* Whether the ring takes words: the 16-bit output's or the port's 8-bit
 * precision's; else the 8-bit output's bytes. */
inline bool wordOutput()
{
    return data.sixteenBit != 0U || data.eightBitWords != 0U;
}

/* asydata.cpp: the data as the file has it, the tables' constants. */
void loadData(Data &data);
extern const u16 PERIOD_BASE[12];
extern const u8 PAN_RIGHTS[32];
extern const u8 PAN_LEFTS[32];
extern const u8 VIBRATO_SINE[32];
extern const char *const CARD_NAMES[5];

/* asyapi.cpp (segment 4). */
void buildVolumes();     /* 4:036D */
void buildClip();        /* 4:0532 */
void pickDriver();       /* 4:058F */
void buildPeriods();     /* 4:093D */
void notifySampleEnd();  /* 4:07F6 */
Mixer sampleMixer();     /* 4:0DE4 */
Mixer streamMixer();     /* 4:0E09 */
Mixer panMixer(u16 pan); /* 4:0E2E */
void setPan(u16 pan);    /* 4:0E65 */

/* asymusic.cpp (segment 2). */
void startMusicSystem();         /* 2:0025 */
void stopMusicSystem();          /* 2:0080 */
void loadSong(const Song &song); /* 2:008E */
void startSong();                /* 2:01EB */
void stopSong();                 /* 2:0276 */

/* asysfx.cpp (segment 3). */
void startSfxSystem(); /* 3:0000 */
void stopSfxSystem();  /* 3:0042 */

/* asymix.cpp (segment 1): a channel's mixer, count samples into the sum at
 * pointer; the loops that make a tick's mix and put it out to the ring;
 * the fills. */
void mixChannel(Mixer kind, Channel &channel, u16 pointer, u16 count);
void fill16();                            /* 1:0ECA */
void fill8();                             /* 1:0E73 */
void fill8Words();                        /* the port's, as fill16 */
void mixOut16(u8 *buffer, u16 count);     /* 1:1EC0 */
void mixOut8(u8 *buffer, u16 count);      /* 1:2144 */
void mixOut8Words(u8 *buffer, u16 count); /* the port's, as mixOut8 */

/* asyplay.cpp (segment 1): the music's tick, a row, the effects; the sound
 * effects' starts. */
void musicTick();   /* 1:1809 */
void startSounds(); /* 1:199F */

/* asyout.cpp (segment 8): the output's start and stop, the units left of
 * the DMA's transfer as its count register gives them; the start's last
 * part, after the DSP's reset, alone (the port's change of depth, setDepth). */
void startOutput(); /* 8:0050 */
void stopOutput();  /* 8:0178 */
u16 dmaCount();
void restartOutput();

/* What the original ends with: a divide error, which the port passes over
 * with a quotient of 0 (passOver). */
void divideError();
inline u32 divide32(u32 dividend, u32 divisor)
{
    if (divisor == 0U) {
        divideError();
        return 0;
    }
    return dividend / divisor;
}
inline u16 divide16(u16 dividend, u16 divisor)
{
    if (divisor == 0U) {
        divideError();
        return 0;
    }
    return static_cast<u16>(dividend / divisor);
}

}
#endif
