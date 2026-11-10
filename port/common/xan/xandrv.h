/* XANDRV32.DLL's entries the game calls (XANPLAY.C), ported (xanlib.h), on
 * the port's pointers: the decoder is the block of memory the caller
 * gives, the chunks and the colour table where the caller has them, the
 * screen the memory the frame is drawn on. */
#ifndef PORT_XANDRV_H
#define PORT_XANDRV_H
#include <cstdint>

namespace xan {

/* Entry 1 (04AAh): a decoder of width x height pixels in format (2 16-bit,
 * 3 8-bit) at memory, its planes cleared: best the packed colour table
 * (format 3; format 2 a table of 256 lumas or none), flags 1 grey, the
 * masks of format 2's pixels, range the chroma's (FFh), bits the movie's.
 * memory, or nullptr with the error (lastError). */
std::uint8_t *create(std::uint32_t width, std::uint32_t height, std::uint32_t format, const std::uint8_t *best,
                     std::uint32_t flags, std::uint32_t red, std::uint32_t green, std::uint32_t blue,
                     std::uint32_t range, std::uint32_t bits, std::uint8_t *memory);
/* Entry 4 (047Ah): the colour table (format 3's the game adjusts). */
std::uint8_t *image(std::uint8_t *decoder);
/* Entry 6 (256Bh): decodes the chunk (a frame) into the planes; flags 1
 * skips the luma's corrections (a late frame). The chunk's chroma table
 * is rewritten in place. */
std::uint32_t decode(std::uint8_t *decoder, std::uint8_t *chunk, std::uint32_t flags);
/* Entry 7 (25F2h): the frame on the screen, pitch pixels a line when flags
 * has 4; flags 2 doubles it, 1 draws it grey, 8 upside down. */
std::uint32_t draw(std::uint8_t *decoder, std::uint8_t *screen, std::uint32_t pitch, std::uint32_t flags);
/* The DLL's error of the last call that failed (2Ch). */
std::uint32_t lastError();

}
#endif
