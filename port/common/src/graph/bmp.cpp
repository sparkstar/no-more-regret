/* Code3 (1010, file name unknown): Windows BMP files. */
#include <cstdio>
#include "bmp.h"

BmpFile::BmpFile()
{
    file = nullptr;
}

/* The original does not clear file first, so open failed when the
 * object's memory held something there; the port clears it. */
/* UNCALLED: nothing calls it. */
BmpFile::BmpFile(const char *path) : file(nullptr)
{
    open(path);
}

BmpFile::~BmpFile()
{
    close();
}

i16 BmpFile::open(const char *path)
{
    if (file != nullptr) {
        return 0;
    }
    file = new File(path, FILE_DENY_WRITE, 0);
    file->read(&header, sizeof(header), 0L);
    file->read(&info, static_cast<ui16>(sizeof(info) + sizeof(colors)), sizeof(header));
    return 1;
}

void BmpFile::close()
{
    if (file == nullptr) {
        return;
    }
    file->close();
    delete file;
    file = nullptr;
}

void BmpFile::draw(Viewport *viewport) const
{
    i16 height;
    i16 width;
    u8 *line;
    i16 i;
    i16 row;

    if (file == nullptr) {
        return;
    }
    height = info.height;
    width = info.width;
    file->seek(header.offset, SEEK_SET);
    for (i = 0, row = height - 1; i < info.height; i++) {
        line = viewport->address(0, row);
        file->read(line, width, -1L);
        row--;
    }
}

void BmpFile::getPalette(std::unique_ptr<RGB[]> &palette)
{
    i16 i;

    if (file != nullptr) {
        if (palette == nullptr) {
            palette = std::make_unique_for_overwrite<RGB[]>(256);
        }
        for (i = 0; i < 256; i++) {
            palette[i].red = colors[i].red >> 2;
            palette[i].green = colors[i].green >> 2;
            palette[i].blue = colors[i].blue >> 2;
        }
    }
}
