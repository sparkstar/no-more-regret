/* The Xan movies' frames. See xanvideo.h. */
#include <cstring>
#include "xanvideo.h"
#include "palette.h"
#include "viewport.h"
#include "widesides.h"
#include "x32code.h"
#include "xandrv.h"
#include "screen.h"

#include <vector>

namespace {

/* The memory the decoder needs for a frame of width by height (AVIREAD.C's,
 * which the reader took from the cache after its ring buffer). */
u32 frameMemory(u32 width, u32 height, i16 doubled)
{
    const u32 halves = (width >> 1) * ((height + 1) >> 1) << 1;
    const u32 pixels = width * height;
    u32 tables = 0x10000L;
    const u32 frame = (width * height) + 4;

    if (doubled != 0) {
        tables *= 2;
    }
    return halves + pixels + tables + frame + 0x44;
}

/* The data of the first chunk of id in Xan's MOVE list (AVIREAD.C's
 * findPalette, findBest and the '_PC_' it read); nullptr when it has none. */
const u8 *moveChunk(const std::vector<u8> *move, u32 id)
{
    if (move == nullptr) {
        return nullptr;
    }
    size_t at = 0;
    while (at + 8 <= move->size()) {
        u32 tag;
        u32 size;
        memcpy(&tag, &(*move)[at], 4);
        memcpy(&size, &(*move)[at + 4], 4);
        if (tag == id) {
            return &(*move)[at + 8];
        }
        at += 8 + ((size + 1) & ~1U);
    }
    return nullptr;
}

/* Sets the movie's palette (8-bit channels when bits is above 3) and gives
 * the driver's frame at image the nearest colour to entry 255 in place of
 * FFh; entry 255 becomes white, the subtitles' colour, and the screen the
 * darkest colour, the subtitles' strip's (XANPLAY.C's). */
void setMoviePalette(u8 *image, const u8 *source, i16 bits, u8 &subtitleBackground)
{
    RGB *colors = new RGB[256];
    i16 entry;
    Palette palette;
    i16 red;
    i16 blue;
    i16 green;
    i16 redDistance;
    i16 greenDistance;
    i16 blueDistance;
    i16 nearest;
    i16 nearestEntry;
    i16 distance;
    i16 darkest;
    u8 *pixels;
    i16 i;
    u16 j;

    memcpy(colors, source, 0x300);
    if (bits > 3) {
        for (i = 0; i < 256; i++) {
            colors[i].red >>= 2;
            colors[i].green >>= 2;
            colors[i].blue >>= 2;
        }
    } else {
        for (i = 0; i < 256; i++) {
            colors[i].red &= 0x3f;
            colors[i].green &= 0x3f;
            colors[i].blue &= 0x3f;
        }
    }
    darkest = 0x7fff;
    nearest = 0x7fff;
    nearestEntry = 0;
    red = colors[255].red;
    green = colors[255].green;
    blue = colors[255].blue;
    /* The entry nearest to entry 255, and the darkest one. */
    for (entry = 0; entry < 255; entry++) {
        redDistance = colors[entry].red - red;
        redDistance *= redDistance;
        greenDistance = colors[entry].green - green;
        greenDistance *= greenDistance;
        blueDistance = colors[entry].blue - blue;
        blueDistance *= blueDistance;
        distance = redDistance + greenDistance + blueDistance;
        if (distance < nearest) {
            nearest = distance;
            nearestEntry = entry;
        }
        redDistance = colors[entry].red * colors[entry].red;
        greenDistance = colors[entry].green * colors[entry].green;
        blueDistance = colors[entry].blue * colors[entry].blue;
        distance = redDistance + greenDistance + blueDistance;
        if (distance < darkest) {
            darkest = distance;
            subtitleBackground = static_cast<u8>(entry);
        }
    }
    pixels = image;
    for (j = 0; j < 0x8000; j++) {
        if (pixels[j] == 0xff) {
            pixels[j] = static_cast<u8>(nearestEntry);
        }
    }
    pixels = image + 0x8000;
    for (j = 0; j < 0x8000; j++) {
        if (pixels[j] == 0xff) {
            pixels[j] = static_cast<u8>(nearestEntry);
        }
    }
    colors[255].red = colors[255].green = colors[255].blue = 63;
    palette.set(colors, 0, 256);
    palette.apply();
    drawViewport->clear(subtitleBackground);
    screenViewport->blit(drawViewport);
    /* The port's wide screen: the screen cleared, its sides covered
     * (widesides.h). */
    coverWideSides();
    delete[] colors;
}

class XanVideo : public VideoDecoder {
public:
    XanVideo(AviFile &newFile, const MovieSettings &settings) : file(newFile), lowResolution(settings.lowResolution) {}

    void start(const MoviePlace &place) override
    {
        const AviStream *video = file.stream(riffId('v', 'i', 'd', 's'));
        const std::vector<u8> *move = video->list(riffId('M', 'O', 'V', 'E'));
        const u8 *pc = moveChunk(move, riffId('_', 'P', 'C', '_'));
        u32 bits = 0;
        if (pc != nullptr) {
            memcpy(&bits, pc, 4);
        }
        const u32 width = file.header().width;
        const u32 height = file.header().height;
        memory.assign(frameMemory(width, height, 1), 0);
        if (place.hiColour) {
            decoder = xan::create(width, height, 2, nullptr, 0, 0xf800, 0x7e0, 0x1f, 0xff, bits, memory.data());
        } else {
            decoder = xan::create(width, height, 3, moveChunk(move, riffId('B', 'E', 'S', 'T')), 0, 0, 0, 0, 0xff, bits,
                                  memory.data());
            if (decoder != nullptr) {
                setMoviePalette(xan::image(decoder), moveChunk(move, riffId('P', 'A', 'L', '8')),
                                static_cast<i16>(bits), dark);
            }
        }
        if (decoder == nullptr) {
            haltMessage("Create XanDecompressor failed.\n");
        }
        /* The movie's first line: the original's is its linear address in
         * the VESA window, at the window's bank. */
        screen = svgaMemory + (static_cast<s32>(640L) * place.y + place.x) * (place.hiColour ? 2 : 1);
    }

    void decode(const AviFile::Chunk &chunk, bool late) override
    {
        xan::decode(decoder, chunk.data, static_cast<u32>((lowResolution ? 1 : 0) | (late ? 1 : 0)));
    }

    void show(const MoviePlace &place) override
    {
        xan::draw(decoder, screen, SCREEN_WIDTH, static_cast<u32>(4 | (place.doubled ? 2 : 0)));
    }

    [[nodiscard]] u8 darkColour() const override { return dark; }
    [[nodiscard]] bool drawsHiColour() const override { return true; }

private:
    AviFile &file;
    bool lowResolution;
    std::vector<u8> memory;
    u8 *decoder = nullptr;
    u8 *screen = nullptr;
    u8 dark = 0;
};

}

std::unique_ptr<VideoDecoder> makeXanVideo(AviFile &file, const MovieSettings &settings)
{
    return std::make_unique<XanVideo>(file, settings);
}
