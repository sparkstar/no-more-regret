/* The NUT container (ffmpeg's doc/nut.txt, version 3), which the capture
 * writes for ffmpeg, to a pipe as well as to a file: a stream of video,
 * raw frames of red, green and blue bytes (fourcc RGB 24, ffmpeg's rgb24),
 * and one of audio, 16-bit signed samples of two channels (PSD 16,
 * ffmpeg's pcm_s16le). Each frame is a key frame with its own size and
 * time in its header; a syncpoint comes before each picture with the sound
 * before it. Only what ffmpeg's demuxer reads: no index, no info packets.
 * (Without an index ffmpeg looks for the last time of a file, not of a
 * pipe, and says "no index at the end" and "read_timestamp failed"; it
 * reads the file all the same. A syncpoint after the last frame would end
 * a pipe with an error.)
 *
 * The packets' checksum is CRC-32 of the polynomial 04C11DB7h, the bits
 * from the top, from 0 and kept as it is, written high byte first (so the
 * CRC of the bytes and the checksum after them is 0): ffmpeg's
 * ff_crc04C11DB7_update. The pieces are pure; the capture (capture.cpp)
 * writes them. */
#ifndef PORT_NUT_H
#define PORT_NUT_H
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nut {

/* The streams' numbers. */
const std::uint8_t VIDEO = 0;
const std::uint8_t AUDIO = 1;

/* The video's size and frames a second, the audio's samples a second and
 * channels. */
struct Format {
    std::uint16_t width;
    std::uint16_t height;
    std::uint32_t frameRate;
    std::uint32_t sampleRate;
    std::uint8_t channels;
};

/* A number as NUT's v: 7 bits a byte from the top, the high bit set on
 * all bytes but the last. */
[[nodiscard]] std::vector<std::uint8_t> varint(std::uint64_t value);
/* The CRC of size bytes, on from crc. */
[[nodiscard]] std::uint32_t checksum(const std::uint8_t *bytes, std::size_t size, std::uint32_t crc);
/* A packet: its start code, the size of what follows it (and the header's
 * checksum when that is over 4096), the payload and its checksum. */
[[nodiscard]] std::vector<std::uint8_t> packet(std::uint64_t startCode, const std::vector<std::uint8_t> &payload);
/* What a file starts with: the identifying string, the main header (the
 * time bases 1/frameRate and 1/sampleRate, one valid frame code for any
 * frame) and the streams' headers. */
[[nodiscard]] std::vector<std::uint8_t> headers(const Format &format);
/* A syncpoint at the video's frame. */
[[nodiscard]] std::vector<std::uint8_t> syncpoint(std::uint64_t videoFrame);
/* A frame's header for size bytes of stream at time, in the stream's time
 * base: frames for the video, samples for the audio. The bytes follow it. */
[[nodiscard]] std::vector<std::uint8_t> frameHeader(std::uint8_t stream, std::uint64_t time, std::uint32_t size);

}
#endif
