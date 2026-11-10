/* The movies' decoders by codec. See videodecoder.h. */
#include "videodecoder.h"
#include "rrv.h"
#include "xanvideo.h"

std::unique_ptr<VideoDecoder> makeVideoDecoder(AviFile &file, const MovieSettings &settings)
{
    const AviStream *video = file.stream(riffId('v', 'i', 'd', 's'));
    if (video == nullptr) {
        return nullptr;
    }
    if (video->handler == riffId('X', 'x', 'a', 'n')) {
        return makeXanVideo(file, settings);
    }
    if (video->handler == riffId('R', 'R', 'V', '1') || video->handler == riffId('R', 'R', 'V', '2') ||
        video->handler == riffId('J', 'Y', 'V', '1')) {
        return makeRrvVideo(file);
    }
    return nullptr;
}
