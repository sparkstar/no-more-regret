/* A movie's AVI file, either game's: its main header, its streams and, in
 * order, the chunks of its movi list. The file is read whole; the chunks'
 * data stay in it (a decoder may rewrite a chunk in place, as Xan's does). */
#ifndef PORT_AVIFILE_H
#define PORT_AVIFILE_H
#include "common.h"

#include <vector>

/* The four characters of a RIFF id as one little-endian number. */
constexpr u32 riffId(char a, char b, char c, char d)
{
    return static_cast<u32>(static_cast<u8>(a)) | static_cast<u32>(static_cast<u8>(b)) << 8 |
           static_cast<u32>(static_cast<u8>(c)) << 16 | static_cast<u32>(static_cast<u8>(d)) << 24;
}

/* The main header ('avih'). */
struct AviHeader {
    u32 microSecPerFrame;
    u32 maxBytesPerSec;
    u32 paddingGranularity;
    u32 flags;
    u32 totalFrames;
    u32 initialFrames;
    u32 streams;
    u32 suggestedBufferSize;
    u32 width;
    u32 height;
};

/* A stream: its header's fields ('strh'), its format ('strf': a bitmap's
 * header and palette, or a wave format) and the lists after it in its
 * 'strl' (Xan's 'MOVE': its palettes, colour tables and indexes). */
struct AviStream {
    u32 type;    /* 'vids' or 'auds' */
    u32 handler; /* the video's codec: 'Xxan', 'RRV1', 'RRV2', 'JYV1' */
    u32 initialFrames;
    u32 length;
    u32 suggestedBufferSize;
    std::vector<u8> format;
    /* A list's type and its chunks (the data after the type). */
    struct List {
        u32 type;
        std::vector<u8> data;
    };
    std::vector<List> lists;

    /* The data of the list of type, nullptr when the stream has none. */
    [[nodiscard]] const std::vector<u8> *list(u32 type) const;
};

class AviFile {
public:
    /* A chunk of the movi list. */
    struct Chunk {
        u32 id;
        u8 *data;
        u32 size;
    };

    /* Reads the file the game opens by path; false when it is not an AVI
     * file this reads. */
    bool open(const char *path);
    [[nodiscard]] const AviHeader &header() const { return main; }
    /* The first stream of type; nullptr when there is none. */
    [[nodiscard]] const AviStream *stream(u32 type) const;
    [[nodiscard]] const std::vector<Chunk> &chunks() const { return movi; }

private:
    std::vector<u8> bytes;
    AviHeader main{};
    std::vector<AviStream> streams;
    std::vector<Chunk> movi;
};
#endif
