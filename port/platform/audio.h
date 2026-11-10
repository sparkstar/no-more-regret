/* The sound's output: the game's sound (the module of port/common/asylum) mixes
 * into a ring of bytes, which the output takes a unit at a time (a byte of
 * 8-bit samples, a word of 16-bit ones), two units a sample (stereo), rate
 * samples a second of the virtual time, from its start round and round, as the
 * Sound Blaster 16's DMA took the original's. What it takes goes out as the
 * time passes it: to the WAV of REGRET_AUDIO and to the window's sound
 * (window.h). */
#ifndef PORT_PLATFORM_AUDIO_H
#define PORT_PLATFORM_AUDIO_H
#include <cstdint>

/* The output of the ring's first units units from now on, at rate. */
void audioOutputStart(const std::uint8_t *ring, std::uint32_t units, std::uint32_t rate, bool words);
/* The output stops, what it took up to now out. */
void audioOutputStop();
/* The units the output has taken since its start; they stay when it stops. */
std::uint64_t audioOutputTaken();
#endif
