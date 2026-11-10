/* The RRV movies' frames. See rrv.h. */
#include <cstring>
#include "rrv.h"
#include "bitfield.h"
#include "palette.h"
#include "x32code.h"
#include "screen.h"

namespace {

/* The run lengths: a 4-bit code picks runBase and runBits extra bits; codes
 * 0 and 8 read none (runs of 0 and 1). */
const u8 RUN_BITS[16] = {0, 7, 3, 0, 1, 16, 5, 0, 1, 8, 4, 0, 2, 24, 6, 0};
const u32 RUN_BASE[16] = {0, 0x80, 8, 0, 2, 0, 0x20, 0, 1, 0x100, 0x10, 0, 4, 0, 0x40, 0};

/* The chunk ids of a frame's pixels and of its pixel pairs. */
const u32 PIXEL_PAIRS = 0x62643030; /* "00db" */

/* The next run length at bit of code (a dword read there, its bytes in
 * memory order): a 4-bit index, then the index's extra bits. code and bit
 * move past what it read. */
u32 readRun(const u8 **code, u8 *bit)
{
    u32 value;

    memcpy(&value, *code, 4);
    const u8 index = static_cast<u8>(bitField(value, *bit, 4));
    *bit += 4;
    *code += *bit >> 3;
    *bit &= 7;
    u32 run = RUN_BASE[index];
    if ((index != 0U) && index != 8) {
        const u8 extra = RUN_BITS[index];
        memcpy(&value, *code, 4);
        run += bitField(value, *bit, extra);
        *bit += extra;
        *code += *bit >> 3;
        *bit &= 7;
    }
    return run;
}

}

RrvDecoder::RrvDecoder(u16 newWidth, u16 newHeight, i16 slices, bool newRrvMode)
    : width(newWidth), height(newHeight), sliceCount(slices), rrvMode(newRrvMode), pairs(false), chunk(nullptr),
      picture(static_cast<size_t>(newWidth) * newHeight)
{
}

void RrvDecoder::fill(const u8 *screen, u16 pitch, i16 x, i16 y, bool doubled)
{
    for (u16 row = 0; row < height; row++) {
        const u8 *line = screen + (static_cast<size_t>(y + (doubled ? 2 * row : row)) * pitch) + x;
        u8 *to = &picture[static_cast<size_t>(row) * width];
        for (u16 column = 0; column < width; column++) {
            to[column] = line[doubled ? 2 * column : column];
        }
    }
}

void RrvDecoder::begin(u32 id, const u8 *data)
{
    /* The original's is00db: only the RRV movies' 00db chunks are of pairs. */
    if (rrvMode) {
        pairs = id == PIXEL_PAIRS;
    }
    chunk = data;
}

/* A slice: the offset of its pixels from its codes, then the codes. The
 * rows of the slice, width pixels each, run after run: a skip leaves the
 * picture, a copy takes the next pixels (each twice across when pairs, a
 * run of pairs twice as long); runs go on from row to row. */
void RrvDecoder::decodeSlice(i16 index)
{
    u32 sliceOffset;
    s32 pixelOffset;

    memcpy(&sliceOffset, chunk + (4 * index), 4);
    const u8 *codes = chunk + sliceOffset;
    memcpy(&pixelOffset, codes, 4);
    codes += 4;
    if (pixelOffset == 0) {
        return;
    }
    const u8 *source = codes + pixelOffset;
    const u8 *code = codes;
    u8 bit = 0;
    u32 run = 0;
    bool copying = true;
    const u16 lines = sliceLines();
    for (u16 line = 0; line < lines; line++) {
        u8 *target = &picture[static_cast<size_t>((lines * index) + line) * width];
        u32 count = width;
        while (count != 0U) {
            if (run == 0U) {
                run = readRun(&code, &bit) << (pairs ? 1 : 0);
                copying = !copying;
            }
            const u32 step = count > run ? run : count;
            if (copying) {
                if (pairs) {
                    for (u32 k = 0; k < step >> 1; k++) {
                        target[2 * k] = target[(2 * k) + 1] = source[k];
                    }
                    source += step >> 1;
                } else {
                    memcpy(target, source, step);
                    source += step;
                }
            }
            count -= step;
            run -= step;
            target += step;
        }
    }
}

void RrvDecoder::showSlice(i16 index, u8 *screen, u16 pitch, i16 x, i16 y, bool doubled) const
{
    const u16 lines = sliceLines();
    for (u16 line = lines * index; line < lines * (index + 1); line++) {
        const u8 *from = &picture[static_cast<size_t>(line) * width];
        u8 *to = screen + (static_cast<size_t>(y + (doubled ? 2 * line : line)) * pitch) + x;
        if (!doubled) {
            memcpy(to, from, width);
            continue;
        }
        for (u16 column = 0; column < width; column++) {
            to[2 * column] = to[(2 * column) + 1] = from[column];
        }
    }
}

namespace {

/* A video stream's format: its bitmap's header and palette (BITMAPINFO). */
struct BitmapFormat {
    u32 size;
    s32 width, height;
    u16 planes, bitCount;
    u32 compression;
    u32 sizeImage;
    s32 xPelsPerMeter, yPelsPerMeter;
    u32 colorsUsed;
    u32 colorsImportant;
    struct {
        u8 blue, green, red, reserved;
    } palette[256];
};

class RrvVideo : public VideoDecoder {
public:
    explicit RrvVideo(AviFile &newFile) : file(newFile) {}

    void start(const MoviePlace &place) override
    {
        const AviStream *video = file.stream(riffId('v', 'i', 'd', 's'));
        const bool rrvMode =
            video->handler == riffId('R', 'R', 'V', '1') || video->handler == riffId('R', 'R', 'V', '2');
        const i16 slices = video->handler == riffId('R', 'R', 'V', '2') ? 15 : 5;
        if (place.background != MovieBackground::KEEP) {
            buildPalette(*video);
        }
        decoder = std::make_unique<RrvDecoder>(static_cast<u16>(file.header().width),
                                               static_cast<u16>(file.header().height), slices, rrvMode);
        decoder->fill(svgaMemory, SCREEN_WIDTH, place.x, place.y, place.doubled);
    }

    void decode(const AviFile::Chunk &chunk, bool /*late*/) override
    {
        decoder->begin(chunk.id, chunk.data);
        for (i16 slice = 0; slice < decoder->slices(); slice++) {
            decoder->decodeSlice(slice);
        }
    }

    void show(const MoviePlace &place) override
    {
        for (i16 slice = 0; slice < decoder->slices(); slice++) {
            decoder->showSlice(slice, svgaMemory, SCREEN_WIDTH, place.x, place.y, place.doubled);
        }
    }

private:
    AviFile &file;
    std::unique_ptr<RrvDecoder> decoder;

    /* The movie's colours sit in the middle of the palette, grey ramps below
     * and above them; entries 8 and 9 are the ring buffer display's grey and
     * red (PLAYER.C's buildPalette). */
    static void buildPalette(const AviStream &video)
    {
        BitmapFormat format{};
        memcpy(&format, video.format.data(), video.format.size() < sizeof format ? video.format.size() : sizeof format);
        const i16 colors = static_cast<i16>(format.colorsImportant);
        const i16 first = static_cast<i16>(256 - colors) / 2;
        BasePalette palette(0, 256);

        palette.capture();
        if (colors != 0) {
            i16 i;

            for (i = 0; i < first; i++) {
                palette.data[i].red = static_cast<i16>((i + 1) * 4) >> 1;
                palette.data[i].green = static_cast<i16>((i + 1) * 4) >> 1;
                palette.data[i].blue = static_cast<i16>((i + 1) * 4) >> 1;
                palette.data[static_cast<i16>(colors + first + i)].red = static_cast<i16>((first + i) * 4) >> 1;
                palette.data[static_cast<i16>(colors + first + i)].green = static_cast<i16>((first + i) * 4) >> 1;
                palette.data[static_cast<i16>(colors + first + i)].blue = static_cast<i16>((first + i) * 4) >> 1;
            }
            palette.data[8].red = 0x7f;
            palette.data[8].green = 0x7f;
            palette.data[8].blue = 0x7f;
            palette.data[9].red = 0x7f;
            palette.data[9].green = 0;
            palette.data[9].blue = 0;
            for (i = first; i < static_cast<i16>(colors + first); i++) {
                palette.data[i].red = format.palette[i].red >> 2;
                palette.data[i].green = format.palette[i].green >> 2;
                palette.data[i].blue = format.palette[i].blue >> 2;
            }
        }
        palette.apply();
    }
};

}

std::unique_ptr<VideoDecoder> makeRrvVideo(AviFile &file)
{
    return std::make_unique<RrvVideo>(file);
}
