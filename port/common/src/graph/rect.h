/* Rectangles and points: 1278:0000-0445 (no file name), the module before
 * GRAPH\SUPERVGA.C. The GUI and the drawing code share them. */
#ifndef PORT_RECT_H
#define PORT_RECT_H
#include "common.h"
struct Point {
    i16 x, y;
    Point() = default;
    constexpr Point(i16 px, i16 py) : x(px), y(py) {}
};
struct Rect {
    s16 left, top, right, bottom;
    /* 1 when other lies wholly inside (1278:0000). */
    [[nodiscard]] bool contains(const Rect &other) const;
    /* 1 when the point lies inside (1278:008a, 00c7). */
    [[nodiscard]] bool contains(Point point) const;
    [[nodiscard]] bool contains(s16 x, s16 y) const;
    /* Clips this rect to other; an empty result becomes all zero (1278:0104). */
    void intersect(const Rect &other);
    /* 1 when the two share a pixel (1278:01b9). */
    [[nodiscard]] bool intersects(const Rect &other) const;
    /* Grows this rect to cover other too (1278:0262). */
    void extend(const Rect &other);
    /* 1 when the two share a pixel or one holds the other (1278:02db). */
    [[nodiscard]] bool overlaps(const Rect &other) const;
    Rect() = default;
    constexpr Rect(s16 l, s16 t, s16 r, s16 b) : left(l), top(t), right(r), bottom(b) {}
    /* The rectangle of one point. */
    Rect(s16 x, s16 y)
    {
        left = x;
        top = y;
        right = x;
        bottom = y;
    }
    explicit Rect(Rect *o)
    {
        left = o->left;
        top = o->top;
        right = o->right;
        bottom = o->bottom;
    }
    void set(s16 l, s16 t, s16 r, s16 b)
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }
    void moveTo(s16 x, s16 y)
    {
        const s16 dx = x - left;
        const s16 dy = y - top;
        left += dx;
        top += dy;
        right += dx;
        bottom += dy;
    }
    void moveBy(s16 dx, s16 dy)
    {
        left += dx;
        top += dy;
        right += dx;
        bottom += dy;
    }
    void inset(s16 n)
    {
        left += n;
        top += n;
        right -= n;
        bottom -= n;
    }
    /* 1 when the two share a pixel (the inline intersects). */
    [[nodiscard]] bool touches(const Rect &other) const
    {
        return left <= other.right && top <= other.bottom && right >= other.left && bottom >= other.top;
    }
    /* 1 when other lies wholly inside this one, and the other way round (the
     * inline contains). */
    [[nodiscard]] bool holds(const Rect &other) const
    {
        return left <= other.left && top <= other.top && right >= other.right && bottom >= other.bottom;
    }
    [[nodiscard]] bool within(const Rect &other) const
    {
        return left >= other.left && top >= other.top && right <= other.right && bottom <= other.bottom;
    }
    [[nodiscard]] s16 width() const { return right - left + 1; }
    [[nodiscard]] s16 height() const { return bottom - top + 1; }
};
/* Signed extents of a Rect. Macros, not members: the +1 folds into the caller's
 * constant (right-left+9 for a width plus 8), as in the dialog layouts. */
#define RECT_WIDTH(r) ((i16)((r).right - (r).left + 1))
#define RECT_HEIGHT(r) ((i16)((r).bottom - (r).top + 1))
ASSERT_SIZE(Point, 0x4);
ASSERT_SIZE(Rect, 0x8);
#endif
