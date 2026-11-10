/* ASYLUM.DLL, the Asylum Sound System: the music and sound effect player the
 * game imports by ordinal (the names are the DLL's own). Only the part the
 * game's modules call, of both games: No Regret's DLL is V1.12, No
 * Remorse's V1.06, whose ordinals are 3 below V1.12's past the 17-19 that
 * V1.06 lacks; the comments give V1.12's and, where it has the function,
 * V1.06's. The port's sound module (port/common/asylum, asyapi.h) is the DLL
 * for both; asylum.cpp gives these over it. */
#ifndef PORT_ASYLUM_H
#define PORT_ASYLUM_H
#include "common.h"

class AmfModule;

/* The setup ASS_Init takes. The original's has the game's function that
 * hooks the card's interrupt after these (assproc.cpp's hookVector); the
 * port's sound has no interrupt. */
struct AssConfig {
    u16 card; /* an index of cardNames (AMFLOAD.C) */
    u16 port;
    u16 irq;
    u16 dma;
    u16 rate; /* the mixing rate */
    u16 unknown0a;
    u16 unknown0c;
};
ASSERT_SIZE(AssConfig, 0xe);

extern "C" {
/* What is left of the DMA transfer (ASYLUM.18; not in V1.06). */
u16 ASS_GetDmaBytesLeft(void);
/* Plays length words of samples at data straight to the card at rate
 * (ASYLUM.19; not in V1.06). */
void ASS_DirectPlay(i16 channel, void *samples, u32 length, i16 unknown, u16 rate);
/* Tells the card that the sample on channel changed from offset for length
 * bytes (ASYLUM.21, V1.06's 18; No Remorse's PLAYER.C streams the movies'
 * sound so). */
void ASS_NotifySampleChange(i16 channel, s32 offset, u32 length);
/* The channel still plays (ASYLUM.22, V1.06's 19). */
i16 ASS_IsChannelBusy(i16 channel);
void ASS_SetSFXVolume(i16 channel, u16 volume); /* ASYLUM.26, V1.06's 23 */
void ASS_StopAllSFX(void);                      /* ASYLUM.27, V1.06's 24 */
void ASS_StopSFX(i16 channel);                  /* ASYLUM.28, V1.06's 25 */
/* Plays the sample on channel (ASYLUM.30, V1.06's 27): a block that stays
 * where it is while it plays. */
void ASS_PlaySFX(void *sample, i16 channel, u16 volume, i16 unknown, i16 loop);
/* The port's: plays the sound of the sound cache's slot, of size bytes, on
 * channel, which follows the slot as the cache moves the sound or puts
 * another there (the original's pointer, which ASS_PlaySFX took, was the
 * slot's selector; SNDCACHE.C). */
void ASS_PlaySlotSFX(void *const *slot, const u32 *size, i16 channel, u16 volume, i16 unknown, i16 loop);
/* The port's: stops the channels that play the sound of the slot, which
 * the sound cache empties or gives another sound (SNDCACHE.C). */
void ASS_StopSlotSFX(void *const *slot);
i16 ASS_GetSoundVolume(void);        /* ASYLUM.31, V1.06's 28 */
void ASS_SetSoundVolume(u16 volume); /* ASYLUM.32, V1.06's 29 */
void ASS_ResumeMusic(void);          /* ASYLUM.33, V1.06's 30 */
void ASS_PauseMusic(void);           /* ASYLUM.34, V1.06's 31 */
void ASS_StopMusic(void);            /* ASYLUM.35, V1.06's 32 */
void ASS_StartMusic(void);           /* ASYLUM.36, V1.06's 33 */
/* Makes module the music to play (ASYLUM.37, V1.06's 34). */
void ASS_PlayMusic(AmfModule *module);
i16 ASS_GetMusicVolume(void);        /* ASYLUM.39, V1.06's 36 */
void ASS_SetMusicVolume(u16 volume); /* ASYLUM.40, V1.06's 37 */
u16 ASS_GetOversampling(void);       /* ASYLUM.48, V1.06's 45 */
void ASS_ClrOversampling(void);      /* ASYLUM.49, V1.06's 46 */
void ASS_SetOversampling(void);      /* ASYLUM.50, V1.06's 47 */
/* callback gets the channel whose sample ended (ASYLUM.52, V1.06's 49). The
 * original takes the game's DS as well, which the DLL loads for it. */
void ASS_RegisterCallback(void (*callback)(i16 channel));
void ASS_GetCardName(i16 card, char *name); /* ASYLUM.53; No Remorse does not call it */
void ASS_UpdateDAC(void);                   /* ASYLUM.59, V1.06's 56 */
void ASS_Uninit(void);                      /* ASYLUM.60, V1.06's 57 */
void ASS_Init(AssConfig *config);           /* ASYLUM.61, V1.06's 58 */
/* The port's (asyapi.h's setDepth): the output's depth, 8 or 16 bits,
 * in place of the DMA channel's (0: the channel's), both at the 16-bit
 * output's level; set while the sound plays, the mix changes at once,
 * the music going on. */
void ASS_SetDepth(u16 bits);
u16 ASS_GetDepth(void);
/* The depth of the output that plays, 0 before ASS_Init. */
u16 ASS_GetOutputBits(void);
}
#endif
