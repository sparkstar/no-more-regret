/* A movie's AVI file. See avifile.h. */
#include <cstring>
#include "avifile.h"
#include "file.h"
#include "utility.h"

namespace {

const u32 RIFF = riffId('R', 'I', 'F', 'F');
const u32 LIST = riffId('L', 'I', 'S', 'T');

u32 word32(const u8 *at)
{
    u32 value;
    memcpy(&value, at, 4);
    return value;
}

/* Calls each(id, data, size) for the chunks from at to end; a list's data
 * starts with its type. */
template <typename Each> void chunksOf(u8 *at, u8 *end, Each each)
{
    while (end - at >= 8) {
        const u32 id = word32(at);
        u32 size = word32(at + 4);
        if (size > static_cast<u32>(end - at - 8)) {
            size = static_cast<u32>(end - at - 8);
        }
        each(id, at + 8, size);
        at += 8 + size + (size & 1);
    }
}

}

const std::vector<u8> *AviStream::list(u32 listType) const
{
    for (const List &each : lists) {
        if (each.type == listType) {
            return &each.data;
        }
    }
    return nullptr;
}

const AviStream *AviFile::stream(u32 type) const
{
    for (const AviStream &each : streams) {
        if (each.type == type) {
            return &each;
        }
    }
    return nullptr;
}

bool AviFile::open(const char *path)
{
    bytes.clear();
    streams.clear();
    movi.clear();
    if (fileExists(path) == 0) {
        return false;
    }
    File file;
    file.open(path, 0, 0);
    if (file.isOpen() == 0U) {
        return false;
    }
    bytes.resize(file.size());
    const u32 read = bytes.empty() ? 0 : file.read(bytes.data(), bytes.size());
    file.close();
    bytes.resize(read);
    if (bytes.size() < 12 || word32(&bytes[0]) != RIFF || word32(&bytes[8]) != riffId('A', 'V', 'I', ' ')) {
        return false;
    }
    bool headed = false;
    chunksOf(&bytes[12], bytes.data() + bytes.size(), [&](u32 id, u8 *data, u32 size) {
        if (id != LIST || size < 4) {
            return;
        }
        const u32 type = word32(data);
        if (type == riffId('h', 'd', 'r', 'l')) {
            chunksOf(data + 4, data + size, [&](u32 inner, u8 *body, u32 length) {
                if (inner == riffId('a', 'v', 'i', 'h') && length >= sizeof(AviHeader)) {
                    memcpy(&main, body, sizeof(AviHeader));
                    headed = true;
                } else if (inner == LIST && length >= 4 && word32(body) == riffId('s', 't', 'r', 'l')) {
                    AviStream stream{};
                    chunksOf(body + 4, body + length, [&](u32 part, u8 *at, u32 bytesOf) {
                        if (part == riffId('s', 't', 'r', 'h') && bytesOf >= 0x24) {
                            stream.type = word32(at);
                            stream.handler = word32(at + 4);
                            stream.initialFrames = word32(at + 16);
                            stream.length = word32(at + 32);
                            stream.suggestedBufferSize = word32(at + 36);
                        } else if (part == riffId('s', 't', 'r', 'f')) {
                            stream.format.assign(at, at + bytesOf);
                        } else if (part == LIST && bytesOf >= 4) {
                            stream.lists.push_back({word32(at), std::vector<u8>(at + 4, at + bytesOf)});
                        }
                    });
                    streams.push_back(std::move(stream));
                }
            });
        } else if (type == riffId('m', 'o', 'v', 'i')) {
            /* The chunks in order, those of its lists ('rec ') too. */
            auto add = [&](auto &self, u8 *from, u8 *to) -> void {
                chunksOf(from, to, [&](u32 inner, u8 *body, u32 length) {
                    if (inner == LIST && length >= 4) {
                        self(self, body + 4, body + length);
                    } else {
                        movi.push_back({inner, body, length});
                    }
                });
            };
            add(add, data + 4, data + size);
        }
    });
    return headed && (stream(riffId('v', 'i', 'd', 's')) != nullptr);
}
