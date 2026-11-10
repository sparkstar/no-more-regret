/* A PNG writer of the port's own: 8-bit indexed pictures and their palette,
 * or pictures of red, green and blue (the 16-bit screen's), uncompressed
 * deflate. The same pixels give the same bytes. */
#ifndef PORT_PNG_H
#define PORT_PNG_H
#include "common.h"

#include <string>
#include <vector>

/* width x height pixels, rows pitch bytes apart; palette holds 256 red,
 * green, blue triples of 8 bits. The PNG's bytes. */
std::vector<u8> pngBytes(const u8 *pixels, u32 width, u32 height, u32 pitch, const u8 palette[256 * 3]);
/* The same written to path; false when it cannot be. */
bool writePng(const std::string &path, const u8 *pixels, u32 width, u32 height, u32 pitch, const u8 palette[256 * 3]);
/* width x height pixels of red, green and blue bytes, row after row. */
std::vector<u8> pngBytesRgb(const u8 *rgb, u32 width, u32 height);
bool writePngRgb(const std::string &path, const u8 *rgb, u32 width, u32 height);
#endif
