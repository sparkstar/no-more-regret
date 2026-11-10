/* See png.h. The picture data is a zlib stream of stored deflate blocks
 * (at most 65535 bytes each) of the rows, each after filter type 0. */
#include "png.h"

#include <algorithm>
#include <array>
#include <fstream>

namespace {

std::array<u32, 256> crcTable()
{
    std::array<u32, 256> table{};
    for (u32 n = 0; n < 256; n++) {
        u32 c = n;
        for (int k = 0; k < 8; k++) {
            c = ((c & 1) != 0U) ? 0xedb88320U ^ (c >> 1) : c >> 1;
        }
        table[n] = c;
    }
    return table;
}

u32 crc(const u8 *data, size_t size, u32 value = 0xffffffffU)
{
    static const std::array<u32, 256> table = crcTable();
    for (size_t i = 0; i < size; i++) {
        value = table[(value ^ data[i]) & 0xff] ^ (value >> 8);
    }
    return value;
}

void put32(std::vector<u8> &out, u32 value)
{
    for (int shift = 24; shift >= 0; shift -= 8) {
        out.push_back(static_cast<u8>(value >> shift));
    }
}

void chunk(std::vector<u8> &out, const char type[4], const std::vector<u8> &data)
{
    put32(out, static_cast<u32>(data.size()));
    const size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    put32(out, crc(out.data() + start, out.size() - start) ^ 0xffffffffU);
}

/* The PNG of height rows of rowBytes at pixels, pitch apart: colour type
 * 3 (indexed, with palette) or 2 (red, green, blue), 8 bits a sample. */
std::vector<u8> png(const u8 *pixels, u32 width, u32 height, u32 pitch, u32 rowBytes, u8 colourType, const u8 *palette)
{
    std::vector<u8> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    std::vector<u8> header;
    put32(header, width);
    put32(header, height);
    header.insert(header.end(), {8, colourType, 0, 0, 0}); /* 8 bits, deflate, no filter choice, no interlace */
    chunk(out, "IHDR", header);
    if (palette != nullptr) {
        chunk(out, "PLTE", std::vector<u8>(palette, palette + (256 * 3)));
    }
    std::vector<u8> raw;
    raw.reserve(static_cast<size_t>(rowBytes + 1) * height);
    for (u32 y = 0; y < height; y++) {
        raw.push_back(0);
        raw.insert(raw.end(), pixels + (static_cast<size_t>(y) * pitch),
                   pixels + (static_cast<size_t>(y) * pitch) + rowBytes);
    }
    std::vector<u8> z = {0x78, 0x01};
    for (size_t at = 0; at < raw.size() || at == 0;) {
        size_t size = std::min<size_t>(raw.size() - at, 65535);
        const bool last = at + size == raw.size();
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<u8>(size));
        z.push_back(static_cast<u8>(size >> 8));
        z.push_back(static_cast<u8>(~size));
        z.push_back(static_cast<u8>(~size >> 8));
        z.insert(z.end(), raw.begin() + at, raw.begin() + at + size);
        at += size;
        if (last) {
            break;
        }
    }
    u32 a = 1;
    u32 b = 0;
    for (const u8 byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    put32(z, b << 16 | a);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", {});
    return out;
}

bool write(const std::string &path, const std::vector<u8> &bytes)
{
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}

}

std::vector<u8> pngBytes(const u8 *pixels, u32 width, u32 height, u32 pitch, const u8 palette[256 * 3])
{
    return png(pixels, width, height, pitch, width, 3, palette);
}

bool writePng(const std::string &path, const u8 *pixels, u32 width, u32 height, u32 pitch, const u8 palette[256 * 3])
{
    return write(path, pngBytes(pixels, width, height, pitch, palette));
}

std::vector<u8> pngBytesRgb(const u8 *rgb, u32 width, u32 height)
{
    return png(rgb, width, height, 3 * width, 3 * width, 2, nullptr);
}

bool writePngRgb(const std::string &path, const u8 *rgb, u32 width, u32 height)
{
    return write(path, pngBytesRgb(rgb, width, height));
}
