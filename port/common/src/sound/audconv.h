/* No Regret's Code137 (1440:0000-036b, file name unknown): converts a
 * movie's sound chunks to the format the card plays with the decoders of
 * xanthunk.asm (1478); and No Remorse's 8-bit sound as its player had the
 * card play it. The subtitles followed in the segment (subtitles.h). */
#ifndef PORT_AUDCONV_H
#define PORT_AUDCONV_H
#include "common.h"
#include "audstrm.h"

/* The formatTag readers give the 'Axan' sound (Xan's movies). */
constexpr int WAVE_FORMAT_XAN = 0x594a;

/* A converter: size is sizeof(AudioConverter). */
struct AudioConverter {
    u32 size;
    u32 unknown4;
    u16 (*decode)(void *dest, const void *source, u16 count);
};
ASSERT_SIZE(AudioConverter, 0xc);

/* A converter from the file's format to the played one, 0 when there is
 * none for it. */
AudioConverter *openConverter(WaveFormat *from, AudioFormat *to);
i16 closeConverter(AudioConverter *converter);
/* Converts length bytes at source into dest; the bytes made. */
u32 convertAudio(AudioConverter *converter, void *source, u32 length, void *dest);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
u32 lastConverterError();
#endif
