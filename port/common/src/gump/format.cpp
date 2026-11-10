/* FORMAT.C (13b0:0000-0418, No Remorse only). See format.h.
 *
 * formatText is a state machine over the text; the states: */
#include "format.h"
#include "cafont.h"
#include "system.h"

constexpr int FORMAT_DONE = 0;
constexpr int FORMAT_COPY = 1;        /* copy a character, then look at it */
constexpr int FORMAT_SPACE = 2;       /* a word ended at a space */
constexpr int FORMAT_PUNCTUATION = 3; /* ... at a punctuation mark */
constexpr int FORMAT_BREAK = 4;       /* break the line */
constexpr int FORMAT_RECT_END = 5;    /* end the rectangle */
constexpr int FORMAT_NEXT_RECT = 6;   /* start the next rectangle */
constexpr int FORMAT_LINE = 7;        /* start a line */
constexpr int FORMAT_FORCE_BREAK = 8; /* '~' */
constexpr int FORMAT_FORCE_RECT = 9;  /* '*' */
constexpr int FORMAT_QUOTE = 10;      /* '"' */
constexpr int FORMAT_END = 11;        /* the text ended */

/* UNCALLED: nothing calls it. */
char *formatText(char *text, i16 fontIndex, Rect *rects, i16 count)
{
    char *src;
    char *dst;
    /* The first states, FORMAT_NEXT_RECT and FORMAT_LINE, set these and
     * glyphHeight before any reads them. */
    char *spaceSrc = nullptr;
    char *spaceDst = nullptr;
    char *markSrc = nullptr;
    char *markDst = nullptr;
    char *quote = nullptr;
    char *end;
    u16 width = 0;
    u16 height = 0;
    u16 x = 0;
    u16 y = 0;
    s16 glyphWidth;
    s16 glyphHeight = 0;
    CAFont font(fontIndex);
    i16 rect;
    i16 state;

    src = text;
    dst = formatBuffer;
    end = formatBuffer + 0x3ff;
    rect = count - 1;
    state = FORMAT_NEXT_RECT;
    while (state != 0) {
        if (dst >= end) {
            halt();
        }
        switch (state) {
        case FORMAT_COPY:
            *dst = *src;
            src++;
            dst++;
            font.glyphSize(*src, &glyphWidth, &glyphHeight);
            x += glyphWidth;
            switch (src[-1]) {
            case 0:
                dst--;
                state = FORMAT_END;
                break;
            case '%':
                dst[-1] = ' ';
                *dst = ' ';
                dst++;
                *dst = ' ';
                dst++;
                *dst = ' ';
                dst++;
                x += glyphWidth * 3;
                [[fallthrough]];
            case ' ':
                state = FORMAT_SPACE;
                break;
            case ',':
            case '!':
            case ';':
            case '.':
            case ':':
            case '?':
                state = FORMAT_PUNCTUATION;
                break;
            case '"':
                state = FORMAT_QUOTE;
                break;
            case '~':
                dst--;
                state = FORMAT_FORCE_BREAK;
                break;
            case '*':
                dst--;
                state = FORMAT_FORCE_RECT;
                break;
            default:
                if (x >= width) {
                    state = FORMAT_BREAK;
                } else {
                    state = FORMAT_COPY;
                }
            }
            break;
        case FORMAT_SPACE:
            spaceSrc = src;
            spaceDst = dst;
            state = FORMAT_COPY;
            break;
        case FORMAT_PUNCTUATION:
            markSrc = src;
            markDst = dst;
            state = FORMAT_COPY;
            break;
        case FORMAT_QUOTE:
            if (quote != nullptr) {
                quote = nullptr;
                state = FORMAT_PUNCTUATION;
            } else {
                quote = src;
                state = FORMAT_COPY;
            }
            break;
        case FORMAT_FORCE_BREAK:
            spaceSrc = spaceDst = nullptr;
            state = FORMAT_BREAK;
            break;
        case FORMAT_BREAK:
            if (spaceSrc != nullptr) {
                src = spaceSrc;
                dst = spaceDst;
            }
            *dst = '\n';
            dst++;
            y += glyphHeight;
            if (y >= height) {
                state = FORMAT_RECT_END;
            } else {
                state = FORMAT_LINE;
            }
            break;
        case FORMAT_LINE:
            x = 0;
            spaceSrc = spaceDst = nullptr;
            markSrc = markDst = nullptr;
            quote = nullptr;
            while (*src == ' ' || *src == '\t') {
                src++;
            }
            state = FORMAT_COPY;
            break;
        case FORMAT_FORCE_RECT:
            markSrc = markDst = spaceSrc = spaceDst = nullptr;
            state = FORMAT_RECT_END;
            break;
        case FORMAT_RECT_END:
            if (markSrc != nullptr) {
                src = markSrc;
                dst = markDst;
            } else if (spaceSrc != nullptr) {
                src = spaceSrc;
                dst = spaceDst;
            }
            while (dst[-1] == static_cast<char>(0xff)) {
                dst--;
            }
            *dst = static_cast<char>(0xff);
            dst++;
            state = FORMAT_NEXT_RECT;
            break;
        case FORMAT_NEXT_RECT:
            rect++;
            if (rect == count) {
                rect = 0;
            }
            width = rects[rect].right - rects[rect].left + 1;
            height = rects[rect].bottom - rects[rect].top;
            font.glyphSize(*src, &glyphWidth, &glyphHeight);
            height -= height % static_cast<ui16>(glyphHeight);
            x = 0;
            y = 0;
            state = FORMAT_LINE;
            break;
        case FORMAT_END:
            while (dst[-1] == static_cast<char>(0xff) || dst[-1] == ' ' || dst[-1] == '\n') {
                dst--;
            }
            *dst = 0;
            /* A short last line joins the rectangle before it. */
            if ((y == 0U) && width / 2 >= x) {
                char *p;

                for (p = dst; p != formatBuffer; p--) {
                    if (*p == '\n') {
                        *p = static_cast<char>(0xff);
                        break;
                    }
                    if (*p == static_cast<char>(0xff)) {
                        *p = '\n';
                    }
                }
            }
            state = FORMAT_DONE;
            break;
        default:
            halt();
        }
    }
    return formatBuffer;
}
