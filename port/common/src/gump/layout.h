/* The port's layout of gumps: sizes placed one after another in lines, lines
 * nested in boxes, in place of the originals' fixed coordinates. The placing
 * is pure (placeLine and the slider's track); a LayoutBox applies it to its
 * nodes. The gumps' nodes are widgets.h's. */
#ifndef PORT_LAYOUT_H
#define PORT_LAYOUT_H
#include "rect.h"

#include <memory>
#include <span>
#include <utility>
#include <vector>

/* A size in pixels. */
struct Extent {
    s16 width;
    s16 height;
};

enum class Axis : u8 { HORIZONTAL, VERTICAL };
/* Where an item lies across its line, or a line's items along it when they
 * take less than the line; FILL (across only) gives an item the line's
 * whole breadth. */
enum class Align : u8 { START, CENTER, END, FILL };

/* An item of a line: the size it wants, whether it takes a share of the
 * room the line has left over (a stretch), and where it lies across. */
struct LineItem {
    Extent extent;
    bool stretch;
    Align across;
};

/* The size a line of items wants: along the axis their lengths and spacing
 * between them, across the largest breadth. */
[[nodiscard]] Extent lineExtent(std::span<const LineItem> items, Axis axis, s16 spacing);

/* Where each item of a line lies in area: one after another along axis,
 * spacing apart. The stretches share the room left over (the first ones a
 * pixel more); without a stretch the items lie by along. Across, each lies
 * by its own across. Items that take more than the area run past its end. */
[[nodiscard]] std::vector<Rect> placeLine(std::span<const LineItem> items, const Rect &area, Axis axis, s16 spacing,
                                          Align along);

/* The first of rects that holds (x, y), or -1. */
[[nodiscard]] i16 itemAt(std::span<const Rect> rects, s16 x, s16 y);

/* A slider's track of steps over travel pixels: the offset of a step's
 * place from the first one's, and the step whose place is nearest to an
 * offset (the ends for offsets past them). */
[[nodiscard]] s16 stepOffset(i16 step, i16 steps, s16 travel);
[[nodiscard]] i16 stepAtOffset(s16 offset, i16 steps, s16 travel);

/* A node of a layout: the size it wants and the place it is given, in the
 * coordinates of the area the layout is placed in. */
class LayoutNode {
public:
    LayoutNode() = default;
    virtual ~LayoutNode() = default;
    LayoutNode(const LayoutNode &) = delete;
    LayoutNode &operator=(const LayoutNode &) = delete;
    LayoutNode(LayoutNode &&) = delete;
    LayoutNode &operator=(LayoutNode &&) = delete;
    [[nodiscard]] virtual Extent extent() = 0;
    virtual void place(const Rect &area) = 0;
    /* How it lies in the line of the box that holds it. */
    bool stretch = false;
    Align across = Align::START;
};

/* Nodes in a line, spacing apart. */
class LayoutBox : public LayoutNode {
public:
    LayoutBox(Axis lineAxis, s16 gap, Align itemsAlong) : axis_(lineAxis), spacing_(gap), along_(itemsAlong) {}
    /* Adds a node made of arguments; it lies across by acrossLine. */
    template <class Node, class... Arguments> Node &add(Align acrossLine, Arguments &&...arguments)
    {
        auto node = std::make_unique<Node>(std::forward<Arguments>(arguments)...);
        Node &added = *node;
        added.across = acrossLine;
        nodes_.push_back(std::move(node));
        return added;
    }
    [[nodiscard]] Extent extent() override;
    void place(const Rect &area) override;

private:
    [[nodiscard]] std::vector<LineItem> items();
    Axis axis_;
    s16 spacing_;
    Align along_;
    std::vector<std::unique_ptr<LayoutNode>> nodes_;
};

/* Room that takes what its line has left over. */
class Stretch : public LayoutNode {
public:
    Stretch() { stretch = true; }
    [[nodiscard]] Extent extent() override { return Extent{0, 0}; }
    void place(const Rect & /*area*/) override {}
};
#endif
