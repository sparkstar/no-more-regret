/* The Xan movies' frames (No Regret's): XANPLAY.C's player of
 * XANDRV32.DLL's decoder (common/xan), behind videodecoder.h's
 * interface. */
#ifndef PORT_XANVIDEO_H
#define PORT_XANVIDEO_H
#include "videodecoder.h"

std::unique_ptr<VideoDecoder> makeXanVideo(AviFile &file, const MovieSettings &settings);
#endif
