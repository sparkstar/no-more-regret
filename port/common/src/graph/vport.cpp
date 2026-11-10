/* GRAPH\VPORT.C (1288:0000-0f26): viewports, lines, rectangles, copies between
 * viewports and the screen, and isometric rectangles. See viewport.h. */
#include "gamestate.h"
#include <cstdlib>
#include "common.h"
#include "supervga.h"
#include "system.h"
#include "viewport.h"
#include "x32code.h"

#define SWAP(a, b) ((a) ^= (b), (b) ^= (a), (a) ^= (b))

Viewport *&screenViewport = gameGlobal<Viewport *>(nullptr);
Viewport *&drawViewport = gameGlobal<Viewport *>(nullptr);
/* 1480:4f18 has no references. */
Viewport *&unusedViewport = gameGlobal<Viewport *>(nullptr);
Viewport *&svgaScreen = gameGlobal<Viewport *>(nullptr);
Viewport *&vgaScreen = gameGlobal<Viewport *>(nullptr);

void Viewport::buildRows()
{
    const i16 width = right() - left() + 1;
    i16 height = bottom() - top() + 1;

    ownedRows_ = std::make_unique_for_overwrite<u32[]>(height);
    rows_ = ownedRows_.get();
    if (rows_ == nullptr) {
        allocationFailure();
    }
    u32 *row = rows_;
    u32 offset = 0;
    while ((height--) != 0) {
        *row++ = offset;
        offset += width;
    }
}

void Viewport::initialize(u16 flags, s16 left, s16 top, s16 right, s16 bottom, void *buffer)
{
    s32 width;
    s32 height;

    left_ = left;
    top_ = top;
    right_ = right;
    bottom_ = bottom;
    width = static_cast<i16>(right - left + 1);
    height = static_cast<i16>(bottom - top + 1);
    flags_ = flags;
    if ((flags & VIEWPORT_SCREEN) != 0) {
        screenViewport = this;
        memory_ = videoMemory;
    } else {
        const s32 size = (width * height) + 16;
        void *memory = nullptr;

        if (buffer == nullptr) {
            memory = new u8[size];
        } else {
            memory = buffer;
            flags_ |= VIEWPORT_BORROWED;
        }
        /* The original keeps the selector and drops the offset, which is 0
         * for the heap's blocks and the buffers the game passes. */
        memory_ = static_cast<u8 *>(memory);
    }
    buildRows();
}

void Viewport::release()
{
    if ((flags_ & VIEWPORT_BORROWED) != 0) {
        /* A copy's rows are another's; the rows a viewport built on a
         * borrowed buffer are its own, which the original lost. */
        memory_ = nullptr;
        rows_ = nullptr;
        ownedRows_.reset();
    } else {
        if (screenViewport == this) {
            screenViewport = nullptr;
        }
        if ((memory_ != nullptr) && ((flags_ & VIEWPORT_SCREEN) == 0)) {
            delete[] memory_;
            memory_ = nullptr;
        }
        rows_ = nullptr;
        ownedRows_.reset();
    }
}

Viewport::~Viewport()
{
    release();
}

Viewport &Viewport::operator=(const Viewport &other)
{
    /* The original copies the object whole; the copy shares the pixels and
     * the rows and owns neither (a borrowed viewport). */
    release();
    memory_ = other.memory_;
    rows_ = other.rows_;
    left_ = other.left_;
    top_ = other.top_;
    right_ = other.right_;
    bottom_ = other.bottom_;
    flags_ = other.flags_ | VIEWPORT_BORROWED;
    return *this;
}

u8 *Viewport::address(s16 x, s16 y)
{
    const s32 offset = (static_cast<s32>(y) * static_cast<i16>(rows_[1] - rows_[0])) + x;
    u8 *pixel;

    /* A viewport of 64K or more is reached through its linear address; the
     * others' offsets wrap at 16 bits. */
    if ((flags_ & VIEWPORT_LARGE) != 0) {
        pixel = memory_ + offset;
    } else {
        pixel = memory_ + static_cast<u16>(offset);
    }
    return pixel;
}

void Viewport::line(s16 x1, s16 y1, s16 x2, s16 y2, u8 color)
{
    i16 error;
    i16 dx;
    i16 dy;
    i16 diagonal;
    i16 straight;
    i16 stepX;
    i16 stepY;
    i16 x;
    i16 y;

    if (y2 == y1) {
        if (x1 > x2) {
            SWAP(x1, x2);
        }
        hline(x1, y1, x2, color);
        return;
    }
    if (x2 == x1) {
        if (y1 > y2) {
            SWAP(y1, y2);
        }
        vline(x1, y1, y2, color);
        return;
    }
    if (static_cast<i16>(abs(static_cast<i16>(x2 - x1))) < static_cast<i16>(abs(static_cast<i16>(y2 - y1)))) {
        if (y1 > y2) {
            SWAP(y1, y2);
            SWAP(x1, x2);
        }
        stepX = x2 > x1 ? 1 : -1;
        dy = y2 - y1;
        dx = abs(static_cast<i16>(x2 - x1));
        error = (dx - dy) * 2;
        diagonal = (dx - dy) * 2;
        straight = dx * 2;
        x = x1;
        y = y1;
        pixel(x, y, color);
        for (y = y1 + 1; y <= y2; y++) {
            if (error >= 0) {
                x += stepX;
                error += diagonal;
            } else {
                error += straight;
            }
            pixel(x, y, color);
        }
    } else {
        if (x1 > x2) {
            SWAP(y1, y2);
            SWAP(x1, x2);
        }
        stepY = y2 > y1 ? 1 : -1;
        dx = x2 - x1;
        dy = abs(static_cast<i16>(y2 - y1));
        error = dy - dx;
        diagonal = (dy - dx) * 2;
        straight = dy * 2;
        x = x1;
        y = y1;
        pixel(x, y, color);
        for (x = x1 + 1; x <= x2; x++) {
            if (error >= 0) {
                y += stepY;
                error += diagonal;
            } else {
                error += straight;
            }
            pixel(x, y, color);
        }
    }
}

void Viewport::clear(u8 color, s16 x1, s16 y1, s16 x2, s16 y2)
{
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0) {
        x1 = left_;
        y1 = top_;
        x2 = right_;
        y2 = bottom_;
    }
    if (((flags_ & VIEWPORT_SCREEN) == 0) || ((flags_ & VIEWPORT_VGA) != 0)) {
        fillRect(this, x1, y1, x2, y2, color);
    } else if (color == 0) {
        currentSystem->setMode(VESA_640X480);
    } else {
        haltMessage("Not implemented yet\n\r");
    }
}

void Viewport::rectangle(u8 color, s16 x1, s16 y1, s16 x2, s16 y2)
{
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0) {
        x1 = left_;
        y1 = top_;
        x2 = right_;
        y2 = bottom_;
    }
    if ((flags_ & VIEWPORT_SCREEN) == 0) {
        hline(x1, y1, x2, color);
        hline(x1, y2, x2, color);
        vline(x1, y1, y2, color);
        vline(x2, y1, y2, color);
    } else {
        halt();
    }
}

void Viewport::blit(Viewport *source, s16 x1, s16 y1, s16 x2, s16 y2, s16 x, s16 y)
{
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0) {
        x1 = source->left_;
        x2 = source->right_;
        y1 = source->top_;
        y2 = source->bottom_;
    }
    if (x1 == left_ && x2 == right_ && y1 == top_ && y2 == bottom_) {
        if ((flags_ & VIEWPORT_SCREEN) != 0) {
            if ((flags_ & VIEWPORT_VGA) != 0) {
                showScreen(source->memory_);
            } else {
                showSvgaScreen(source->memory_, superVga.bankStep);
            }
        } else if ((source->flags_ & VIEWPORT_SCREEN) != 0) {
            copyFromScreen(this, x1, y1, x2, y2, x, y, superVga.bankStep);
        } else {
            copyViewport(source, this, x1, y1, x2, y2, x, y);
        }
    } else if ((flags_ & VIEWPORT_SCREEN) != 0) {
        if ((flags_ & VIEWPORT_LARGE) != 0) {
            copyToSvgaScreen(source, x1, y1, x2, y2, x, y, superVga.bankStep);
        } else {
            copyToScreen(source, x1, y1, x2, y2, x, y);
        }
    } else if ((source->flags_ & VIEWPORT_SCREEN) != 0) {
        copyFromScreen(this, x1, y1, x2, y2, x, y, superVga.bankStep);
    } else {
        copyViewport(source, this, x1, y1, x2, y2, x, y);
    }
}

/* UNCALLED: nothing calls it. */
void Viewport::blitMasked(Viewport *source, s16 x1, s16 y1, s16 x2, s16 y2, s16 x, s16 y)
{
    if (source == nullptr) {
        return;
    }
    if (x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0) {
        x1 = source->left_;
        x2 = source->right_;
        y1 = source->top_;
        y2 = source->bottom_;
    }
    u8 *from = nullptr;
    u8 *to = nullptr;
    for (i16 row = 0; row < static_cast<i16>(y2 - y1); row++) {
        from = source->address(x1, y1 + row);
        to = address(x, y + row);
        for (i16 column = x1; column < x2; column++, from++, to++) {
            if (*from != TRANSPARENT) {
                *to = *from;
            }
        }
    }
}

/* UNCALLED: nothing calls it. */
bool Viewport::isoCorners(Point from, Point to, Point *top, Point *right, Point *bottom, Point *left)
{
    i16 u;
    i16 v;
    const i16 dx = to.x - from.x;
    const i16 dy = to.y - from.y;
    const i16 rise = dy * 16;
    const i16 run = dx;
    i16 slope = 0;

    if (rise < 48 && rise > -48 && run < 6 && run > -6) {
        return false;
    }
    if (run != 0) {
        slope = rise / run;
    }
    if (slope > 8 || slope < -8 || (run == 0)) {
        if ((dx & 1) != 0) {
            to.x--;
        }
        if (from.y < to.y) {
            *top = from;
            *bottom = to;
        } else {
            *top = to;
            *bottom = from;
        }
        u = bottom->y - (bottom->x >> 1);
        v = top->y + (top->x >> 1);
        left->x = v - u;
        left->y = -(left->x >> 1) + v;
        right->x = top->x + (bottom->x - left->x);
        right->y = top->y + (bottom->y - left->y);
    } else {
        /* (!dx) & 1 in the original, where the first case tests dx & 1. */
        if ((static_cast<int>(dx == 0) & 1) != 0) {
            to.x--;
        }
        if (from.x < to.x) {
            *left = from;
            *right = to;
        } else {
            *left = to;
            *right = from;
        }
        u = right->y - (right->x >> 1);
        v = left->y + (left->x >> 1);
        top->x = v - u;
        top->y = -(top->x >> 1) + v;
        bottom->x = left->x + (right->x - top->x);
        bottom->y = left->y + (right->y - top->y);
    }
    return true;
}

/* UNCALLED: nothing calls it. */
void Viewport::isoRectangle(s16 x1, s16 y1, s16 x2, s16 y2, u8 color)
{
    Point top;
    Point right;
    Point bottom;
    Point left;

    if (isoCorners(Point(x1, y1), Point(x2, y2), &top, &right, &bottom, &left)) {
        line(top.x, top.y, left.x, left.y, color);
        line(left.x, left.y, bottom.x, bottom.y, color);
        line(bottom.x, bottom.y, right.x, right.y, color);
        line(right.x, right.y, top.x, top.y, color);
    }
}

/* UNCALLED: nothing calls it. */
void Viewport::fillIsoRectangle(s16 x1, s16 y1, s16 x2, s16 y2, u8 color)
{
    Point top;
    Point right;
    Point bottom;
    Point left;

    if (isoCorners(Point(x1, y1), Point(x2, y2), &top, &right, &bottom, &left)) {
        i16 start;
        i16 end;
        i16 y;
        i16 startStep;
        i16 endStep;

        for (y = top.y, start = top.x, end = top.x + 1, startStep = -2, endStep = 2; y <= bottom.y;
             y++, start += startStep, end += endStep) {
            line(start, y, end, y, color);
            if (y == left.y) {
                startStep = 2;
            }
            if (y == right.y) {
                endStep = -2;
            }
        }
    }
}

/* UNCALLED: nothing calls it. */
void Viewport::setBank(s16 bank)
{
    setWindow(bank * superVga.bankStep);
}
