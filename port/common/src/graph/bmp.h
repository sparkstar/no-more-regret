/* Code3 (1010, file name unknown): Windows BMP files of 256 colours, read into
 * a viewport and a palette (the pictures of the help, save and credits
 * screens). */
#ifndef PORT_BMP_H
#define PORT_BMP_H
#include "common.h"
#include "file.h"
#include "palette.h"
#include "viewport.h"

struct __attribute__((packed)) BmpFileHeader {
    u16 type;
    u32 size;
    u16 reserved1;
    u16 reserved2;
    /* Where the pixels start. */
    u32 offset;
};
struct __attribute__((packed)) BmpInfoHeader {
    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bitCount;
    u32 compression;
    u32 imageSize;
    s32 xPixelsPerMeter;
    s32 yPixelsPerMeter;
    u32 colorsUsed;
    u32 colorsImportant;
};
struct __attribute__((packed)) BmpColor {
    u8 blue;
    u8 green;
    u8 red;
    u8 reserved;
};
ASSERT_FILE_SIZE(BmpFileHeader, 0xe);
ASSERT_FILE_SIZE(BmpInfoHeader, 0x28);
ASSERT_FILE_SIZE(BmpColor, 4);

class BmpFile {
public:
    File *file;
    BmpFileHeader header;
    BmpInfoHeader info;
    BmpColor colors[256];

    BmpFile(); /* 1010:0000 */
    /* Opens path (1010:0032). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    explicit BmpFile(const char *path);
    ~BmpFile(); /* 1010:0069 */
    /* Reads the headers and the colours; 0 when a file is open already or
     * path cannot be opened (1010:0098). */
    i16 open(const char *path);
    void close(); /* 1010:0117 */
    /* Reads the rows, bottom up, into the viewport from its top left
     * (1010:0167). */
    void draw(Viewport *viewport) const;
    /* The colours in the palette's 6 bits; a new palette when it is 0
     * (1010:01eb). */
    void getPalette(std::unique_ptr<RGB[]> &palette);
};
ASSERT_SIZE(BmpFile, 0x43a);
#endif
