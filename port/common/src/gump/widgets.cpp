/* The port's widgets. See widgets.h. */
#include "widgets.h"
#include "widgets_game.h"
#include "cafont.h"
#include "drawframe.h"
#include "gamestate.h"
#include "gumpshap.h"
#include "hangul.h"
#include "utf8.h"
#include "viewport.h"

#include <algorithm>
#include <array>
#include <map>

namespace {

/* The options gump's slider (gumps.flx, the same in both games): its knob
 * and its arrows, whose codes their commands carry. */
const u16 KNOB_SHAPE = 0x30;
const u16 LOWER_ARROW_SHAPE = 0x31;
const u16 HIGHER_ARROW_SHAPE = 0x32;
const u8 LOWER_ARROW = 0;
const u8 HIGHER_ARROW = 1;
/* Pixels between an arrow and the track, the heading and the track, and
 * two choices. */
const s16 ARROW_GAP = 4;
const s16 HEADING_GAP = 3;
const s16 CHOICE_GAP = 14;
/* The track: its least width (the options gump's is about as wide), its
 * height (the knob's), the room between a bracket and the knob's ends, and
 * the ticks' pitch. */
const s16 TRACK_WIDTH = 150;
const s16 TRACK_HEIGHT = 11;
const s16 BRACKET_ROOM = 3;
const s16 TICK_PITCH = 3;

/* The glyph of a code point in a font of fonts.flx, whose frames are its
 * characters' (ASCII's codes); nullptr for one it does not have. */
ShapeFrame *glyphFrame(i16 font, char32_t point)
{
    if (point >= 0x80) {
        return nullptr;
    }
    ShapeHeader *shape = fontCache->loadEntry(static_cast<u16>(font));
    if (point >= static_cast<char32_t>(shape->frameCount) || shape->frames[point].size == 0) {
        return nullptr;
    }
    return fontCache->getFrame(static_cast<u16>(font), static_cast<u16>(point));
}

/* faceColour's colours, each font's once counted (in the arena,
 * platform/gamestate.h). */
std::map<i16, u8> &fontFaces = gameGlobal<std::map<i16, u8>>();

/* The colour most of a font's glyphs' pixels have (its 0 left out), which
 * text draws in an Ink's face. Counted once for each font. */
u8 faceColour(i16 font)
{
    const auto found = fontFaces.find(font);
    if (found != fontFaces.end()) {
        return found->second;
    }
    std::array<u32, 256> counts{};
    for (char32_t point = 0; point < 0x80; point++) {
        ShapeFrame *frame = glyphFrame(font, point);
        if (frame == nullptr || frame->width <= 0 || frame->height <= 0) {
            continue;
        }
        std::vector<u8> scratch(static_cast<std::size_t>(frame->width) * static_cast<std::size_t>(frame->height));
        std::vector<u32> rows(static_cast<std::size_t>(frame->height));
        for (std::size_t row = 0; row < rows.size(); row++) {
            rows[row] = static_cast<u32>(row * static_cast<std::size_t>(frame->width));
        }
        const ViewportPixels target{scratch.data(), rows.data(), 0, 0, frame->width - 1, frame->height - 1};
        drawframe::draw<false>(target, frame->xOffset, frame->yOffset, reinterpret_cast<const u8 *>(frame),
                               [&counts](u8 & /*out*/, u8 in) { counts[in]++; });
    }
    counts[0] = 0;
    const u8 face = static_cast<u8>(std::ranges::max_element(counts) - counts.begin());
    fontFaces[font] = face;
    return face;
}

/* Where a font's letters and the Hangul lie in a line of text: the
 * letters' origin below the line's top (the 'A''s), the Hangul's top (its
 * bottom on the 'A''s, as the subtitles have it), and the line's height. */
struct LineMetrics {
    s16 ascent;
    s16 hangulTop;
    s16 height;
};

LineMetrics lineMetrics(i16 font)
{
    const ShapeFrame *letter = glyphFrame(font, U'A');
    const auto letterHeight = static_cast<s16>(letter->height);
    const auto hangulTop = static_cast<s16>(std::max(letterHeight - HANGUL_SIZE, 0));
    return LineMetrics{static_cast<s16>(letter->yOffset), hangulTop,
                       static_cast<s16>(std::max<int>(letterHeight, hangulTop + HANGUL_SIZE))};
}

Ink choiceInk(bool chosen, bool warning)
{
    if (chosen) {
        return warning ? Ink{WIDGET_WARNING_FACE, WIDGET_WARNING_EDGE} : Ink{WIDGET_FACE, WIDGET_EDGE};
    }
    return warning ? Ink{WIDGET_DIM_WARNING_FACE, WIDGET_DIM_WARNING_EDGE} : Ink{WIDGET_DIM_FACE, WIDGET_DIM_EDGE};
}

}

Extent textExtent(i16 font, const std::string &text)
{
    CAFont letters(font);
    int width = 0;
    for (const char32_t point : codePoints(text)) {
        if (glyphFrame(font, point) != nullptr) {
            width += letters.glyphWidth(static_cast<u8>(point));
        } else if (hangulGlyph(point) != nullptr) {
            width += HANGUL_ADVANCE;
        }
    }
    return Extent{static_cast<s16>(width), lineMetrics(font).height};
}

void drawText(i16 font, const std::string &text, s16 x, s16 y, Ink ink)
{
    CAFont letters(font);
    const LineMetrics metrics = lineMetrics(font);
    const u8 face = faceColour(font);
    const ViewportPixels target = drawViewport->pixels();
    s16 pen = x;
    for (const char32_t point : codePoints(text)) {
        ShapeFrame *frame = glyphFrame(font, point);
        if (frame != nullptr) {
            drawframe::draw<false>(target, pen, y + metrics.ascent, reinterpret_cast<const u8 *>(frame),
                                   [face, ink](u8 &out, u8 in) {
                                       if (in != 0) {
                                           out = in == face ? ink.face : ink.edge;
                                       }
                                   });
            pen = static_cast<s16>(pen + letters.glyphWidth(static_cast<u8>(point)));
            continue;
        }
        const HangulGlyph *syllable = hangulGlyph(point);
        if (syllable != nullptr) {
            drawHangul(*syllable, pen, static_cast<s16>(y + metrics.hangulTop), ink.face);
            pen = static_cast<s16>(pen + HANGUL_ADVANCE);
        }
    }
}

std::vector<Rect> ChoiceRow::places() const
{
    std::vector<LineItem> items;
    items.reserve(choices_.size());
    for (const Choice &choice : choices_) {
        items.push_back(LineItem{textExtent(CHOICE_FONT, choice.text), false, Align::START});
    }
    const Extent whole = lineExtent(items, Axis::HORIZONTAL, CHOICE_GAP);
    return placeLine(items, Rect(0, 0, static_cast<s16>(whole.width - 1), static_cast<s16>(whole.height - 1)),
                     Axis::HORIZONTAL, CHOICE_GAP, Align::START);
}

Extent ChoiceRow::preferredExtent()
{
    std::vector<LineItem> items;
    items.reserve(choices_.size());
    for (const Choice &choice : choices_) {
        items.push_back(LineItem{textExtent(CHOICE_FONT, choice.text), false, Align::START});
    }
    return lineExtent(items, Axis::HORIZONTAL, CHOICE_GAP);
}

void ChoiceRow::paintSelf(i16 x, i16 y)
{
    const std::vector<Rect> rects = places();
    for (std::size_t i = 0; i < choices_.size(); i++) {
        const bool chosen = static_cast<i16>(i) == chosen_;
        drawText(CHOICE_FONT, choices_[i].text, static_cast<s16>(x + rects[i].left), static_cast<s16>(y + rects[i].top),
                 choiceInk(chosen, choices_[i].warning));
    }
}

void ChoiceRow::handleLeftButton(NewGumpEvent *event)
{
    if (!event->isMousePress()) {
        return;
    }
    const i16 index = itemAt(places(), eventX(event), eventY(event));
    if (index < 0) {
        return;
    }
    if (touched) {
        touched();
    }
    choose(index);
}

void ChoiceRow::step(i16 direction)
{
    const auto last = static_cast<i16>(choices_.size() - 1);
    choose(std::clamp(static_cast<i16>(chosen_ + direction), static_cast<i16>(0), last));
}

void ChoiceRow::choose(i16 index)
{
    if (index == chosen_) {
        return;
    }
    chosen_ = index;
    if (changed) {
        changed(index);
    }
}

void TextButton::paintSelf(i16 x, i16 y)
{
    drawText(CHOICE_FONT, text_, x, y, choiceInk(true, false));
}

void TextButton::handleLeftButton(NewGumpEvent *event)
{
    if (!event->isMousePress()) {
        return;
    }
    if (touched) {
        touched();
    }
    activate();
}

void TextButton::activate()
{
    if (pressed) {
        pressed();
    }
}

Slider::Slider(NewGump *owner, i16 stepCount)
    : Widget(owner, EVENT_LEFT_BUTTON | EVENT_COMMAND), steps_(stepCount),
      lower_(new ArrowButton(LOWER_ARROW, this, LOWER_ARROW_SHAPE)),
      higher_(new ArrowButton(HIGHER_ARROW, this, HIGHER_ARROW_SHAPE))
{
}

Extent Slider::preferredExtent()
{
    const Extent text = textExtent(HEADING_FONT, heading_);
    const int inner = std::max<int>(TRACK_WIDTH, text.width);
    const int width = lower_->NewGump::width() + ARROW_GAP + inner + ARROW_GAP + higher_->NewGump::width();
    const int height = std::max<int>(lower_->NewGump::height(), text.height + HEADING_GAP + TRACK_HEIGHT);
    return Extent{static_cast<s16>(width), static_cast<s16>(height)};
}

void Slider::place(const Rect &area)
{
    /* As wide as the place when it is wider (a column's sliders alike). */
    const Extent wanted = preferredExtent();
    const Extent size{std::max<s16>(wanted.width, RECT_WIDTH(area)), wanted.height};
    bounds.set(area.left, area.top, static_cast<s16>(area.left + size.width - 1),
               static_cast<s16>(area.top + size.height - 1));
    const auto arrowTop = static_cast<s16>((size.height - lower_->NewGump::height()) / 2);
    lower_->bounds.moveTo(0, arrowTop);
    higher_->bounds.moveTo(static_cast<s16>(size.width - higher_->NewGump::width()), arrowTop);
}

Rect Slider::track() const
{
    const Extent text = textExtent(HEADING_FONT, heading_);
    const int height = bounds.bottom - bounds.top + 1;
    const int top = ((height - (text.height + HEADING_GAP + TRACK_HEIGHT)) / 2) + text.height + HEADING_GAP;
    const int left = lower_->NewGump::width() + ARROW_GAP;
    const int right = (bounds.right - bounds.left) - higher_->NewGump::width() - ARROW_GAP;
    return Rect(static_cast<s16>(left), static_cast<s16>(top), static_cast<s16>(right),
                static_cast<s16>(top + TRACK_HEIGHT - 1));
}

s16 Slider::travel() const
{
    const Rect line = track();
    const Rect knob = gumpShapes->dimensions(KNOB_SHAPE, 0);
    return static_cast<s16>(RECT_WIDTH(line) - (2 * BRACKET_ROOM) - RECT_WIDTH(knob));
}

i16 Slider::stepAt(s16 x) const
{
    const Rect line = track();
    const Rect knob = gumpShapes->dimensions(KNOB_SHAPE, 0);
    const auto offset = static_cast<s16>(x - line.left - BRACKET_ROOM - (RECT_WIDTH(knob) / 2));
    return stepAtOffset(offset, steps_, travel());
}

void Slider::paintSelf(i16 x, i16 y)
{
    const Rect line = track();
    const Extent text = textExtent(HEADING_FONT, heading_);
    drawText(HEADING_FONT, heading_, static_cast<s16>(x + line.left),
             static_cast<s16>(y + line.top - HEADING_GAP - text.height), Ink{WIDGET_FACE, WIDGET_EDGE});
    const auto left = static_cast<s16>(x + line.left);
    const auto right = static_cast<s16>(x + line.right);
    const auto top = static_cast<s16>(y + line.top);
    const auto bottom = static_cast<s16>(y + line.bottom);
    /* The brackets, as the options gump's art draws them. */
    drawVLine(left, top, bottom, WIDGET_FACE);
    drawHLine(left, top, static_cast<s16>(left + 1), WIDGET_FACE);
    drawHLine(left, bottom, static_cast<s16>(left + 1), WIDGET_FACE);
    drawVLine(right, top, bottom, WIDGET_FACE);
    drawHLine(static_cast<s16>(right - 1), top, right, WIDGET_FACE);
    drawHLine(static_cast<s16>(right - 1), bottom, right, WIDGET_FACE);
    /* The ticks, bright up to the knob. */
    const auto knob = static_cast<s16>(left + BRACKET_ROOM + stepOffset(current_, steps_, travel()));
    for (auto tick = static_cast<s16>(left + BRACKET_ROOM); tick <= right - BRACKET_ROOM;
         tick = static_cast<s16>(tick + TICK_PITCH)) {
        drawVLine(tick, static_cast<s16>(top + 2), static_cast<s16>(bottom - 2),
                  tick <= knob ? WIDGET_FACE : WIDGET_EDGE);
    }
    gumpShapes->draw(knob, top, KNOB_SHAPE, 0, 0);
}

void Slider::handleLeftButton(NewGumpEvent *event)
{
    if (event->isMousePress()) {
        const Rect line = track();
        const s16 x = eventX(event);
        const s16 y = eventY(event);
        if (x < line.left || x > line.right || y < line.top - HEADING_GAP || y > line.bottom + HEADING_GAP) {
            return;
        }
        dragging_ = true;
        capture();
        eventMask |= EVENT_MOTION;
        if (touched) {
            touched();
        }
        moveTo(stepAt(x));
        return;
    }
    if (event->isMouseRelease() && dragging_) {
        dragging_ = false;
        release();
        eventMask &= ~static_cast<u32>(EVENT_MOTION);
    }
}

void Slider::handleMouseMotion(NewGumpEvent *event)
{
    if (dragging_) {
        moveTo(stepAt(eventX(event)));
    }
}

void Slider::handleCommand(NewGumpEvent *event)
{
    if (touched) {
        touched();
    }
    step(event->sender->id.instance() == LOWER_ARROW ? -1 : 1);
}

void Slider::step(i16 direction)
{
    moveTo(static_cast<i16>(current_ + direction));
}

void Slider::moveTo(i16 value)
{
    const i16 clamped = std::clamp(value, static_cast<i16>(0), static_cast<i16>(steps_ - 1));
    if (clamped == current_) {
        return;
    }
    current_ = clamped;
    if (changed) {
        changed(clamped);
    }
}
