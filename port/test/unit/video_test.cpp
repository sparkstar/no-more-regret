/* The port's display (port/system/video.h), which has no oracle: the DAC's
 * ports and screen.h's palette, the vertical retrace in virtual time, the
 * VESA BIOS and SuperVga's choice of a mode, and the PNG of the screen, which
 * zlib, when the build has it, unpacks as a decoder apart from the port's
 * writer. The game's haltMessage and the kernel's mouse handler are the
 * test's. */
#include "pc.h"
#include "screen.h"
#include "supervga.h"
#include "video.h"
#include "x32code.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#ifdef PORT_HAVE_ZLIB
#include <zlib.h>
#endif

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH video: %s\n", what.c_str());
    }
}

u32 big32(const u8 *p)
{
    return (u32)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

}

void haltMessage(const char *message, std::source_location place)
{
    std::printf("halt %s %u: %s\n", place.file_name(), (unsigned)place.line(), message);
    std::exit(2);
}

const char *haltFormat(const char *format, ...)
{
    return format;
}

int main()
{
    /* The palette: 6 bits a component, from an index that wraps. */
    const u8 colours[6] = {4, 5, 6, 0x7f, 8, 9};
    screenSetColors(255, 2, colours);
    u8 back[6];
    screenGetColors(255, 2, back);
    check(back[0] == 4 && back[1] == 5 && back[2] == 6 && back[3] == 0x3f && back[4] == 8 && back[5] == 9,
          "the colours back, 6 bits each, the index wrapping");

    /* The modes and SuperVga. */
    SuperVga vga;
    check(vga.width() == -1, "no mode chosen");
    check(vga.selectMode(VESA_640X480) && vga.bankStep == 1 && vga.width() == 640 && vga.height() == 480 &&
              vga.granularity() == 64,
          "mode 101h: 640x480, windows of 64K");
    u16 modeWidth, modeHeight;
    check(!screenModeSize(0x100, &modeWidth, &modeHeight) && !vga.selectMode(0x100), "a mode there is not");
    check(screenModeSize(VESA_1024X768, &modeWidth, &modeHeight) && modeWidth == 1024 && modeHeight == 768,
          "mode 105h's size");
    vga.selectMode(VESA_640X480);
    vga.setMode();
    check(screenMode() == VESA_640X480, "the mode set");

    /* The retrace at 60 Hz: a wait ends at the start of the next, a look at
     * it a tick after the one before. */
    screenWaitRetrace();
    check(pcTime() % (PIT_HZ / 60) == 0, "the wait ends as a retrace starts");
    std::uint64_t start = pcTime();
    screenWaitRetrace();
    check(pcTime() - start == PIT_HZ / 60, "screen.h's wait to the next retrace: " + std::to_string(pcTime() - start));

    /* The screen as a PNG: 640x480 indexed, the DAC's colours in 8 bits. */
    for (u32 y = 0; y < 480; y++) {
        for (u32 x = 0; x < 640; x++) {
            svgaMemory[y * 640 + x] = (u8)(x ^ y);
        }
    }
    u8 levels[256 * 3];
    for (int i = 0; i < 256 * 3; i++) {
        levels[i] = (u8)(i & 0x3f);
    }
    screenSetColors(0, 256, levels);
    std::filesystem::path path = std::filesystem::temp_directory_path() / "port-video-test.png";
    check(writeScreen(path.string()), "written");
    std::vector<u8> png;
    {
        std::ifstream in(path, std::ios::binary);
        png.assign(std::istreambuf_iterator<char>(in), {});
    }
    /* Closed first: Windows keeps an open file. */
    std::filesystem::remove(path);
    const u8 signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    check(png.size() > 8 && !std::memcmp(png.data(), signature, 8), "the signature");
    std::vector<u8> palette, data;
    u32 width = 0, height = 0;
    for (size_t at = 8; at + 12 <= png.size();) {
        u32 size = big32(&png[at]);
        std::string type(png.begin() + at + 4, png.begin() + at + 8);
        const u8 *body = &png[at + 8];
        if (type == "IHDR") {
            width = big32(body);
            height = big32(body + 4);
            check(body[8] == 8 && body[9] == 3, "8-bit indexed");
        } else if (type == "PLTE") {
            palette.assign(body, body + size);
        } else if (type == "IDAT") {
            data.insert(data.end(), body, body + size);
        }
        at += 12 + size;
    }
    check(width == 640 && height == 480, "640x480");
    check(palette.size() == 768 && palette[0] == 0 && palette[1] == 4 && palette[0x3f] == 0xff,
          "the palette in 8 bits: level << 2 | level >> 4");
#ifdef PORT_HAVE_ZLIB
    std::vector<u8> raw(641 * 480);
    uLongf size = (uLongf)raw.size();
    check(uncompress(raw.data(), &size, data.data(), (uLong)data.size()) == Z_OK && size == raw.size(),
          "zlib unpacks it");
    bool same = true;
    for (u32 y = 0; y < 480 && same; y++) {
        same = raw[y * 641] == 0 && !std::memcmp(&raw[y * 641 + 1], &svgaMemory[y * 640], 640);
    }
    check(same, "the rows, each after filter 0");
#endif

    /* The hi-colour movies' mode (VESA 111h): 16-bit colours of 5, 6 and 5
     * bits, their top bits again below in the view and the PNG, which is
     * of red, green and blue. */
    check(vga.selectMode(VESA_640X480_64K) && vga.width() == 640 && vga.height() == 480, "mode 111h: 640x480");
    vga.setMode();
    check(screenMode() == VESA_640X480_64K, "mode 111h set");
    ScreenView view;
    check(viewScreen(view) && view.bits == 16 && view.width == 640 && view.height == 480 && view.pitch == 1280,
          "the view of 16-bit pixels, 1280 bytes a row");
    auto put = [&](u32 x, u32 y, u16 c) {
        svgaMemory[y * 1280 + 2 * x] = (u8)c;
        svgaMemory[y * 1280 + 2 * x + 1] = (u8)(c >> 8);
    };
    for (u32 y = 0; y < 480; y++) {
        for (u32 x = 0; x < 640; x++) {
            put(x, y, (u16)(x * 97 + y * 31));
        }
    }
    put(0, 0, 0xf800);
    put(1, 0, 0x07e0);
    put(2, 0, 0x001f);
    put(3, 0, 0x8410);
    put(4, 0, 0xffff);
    put(5, 0, 0x0000);
    check(viewColour(view, 0, 0) == 0xff0000 && viewColour(view, 1, 0) == 0x00ff00 &&
              viewColour(view, 2, 0) == 0x0000ff,
          "red, green and blue at their top");
    check(viewColour(view, 3, 0) == 0x848284 && viewColour(view, 4, 0) == 0xffffff && viewColour(view, 5, 0) == 0,
          "the top bits again below: 10000b is 84h, 100000b 82h");
    check(writeScreen(path.string()), "the 16-bit screen written");
    {
        std::ifstream in(path, std::ios::binary);
        png.assign(std::istreambuf_iterator<char>(in), {});
    }
    std::filesystem::remove(path);
    data.clear();
    palette.clear();
    u8 depth = 0, colourType = 0;
    for (size_t at = 8; at + 12 <= png.size();) {
        u32 chunkSize = big32(&png[at]);
        std::string type(png.begin() + at + 4, png.begin() + at + 8);
        const u8 *body = &png[at + 8];
        if (type == "IHDR") {
            width = big32(body);
            height = big32(body + 4);
            depth = body[8];
            colourType = body[9];
        } else if (type == "PLTE") {
            palette.assign(body, body + chunkSize);
        } else if (type == "IDAT") {
            data.insert(data.end(), body, body + chunkSize);
        }
        at += 12 + chunkSize;
    }
    check(width == 640 && height == 480 && depth == 8 && colourType == 2 && palette.empty(),
          "640x480 of red, green and blue, 8 bits each, no palette");
#ifdef PORT_HAVE_ZLIB
    std::vector<u8> rgb(1921 * 480);
    uLongf rgbSize = (uLongf)rgb.size();
    check(uncompress(rgb.data(), &rgbSize, data.data(), (uLong)data.size()) == Z_OK && rgbSize == rgb.size(),
          "zlib unpacks the 16-bit screen's");
    bool alike = true;
    for (u16 y = 0; y < 480 && alike; y++) {
        alike = rgb[y * 1921] == 0;
        for (u16 x = 0; x < 640 && alike; x++) {
            u32 c = viewColour(view, x, y);
            const u8 *at = &rgb[y * 1921 + 1 + 3 * x];
            alike = at[0] == (u8)(c >> 16) && at[1] == (u8)(c >> 8) && at[2] == (u8)c;
        }
    }
    check(alike, "the 16-bit rows as the view's colours");
#endif

    /* Back to text: no screen to write. */
    screenSetMode(3, false);
    check(!writeScreen(path.string()), "no PNG in a text mode");

    std::printf("video: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
