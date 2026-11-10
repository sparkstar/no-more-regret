/* 1440:0000-036b, file name unknown: the movies' sound conversion. See
 * audconv.h. */
#include "gamestate.h"
#include "audconv.h"
#include "xanthunk.h"

/* Why the last converter call failed: 1 a format it cannot convert, 2 no
 * memory, 3 not a converter. */
u32 &converterError = gameGlobal<u32>(0);
/* No references. */
u32 &unknown7592 = gameGlobal<u32>(0);
u32 &unknown7596 = gameGlobal<u32>(0);
/* No references: the bytes between LXLOAD and XANPLAY's variables. */
u8 (&unknown9700)[514] = gameGlobal<u8[514]>();

namespace {
/* A PCM format the card plays; the 'Axan' format of the movies. */
i16 isPlayableFormat(AudioFormat *format)
{
    if (format == nullptr) {
        return 0;
    }
    if (format->formatTag != 1) {
        return 0;
    }
    if (format->channels < 1 || format->channels > 2) {
        return 0;
    }
    if (format->bitsPerSample != 16 && format->bitsPerSample != 8) {
        return 0;
    }
    if (static_cast<i16>(format->bitsPerSample >> 3 << (format->channels >> 1)) != format->blockAlign) {
        return 0;
    }
    if (format->samplesPerSec * format->blockAlign != format->avgBytesPerSec) {
        return 0;
    }
    return 1;
}

i16 isXanFormat(WaveFormat *format)
{
    if (format == nullptr) {
        return 0;
    }
    if (format->formatTag != WAVE_FORMAT_XAN) {
        return 0;
    }
    if (format->channels < 1 || format->channels > 2) {
        return 0;
    }
    if (format->bitsPerSample != 16) {
        return 0;
    }
    if ((format->samplesPerSec / 15 + 1) * format->channels != format->blockAlign) {
        return 0;
    }
    if (format->blockAlign * static_cast<s32>(15L) != format->avgBytesPerSec) {
        return 0;
    }
    return 1;
}
}

/* UNCALLED: nothing calls it. */
u32 lastConverterError()
{
    return converterError;
}

namespace {
i16 isConverter(AudioConverter *converter)
{
    if ((converter == nullptr) || converter->size != static_cast<ui16>(sizeof(AudioConverter))) {
        converterError = 3;
        return 0;
    }
    return 1;
}

/* No Remorse's sound (PCM, 8 bits, mono) as 16-bit stereo, each sample as
 * ASYLUM.DLL's mixer made it of PLAYER.C's looped effect: its signed byte
 * through the 16-bit output's volume table at 41h, sample * 64 with the
 * negatives one place over (-1 is 0). */
u16 pcm8MonoToStereo16(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);

    for (u16 i = 0; i < count; i++) {
        const std::int8_t sample = static_cast<std::int8_t>(in[i] ^ 0x80);
        const u16 value = static_cast<u16>((sample < 0 ? sample + 1 : sample) * 64);

        out[4 * i] = out[(4 * i) + 2] = static_cast<u8>(value);
        out[(4 * i) + 1] = out[(4 * i) + 3] = static_cast<u8>(value >> 8);
    }
    return static_cast<u16>(count * 4);
}

i16 isPcm8Mono(WaveFormat *format)
{
    return static_cast<i16>((format != nullptr) && format->formatTag == 1 && format->channels == 1 &&
                            format->bitsPerSample == 8);
}
}

AudioConverter *openConverter(WaveFormat *from, AudioFormat *to)
{
    AudioConverter *converter = nullptr;
    u16 (*decode)(void *, const void *, u16) = nullptr;

    converterError = 0;
    if ((from != nullptr) && (isPcm8Mono(from) != 0) && (isPlayableFormat(to) != 0) && to->channels == 2 &&
        to->bitsPerSample == 16) {
        converter = new AudioConverter;
        converter->size = sizeof(AudioConverter);
        converter->unknown4 = 0;
        converter->decode = pcm8MonoToStereo16;
        return converter;
    }
    if (from != nullptr) {
        if (isXanFormat(from) == 0) {
            converterError = 1;
            return nullptr;
        }
        if (isPlayableFormat(to) == 0) {
            converterError = 1;
            return nullptr;
        }
        switch (from->channels) {
        case 1:
            if (to->bitsPerSample == 8) {
                if (to->channels == 1) {
                    decode = xanMono8;
                } else {
                    decode = xanMonoToStereo8;
                }
            } else if (to->channels == 1) {
                decode = xanMono16;
            } else {
                decode = xanMonoToStereo16;
            }
            break;
        case 2:
            if (to->bitsPerSample == 8) {
                if (to->channels == 1) {
                    decode = xanStereoToMono8;
                } else {
                    decode = xanStereo8;
                }
            } else if (to->channels == 1) {
                decode = xanStereoToMono16;
            } else {
                decode = xanStereo16;
            }
            break;
        }
        converter = new AudioConverter;
        converter->size = sizeof(AudioConverter);
        converter->unknown4 = 0;
        converter->decode = decode;
    }
    return converter;
}

i16 closeConverter(AudioConverter *converter)
{
    if (isConverter(converter) == 0) {
        return 0;
    }
    delete converter;
    return 1;
}

u32 convertAudio(AudioConverter *converter, void *source, u32 length, void *dest)
{
    if (isConverter(converter) == 0) {
        return 0;
    }
    return converter->decode(dest, source, length);
}
