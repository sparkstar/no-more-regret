/* VGA palettes: GRAPH\BASEPAL.C (1268) and GRAPH\PALETTE.C (1270). */
#ifndef PORT_PALETTE_H
#define PORT_PALETTE_H
#include "common.h"

#include <memory>
struct RGB {
    u8 red, green, blue;
    /* With a declared constructor, Borland copies an RGB field by field
     * instead of through its struct-push helper. */
    RGB() = default;
    RGB(u8 r, u8 g, u8 b)
    {
        red = r;
        green = g;
        blue = b;
    }
};
class BasePalette {
public:
    /* count entries, its own. */
    std::unique_ptr<RGB[]> data;
    u16 count;
    u8 first;
    BasePalette()
    {
        first = 0;
        count = 256;
    }
    BasePalette(u8 firstColour, u16 colourCount) { allocate(firstColour, colourCount); }
    ~BasePalette();
    /* Replaces data with count new entries (1268:0000). */
    void allocate(u8 start, u16 size);
    /* Reads or writes the hardware palette; -1 means the whole range. */
    void capture(s16 start = -1, s16 end = -1);
    void apply(s16 start = -1, s16 end = -1);
    /* The entry nearest to a color: by the sum of the component differences
     * (1268:01eb) or of their squares (1268:02b9). */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u8 closest(u8 red, u8 green, u8 blue) const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u8 closestSquared(u8 red, u8 green, u8 blue) const;
    /* The entry of start..start+count-1 other than index nearest to it
     * (1268:038c). */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u8 closestTo(u8 index, i16 start, i16 size) const;
    /* Fills egaColors with the entries nearest to the 16 EGA colors (1268:04c2). */
    /* UNCALLED: nothing calls it. */
    void matchEgaColors() const;
    /* Replaces data with a copy of count entries of colors (1270:0305). */
    void set(RGB *colors, u8 start, u16 size);
    /* Reads 256 entries at offset of path (1270:03a3). */
    void load(const char *path, s32 offset);
    /* UNCALLED: nothing calls it. */
    void clear() const; /* 1270:0473 */
    /* Converts every entry to YIQ: red becomes the luminance Y, green I and
     * blue Q (1270:04a1). */
    void toYiq() const;
};
class Palette : public BasePalette {
public:
    Palette() = default;
    explicit Palette(const RGB &color) { fill(color); }
    /* A copy of other's entries. */
    explicit Palette(const BasePalette &other) { set(other.data.get(), other.first, other.count); }
    /* The same for a Palette: BCC takes the constructor above for one too
     * (jumpToGreyScale, 1038:0605, calls BasePalette::set), where a modern
     * compiler takes the implicit copy, which shares the entries and frees
     * them with the copy (the CCTV's grey view would free the game palette's). */
    Palette(const Palette &other) : BasePalette() { set(other.data.get(), other.first, other.count); }
    /* A copy of 256 entries of colors. */
    explicit Palette(RGB *colors) { set(colors, 0, 256); }
    /* UNCALLED: nothing calls it. */
    Palette(u8 firstColour, u16 colourCount) : BasePalette(firstColour, colourCount) {}
    /* The 256 entries of path. GAMEPAL.C reads its local path straight, with
     * no argument temporary: the path is taken by reference. */
    explicit Palette(char *&path) { load(path, 0); }
    void fill(RGB color);
};
ASSERT_SIZE(RGB, 0x3);
ASSERT_SIZE(BasePalette, 0x7);
ASSERT_SIZE(Palette, 0x7);
#endif
