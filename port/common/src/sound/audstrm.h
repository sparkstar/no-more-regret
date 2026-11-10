/* Code139 (1450, file name unknown): a movie's sound, played from a ring
 * buffer straight to the card with ASYLUM.DLL's DirectPlay. Each block that
 * ends makes ASYLUM.DLL call feedAudio, which plays the next one. The
 * functions that report success return 1. */
#ifndef PORT_AUDSTRM_H
#define PORT_AUDSTRM_H
#include "common.h"

/* A sound format, as in a PCMWAVEFORMAT. The movie readers keep the file's
 * ('strf'). */
struct WaveFormat {
    u16 formatTag;
    i16 channels;
    s32 samplesPerSec;
    s32 avgBytesPerSec;
    i16 blockAlign;
    i16 bitsPerSample;
};
ASSERT_SIZE(WaveFormat, 0x10);

/* The format a stream plays. */
struct AudioFormat : WaveFormat {
    u32 unknown10;
};
ASSERT_SIZE(AudioFormat, 0x14);

/* What audioOpen is asked for. */
struct AudioRequest {
    u32 size; /* of this structure, 10h */
    u32 unknown4;
    AudioFormat *format;
    u32 bufferSize;
};
ASSERT_SIZE(AudioRequest, 0x10);

class AudioStream {
public:
    u8 *buffer;
    u16 position; /* where the next block starts */
    u16 size;
    u16 chunk; /* the bytes of a block */
    u16 playing;
    AudioFormat format;

    AudioStream();  /* 1450:01b7 */
    ~AudioStream(); /* 1450:01f0 */
};
ASSERT_SIZE(AudioStream, 0x20);

i16 audioOpen(AudioRequest *request, AudioStream **result); /* 1450:00f9 */
void audioClose(AudioStream *stream);                       /* 1450:025c */
/* UNCALLED: nothing calls it. */
i16 audioFormat(AudioStream *stream, AudioFormat *format); /* 1450:0270 */
i16 audioStop(AudioStream *stream);                        /* 1450:0292 */
i16 audioStart(AudioStream *stream);                       /* 1450:02a5 */
i16 audioSeek(AudioStream *stream, u32 position);          /* 1450:0323 */
i16 audioPosition(AudioStream *stream, u32 *position);     /* 1450:0337 */
i16 audioBuffer(AudioStream *stream, u8 **buffer);         /* 1450:0354 */
/* Has nothing to do. */
i16 audioUpdate(AudioStream *stream); /* 1450:0372 */

void feedAudio(i16 /*unused*/);
#endif
