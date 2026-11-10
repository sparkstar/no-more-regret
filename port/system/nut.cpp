/* The NUT container's pieces (nut.h). */
#include "nut.h"

#include <array>
#include <cstring>

namespace nut {

namespace {

/* The start codes: 'N', a letter and six bytes. */
const std::uint64_t MAIN_STARTCODE = 0x7A561F5F04ADULL + (static_cast<std::uint64_t>('N' << 8 | 'M') << 48);
const std::uint64_t STREAM_STARTCODE = 0x11405BF2F9DBULL + (static_cast<std::uint64_t>('N' << 8 | 'S') << 48);
const std::uint64_t SYNCPOINT_STARTCODE = 0xE4ADEECA4569ULL + (static_cast<std::uint64_t>('N' << 8 | 'K') << 48);
const char ID_STRING[] = "nut/multimedia container";

/* The frame codes' flags. */
const std::uint32_t FLAG_KEY = 1;
const std::uint32_t FLAG_CODED_PTS = 8;
const std::uint32_t FLAG_STREAM_ID = 16;
const std::uint32_t FLAG_SIZE_MSB = 32;
const std::uint32_t FLAG_CHECKSUM = 64;
const std::uint32_t FLAG_INVALID = 8192;
/* The one frame code the frames take, and the times' low bits a frame's
 * coded time would hold (msb_pts_shift): a coded time of at least 1 << 7
 * is the whole time plus that. */
const std::uint8_t FRAME_CODE = 1;
const std::uint32_t PTS_SHIFT = 7;
/* The bytes a syncpoint is from the frames after it at most (ffmpeg's
 * largest). */
const std::uint32_t MAX_DISTANCE = 65536;

const std::array<std::uint32_t, 256> CRC_TABLE = [] {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t i = 0; i < 256; i++) {
        std::uint32_t crc = i << 24;
        for (int bit = 0; bit < 8; bit++) {
            crc = crc & 0x80000000U ? crc << 1 ^ 0x04C11DB7U : crc << 1;
        }
        table[i] = crc;
    }
    return table;
}();

void append(std::vector<std::uint8_t> &bytes, const std::vector<std::uint8_t> &more)
{
    bytes.insert(bytes.end(), more.begin(), more.end());
}

void appendBigEndian(std::vector<std::uint8_t> &bytes, std::uint64_t value, int count)
{
    for (int k = count - 1; k >= 0; k--) {
        bytes.push_back(static_cast<std::uint8_t>(value >> 8 * k));
    }
}

/* A signed number as NUT's s: 2v - 1 for v above 0, -2v else. */
std::vector<std::uint8_t> signedVarint(std::int64_t value)
{
    return varint(value > 0 ? (2 * static_cast<std::uint64_t>(value)) - 1 : 2 * static_cast<std::uint64_t>(-value));
}

/* NUT's vb: the length, then the bytes. */
std::vector<std::uint8_t> bytesOf(const char *text, std::size_t size)
{
    std::vector<std::uint8_t> bytes = varint(size);
    bytes.insert(bytes.end(), text, text + size);
    return bytes;
}

std::vector<std::uint8_t> mainHeader(const Format &format)
{
    std::vector<std::uint8_t> payload;
    append(payload, varint(3));            /* version */
    append(payload, varint(2));            /* streams */
    append(payload, varint(MAX_DISTANCE)); /* max_distance */
    append(payload, varint(2));            /* time bases */
    append(payload, varint(1));
    append(payload, varint(format.frameRate));
    append(payload, varint(1));
    append(payload, varint(format.sampleRate));
    /* The frame codes: 0 invalid (no fields: as many as the size's
     * multiplier, 1); then 1 to 255 ('N' is invalid of itself) a key frame
     * with its stream, time, size (lsb 0, multiplier 1) and a checksum. */
    append(payload, varint(FLAG_INVALID));
    append(payload, varint(0));
    append(payload, varint(FLAG_KEY | FLAG_CODED_PTS | FLAG_STREAM_ID | FLAG_SIZE_MSB | FLAG_CHECKSUM));
    append(payload, varint(6));
    append(payload, signedVarint(0)); /* time's delta */
    append(payload, varint(1));       /* size's multiplier */
    append(payload, varint(0));       /* stream */
    append(payload, varint(0));       /* size's low part */
    append(payload, varint(0));       /* reserved */
    append(payload, varint(254));     /* count */
    append(payload, varint(0));       /* elision headers: none but the empty one */
    return packet(MAIN_STARTCODE, payload);
}

std::vector<std::uint8_t> streamHeader(std::uint8_t stream, const Format &format)
{
    std::vector<std::uint8_t> payload;
    append(payload, varint(stream));
    append(payload, varint(stream == VIDEO ? 0 : 1)); /* class: video, audio */
    append(payload, stream == VIDEO ? bytesOf("RGB\x18", 4) : bytesOf("PSD\x10", 4));
    append(payload, varint(stream)); /* time base: the stream's own */
    append(payload, varint(PTS_SHIFT));
    append(payload, varint(MAX_DISTANCE)); /* max_pts_distance: the frames have checksums */
    append(payload, varint(0));            /* decode delay */
    append(payload, varint(0));            /* flags */
    append(payload, varint(0));            /* codec's data */
    if (stream == VIDEO) {
        append(payload, varint(format.width));
        append(payload, varint(format.height));
        append(payload, varint(1)); /* square pixels */
        append(payload, varint(1));
        append(payload, varint(0)); /* colour space: unknown */
    } else {
        append(payload, varint(format.sampleRate));
        append(payload, varint(1));
        append(payload, varint(format.channels));
    }
    return packet(STREAM_STARTCODE, payload);
}

}

std::vector<std::uint8_t> varint(std::uint64_t value)
{
    std::vector<std::uint8_t> bytes;
    int groups = 1;
    while (groups < 10 && ((value >> 7 * groups) != 0U)) {
        groups++;
    }
    for (int k = groups - 1; k >= 0; k--) {
        const std::uint8_t low = static_cast<std::uint8_t>(value >> 7 * k & 0x7f);
        bytes.push_back((k != 0) ? low | 0x80 : low);
    }
    return bytes;
}

std::uint32_t checksum(const std::uint8_t *bytes, std::size_t size, std::uint32_t crc)
{
    for (std::size_t k = 0; k < size; k++) {
        crc = crc << 8 ^ CRC_TABLE[(crc >> 24 ^ bytes[k]) & 0xff];
    }
    return crc;
}

std::vector<std::uint8_t> packet(std::uint64_t startCode, const std::vector<std::uint8_t> &payload)
{
    std::vector<std::uint8_t> bytes;
    appendBigEndian(bytes, startCode, 8);
    const std::uint64_t forward = payload.size() + 4;
    append(bytes, varint(forward));
    if (forward > 4096) {
        appendBigEndian(bytes, checksum(bytes.data(), bytes.size(), 0), 4);
    }
    append(bytes, payload);
    appendBigEndian(bytes, checksum(payload.data(), payload.size(), 0), 4);
    return bytes;
}

std::vector<std::uint8_t> headers(const Format &format)
{
    std::vector<std::uint8_t> bytes(ID_STRING, ID_STRING + std::strlen(ID_STRING) + 1);
    append(bytes, mainHeader(format));
    append(bytes, streamHeader(VIDEO, format));
    append(bytes, streamHeader(AUDIO, format));
    return bytes;
}

std::vector<std::uint8_t> syncpoint(std::uint64_t videoFrame)
{
    std::vector<std::uint8_t> payload;
    append(payload, varint((videoFrame * 2) + VIDEO)); /* the time and its time base */
    append(payload, varint(0));                        /* back_ptr: this syncpoint */
    return packet(SYNCPOINT_STARTCODE, payload);
}

std::vector<std::uint8_t> frameHeader(std::uint8_t stream, std::uint64_t time, std::uint32_t size)
{
    std::vector<std::uint8_t> bytes{FRAME_CODE};
    append(bytes, varint(stream));
    append(bytes, varint(time + (1U << PTS_SHIFT)));
    append(bytes, varint(size));
    appendBigEndian(bytes, checksum(bytes.data(), bytes.size(), 0), 4);
    return bytes;
}

}
