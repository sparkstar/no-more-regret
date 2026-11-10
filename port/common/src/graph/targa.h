/* MISC\TARGA.C (12c8:0000-0970): Targa (TGA) images of 8-bit color-mapped
 * pixels, read into and written from a viewport and a palette. */
#ifndef PORT_TARGA_H
#define PORT_TARGA_H
#include "common.h"
#include "palette.h"
#include "viewport.h"

#include <memory>

/* The 18-byte header at the start of the file. */
struct __attribute__((packed)) TargaHeader {
    u8 idLength;
    u8 colorMapType;
    u8 imageType;
    u16 colorMapStart;
    u16 colorMapLength;
    u8 colorMapDepth; /* bits per palette entry */
    u16 xOrigin;
    u16 yOrigin;
    u16 width;
    u16 height;
    u8 pixelDepth;
    u8 descriptor;
};
ASSERT_FILE_SIZE(TargaHeader, 0x12);
/* TargaHeader::imageType: uncompressed color-mapped, the only one read. */
constexpr int TARGA_MAPPED = 1;
/* TargaHeader::descriptor: the first row is the top one. */
constexpr int TARGA_TOP_FIRST = 0x20;

class Targa {
public:
    /* All zero until read or made, as the original's memset of the object;
     * the field, the viewport and the palette are its own. */
    TargaHeader header{};
    /* The image identification field, 0-terminated. */
    std::unique_ptr<char[]> id;
    char path[40]{};
    std::unique_ptr<Viewport> viewport;
    std::unique_ptr<Palette> palette;

    /* Reads path when it exists, else starts a black width x height image
     * with depth bits per pixel that save writes to path. */
    Targa(const char *imagePath, u16 width, u16 height, u16 depth);
    ~Targa();
    void create(const char *name, u16 width, u16 height, u16 depth);
    void load(const char *name);
    /* Writes the image to path, or to the path it has when path is 0;
     * with paletteOnly, the header and the palette only. */
    void save(const char *name, u8 paletteOnly);
};
ASSERT_SIZE(Targa, 0x46);
#endif
