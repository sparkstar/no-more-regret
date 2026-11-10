/* The Asylum Sound System (ASYLUM.DLL) as the port's module: the exports
 * the game calls (asylum.h's ASS_*, which common/src/sound/asylum.cpp gives
 * over these), of plain types and pointers. The output is the platform's
 * (audio.h). */
#ifndef PORT_ASYAPI_H
#define PORT_ASYAPI_H
#include <cstdint>

namespace asylum {

/* ASS_Init's config: the card (the port's output is the Sound Blaster
 * 16's, card 3's, for every one), its port and IRQ, which nothing uses,
 * its DMA channel (4 and up: 16-bit output, else 8-bit), the mixing rate,
 * whether the music and the sound are on. The original's has the game's
 * hook of the card's interrupt after them; the module has no interrupt. */
struct Config {
    std::uint16_t card, port, irq, dma, rate, music, sound;
};

/* A song as AMFLOAD.C loads it: the header (A66h bytes), the patterns and
 * the 64 samples, and the sizes of the blocks the game allocated them in
 * (the original's selectors' limits, past which a read faulted). */
struct Song {
    const std::uint8_t *header;
    const std::uint8_t *patterns;
    std::uint32_t patternsSize;
    const std::uint8_t *samples[64];
    std::uint32_t sizes[64];
};

/* The game's function the mixer calls when a sound effect's sample ends,
 * with its channel. */
using Callback = void (*)(std::int16_t channel);

void init(const Config &config); /* ASYLUM.61 */
void uninit();                   /* ASYLUM.60 */
/* Mixes the half of the output the output does not play, when it is due
 * (the game's timer calls it at 90 Hz). */
void updateDac();                                /* ASYLUM.59 */
void getCardName(std::int16_t card, char *name); /* ASYLUM.53 */
void registerCallback(Callback callback);        /* ASYLUM.52 */
void setOversampling();                          /* ASYLUM.50 */
void clrOversampling();                          /* ASYLUM.49 */
std::uint16_t getOversampling();                 /* ASYLUM.48 */
void setMusicVolume(std::uint16_t volume);       /* ASYLUM.40 */
std::int16_t getMusicVolume();                   /* ASYLUM.39 */
void playMusic(const Song &song);                /* ASYLUM.37 */
void startMusic();                               /* ASYLUM.36 */
void stopMusic();                                /* ASYLUM.35 */
void pauseMusic();                               /* ASYLUM.34 */
void resumeMusic();                              /* ASYLUM.33 */
void setSoundVolume(std::uint16_t volume);       /* ASYLUM.32 */
std::int16_t getSoundVolume();                   /* ASYLUM.31 */
/* A sound of SOUND.FLX (its length at 4, its loop at 8 and 0Ch, its rate
 * at 12h, its bytes at 14h) (ASYLUM.30): of the sound cache, at *slot, size
 * bytes, which the channel follows as the cache moves the sound or puts
 * another in the slot (the original's pointer was the slot's selector); or
 * a block of size bytes at sample that stays where it is. */
void playSfx(void *const *slot, const std::uint32_t *size, std::int16_t channel, std::uint16_t volume,
             std::int16_t pitch, std::int16_t loop);
void playSfx(const std::uint8_t *sample, std::uint32_t size, std::int16_t channel, std::uint16_t volume,
             std::int16_t pitch, std::int16_t loop);
void stopSfx(std::int16_t channel); /* ASYLUM.28 */
/* The port's: stops, as stopSfx, the sound effects that play the sound of
 * the slot (playSfx's), which the sound cache empties or gives another. */
void stopSlotSfx(void *const *slot);
/* Whether a sound effect's channel plays, 1 or 0 (ASYLUM.22): channels
 * 0-11, and 12 as stopSfx has it. */
std::int16_t isChannelBusy(std::int16_t channel);
/* Does nothing (ASYLUM.21): the mixer reads a sound's bytes where they are,
 * so a change in them plays without it. No Remorse's PLAYER.C calls it
 * after it writes a chunk of a movie's sound into its looped effect. */
void notifySampleChange(std::int16_t channel, std::int32_t offset, std::uint32_t length);
void stopAllSfx();                                             /* ASYLUM.27 */
void setSfxVolume(std::int16_t channel, std::uint16_t volume); /* ASYLUM.26 */
/* length dwords of 16-bit stereo samples at samples, at rate. */
void directPlay(std::int16_t channel, const std::uint8_t *samples, std::uint32_t length, std::int16_t unknown,
                std::uint16_t rate); /* ASYLUM.19 */
/* The bytes of the output left before the half the mixer fills next. */
std::uint16_t getDmaBytesLeft(); /* ASYLUM.18 */

/* The port's: the output's depth, 8 or 16 bits, in place of the DMA
 * channel's (0: the channel's, 4 and up 16-bit). 16 is the 16-bit output;
 * 8 the 8-bit output's mix (its tables, its mixers, the music's channels
 * at its pans), put out as words at the 16-bit output's level, a quarter
 * of the 8-bit output's (mixOut8Words). Set before init, init takes it;
 * set while the sound plays, the mix changes at once, the ring and the
 * music going on (from the DMA channel's 8-bit output, a new start of the
 * depth's). */
void setDepth(std::uint16_t bits);
std::uint16_t getDepth();
/* The depth of the output that plays, 0 before init. */
std::uint16_t outputBits();

}
#endif
