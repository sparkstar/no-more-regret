/* MISC\TARGA.C (12c8:0000-0970): Targa images. See targa.h. */
#include <cstring>
#include "targa.h"
#include "file.h"
#include "utility.h"

Targa::Targa(const char *imagePath, u16 width, u16 height, u16 depth)
{
    if (imagePath == nullptr || (fileExists(imagePath) == 0)) {
        create(imagePath, width, height, depth);
    } else {
        load(imagePath);
    }
}

/* The members free what they own. */
Targa::~Targa() = default;

void Targa::create(const char *name, u16 width, u16 height, u16 depth)
{
    if (name != nullptr) {
        strcpy(path, name);
    }
    memset(&header, 0, sizeof(header));
    header.colorMapType = 1;
    header.imageType = TARGA_MAPPED;
    header.colorMapLength = 256;
    header.colorMapDepth = 24;
    header.width = width;
    header.height = height;
    header.pixelDepth = depth;
    header.descriptor |= TARGA_TOP_FIRST;
    palette.reset();
    viewport.reset();
    id.reset();
    palette = std::make_unique<Palette>(RGB(0, 0, 0));
    viewport = std::make_unique<Viewport>(VIEWPORT_LARGE, 0, 0, header.width - 1, header.height - 1);
    if (palette == nullptr || viewport == nullptr) {
        halt();
    }
    viewport->clear(0);
}

void Targa::load(const char *name)
{
    strcpy(path, name);
    const File file(path, FILE_READ, 0);
    file.read(&header, sizeof(header));
    if (header.idLength != 0U) {
        id.reset();
        id = std::make_unique_for_overwrite<char[]>(header.idLength + 1);
        if (id == nullptr) {
            allocationFailure();
        }
        file.read(id.get(), header.idLength);
        id[header.idLength] = 0;
    }
    if (header.colorMapLength != 0U) {
        palette.reset();
        palette = std::make_unique<Palette>(RGB(0, 0, 0));
        if (palette == nullptr) {
            allocationFailure();
        }
        u32 entry = 0;
        u16 size = 0;
        u16 bits = 0;
        u32 mask = 0;
        u16 i;
        switch (header.colorMapDepth) {
        case 32:
            size = 4;
            bits = 8;
            break;
        case 24:
            size = 3;
            bits = 8;
            break;
        default:
            size = 2;
            bits = 5;
            break;
        }
        mask = (static_cast<s32>(1L) << bits) - 1;
        /* The entries are blue, green, red from the low bits up. */
        for (i = 0; i < header.colorMapLength; i++) {
            entry = 0;
            file.read(&entry, size);
            palette->data[i].blue = entry & mask;
            entry >>= bits;
            palette->data[i].green = entry & mask;
            entry >>= bits;
            palette->data[i].red = entry & mask;
        }
    }
    viewport.reset();
    viewport = std::make_unique<Viewport>(VIEWPORT_LARGE, 0, 0, header.width - 1, header.height - 1);
    if (viewport == nullptr) {
        allocationFailure();
    }
    viewport->clear(0);
    switch (header.imageType) {
    case TARGA_MAPPED:
        file.read(viewport->address(0, 0), static_cast<u32>(header.width) * header.height);
        break;
    default:
        halt();
        break;
    }
}

void Targa::save(const char *name, u8 paletteOnly)
{
    if (name != nullptr) {
        strcpy(path, name);
    }
    /* The original also tests path's address (!path), which is never 0. */
    if (*path == 0) {
        return;
    }
    const File file(path, FILE_READWRITE, 1);
    file.write(&header, sizeof(header));
    if (header.idLength != 0U) {
        file.write(id.get(), header.idLength);
    }
    if (header.colorMapType != 0U) {
        RGB colors[256];
        i16 i;

        /* The palette's 6-bit components as 8-bit blue, green, red. */
        for (i = 0; i < 256; i++) {
            colors[i].blue = palette->data[i].red << 2;
            colors[i].green = palette->data[i].green << 2;
            colors[i].red = palette->data[i].blue << 2;
        }
        file.write(colors, sizeof(colors));
    }
    if (paletteOnly == 0U) {
        file.write(viewport->address(0, 0), static_cast<u32>(header.height) * header.width);
    }
}
