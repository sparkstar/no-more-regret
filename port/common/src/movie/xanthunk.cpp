/* The decoders of the 'Axan' sound of Xan movies from xanthunk.asm
 * (1478:0000-041f) in C++. The rest of the module, the calls into
 * XANDRV32.DLL, goes away with the port's own movie decoder.
 *
 * Each turns count bytes of codes at source into samples at dest and
 * returns the bytes made. The codes start with the first sample of each
 * channel as a word; then each code byte steps a channel. Its low two bits
 * lower the channel's shift by twice their value (it stops at 0), or raise
 * it by one when both are set; the other six, as the high byte of a word,
 * shifted right by the shift, are the step. The 8-bit decoders keep the high
 * byte of each sample. The mixing decoders (stereo to mono) add the steps to
 * 32-bit sums and take half of the two channels' sum.
 *
 * Like the originals they decode one code (one of each channel in stereo)
 * before they look at count, and they stop once they have read count bytes
 * or more: a stereo count that is odd reads one byte past it. The game's
 * buffers start low enough that source + count stays below 64K, where the
 * originals' 16-bit comparison would end at once. */
#include "xanthunk.h"

namespace {

/* A channel's shift, which starts at 4. */
struct Channel {
    u8 shift = 4;

    /* The step of code, moving the shift as the original's STEP does. */
    s16 step(u8 code)
    {
        const u8 k = code & 3;
        if (k == 3) {
            shift = static_cast<u8>(shift + 1);
        } else if (k != 0) {
            shift = static_cast<u8>(shift - (2 * k));
            if (static_cast<std::int8_t>(shift) < 0) {
                shift = 0;
            }
        }
        return static_cast<s16>(static_cast<s16>(static_cast<u16>((code & 0xfc) << 8)) >> (shift & 31));
    }
};

s16 word(const u8 *p)
{
    return static_cast<s16>(p[0] | p[1] << 8);
}

void put(u8 *&out, u16 sample)
{
    out[0] = static_cast<u8>(sample);
    out[1] = static_cast<u8>(sample >> 8);
    out += 2;
}

}

u16 xanMono16(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    u16 sample = static_cast<u16>(word(in));
    u32 at = 2;
    Channel channel;
    put(out, sample);
    do {
        sample = static_cast<u16>(sample + channel.step(in[at++]));
        put(out, sample);
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}

u16 xanMono8(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    u16 sample = static_cast<u16>(word(in));
    u32 at = 2;
    Channel channel;
    *out++ = static_cast<u8>(sample >> 8);
    do {
        sample = static_cast<u16>(sample + channel.step(in[at++]));
        *out++ = static_cast<u8>(sample >> 8);
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}

u16 xanStereo16(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    u16 left = static_cast<u16>(word(in));
    u16 right = static_cast<u16>(word(in + 2));
    u32 at = 4;
    Channel leftChannel;
    Channel rightChannel;
    put(out, left);
    put(out, right);
    do {
        left = static_cast<u16>(left + leftChannel.step(in[at]));
        put(out, left);
        right = static_cast<u16>(right + rightChannel.step(in[at + 1]));
        put(out, right);
        at += 2;
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}

u16 xanStereo8(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    u16 left = static_cast<u16>(word(in));
    u16 right = static_cast<u16>(word(in + 2));
    u32 at = 4;
    Channel leftChannel;
    Channel rightChannel;
    *out++ = static_cast<u8>(left >> 8);
    *out++ = static_cast<u8>(right >> 8);
    do {
        left = static_cast<u16>(left + leftChannel.step(in[at]));
        *out++ = static_cast<u8>(left >> 8);
        right = static_cast<u16>(right + rightChannel.step(in[at + 1]));
        *out++ = static_cast<u8>(right >> 8);
        at += 2;
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}

u16 xanStereoToMono16(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    s32 left = word(in);
    s32 right = word(in + 2);
    u32 at = 4;
    Channel leftChannel;
    Channel rightChannel;
    put(out, static_cast<u16>((left + right) >> 1));
    do {
        left += leftChannel.step(in[at]);
        right += rightChannel.step(in[at + 1]);
        put(out, static_cast<u16>((right + left) >> 1));
        at += 2;
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}

u16 xanStereoToMono8(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    s32 left = word(in);
    s32 right = word(in + 2);
    u32 at = 4;
    Channel leftChannel;
    Channel rightChannel;
    *out++ = static_cast<u8>((left + right) >> 9);
    do {
        left += leftChannel.step(in[at]);
        right += rightChannel.step(in[at + 1]);
        *out++ = static_cast<u8>((right + left) >> 9);
        at += 2;
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}

u16 xanMonoToStereo16(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    u16 sample = static_cast<u16>(word(in));
    u32 at = 2;
    Channel channel;
    put(out, sample);
    put(out, sample);
    do {
        sample = static_cast<u16>(sample + channel.step(in[at++]));
        put(out, sample);
        put(out, sample);
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}

u16 xanMonoToStereo8(void *dest, const void *source, u16 count)
{
    const u8 *in = static_cast<const u8 *>(source);
    u8 *out = static_cast<u8 *>(dest);
    u16 sample = static_cast<u16>(word(in));
    u32 at = 2;
    Channel channel;
    *out++ = static_cast<u8>(sample >> 8);
    *out++ = static_cast<u8>(sample >> 8);
    do {
        sample = static_cast<u16>(sample + channel.step(in[at++]));
        *out++ = static_cast<u8>(sample >> 8);
        *out++ = static_cast<u8>(sample >> 8);
    } while (at < count);
    return static_cast<u16>(out - static_cast<u8 *>(dest));
}
