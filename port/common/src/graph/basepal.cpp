/* GRAPH\BASEPAL.C (1268:0000-0626): a block of palette entries, the VGA DAC
 * and nearest-color searches. See palette.h. */
#include "common.h"
#include "gamestate.h"
#include "palette.h"
#include "screen.h"

/* The palette entries that show the 16 EGA colors (1480:4e94). */
u8 (&egaColors)[16] = gameGlobal<u8[16]>({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

void BasePalette::allocate(u8 start, u16 size)
{
    data.reset();
    data = std::make_unique_for_overwrite<RGB[]>(size);
    if (data == nullptr) {
        allocationFailure();
    }
    count = size;
    first = start;
}

/* data frees itself. */
BasePalette::~BasePalette() = default;

void BasePalette::capture(s16 start, s16 end)
{
    u16 i;
    RGB *entry;

    if (start == -1) {
        start = first;
    }
    if (end == -1) {
        end = count;
    }
    if ((data == nullptr) || count < static_cast<ui16>(end)) {
        allocate(start, end);
    }
    i = start;
    entry = data.get();
    while (i < static_cast<ui16>(end)) {
        screenGetColors(static_cast<u8>(i), 1, reinterpret_cast<u8 *>(entry));
        entry++;
        i++;
    }
}

void BasePalette::apply(s16 start, s16 end)
{
    u16 i;
    RGB *entry;

    if (end == -1) {
        end = count;
    }
    if (start == -1) {
        start = first;
    }
    if ((data == nullptr) || count < static_cast<ui16>(end)) {
        allocate(start, end);
    }
    i = start;
    entry = data.get();
    while (i < static_cast<ui16>(end)) {
        screenSetColors(static_cast<u8>(i), 1, reinterpret_cast<u8 *>(entry));
        entry++;
        i++;
    }
}

/* UNCALLED: nothing calls it. */
u8 BasePalette::closest(u8 red, u8 green, u8 blue) const
{
    u8 best = 0;
    i16 bestDistance = 0x7fff;
    u16 i = 0;
    i16 redDistance;
    i16 greenDistance;
    i16 blueDistance;
    i16 distance;

    do {
        redDistance = red - data[i].red;
        if (redDistance < 0) {
            redDistance = -redDistance;
        }
        greenDistance = green - data[i].green;
        if (greenDistance < 0) {
            greenDistance = -greenDistance;
        }
        blueDistance = blue - data[i].blue;
        if (blueDistance < 0) {
            blueDistance = -blueDistance;
        }
        distance = redDistance + greenDistance + blueDistance;
        if (distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
        i++;
    } while (i < count);
    return best;
}

/* UNCALLED: nothing calls it. */
u8 BasePalette::closestSquared(u8 red, u8 green, u8 blue) const
{
    u8 best = 0;
    s32 bestDistance = 0x0fffffffL;
    u16 i = 0;
    s32 redDistance;
    s32 greenDistance;
    s32 blueDistance;
    s32 distance;

    do {
        redDistance = static_cast<i16>(red - data[i].red);
        greenDistance = static_cast<i16>(green - data[i].green);
        blueDistance = static_cast<i16>(blue - data[i].blue);
        distance = redDistance * redDistance + greenDistance * greenDistance + blueDistance * blueDistance;
        if (distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
        i++;
    } while (i < count);
    return best;
}

/* UNCALLED: nothing calls it. */
u8 BasePalette::closestTo(u8 index, i16 start, i16 size) const
{
    u8 best = 0;
    s32 bestDistance = 0x0fffffffL;
    i16 i = start;
    const s32 red = data[index].red;
    const s32 green = data[index].green;
    const s32 blue = data[index].blue;
    s32 redDistance;
    s32 greenDistance;
    s32 blueDistance;
    s32 distance;

    do {
        redDistance = red - data[i].red;
        greenDistance = green - data[i].green;
        blueDistance = blue - data[i].blue;
        distance = redDistance * redDistance + greenDistance * greenDistance + blueDistance * blueDistance;
        if (distance < bestDistance && index != i) {
            best = i;
            bestDistance = distance;
        }
        i++;
    } while (i < static_cast<i16>(start + size));
    return best;
}

/* UNCALLED: nothing calls it. */
void BasePalette::matchEgaColors() const
{
    egaColors[0] = closest(0, 0, 0);
    egaColors[1] = closest(0, 0, 42);
    egaColors[2] = closest(0, 42, 0);
    egaColors[3] = closest(0, 42, 42);
    egaColors[4] = closest(42, 0, 0);
    egaColors[5] = closest(42, 0, 42);
    egaColors[6] = closest(42, 21, 0);
    egaColors[7] = closest(42, 42, 42);
    egaColors[8] = closest(21, 21, 21);
    egaColors[9] = closest(21, 21, 63);
    egaColors[10] = closest(21, 63, 21);
    egaColors[11] = closest(21, 63, 63);
    egaColors[12] = closest(63, 21, 21);
    egaColors[13] = closest(63, 21, 63);
    egaColors[14] = closest(63, 63, 21);
    egaColors[15] = closest(63, 63, 63);
}
