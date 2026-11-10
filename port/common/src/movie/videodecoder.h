/* A movie's frames as its format makes them: the decoders of the two
 * games' formats behind one interface, picked by the video stream's codec
 * (Xan's 'Xxan', RRV's 'RRV1', 'RRV2' and 'JYV1'), so that the player does
 * not know which game's movie it plays. */
#ifndef PORT_VIDEODECODER_H
#define PORT_VIDEODECODER_H
#include "common.h"
#include "avifile.h"
#include "movie.h"

#include <memory>

class VideoDecoder {
public:
    virtual ~VideoDecoder() = default;
    /* Sets the decoder up for the movie at place: its palette as its
     * format's player set it, its picture as the screen has it. */
    virtual void start(const MoviePlace &place) = 0;
    /* A frame's chunk into the picture; late: for the frames after it only,
     * not to be shown. */
    virtual void decode(const AviFile::Chunk &chunk, bool late) = 0;
    /* The picture on the screen at place. */
    virtual void show(const MoviePlace &place) = 0;
    /* The colours of a two-tone subtitle strip: the palette's white entry
     * and its darkest (Xan's player made entry 255 white). */
    [[nodiscard]] virtual u8 lightColour() const { return 255; }
    [[nodiscard]] virtual u8 darkColour() const { return 0; }
    /* Whether it draws on the 16-bit screen. */
    [[nodiscard]] virtual bool drawsHiColour() const { return false; }
};

/* The decoder for the file's video, nullptr when its codec is none of
 * these. */
std::unique_ptr<VideoDecoder> makeVideoDecoder(AviFile &file, const MovieSettings &settings);
#endif
