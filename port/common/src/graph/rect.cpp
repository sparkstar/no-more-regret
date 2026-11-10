/* 1278:0000-0445, file name unknown: Rect tests and clipping. See rect.h. */
#include "common.h"
#include "rect.h"

bool Rect::contains(const Rect &other) const
{
    return other.left >= left && right >= other.left && other.top >= top && other.top <= bottom &&
           other.right >= left && other.right <= right && other.bottom >= top && other.bottom <= bottom;
}

bool Rect::contains(Point point) const
{
    return left <= point.x && right >= point.x && top <= point.y && bottom >= point.y;
}

bool Rect::contains(s16 x, s16 y) const
{
    return left <= x && right >= x && top <= y && bottom >= y;
}

void Rect::intersect(const Rect &other)
{
    if (other.left > left) {
        left = other.left;
    }
    if (other.right < right) {
        right = other.right;
    }
    if (other.top > top) {
        top = other.top;
    }
    if (other.bottom < bottom) {
        bottom = other.bottom;
    }
    if (static_cast<i16>(right - left) < 0 || static_cast<i16>(bottom - top) < 0) {
        left = top = right = bottom = 0;
    }
}

bool Rect::intersects(const Rect &other) const
{
    Rect overlap;

    overlap.left = left > other.left ? left : other.left;
    overlap.top = top > other.top ? top : other.top;
    overlap.right = right < other.right ? right : other.right;
    overlap.bottom = bottom < other.bottom ? bottom : other.bottom;
    return RECT_WIDTH(overlap) > 0 && RECT_HEIGHT(overlap) > 0;
}

void Rect::extend(const Rect &other)
{
    if (other.left < left) {
        left = other.left;
    }
    if (other.top < top) {
        top = other.top;
    }
    if (other.right > right) {
        right = other.right;
    }
    if (other.bottom > bottom) {
        bottom = other.bottom;
    }
}

bool Rect::overlaps(const Rect &other) const
{
    s16 l;
    s16 t;
    s16 r;
    s16 b;

    if (right < other.left || other.top > bottom || other.right < left || other.bottom < top) {
        return false;
    }
    l = left > other.left ? left : other.left;
    t = top > other.top ? top : other.top;
    r = right < other.right ? right : other.right;
    b = bottom < other.bottom ? bottom : other.bottom;
    if (static_cast<i16>(r - l + 1) > 0 && static_cast<i16>(b - t + 1) > 0) {
        return true;
    }
    if (other.left >= left && other.top >= top && other.right <= right && other.bottom <= bottom) {
        return true;
    }
    if (left >= other.left && top >= other.top && right <= other.right && bottom <= other.bottom) {
        return true;
    }
    return false;
}
