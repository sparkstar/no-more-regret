/* The port's widgets: gumps drawn in the style of the originals' options
 * gump, which tell a layout (layout.h) the size they want. Their letters
 * are the fonts of fonts.flx in the colours of the options gump's art
 * (each game's widgets_game.h), their Hangul the Hangul font's (hangul.h);
 * the slider has that gump's knob and arrows. They are the port's own: no
 * original's layout binds them. */
#ifndef PORT_WIDGETS_H
#define PORT_WIDGETS_H
#include "layout.h"
#include "mainmenu.h"
#include "newgump.h"
#include "shapebtn.h"

#include <functional>
#include <string>
#include <vector>

/* The fonts the widgets write in: the headings' (the size of the large
 * letters of the options gump's art) and the choices' (the design of its
 * buttons' letters). */
inline constexpr i16 HEADING_FONT = 14;
inline constexpr i16 CHOICE_FONT = 7;

/* The colours text is drawn in: its font's most used colour becomes face,
 * the font's other colours edge (its 0, the black under some letters, is
 * left out); the Hangul font's one colour becomes face. */
struct Ink {
    u8 face;
    u8 edge;
};

/* The size of UTF-8 text in a font of fonts.flx, the characters the font
 * has (ASCII) in it and the Hangul in the Hangul font; others are left out.
 * Its height is the taller of the font's 'A' and a Hangul syllable, for any
 * text. */
[[nodiscard]] Extent textExtent(i16 font, const std::string &text);
/* Draws the text with its top left corner at (x, y) of the draw viewport. */
void drawText(i16 font, const std::string &text, s16 x, s16 y, Ink ink);

/* A gump of the port's layouts. */
class Widget : public NewGump {
public:
    Widget(NewGump *owner, u32 mask) : NewGump(NewGumpId(), owner, mask) {}
    [[nodiscard]] virtual Extent preferredExtent() = 0;
    /* Puts it at area, in its parent's coordinates. */
    virtual void place(const Rect &area) { bounds = area; }
    /* The keys' left (-1) and right (1) on it. */
    virtual void step(i16 /*direction*/) {}
    /* The key Enter on it. */
    virtual void activate() {}
    /* Told when the mouse changes it. */
    std::function<void()> touched;
};

/* A widget in a layout. */
class WidgetNode : public LayoutNode {
public:
    explicit WidgetNode(Widget *placed) : widget_(placed) {}
    [[nodiscard]] Extent extent() override { return widget_->preferredExtent(); }
    void place(const Rect &area) override { widget_->place(area); }

private:
    Widget *widget_;
};

/* A gump of the originals in a layout: it keeps the size it has and moves
 * to the place's corner. */
class GumpNode : public LayoutNode {
public:
    explicit GumpNode(NewGump *placed) : gump_(placed) {}
    [[nodiscard]] Extent extent() override
    {
        return Extent{static_cast<s16>(gump_->width()), static_cast<s16>(gump_->height())};
    }
    void place(const Rect &area) override { gump_->bounds.moveTo(area.left, area.top); }

private:
    NewGump *gump_;
};

/* A line of text. */
class Label : public Widget {
public:
    Label(NewGump *owner, i16 labelFont, Ink labelInk) : Widget(owner, 0), font_(labelFont), ink_(labelInk) {}
    void setText(std::string newText) { text_ = std::move(newText); }
    [[nodiscard]] Extent preferredExtent() override { return textExtent(font_, text_); }
    void paintSelf(i16 x, i16 y) override { drawText(font_, text_, x, y, ink_); }

private:
    i16 font_;
    Ink ink_;
    std::string text_;
};

/* A row of choices, one of them chosen, as the options gump's pairs of
 * buttons: the chosen one in the face colours (a warning, as the
 * originals' OFF, in their red), the others dimmed. A click or the keys
 * choose; changed hears the choice. */
class ChoiceRow : public Widget {
public:
    struct Choice {
        std::string text;
        bool warning;
    };
    explicit ChoiceRow(NewGump *owner) : Widget(owner, EVENT_LEFT_BUTTON) {}
    void setChoices(std::vector<Choice> newChoices) { choices_ = std::move(newChoices); }
    void setChosen(i16 index) { chosen_ = index; }
    [[nodiscard]] Extent preferredExtent() override;
    void paintSelf(i16 x, i16 y) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void step(i16 direction) override;
    std::function<void(i16)> changed;

private:
    /* The choices' places from the row's corner. */
    [[nodiscard]] std::vector<Rect> places() const;
    void choose(i16 index);
    std::vector<Choice> choices_;
    i16 chosen_ = 0;
};

/* A button of text, drawn as a chosen choice of a row: a click or the key
 * Enter (activate) presses it; pressed hears it. */
class TextButton : public Widget {
public:
    explicit TextButton(NewGump *owner) : Widget(owner, EVENT_LEFT_BUTTON) {}
    void setText(std::string newText) { text_ = std::move(newText); }
    [[nodiscard]] Extent preferredExtent() override { return textExtent(CHOICE_FONT, text_); }
    void paintSelf(i16 x, i16 y) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void activate() override;
    std::function<void()> pressed;

private:
    std::string text_;
};

/* The options gump's Done button (gumps.flx 0x2b, QUIT), which a click
 * anywhere in its box presses (ShapeButtonGump takes only the frame's drawn
 * pixels); its command's code is DONE_CODE. */
inline constexpr u8 DONE_CODE = 0x62;
class DoneButton final : public ShapeButtonGump {
public:
    explicit DoneButton(NewGump *parentGump) : ShapeButtonGump(DONE_CODE, parentGump, DONE_SHAPE) {}
    bool acceptsEvent(NewGumpEvent *event) override { return eventPointInBounds(event); }

private:
    static constexpr u16 DONE_SHAPE = 0x2b;
};

/* A slider in the options gump's style: its heading over a track of steps
 * with the knob (gumps.flx 0x30), between the arrows (0x31, 0x32, the
 * options gump's ArrowButton, which repeat while held), as wide as its
 * place when that is wider. The arrows, a click on the track or a drag of
 * the knob, or the keys move it; changed hears the step. */
class Slider : public Widget {
public:
    Slider(NewGump *owner, i16 stepCount);
    void setHeading(std::string text) { heading_ = std::move(text); }
    void setStep(i16 value) { current_ = value; }
    [[nodiscard]] Extent preferredExtent() override;
    void place(const Rect &area) override;
    void paintSelf(i16 x, i16 y) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void handleMouseMotion(NewGumpEvent *event) override;
    void handleCommand(NewGumpEvent *event) override;
    void step(i16 direction) override;
    std::function<void(i16)> changed;

private:
    /* The track, from the slider's corner, and the knob's travel on it. */
    [[nodiscard]] Rect track() const;
    [[nodiscard]] s16 travel() const;
    /* The step nearest to x, from the slider's corner. */
    [[nodiscard]] i16 stepAt(s16 x) const;
    void moveTo(i16 value);
    std::string heading_;
    i16 steps_;
    i16 current_ = 0;
    bool dragging_ = false;
    ArrowButton *lower_;
    ArrowButton *higher_;
};
#endif
