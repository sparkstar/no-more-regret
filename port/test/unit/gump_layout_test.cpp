/* The port's layout of gumps (port/common/src/gump/layout.cpp): a line's
 * size, the places of its items along and across, the stretches' shares,
 * the items past a short area, the hit of a point, a slider's track. */
#include "layout.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH gump_layout: %s\n", what.c_str());
    }
}

std::string text(const Rect &rect)
{
    return "(" + std::to_string(rect.left) + "," + std::to_string(rect.top) + ")-(" + std::to_string(rect.right) + "," +
           std::to_string(rect.bottom) + ")";
}

bool same(const Rect &a, const Rect &b)
{
    return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
}

LineItem item(s16 width, s16 height, Align across = Align::START, bool stretch = false)
{
    return LineItem{Extent{width, height}, stretch, across};
}

void lineExtents()
{
    struct Case {
        const char *name;
        std::vector<LineItem> items;
        Axis axis;
        s16 spacing;
        Extent expected;
    };
    const Case cases[] = {
        {"nothing", {}, Axis::HORIZONTAL, 5, Extent{0, 0}},
        {"one item", {item(10, 4)}, Axis::HORIZONTAL, 5, Extent{10, 4}},
        {"a row: lengths and spacing, the tallest",
         {item(10, 4), item(20, 9), item(5, 2)},
         Axis::HORIZONTAL,
         3,
         Extent{41, 9}},
        {"a column: heights and spacing, the widest",
         {item(10, 4), item(20, 9), item(5, 2)},
         Axis::VERTICAL,
         3,
         Extent{20, 21}},
        {"a stretch adds no length", {item(10, 4), item(0, 0, Align::START, true)}, Axis::HORIZONTAL, 2, Extent{12, 4}},
    };
    for (const Case &c : cases) {
        const Extent got = lineExtent(c.items, c.axis, c.spacing);
        check(got.width == c.expected.width && got.height == c.expected.height, std::string("lineExtent, ") + c.name +
                                                                                    ": " + std::to_string(got.width) +
                                                                                    "x" + std::to_string(got.height));
    }
}

void placeLines()
{
    struct Case {
        const char *name;
        std::vector<LineItem> items;
        Rect area;
        Axis axis;
        s16 spacing;
        Align along;
        std::vector<Rect> expected;
    };
    const Case cases[] = {
        {"a row from the start",
         {item(10, 4), item(20, 6)},
         Rect(100, 50, 199, 59),
         Axis::HORIZONTAL,
         2,
         Align::START,
         {Rect(100, 50, 109, 53), Rect(112, 50, 131, 55)}},
        {"a row in the middle: 68 spare, 34 before",
         {item(10, 4), item(20, 6)},
         Rect(100, 50, 199, 59),
         Axis::HORIZONTAL,
         2,
         Align::CENTER,
         {Rect(134, 50, 143, 53), Rect(146, 50, 165, 55)}},
        {"a row at the end",
         {item(10, 4), item(20, 6)},
         Rect(100, 50, 199, 59),
         Axis::HORIZONTAL,
         2,
         Align::END,
         {Rect(168, 50, 177, 53), Rect(180, 50, 199, 55)}},
        {"across: start, centre, end, fill",
         {item(4, 2, Align::START), item(4, 2, Align::CENTER), item(4, 2, Align::END), item(4, 2, Align::FILL)},
         Rect(0, 0, 99, 9),
         Axis::HORIZONTAL,
         0,
         Align::START,
         {Rect(0, 0, 3, 1), Rect(4, 4, 7, 5), Rect(8, 8, 11, 9), Rect(12, 0, 15, 9)}},
        {"a column with a stretch: the foot at the end",
         {item(30, 10), item(0, 0, Align::START, true), item(8, 5)},
         Rect(0, 0, 39, 99),
         Axis::VERTICAL,
         4,
         Align::START,
         {Rect(0, 0, 29, 9), Rect(0, 14, -1, 90), Rect(0, 95, 7, 99)}},
        {"two stretches share 5: 3 and 2",
         {item(0, 0, Align::START, true), item(10, 1), item(0, 0, Align::START, true)},
         Rect(0, 0, 14, 0),
         Axis::HORIZONTAL,
         0,
         Align::CENTER,
         {Rect(0, 0, 2, -1), Rect(3, 0, 12, 0), Rect(13, 0, 14, -1)}},
        {"items longer than the area run past its end",
         {item(30, 1), item(30, 1)},
         Rect(0, 0, 39, 0),
         Axis::HORIZONTAL,
         0,
         Align::CENTER,
         {Rect(0, 0, 29, 0), Rect(30, 0, 59, 0)}},
    };
    for (const Case &c : cases) {
        const std::vector<Rect> got = placeLine(c.items, c.area, c.axis, c.spacing, c.along);
        check(got.size() == c.expected.size(), std::string("placeLine, ") + c.name + ": the number of places");
        for (std::size_t i = 0; i < got.size() && i < c.expected.size(); i++) {
            check(same(got[i], c.expected[i]), std::string("placeLine, ") + c.name + ", item " + std::to_string(i) +
                                                   ": " + text(got[i]) + ", not " + text(c.expected[i]));
        }
    }
}

void hits()
{
    const std::vector<Rect> rects = {Rect(0, 0, 9, 9), Rect(20, 0, 29, 9)};
    struct Case {
        const char *name;
        s16 x, y;
        i16 expected;
    };
    const Case cases[] = {
        {"inside the first", 5, 5, 0},        {"the first's corner", 9, 9, 0}, {"between them", 15, 5, -1},
        {"the second's left edge", 20, 0, 1}, {"below them", 25, 10, -1},
    };
    for (const Case &c : cases) {
        check(itemAt(rects, c.x, c.y) == c.expected, std::string("itemAt, ") + c.name);
    }
}

void tracks()
{
    struct Case {
        const char *name;
        i16 step, steps;
        s16 travel;
        s16 offset;
    };
    const Case offsets[] = {
        {"the first step", 0, 23, 132, 0}, {"the last step", 22, 23, 132, 132}, {"the middle", 11, 23, 132, 66},
        {"a mouse speed", 2, 4, 140, 93},  {"one step only", 0, 1, 140, 0},
    };
    for (const Case &c : offsets) {
        check(stepOffset(c.step, c.steps, c.travel) == c.offset, std::string("stepOffset, ") + c.name);
    }
    struct Back {
        const char *name;
        s16 offset;
        i16 steps;
        s16 travel;
        i16 step;
    };
    const Back steps[] = {
        {"the first place", 0, 23, 132, 0},         {"the last place", 132, 23, 132, 22},
        {"nearer the lower", 8, 23, 132, 1},        {"half way rounds up", 3, 23, 132, 1},
        {"before the track", -40, 23, 132, 0},      {"past the track", 400, 23, 132, 22},
        {"a mouse speed's nearest", 60, 4, 140, 1}, {"no travel", 10, 4, 0, 0},
    };
    for (const Back &c : steps) {
        check(stepAtOffset(c.offset, c.steps, c.travel) == c.step, std::string("stepAtOffset, ") + c.name);
    }
}

}

int main()
{
    lineExtents();
    placeLines();
    hits();
    tracks();
    std::printf("gump_layout: %d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
