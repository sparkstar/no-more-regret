/* 1430:0000-0c6e, VFRAME.C: compressed frames of a screen rectangle. See
 * vframe.h. */
#include <mem.h>
#include "vframe.h"
#include "system.h"
#include "host.h"

/* The work buffers. */
constexpr int BUFFER_SIZE = 65500;

/* packRuns's steps for each pixel. */
constexpr int PACK_NEXT = 0;    /* a transparent pixel, a drawn one or the band's end */
constexpr int PACK_DRAWN = 1;   /* count the drawn pixel in its run */
constexpr int PACK_REPEAT = 2;  /* start or lengthen a RUN_REPEAT */
constexpr int PACK_LITERAL = 3; /* the pixel goes in the data as it is */
constexpr int PACK_END = 4;

/* Packs area of screen at *out; the end of the frame. */
/* UNCALLED: nothing calls it. */
u32 packRuns(Viewport *screen, Rect area, u8 **out)
{
    u8 *data = new u8[BUFFER_SIZE];
    u8 *runs = new u8[BUFFER_SIZE];
    i16 width = area.width() - 1;
    i16 height = area.height() - 1;

    if (width == 0) {
        width = 1;
    }
    if (height == 0) {
        height = 1;
    }
    Viewport *image = new Viewport(1, 0, 0, width, height);

    memset(runs, 0, BUFFER_SIZE);
    memset(data, 0, BUFFER_SIZE);
    u8 *dataCursor = data;
    u8 *runCursor = runs;

    image->clear(TRANSPARENT);
    image->blit(screen, area.left, area.top, area.right, area.bottom);
    RunFrame header;

    header.width = area.width();
    header.height = area.height();
    header.left = area.left;
    header.top = area.top;
    memset(header.runOffsets, 0, sizeof(header.runOffsets));
    memset(header.dataOffsets, 0, sizeof(header.dataOffsets));
    const s32 pixelCount = static_cast<i16>(area.height() * area.width());
    i16 state = PACK_NEXT;
    u8 *source = image->memory();
    i16 band = 0;
    const i16 bandPixels = header.width * RUN_LINES;
    s32 pixels = 0;
    u16 y;

    for (y = 0; y < static_cast<ui16>(height); y += RUN_LINES) {
        state = PACK_NEXT;
        bool transparent = true;
        bool repeating = false;
        u16 previous = 0x100;
        i16 repeats = 0;
        s32 column = 0;

        while (state != PACK_END) {
            switch (state) {
            case PACK_NEXT:
                if (bandPixels == column++ || pixels++ == pixelCount) {
                    state = PACK_END;
                } else if (*source == TRANSPARENT) {
                    if (!transparent) {
                        transparent = true;
                        runCursor++;
                    }
                    if (*runCursor == 0xff) {
                        runCursor += 2;
                    }
                    (*runCursor)++;
                    source++;
                } else {
                    if (transparent) {
                        transparent = false;
                        runCursor++;
                    }
                    if (*runCursor == 0xff) {
                        runCursor += 2;
                    }
                    state = PACK_DRAWN;
                }
                break;
            case PACK_DRAWN:
                (*runCursor)++;
                if (static_cast<ui16>(*source) == previous && repeats == 2) {
                    state = PACK_REPEAT;
                } else {
                    state = PACK_LITERAL;
                }
                break;
            case PACK_LITERAL:
                repeating = false;
                /* A pixel of the code's value goes as a repeat of one. */
                if (*source == RUN_REPEAT) {
                    repeats = 0;
                    previous = RUN_REPEAT;
                    state = PACK_REPEAT;
                    break;
                }
                *dataCursor = *source;
                dataCursor++;
                if (static_cast<ui16>(*source) == previous) {
                    repeats++;
                } else {
                    repeats = 0;
                }
                previous = *source;
                source++;
                state = PACK_NEXT;
                break;
            case PACK_REPEAT:
                if (!repeating) {
                    if (repeats == 0) {
                        *dataCursor = RUN_REPEAT;
                        dataCursor++;
                        *dataCursor = 1;
                        dataCursor++;
                        *dataCursor = RUN_REPEAT;
                        dataCursor++;
                    } else {
                        dataCursor[-3] = RUN_REPEAT;
                        dataCursor[-2] = 4;
                    }
                    repeating = true;
                } else if (dataCursor[-2] < 0xff) {
                    dataCursor[-2]++;
                } else {
                    dataCursor[-2] = 0xff;
                    *dataCursor = RUN_REPEAT;
                    dataCursor++;
                    *dataCursor = 1;
                    dataCursor++;
                    *dataCursor = dataCursor[-3];
                    dataCursor++;
                }
                source++;
                state = PACK_NEXT;
                break;
            }
        }
        if (transparent) {
            runCursor += 2;
        } else {
            runCursor++;
        }
        band++;
        header.runOffsets[band] = static_cast<u16>(runCursor - runs);
        header.dataOffsets[band] = static_cast<u16>(dataCursor - data);
    }
    header.dataSize = static_cast<u16>(dataCursor - data);
    header.runSize = static_cast<u16>(runCursor - runs) + 1;
    u8 *frame = *out;

    _fmemcpy(frame, &header, sizeof(header));
    frame += sizeof(header);
    _fmemcpy(frame, data, header.dataSize);
    frame += header.dataSize;
    _fmemcpy(frame, runs, header.runSize);
    frame += header.runSize;
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
    if (runs != nullptr) {
        delete[] runs;
        runs = nullptr;
    }
    if (image != nullptr) {
        delete image;
        image = nullptr;
    }
    return static_cast<u16>(frame - *out);
}

/* Draws bands firstBand..lastBand (-1: from the first, to the last) of frame
 * with its origin at (x,y). */
/* UNCALLED: nothing calls it. */
void unpackRuns(Viewport *target, RunFrame *frame, i16 x, i16 y, i16 firstBand, i16 lastBand)
{
    u8 *data = reinterpret_cast<u8 *>(frame) + FRAME_HEAD;
    u8 *runs = data + frame->dataSize;
    u8 *dataCursor = data;
    u8 *runCursor = runs;
    bool transparent = true;
    u8 count = 0;
    u16 repeats = 0;
    u16 column = 0;
    i16 line = 0;
    u8 value = 0;
    const i16 firstLine = firstBand == -1 ? 0 : firstBand * RUN_LINES;
    i16 lastLine = lastBand == -1 ? frame->height : lastBand * RUN_LINES;

    lastLine = frame->height < static_cast<ui16>(lastLine) ? frame->height : lastLine;
    i16 band = firstBand == -1 ? 0 : firstBand;
    u8 *pixel = nullptr;

    for (line = firstLine; line < lastLine; line++) {
        if (line % RUN_LINES == 0) {
            runCursor = runs + frame->runOffsets[band];
            dataCursor = data + frame->dataOffsets[band];
            transparent = true;
            count = 0;
            band++;
            repeats = 0;
        }
        pixel = target->address(frame->left + x, line + frame->top + y);
        for (column = 0; column < frame->width; pixel++, count++, column++) {
            if (runs + frame->runSize <= runCursor) {
                break;
            }
            if (*runCursor == count) {
                count = 0;
                do {
                    transparent = !transparent;
                    runCursor++;
                    if (runs + frame->runSize <= runCursor) {
                        break;
                    }
                } while (*runCursor == 0);
            }
            if (!transparent) {
                if (repeats != 0U) {
                    *pixel = value;
                    if (--repeats == 0U) {
                        dataCursor++;
                    }
                    continue;
                }
                if (*dataCursor == RUN_REPEAT) {
                    dataCursor++;
                    repeats = *dataCursor - 1;
                    dataCursor++;
                    *pixel = *dataCursor;
                    value = *dataCursor;
                    if (repeats == 0U) {
                        dataCursor++;
                    }
                } else {
                    *pixel = *dataCursor;
                    dataCursor++;
                }
            }
        }
    }
}

/* UNCALLED: nothing calls it. */
u32 packBands(Viewport *screen, Rect area, u8 *out)
{
    i16 width = area.width() - 1;
    i16 height = area.height() - 1;

    if (width == 0) {
        width = 1;
    }
    if (height == 0) {
        height = 1;
    }
    u8 *data = new u8[BUFFER_SIZE];
    Viewport *image = new Viewport(1, 0, 0, width, height);

    memset(data, 0, BUFFER_SIZE);
    u8 *cursor = data;

    image->clear(TRANSPARENT);
    image->blit(screen, area.left, area.top, area.right, area.bottom);
    BandFrame header;

    header.width = area.width();
    header.height = area.height();
    header.left = area.left;
    header.top = area.top;
    memset(header.offsets, 0, sizeof(header.offsets));
    i16 band = 0;
    i16 repeats = 0;
    i16 previous = -1;
    bool packing = false;
    u8 *pixel = image->address(0, 0);
    i16 y;

    for (y = 0; static_cast<ui16>(y) < header.height; y++) {
        if (y % BAND_LINES == 0) {
            header.offsets[band] = static_cast<u16>(cursor - data);
            band++;
            packing = false;
            repeats = 0;
            previous = -1;
        }
        for (u16 x = 0; x < header.width; x++, pixel++) {
            if (packing) {
                if (*pixel == previous && cursor[-1] < 0xff) {
                    cursor[-1]++;
                } else {
                    if (cursor[-1] < BAND_SHORT) {
                        cursor[-3] = cursor[-1];
                        cursor--;
                    }
                    packing = false;
                    *cursor = *pixel;
                    cursor++;
                }
            } else {
                if (*pixel == previous) {
                    repeats++;
                } else {
                    repeats = 0;
                }
                if (*pixel == BAND_REPEAT) {
                    hostDebugBreak();
                    halt();
#pragma GCC diagnostic push
                    /* The original's code after the halt. */
#pragma GCC diagnostic ignored "-Wunreachable-code"
                    *cursor = BAND_REPEAT;
                    cursor++;
#pragma GCC diagnostic pop
                }
                *cursor = *pixel;
                cursor++;
                if (repeats == 3) {
                    packing = true;
                    repeats = 0;
                    cursor[-4] = BAND_REPEAT;
                    cursor[-2] = 0;
                    cursor--;
                }
            }
            previous = *pixel;
        }
    }
    u8 *frame = out;

    /* Copies FRAME_HEAD bytes from the smaller head: the rest of the frame's
     * head is what follows header on the stack. */
#pragma GCC diagnostic push
    /* The original's read past header; nothing calls packBands. */
#pragma GCC diagnostic ignored "-Warray-bounds"
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
    _fmemcpy(frame, &header, FRAME_HEAD);
#pragma GCC diagnostic pop
    frame += FRAME_HEAD;
    _fmemcpy(frame, data, static_cast<u16>(cursor - data) + 1);
    frame += static_cast<u16>(cursor - data) + 1;
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
    if (image != nullptr) {
        delete image;
        image = nullptr;
    }
    return static_cast<u16>(frame - out);
}

/* Unpacks bands bands from band (-1: all the rest). */
/* UNCALLED: nothing calls it. */
void unpackBands(Viewport *target, BandFrame *frame, i16 x, i16 y, i16 band, i16 bands)
{
    u8 *data = reinterpret_cast<u8 *>(frame) + frame->offsets[band] + FRAME_HEAD;
    i16 lines = bands == -1 ? frame->height : bands * BAND_LINES;

    if (static_cast<ui16>(lines + (band * BAND_LINES)) > frame->height) {
        lines = frame->height - band * BAND_LINES;
    }
    i16 repeats = 0;
    /* The first pixel sets it (repeats starts at 0). */
    u8 value = 0;
    const i16 left = x + frame->left;
    const i16 top = y + frame->top + (band * BAND_LINES);
    i16 row;

    for (row = 0; row < lines; row++) {
        u8 *pixel = target->address(left, top + row);

        for (u16 column = 0; column < frame->width; column++, pixel++) {
            if (repeats > 0) {
                repeats--;
            } else if (*data < BAND_SHORT) {
                repeats = *data + 3;
                data++;
                value = *data;
                data++;
            } else if (*data == BAND_REPEAT) {
                data++;
                value = *data;
                data++;
                repeats = *data + 3;
                data++;
            } else {
                value = *data;
                data++;
            }
            if (value != TRANSPARENT) {
                *pixel = value;
            }
        }
    }
}

void drawBands(Viewport *target, BandFrame *frame, i16 x, i16 y, i16 band, i16 bands)
{
    u8 *data = reinterpret_cast<u8 *>(frame) + frame->offsets[band] + FRAME_HEAD;
    i16 lines = bands == -1 ? frame->height : bands * BAND_LINES;

    if (static_cast<ui16>(lines + (band * BAND_LINES)) > frame->height) {
        lines = frame->height - band * BAND_LINES;
    }
    const i16 left = x + frame->left;
    const i16 top = y + frame->top + (band * BAND_LINES);

    drawRleFrame(data, left, top, lines, target, frame->width);
}

void drawBandsBanked(Viewport *target, BandFrame *frame, i16 x, i16 y, i16 band, i16 bands)
{
    u8 *data = reinterpret_cast<u8 *>(frame) + frame->offsets[band] + FRAME_HEAD;
    i16 lines = bands == -1 ? frame->height : bands * BAND_LINES;

    if (static_cast<ui16>(lines + (band * BAND_LINES)) > frame->height) {
        lines = frame->height - band * BAND_LINES;
    }
    const i16 left = x + frame->left;
    const i16 top = y + frame->top + (band * BAND_LINES);
    const u16 bankStep = superVga.bankStep;

    drawRleFrameBanked(data, left, top, lines, target, frame->width, bankStep);
}
