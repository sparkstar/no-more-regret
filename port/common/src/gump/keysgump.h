/* The port's key settings, which the settings gump's CONTROLS opens: the
 * keys' preset (bindings.h: every action's inputs the original's or the
 * modern layout's), whether the mouse and the sticks roll (bindings.h's
 * mouseRoll: MOUSE ROLL on or off) and the game's actions in groups, each a
 * button with its inputs. A click on an action waits for an input
 * (bindings.h's startCapture: a key, a modifier alone, a gamepad's button,
 * stick or trigger, a mouse's button or wheel, or the action's own button
 * clicked again with the left or the right button), which is added to the
 * action's; a right click on it, or Esc while it waits, clears them; a click
 * anywhere else while it waits ends the wait. The keys: up and down along
 * the actions, left and right through the groups, Enter waits on the action,
 * Escape closes. The settings gump writes enhanced.cfg as it closes. The
 * gump fills the screen. */
#ifndef PORT_KEYSGUMP_H
#define PORT_KEYSGUMP_H
#include "bindings.h"
#include "layout.h"
#include "settings.h"
#include "shapebtn.h"
#include "widgets.h"

#include <string>
#include <span>
#include <vector>

/* A group of actions and the words of its tab. */
struct ActionGroup {
    Words words;
    std::span<const Action> actions;
};
/* The groups in their tabs' order: moving, the keypad, the weapons, the
 * items, the game's own; every action of both games is in one of them. */
[[nodiscard]] std::span<const ActionGroup> actionGroups();
/* The words of an action's button. */
[[nodiscard]] Words actionWords(Action action);
/* The inputs of enhanced.cfg's text (boundText) as a button shows them:
 * capitals, a space for an underscore, a character's two cases once (u U
 * as U; a capital alone as SHIFT+B), Num Lock's state after a keypad's key,
 * the inputs a comma and a space apart. */
[[nodiscard]] std::string inputsLabel(const std::string &inputs);

class KeysGump : public NewGump {
public:
    KeysGump();
    ~KeysGump() override;
    KeysGump(const KeysGump &) = delete;
    KeysGump &operator=(const KeysGump &) = delete;
    KeysGump(KeysGump &&) = delete;
    KeysGump &operator=(KeysGump &&) = delete;
    void execute(NewGumpEvent *event) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void handleRightButton(NewGumpEvent *event) override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;

    /* An action's button: its name in a column of the names' width, then its
     * inputs, or while it waits the words of the wait. */
    class ActionButton : public Widget {
    public:
        ActionButton(KeysGump *owner, std::size_t row)
            : Widget(owner, EVENT_LEFT_BUTTON | EVENT_RIGHT_BUTTON), keys_(owner), row_(row)
        {
        }
        void setAction(Action shown, s16 nameWidth);
        [[nodiscard]] Action action() const { return action_; }
        [[nodiscard]] Extent preferredExtent() override;
        void paintSelf(i16 x, i16 y) override;
        void handleLeftButton(NewGumpEvent *event) override;
        void handleRightButton(NewGumpEvent *event) override;

    private:
        KeysGump *keys_;
        std::size_t row_;
        Action action_ = Action::NONE;
        s16 nameWidth_ = 0;
    };

private:
    /* A button clicked with the left or the right button. */
    void pressed(std::size_t row, bool right);
    /* The wait for the row's action's input. */
    void wait(std::size_t row);
    /* The wait ended with nothing (a click elsewhere, another tab). */
    void endWait();
    void showGroup(i16 group);
    /* The widgets' texts in the language and the bindings now, and their
     * places again. */
    void refresh();
    /* The frame round the row the keys stand on; all 0 for none. */
    [[nodiscard]] Rect focusFrame();
    LayoutBox layout_;
    Label *title_;
    ChoiceRow *preset_;
    Label *mouseRollLabel_;
    ChoiceRow *mouseRoll_;
    ChoiceRow *groups_;
    std::vector<ActionButton *> buttons_;
    Label *hint_;
    ShapeButtonGump *done_;
    /* The row the keys stand on, and whether the buttons show a wait. */
    i16 focus_ = 0;
    bool waitShown_ = false;
};
#endif
