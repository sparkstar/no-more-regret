/* Code139 (1450, file name unknown): a movie's sound through ASYLUM.DLL's
 * DirectPlay. See audstrm.h. */
#include <mem.h>
#include "gamestate.h"
#include "audstrm.h"
#include "waits.h"
#include "asylum.h"
#include "sndmgr.h"

/* The stream whose blocks feedAudio plays; 0 when none. */
AudioStream *&playingStream = gameGlobal<AudioStream *>(nullptr);

/* ASYLUM.DLL calls it when a block has been played. */
void feedAudio(i16 /*unused*/)
{
    if ((playingStream != nullptr) && (playingStream->playing != 0U)) {
        const u16 position = playingStream->position;
        const u16 chunk = playingStream->chunk;
        const u16 size = playingStream->size;
        u8 *data = playingStream->buffer + position;

        if (static_cast<ui16>(position + chunk) >= size) {
            ASS_DirectPlay(0, data, static_cast<ui16>(size - position) >> 2, 0, playingStream->format.samplesPerSec);
            playingStream->position = 0;
        } else {
            ASS_DirectPlay(0, data, chunk >> 2, 0, playingStream->format.samplesPerSec);
            playingStream->position += chunk;
        }
    } else {
        playingStream = nullptr;
    }
}

namespace {
/* Plays length bytes at data of stream. */
void playAudio(AudioStream *stream, void *data, u16 length)
{
    playingStream = stream;
    ASS_DirectPlay(0, data, length >> 2, 0, playingStream->format.samplesPerSec);
}
}

i16 audioOpen(AudioRequest *request, AudioStream **result)
{
    AudioStream *stream;

    if ((stream = new AudioStream) == nullptr) {
        return 0;
    }
    stream->format = *request->format;
    if ((stream->buffer = new u8[request->bufferSize]) == nullptr) {
        return 0;
    }
    stream->size = request->bufferSize;
    stream->chunk = stream->size / 8 > 0x80 ? 0x80 : stream->size / 8;
    *result = stream;
    ASS_StopAllSFX();
    ASS_RegisterCallback(feedAudio);
    return 1;
}

AudioStream::AudioStream()
{
    _fmemset(this, 0, sizeof(AudioStream));
}

AudioStream::~AudioStream()
{
    audioStop(this);
    ASS_RegisterCallback(channelDone);
    if (buffer != nullptr) {
        delete[] buffer;
    }
}

void audioClose(AudioStream *stream)
{
    /* feedAudio, from ASYLUM.DLL's callback, ends the stream, which goes
     * after it (a wait of the steps). */
    audioStop(stream);
    openWaitWhile([]() { return playingStream != nullptr; },
                  [stream]() {
                      delete stream;
                      return false;
                  });
}

/* UNCALLED: nothing calls it. */
i16 audioFormat(AudioStream *stream, AudioFormat *format)
{
    *format = stream->format;
    return 1;
}

i16 audioStop(AudioStream *stream)
{
    stream->playing = 0;
    return 1;
}

i16 audioStart(AudioStream *stream)
{
    const u16 position = stream->position;

    if (stream->playing != 0U) {
        return 1;
    }
    stream->playing = 1;
    if (static_cast<ui16>(position + stream->chunk) >= stream->size) {
        stream->position = 0;
        playAudio(stream, stream->buffer + position, stream->size - position);
    } else {
        stream->position += stream->chunk;
        playAudio(stream, stream->buffer + position, stream->chunk);
    }
    return 1;
}

i16 audioSeek(AudioStream *stream, u32 position)
{
    stream->position = position;
    return 1;
}

i16 audioPosition(AudioStream *stream, u32 *position)
{
    *position = stream->position;
    return 1;
}

i16 audioBuffer(AudioStream *stream, u8 **buffer)
{
    *buffer = stream->buffer;
    return 1;
}

i16 audioUpdate(AudioStream * /*unused*/)
{
    return 1;
}
