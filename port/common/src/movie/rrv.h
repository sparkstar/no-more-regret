/* The RRV movies' frames (No Remorse's flics): PLAYER.C's slices of runs
 * decoded into a picture of the decoder's own, which the player copies to the
 * screen a slice or a frame at a time. A frame's chunk starts with the
 * offsets of its slices; a slice with the offset of its pixels from its
 * codes, then the codes: 4-bit indexes of run lengths and their extra bits,
 * skip and copy runs in turn, the first a skip. The runs of "00db" chunks of
 * the RRV movies are of pixel pairs (each pixel drawn twice across). */
#ifndef PORT_RRV_H
#define PORT_RRV_H
#include "common.h"
#include "videodecoder.h"

#include <memory>
#include <vector>

class RrvDecoder {
public:
    /* A picture of width by height in slices; pairs when the RRV movie's
     * 00db chunks hold pixel pairs (rrvMode). */
    RrvDecoder(u16 width, u16 height, i16 slices, bool rrvMode);
    /* The picture starts as the screen under it: at x, y of a screen of
     * pitch bytes a line, each pixel 2 across on every other line when
     * doubled (PLAYER.C's doubled drawing leaves the lines between). */
    void fill(const u8 *screen, u16 pitch, i16 x, i16 y, bool doubled);
    /* The next frame's chunk (its id and data after the RIFF header). */
    void begin(u32 id, const u8 *data);
    /* Decodes a slice of the chunk into the picture. */
    void decodeSlice(i16 index);
    /* Copies a slice's lines of the picture to the screen, as fill takes
     * them. */
    void showSlice(i16 index, u8 *screen, u16 pitch, i16 x, i16 y, bool doubled) const;
    [[nodiscard]] i16 slices() const { return sliceCount; }

private:
    u16 width;
    u16 height;
    i16 sliceCount;
    bool rrvMode;
    bool pairs;
    const u8 *chunk;
    std::vector<u8> picture;

    [[nodiscard]] u16 sliceLines() const { return static_cast<u16>(height / sliceCount); }
};

/* The RRV movies behind videodecoder.h's interface: a frame's slices decoded and
 * put on the screen, the movie's palette as PLAYER.C's buildPalette made it
 * (not for a movie on the screen kept as it is). */
std::unique_ptr<VideoDecoder> makeRrvVideo(AviFile &file);
#endif
