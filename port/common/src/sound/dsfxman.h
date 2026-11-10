/* DSFXMAN.C (12d8:0386-05f7): the game's sound effects, played by number
 * from sound\sound.flx through the sound cache and the channels of SNDMGR.C.
 * Each call does nothing until the effects are started. */
#ifndef PORT_DSFXMAN_H
#define PORT_DSFXMAN_H
#include "common.h"
#include "fastflex.h"

extern u8 &effectsStarted;

/* Stops the archive, the cache, the channels and the sound sources that
 * initSoundEffects started. */
void closeSoundEffects();
void playEffect(i16 sound);
void stopEffect(i16 sound);
void stopAllEffects();
/* Stops every effect; resumeEffects does nothing. */
void pauseEffects();
void resumeEffects();
bool isEffectPlaying(i16 sound);
/* Starts the effects when the configuration turned sound on. */
void startSoundEffects();

/* UNCALLED: nothing calls it. */
void playEffect(i16 sound, u8 volume);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void playEffectData(void *sample);
/* UNCALLED: nothing calls it. */
void loopEffect(i16 sound);
/* UNCALLED: nothing calls it. */
void loopEffect(i16 sound, u16 volume);
#endif
