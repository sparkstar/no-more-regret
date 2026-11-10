/* The port's layout of gumps. See layout.h. */
#include "layout.h"

#include <algorithm>

namespace {

[[nodiscard]] int lengthOf(const Extent &extent, Axis axis)
{
    return axis == Axis::HORIZONTAL ? extent.width : extent.height;
}

[[nodiscard]] int breadthOf(const Extent &extent, Axis axis)
{
    return axis == Axis::HORIZONTAL ? extent.height : extent.width;
}

/* Where something length long starts in room long, lying by align. */
[[nodiscard]] int startIn(int room, int length, Align align)
{
    const int spare = std::max(room - length, 0);
    switch (align) {
    case Align::CENTER:
        return spare / 2;
    case Align::END:
        return spare;
    case Align::START:
    case Align::FILL:
        break;
    }
    return 0;
}

}

Extent lineExtent(std::span<const LineItem> items, Axis axis, s16 spacing)
{
    int length = 0;
    int breadth = 0;
    for (const LineItem &item : items) {
        length += lengthOf(item.extent, axis);
        breadth = std::max(breadth, breadthOf(item.extent, axis));
    }
    if (!items.empty()) {
        length += spacing * static_cast<int>(items.size() - 1);
    }
    if (axis == Axis::HORIZONTAL) {
        return Extent{static_cast<s16>(length), static_cast<s16>(breadth)};
    }
    return Extent{static_cast<s16>(breadth), static_cast<s16>(length)};
}

std::vector<Rect> placeLine(std::span<const LineItem> items, const Rect &area, Axis axis, s16 spacing, Align along)
{
    const bool horizontal = axis == Axis::HORIZONTAL;
    const int room = horizontal ? RECT_WIDTH(area) : RECT_HEIGHT(area);
    const int roomAcross = horizontal ? RECT_HEIGHT(area) : RECT_WIDTH(area);
    const int wanted = lengthOf(lineExtent(items, axis, spacing), axis);
    const int spare = std::max(room - wanted, 0);
    const auto stretches =
        static_cast<int>(std::ranges::count_if(items, [](const LineItem &item) { return item.stretch; }));

    int position = stretches > 0 ? 0 : startIn(room, wanted, along);
    int stretchesSeen = 0;
    std::vector<Rect> out;
    for (const LineItem &item : items) {
        int length = lengthOf(item.extent, axis);
        if (item.stretch) {
            length += spare / stretches;
            if (stretchesSeen < spare % stretches) {
                length++;
            }
            stretchesSeen++;
        }
        const int breadth = item.across == Align::FILL ? roomAcross : breadthOf(item.extent, axis);
        const int offset = startIn(roomAcross, breadth, item.across);
        if (horizontal) {
            const int left = area.left + position;
            const int top = area.top + offset;
            out.emplace_back(static_cast<s16>(left), static_cast<s16>(top), static_cast<s16>(left + length - 1),
                             static_cast<s16>(top + breadth - 1));
        } else {
            const int left = area.left + offset;
            const int top = area.top + position;
            out.emplace_back(static_cast<s16>(left), static_cast<s16>(top), static_cast<s16>(left + breadth - 1),
                             static_cast<s16>(top + length - 1));
        }
        position += length + spacing;
    }
    return out;
}

i16 itemAt(std::span<const Rect> rects, s16 x, s16 y)
{
    for (std::size_t i = 0; i < rects.size(); i++) {
        const Rect &rect = rects[i];
        if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom) {
            return static_cast<i16>(i);
        }
    }
    return -1;
}

s16 stepOffset(i16 step, i16 steps, s16 travel)
{
    if (steps < 2) {
        return 0;
    }
    return static_cast<s16>(step * travel / (steps - 1));
}

i16 stepAtOffset(s16 offset, i16 steps, s16 travel)
{
    if (steps < 2 || travel <= 0) {
        return 0;
    }
    const int nearest = ((offset * (steps - 1)) + (travel / 2)) / travel;
    return static_cast<i16>(std::clamp(nearest, 0, steps - 1));
}

std::vector<LineItem> LayoutBox::items()
{
    std::vector<LineItem> out;
    out.reserve(nodes_.size());
    for (const std::unique_ptr<LayoutNode> &node : nodes_) {
        out.push_back(LineItem{node->extent(), node->stretch, node->across});
    }
    return out;
}

Extent LayoutBox::extent()
{
    return lineExtent(items(), axis_, spacing_);
}

void LayoutBox::place(const Rect &area)
{
    const std::vector<Rect> places = placeLine(items(), area, axis_, spacing_, along_);
    for (std::size_t i = 0; i < nodes_.size(); i++) {
        nodes_[i]->place(places[i]);
    }
}
