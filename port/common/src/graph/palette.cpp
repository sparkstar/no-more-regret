/* GRAPH\PALETTE.C (1270:0253-0615): filling, copying and loading palettes and
 * converting them to YIQ. See palette.h. */
#include "common.h"
#include "file.h"
#include "palette.h"

void Palette::fill(RGB color)
{
    u16 i;

    data.reset();
    data = std::make_unique_for_overwrite<RGB[]>(256);
    if (data == nullptr) {
        allocationFailure();
    }
    first = 0;
    count = 256;
    for (i = 0; i < count; i++) {
        data[i] = color;
    }
}

void BasePalette::set(RGB *colors, u8 start, u16 size)
{
    data.reset();
    data = std::make_unique_for_overwrite<RGB[]>(size);
    if (data == nullptr) {
        allocationFailure();
    }
    first = start;
    count = size;
    memcpy(data.get(), colors, size * 3);
}

void BasePalette::load(const char *path, s32 offset)
{
    const File file(path, FILE_READ, 0);

    data.reset();
    first = 0;
    count = 256;
    data = std::make_unique_for_overwrite<RGB[]>(count);
    if (data == nullptr) {
        allocationFailure();
    }
    file.read(data.get(), static_cast<ui16>(count * 3), offset);
}

/* UNCALLED: nothing calls it. */
void BasePalette::clear() const
{
    if (data != nullptr) {
        memset(static_cast<void *>(data.get()), 0, count * 3);
    }
}

void BasePalette::toYiq() const
{
    if (data != nullptr) {
        s32 red;
        s32 green;
        s32 blue;
        const s32 yRed = 299;
        const s32 yGreen = 587;
        const s32 yBlue = 114;
        const s32 iRed = 596;
        const s32 iGreen = 275;
        const s32 iBlue = 321;
        const s32 qRed = 212;
        const s32 qGreen = 528;
        const s32 qBlue = 311;
        u16 i;

        for (i = 0; i < count; i++) {
            red = data[i].red;
            green = data[i].green;
            blue = data[i].blue;
            data[i].red = (red * yRed + green * yGreen + blue * yBlue) / 1000;
            data[i].green = (red * iRed - green * iGreen - blue * iBlue) / 1000;
            data[i].blue = (red * qRed - green * qGreen + blue * qBlue) / 1000;
        }
    }
}
