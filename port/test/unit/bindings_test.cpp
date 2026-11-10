/* The bindings of the controls (common/src/input/bindings.h) over a game's
 * table, one program a game (bindings, remorse_bindings).
 *
 * With the original's bindings (no bind_ lines in enhanced.cfg) every key
 * the window can send, with every set of modifiers and locks, pressed and
 * let go, comes to the controller as it is, with the action the controller
 * finds from it (controlAction): the controller does what the original did.
 * The only keys that do not come are the ones an action needs a modifier
 * for that is not held (No Regret's R without Ctrl), which the original's
 * controller passes over. The shift state of the mouse's and the
 * joystick's keys is the original's (modeShift), and the world's gump gets
 * every key code as it is.
 *
 * Bound otherwise, a key gives its action's key, the action's own keys no
 * longer come, a mode held from another key gives the mode's modifier bits,
 * a key code gives its action's code; an action's inputs written and read
 * again are the same, and a line the game cannot read leaves them.
 *
 * The other devices: each input of the table gives its action in the world
 * or in the selection of items, a mode's none; the selection's codes come as
 * they are; a mode held from a gamepad's input, the run's lock going round
 * at a key's or an input's press, a pulse not a mode's; an action's key and
 * code.
 *
 * The modern preset: its table's inputs and the original's
 * for the rest, and no key the window can send gives both an action of
 * the controller and one of the world's gump where the original's did not.
 *
 * The key settings' changes (16 inputs an action at most), its wait for an
 * input and its words (keysgump.h).
 *
 * The bindings' lines of enhanced.cfg (enhanced.h): written and read again
 * the same; its sound_bits=, fullscreen= and widescreen= too. */
#include "asylum.h"
#include "bindings.h"
#include "devices.h"
#include "enhanced.h"
#include "host.h"
#include "input.h"
#include "keysgump.h"
#include "kernel.h"
#include "mousectl.h"
#include "screen.h"
#include "newgump.h"

#include <cstdio>
#include <new>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH bindings: %s\n", what.c_str());
    }
}

std::string hex(unsigned value)
{
    char text[16];
    std::snprintf(text, sizeof text, "%x", value);
    return text;
}

/* The keys window.cpp sends (its setOne), scan code and E0h. */
struct Key {
    u8 scan;
    u8 extended;
};
std::vector<Key> windowKeys()
{
    std::vector<Key> keys;
    for (u8 scan = 0x01; scan <= 0x44; scan++) {
        keys.push_back({scan, 0});
    }
    for (u8 scan = 0x45; scan <= 0x53; scan++) {
        keys.push_back({scan, 0});
    }
    for (u8 scan : {0x56, 0x57, 0x58}) {
        keys.push_back({scan, 0});
    }
    for (u8 scan :
         {0x1c, 0x1d, 0x35, 0x38, 0x47, 0x48, 0x49, 0x4b, 0x4d, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x5b, 0x5c, 0x5d}) {
        keys.push_back({scan, 1});
    }
    return keys;
}

/* The shift states to try: the modifiers alone and together (the BIOS's
 * bits), with the locks. */
std::vector<u16> shiftStates()
{
    const u16 modifiers[] = {
        0,
        KEY_LEFT_SHIFT,
        KEY_RIGHT_SHIFT,
        KEY_CONTROL | KEY_LEFT_CONTROL,
        KEY_CONTROL | KEY_RIGHT_CONTROL,
        KEY_ALT | KEY_LEFT_ALT,
        KEY_ALT | KEY_RIGHT_ALT,
        KEY_CONTROL | KEY_LEFT_CONTROL | KEY_ALT | KEY_LEFT_ALT,
        KEY_LEFT_SHIFT | KEY_CONTROL | KEY_LEFT_CONTROL,
        KEY_LEFT_SHIFT | KEY_ALT | KEY_LEFT_ALT,
        KEY_LEFT_SHIFT | KEY_RIGHT_SHIFT | KEY_CONTROL | KEY_LEFT_CONTROL | KEY_ALT | KEY_RIGHT_ALT,
    };
    std::vector<u16> states;
    for (u16 modifier : modifiers) {
        for (u16 locks : {0, KEY_NUM_LOCK, KEY_CAPS_LOCK, KEY_NUM_LOCK | KEY_CAPS_LOCK, KEY_SCROLL_LOCK | KEY_INSERT}) {
            states.push_back(static_cast<u16>(modifier | locks));
        }
    }
    return states;
}

KeyEvent keyOf(u8 scan, u8 extended, u16 shift, u8 pressed)
{
    KeyEvent key;
    key.clear();
    key.scan = scan;
    key.extended = extended;
    key.shift = shift;
    key.pressed = pressed;
    key.repeat = pressed ? 1 : 0;
    key.time = 1234;
    key.translate();
    return key;
}

bool sameKey(const KeyEvent &a, const KeyEvent &b)
{
    return a.ascii == b.ascii && a.scan == b.scan && a.shift == b.shift && a.time == b.time && a.pressed == b.pressed &&
           a.extended == b.extended && a.repeat == b.repeat;
}

/* An action's inputs as the game's table has them. */
std::string originalText(Action action)
{
    for (std::size_t i = 0; i < GAME_ACTION_DEFAULT_COUNT; i++) {
        if (GAME_ACTION_DEFAULTS[i].action == action) {
            return GAME_ACTION_DEFAULTS[i].inputs;
        }
    }
    return "";
}

std::string described(const KeyEvent &key)
{
    return "scan " + hex(key.scan) + (key.extended ? " E0" : "") + " shift " + hex(key.shift) +
           (key.pressed ? " down" : " up");
}

/* Whether the key is an action's only with modifiers: with all of them held
 * an action takes it (with the ones held, none: the original passes over
 * it). */
bool needsModifiers(const KeyEvent &key)
{
    ControlKey bound;
    u16 all = static_cast<u16>(key.shift | KEY_LEFT_SHIFT | KEY_CONTROL | KEY_LEFT_CONTROL | KEY_ALT | KEY_LEFT_ALT);
    return controlKey(keyOf(key.scan, key.extended, all, 1), &bound) && bound.action != Action::NONE;
}

void originalControls()
{
    resetBindings();
    std::size_t keys = 0, dropped = 0;
    for (const Key &each : windowKeys()) {
        for (u16 shift : shiftStates()) {
            for (u8 pressed : {1, 0}) {
                KeyEvent key = keyOf(each.scan, each.extended, shift, pressed);
                ControlKey bound;
                keys++;
                if (!controlKey(key, &bound)) {
                    dropped++;
                    check(needsModifiers(key), "dropped although no action needs a modifier on it: " + described(key));
                    continue;
                }
                check(sameKey(bound.key, key),
                      "the original's binding changed the key: " + described(key) + " came as " + described(bound.key));
                check(bound.action == controlAction(&key),
                      "the action is the controller's own finding: " + described(key));
            }
        }
    }
    std::printf("bindings: %zu keys with the original's bindings, %zu not coming\n", keys, dropped);
    for (u32 shift = 0; shift < 0x1000; shift++) {
        if (modeShift(static_cast<u16>(shift)) != shift) {
            check(false, "modeShift changed shift " + hex(shift));
            break;
        }
    }
    for (int code = 0; code < 0x1000; code++) {
        i16 bound = 0;
        if (!worldCode(static_cast<i16>(code), &bound) || bound != code) {
            check(false, "the world's code " + hex(static_cast<unsigned>(code)) + " came as " +
                             hex(static_cast<unsigned>(bound)));
        }
    }
}

void otherBindings()
{
    /* The slide to the left from Q: Q gives the grey Delete, which no
     * longer comes itself. */
    resetBindings();
    check(bindAction(Action::SLIDE_LEFT, "q"), "slide_left=q read");
    ControlKey bound;
    check(controlKey(keyOf(0x10, 0, 0, 1), &bound) && bound.action == Action::SLIDE_LEFT && bound.key.scan == 0x53 &&
              bound.key.extended == 1,
          "Q gives the slide's Delete");
    check(!controlKey(keyOf(0x53, 1, 0, 1), &bound), "the grey Delete bound to nothing does not come");
    check(controlKey(keyOf(0x53, 0, 0, 1), &bound) && bound.action == Action::KEYPAD_USE,
          "the keypad's . still the keypad's use");
    /* The combat step held on C: a turn with C down is a turn with Alt;
     * Alt itself no longer is. */
    check(bindAction(Action::COMBAT_STEP, "c"), "combat_step=c read");
    plainKeys[0x2e].count = 1;
    check(controlKey(keyOf(0x4b, 1, 0, 1), &bound) && bound.action == Action::TURN_LEFT &&
              keyMode(&bound.key, Action::COMBAT_STEP),
          "C held: the turn comes with the combat step");
    plainKeys[0x2e].count = 0;
    check(controlKey(keyOf(0x4b, 1, KEY_ALT | KEY_LEFT_ALT, 1), &bound) && bound.action == Action::TURN_LEFT &&
              !keyMode(&bound.key, Action::COMBAT_STEP),
          "Alt no longer the combat step");
    check(modeShift(KEY_ALT | KEY_LEFT_ALT) == 0, "nor in the mouse's keys");
    /* An action held: its key down. */
    check(bindAction(Action::FIRE, "f"), "fire=f read");
    plainKeys[0x21].count = 3;
    check(actionHeld(Action::FIRE) == 3, "fire held on F");
    plainKeys[0x21].count = 0;
    check(actionHeld(Action::FIRE) == 0, "and let go");
    /* A world's action from another key: F gives U, U no longer comes. */
    check(bindAction(Action::USE_ITEM, "f F"), "use_item=f F read");
    i16 code = 0;
    check(worldCode('f', &code) && code == 'u', "f gives u");
    /* A key that is none of the action's own codes gives its first (U and
     * u are the same case of the world's gump). */
    check(worldCode('F', &code) && code == 'u', "F gives u");
    check(!worldCode('u', &code), "u bound to nothing does not come");
    /* The text written and read again; a line the game cannot read. */
    resetBindings();
    for (std::size_t i = 0; i < GAME_ACTION_DEFAULT_COUNT; i++) {
        Action action = GAME_ACTION_DEFAULTS[i].action;
        std::string text = boundText(action);
        check(text == GAME_ACTION_DEFAULTS[i].inputs, std::string(actionName(action)) + ": " + text + " written");
        check(bindAction(action, text.c_str()) && boundText(action) == text,
              std::string(actionName(action)) + " read again");
        check(actionNamed(actionName(action)) == action, std::string(actionName(action)) + "'s name");
    }
    check(!bindAction(Action::FIRE, "space nosuchkey"), "an unknown key is not read");
    check(boundText(Action::FIRE) == originalText(Action::FIRE), "and leaves the inputs");
    check(!bindAction(Action::COMBAT_STEP, "ctrl+c"), "a mode with a modifier is not read");
    check(bindAction(Action::FIRE, ""), "an action bound to nothing");
    check(!controlKey(keyOf(0x39, 0, 0, 1), &bound), "Space then does not come");
    resetBindings();
}

}

namespace {

/* The inputs of an action in the game's table as words. */
std::vector<std::string> wordsOf(const char *text)
{
    std::vector<std::string> words;
    std::string all(text), word;
    for (char c : all + " ") {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
            }
            word.clear();
        } else {
            word += c;
        }
    }
    return words;
}

bool isMode(Action action)
{
    return action == Action::RUN || action == Action::RUN_LOCK || action == Action::COMBAT_STEP ||
           action == Action::ROLL;
}

bool isSelection(Action action)
{
    return action >= Action::SELECTION_PREVIOUS && action <= Action::SELECTION_CANCEL;
}

void devices()
{
    resetBindings();
    /* Every device's input of the table gives its action where the action
     * is (the world or the selection); a mode's input gives no action of
     * the mode. */
    std::size_t inputs = 0;
    for (std::size_t i = 0; i < GAME_ACTION_DEFAULT_COUNT; i++) {
        Action action = GAME_ACTION_DEFAULTS[i].action;
        for (const std::string &word : wordsOf(GAME_ACTION_DEFAULTS[i].inputs)) {
            Device device = Device::NONE;
            for (std::size_t d = 1; d < static_cast<std::size_t>(Device::COUNT); d++) {
                if (word == deviceName(static_cast<Device>(d))) {
                    device = static_cast<Device>(d);
                }
            }
            if (device == Device::NONE) {
                continue;
            }
            inputs++;
            if (isMode(action)) {
                check(deviceAction(device, false) != action && deviceAction(device, true) != action,
                      word + " gives no action of the mode " + actionName(action));
                check(action != Action::RUN_LOCK || runLockInput(device), word + " is the run's lock's");
            } else {
                check(deviceAction(device, isSelection(action)) == action, word + " gives " + actionName(action));
            }
        }
    }
    std::printf("bindings: %zu inputs of the other devices in the table\n", inputs);
    check(deviceAction(Device::MOUSE_MIDDLE, false) == Action::NONE &&
              deviceAction(Device::PAD_GUIDE, false) == Action::NONE,
          "the middle button and Home bound to nothing");
    check(deviceAction(Device::PAD_A, false) == Action::JUMP &&
              deviceAction(Device::PAD_A, true) == Action::SELECTION_CHOOSE,
          "A: the jump in the world, the choice in the selection");
    /* The selection's codes as they are. */
    for (int code = 0; code < 0x1000; code++) {
        i16 bound = 0;
        if (!selectionCode(static_cast<i16>(code), &bound) || bound != code) {
            check(false, "the selection's code " + hex(static_cast<unsigned>(code)) + " came as " +
                             hex(static_cast<unsigned>(bound)));
        }
    }
    check(selectionAction(',') == Action::SELECTION_PREVIOUS && selectionAction('S') == Action::SELECTION_NEXT &&
              selectionAction(KEYCODE_SCAN | SCAN_INSERT) == Action::SELECTION_CHOOSE &&
              selectionAction('u') == Action::SELECTION_USE &&
              selectionAction(KEYCODE_SCAN | SCAN_DOWN) == Action::SELECTION_CANCEL,
          "the selection's actions of the original's codes");
    check(actionCode(Action::SELECTION_CANCEL) == ASCII_ESCAPE && actionCode(Action::USE_ITEM) == 'u',
          "an action's code: its first");
    i16 code = 0;
    check(bindAction(Action::SELECTION_USE, "f") && selectionCode('f', &code) && code == 'u',
          "selection_use=f: F gives u in the selection");
    check(!selectionCode('u', &code), "u bound to nothing does not come there");
    check(worldCode('f', &code) && code == 'f', "and F stays itself in the world");
    resetBindings();
    /* A mode held from a gamepad's trigger: a turn comes with the combat
     * step. */
    ControlKey bound;
    devicePress(Device::PAD_LT, Action::NONE);
    check(deviceDown(Device::PAD_LT), "the left trigger down");
    check(controlKey(keyOf(0x4b, 1, 0, 1), &bound) && bound.action == Action::TURN_LEFT &&
              keyMode(&bound.key, Action::COMBAT_STEP),
          "the left trigger held: the turn comes with the combat step");
    check(modeShift(0) & KEY_LEFT_ALT, "and the mouse's keys too");
    deviceRelease(Device::PAD_LT);
    check(!deviceDown(Device::PAD_LT) && !modeShift(0), "let go");
    /* The run's lock goes round at each press of its key or input. */
    check(bindAction(Action::RUN_LOCK, "capslock k pad_ls"), "run_lock=capslock k pad_ls read");
    controlKey(keyOf(0x25, 0, 0, 1), &bound);
    controlKey(keyOf(0x25, 0, 0, 0), &bound);
    check(controlKey(keyOf(0x48, 1, 0, 1), &bound) && keyMode(&bound.key, Action::RUN_LOCK), "K: the run locked");
    check(controlKey(keyOf(0x48, 1, KEY_CAPS_LOCK, 1), &bound) && !keyMode(&bound.key, Action::RUN_LOCK),
          "Caps Lock on turns it round again");
    devicePress(Device::PAD_LS, Action::NONE);
    deviceRelease(Device::PAD_LS);
    check(controlKey(keyOf(0x48, 1, 0, 1), &bound) && !keyMode(&bound.key, Action::RUN_LOCK),
          "the left stick's press: unlocked");
    toggleRunLock();
    check(controlKey(keyOf(0x48, 1, 0, 1), &bound) && keyMode(&bound.key, Action::RUN_LOCK), "turned back");
    toggleRunLock();
    /* A pulse is no mode's input; a held one is. */
    check(!bindAction(Action::COMBAT_STEP, "wheel_up") && !bindAction(Action::RUN, "mouse_move_left"),
          "the wheel and the motion are no mode's");
    check(bindAction(Action::COMBAT_STEP, "mouse_right pad_lb"), "a button is");
    check(boundText(Action::COMBAT_STEP) == "mouse_right pad_lb", "and written as read");
    /* An action's key: its first key, with the modifiers it needs. */
    resetBindings();
    KeyEvent fire = actionKey(Action::FIRE, 1, 1);
    check(fire.scan == SCAN_SPACE && !fire.extended && fire.pressed == 1 && fire.repeat == 1 && fire.ascii == ' ',
          "fire's key: Space");
    KeyEvent up = actionKey(Action::MOVE_FORWARD, 0, 0);
    check(up.scan == SCAN_UP && up.extended && up.pressed == 0, "move_forward's key: the grey up, let go");
    KeyEvent louder = actionKey(Action::MUSIC_LOUDER, 1, 1);
    check(louder.scan == 0x0d && (louder.isControlled() != 0) == (originalText(Action::MUSIC_LOUDER) == "ctrl+equals"),
          "music_louder's key with Ctrl as the game's needs it");
    check(actionKey(Action::READY_WEAPON, 1, 1).scan == 0, "ready_weapon has no key");
    resetBindings();
}

}

namespace {

/* The code the dispatcher gives the gumps for a key pressed (dispatch.cpp's
 * keyCode). */
i16 gumpCode(const KeyEvent &key)
{
    i16 code = 0;
    if (key.shift & (KEY_RIGHT_SHIFT | KEY_LEFT_SHIFT | KEY_CONTROL | KEY_ALT)) {
        if (key.shift & KEY_ALT) {
            code = static_cast<i16>(key.scan + KEYCODE_ALT);
        }
        if (key.shift & KEY_CONTROL) {
            code = static_cast<i16>(key.scan + KEYCODE_CONTROL);
        }
        if (key.shift & (KEY_RIGHT_SHIFT | KEY_LEFT_SHIFT)) {
            code = key.ascii ? static_cast<i16>(key.ascii) : static_cast<i16>(key.scan + KEYCODE_SHIFT);
        }
    } else if (key.ascii) {
        code = key.ascii;
    } else {
        code = static_cast<i16>(key.scan | KEYCODE_SCAN);
    }
    return code;
}

/* The keys pressed that give both an action of the controller and one of
 * the world's gump, as scan, E0h and shift state. */
std::vector<std::string> doubleKeys()
{
    std::vector<std::string> both;
    for (const Key &each : windowKeys()) {
        for (u16 shift : shiftStates()) {
            KeyEvent key = keyOf(each.scan, each.extended, shift, 1);
            ControlKey control;
            i16 code = 0;
            if (controlKey(key, &control) && control.action != Action::NONE && worldCode(gumpCode(key), &code) &&
                worldAction(code) != Action::NONE) {
                both.push_back(described(key) + " gives " + actionName(control.action) + " and " +
                               actionName(worldAction(code)));
            }
        }
    }
    return both;
}

void modernPreset()
{
    setKeyPreset(KeyPreset::ORIGINAL);
    std::vector<std::string> original = doubleKeys();
    KeyPreset named = KeyPreset::ORIGINAL;
    check(keyPresetNamed("modern", &named) && named == KeyPreset::MODERN && !keyPresetNamed("wasd", &named) &&
              std::string(keyPresetName(KeyPreset::MODERN)) == "modern",
          "the presets' names");
    setKeyPreset(KeyPreset::MODERN);
    check(keyPreset() == KeyPreset::MODERN, "the modern preset");
    for (std::size_t i = 0; i < GAME_ACTION_DEFAULT_COUNT; i++) {
        Action action = GAME_ACTION_DEFAULTS[i].action;
        std::string expected = GAME_ACTION_DEFAULTS[i].inputs;
        for (std::size_t m = 0; m < GAME_MODERN_BINDING_COUNT; m++) {
            if (GAME_MODERN_BINDINGS[m].action == action) {
                expected = GAME_MODERN_BINDINGS[m].inputs;
            }
        }
        check(boundText(action) == expected, std::string(actionName(action)) + ": " + boundText(action) +
                                                 " in the modern preset, " + expected + " in its table");
    }
    std::size_t added = 0;
    for (const std::string &both : doubleKeys()) {
        bool before = false;
        for (const std::string &old : original) {
            before = before || old == both;
        }
        if (!before) {
            added++;
            check(false, "the modern preset: " + both);
        }
    }
    std::printf("bindings: %zu keys give two actions with the original's bindings, %zu more with the modern\n",
                original.size(), added);
    /* W moves; the world does not get its next weapon. */
    ControlKey bound;
    i16 code = 0;
    check(controlKey(keyOf(0x11, 0, 0, 1), &bound) && bound.action == Action::MOVE_FORWARD && bound.key.scan == 0x48 &&
              bound.key.extended,
          "W: forward, as the grey up");
    check(!worldCode('w', &code), "and no w for the world");
    check(worldCode('e', &code) && code == 's', "E: the selection of items");
    check(controlKey(keyOf(0x1e, 0, KEY_CONTROL | KEY_LEFT_CONTROL, 1), &bound) && bound.action == Action::ROLL_LEFT,
          "Ctrl+A: the roll to the left");
    /* Ctrl+A's Ctrl is its own, not the roll's (whose Ctrl+Insert does
     * nothing): Insert comes alone, and Ctrl+D's PageUp; Ctrl+W is the
     * forward roll, the grey up with the roll. */
    check(bound.key.scan == 0x52 && bound.key.extended && !keyMode(&bound.key, Action::ROLL),
          "Ctrl+A: Insert without the roll");
    check(controlKey(keyOf(0x20, 0, KEY_CONTROL | KEY_LEFT_CONTROL, 1), &bound) && bound.action == Action::ROLL_RIGHT &&
              bound.key.scan == 0x49 && !keyMode(&bound.key, Action::ROLL),
          "Ctrl+D: PageUp without the roll");
    check(controlKey(keyOf(0x11, 0, KEY_CONTROL | KEY_LEFT_CONTROL, 1), &bound) &&
              bound.action == Action::MOVE_FORWARD && keyMode(&bound.key, Action::ROLL),
          "Ctrl+W: the grey up with the roll");
    /* The roll on the mouse's right button: held from the driver's press,
     * before the world's gump gets it after the double click's wait (a
     * process made as for a load, which the kernel does not know). */
    check(bindAction(Action::ROLL, "ctrl mouse_right"), "roll=ctrl mouse_right read");
    alignas(MouseCtl) static unsigned char mouseMemory[sizeof(MouseCtl)] = {};
    MouseCtl *mouse = ::new (mouseMemory) MouseCtl(Loading{});
    MouseCtl *const kept = mouseCtl;
    mouseCtl = mouse;
    mouse->buttons = MOUSE_BUTTON_RIGHT;
    check(controlKey(keyOf(0x11, 0, 0, 1), &bound) && keyMode(&bound.key, Action::ROLL),
          "the right button down: W with the roll");
    mouse->buttons = 0;
    check(controlKey(keyOf(0x11, 0, 0, 1), &bound) && !keyMode(&bound.key, Action::ROLL), "up: W without");
    mouseCtl = kept;
    resetBindings();
    /* A bind_ line after the preset changes its action alone; the reset
     * goes back to the preset's. */
    check(bindAction(Action::JUMP, "j"), "jump=j read");
    check(boundText(Action::JUMP) == "j" && boundText(Action::MOVE_FORWARD) == "up w lstick_up",
          "one action changed, the preset's others kept");
    resetBindings();
    check(boundText(Action::JUMP) != "j", "the reset: the preset's again");
    setKeyPreset(KeyPreset::ORIGINAL);
    check(boundText(Action::MOVE_FORWARD) == originalText(Action::MOVE_FORWARD), "and the original's");
}

/* The bindings' lines of enhanced.cfg (enhanced.h): the preset and every action's inputs
 * written and read again are the same; a bind_ line the game cannot read
 * leaves its action, comments and other keys are passed over, and keys=
 * alone brings the preset's inputs back. */
void keyFileText()
{
    setKeyPreset(KeyPreset::MODERN);
    check(bindAction(Action::JUMP, "k"), "jump=k read");
    const std::string text = bindingsText();
    check(text.starts_with("keys=modern\r\n"), "the preset's line");
    check(text.find("\r\nbind_jump=k\r\n") != std::string::npos, "jump's line");
    setKeyPreset(KeyPreset::ORIGINAL);
    check(readBindingsText(text).empty(), "read again: every line");
    check(keyPreset() == KeyPreset::MODERN && boundText(Action::JUMP) == "k", "read again: the preset and jump's k");
    check(bindingsText() == text, "written again the same");
    const std::vector<std::string> unread =
        readBindingsText("# a comment\n/ another\nsound=on\nbind_jump=nokey\nbind_nothing=j\nkeys=wasd\n");
    check(unread == std::vector<std::string>{"bind_jump=nokey", "bind_nothing=j", "keys=wasd"},
          "the lines it cannot read, told");
    check(keyPreset() == KeyPreset::MODERN && boundText(Action::JUMP) == "k", "and left");
    check(readBindingsText("  KEYS = Original \r\n").empty(), "keys= alone read");
    check(keyPreset() == KeyPreset::ORIGINAL && boundText(Action::JUMP) == originalText(Action::JUMP),
          "keys= alone (spaces, capitals): the original's inputs");
}

/* MOUSE ROLL (bindings.h's mouseRoll): on by default, the mouse's and the
 * sticks' turn keys with every mode held; its line of enhanced.cfg written
 * and read again; off, the turn keys without the roll's Ctrl and the combat
 * step's Alt, the run's Shift kept; a text without the line leaves it. */
void mouseRollText()
{
    setKeyPreset(KeyPreset::ORIGINAL);
    const u16 held = KEY_CONTROL | KEY_LEFT_CONTROL | KEY_ALT | KEY_LEFT_ALT | KEY_LEFT_SHIFT;
    check(mouseRoll(), "the mouse rolls by default");
    check(deviceTurnShift(held) == held, "on: every mode held");
    check(bindingsText().find("\r\nmouse_roll=1\r\n") != std::string::npos, "on: its line");
    check(readBindingsText("mouse_roll=0\r\n").empty() && !mouseRoll(), "mouse_roll=0 read");
    check(deviceTurnShift(held) == KEY_LEFT_SHIFT, "off: without Ctrl and Alt");
    check(bindingsText().find("\r\nmouse_roll=0\r\n") != std::string::npos, "off: its line");
    check(readBindingsText("keys=modern\r\n").empty() && !mouseRoll(), "a text without it leaves it");
    check(readBindingsText("mouse_roll=yes\r\n") == std::vector<std::string>{"mouse_roll=yes"} && !mouseRoll(),
          "a value it cannot read, told and left");
    setMouseRoll(true);
    setKeyPreset(KeyPreset::ORIGINAL);
}

/* The key settings (keysgump.h): the inputs a key gives each domain; inputs
 * added once, taken from the actions they gave the same with, refused for
 * another domain; the wait for an input: a key, a modifier alone, Esc's
 * clearing, the other devices, the window's keys taken; the buttons' words
 * of the inputs. */
void keySettings()
{
    setKeyPreset(KeyPreset::ORIGINAL);
    const u16 ctrl = KEY_CONTROL | KEY_LEFT_CONTROL;
    check(keyInputText(Action::JUMP, 0x25, 0, 0) == "k", "the controller's k");
    check(keyInputText(Action::JUMP, 0x25, 0, ctrl) == "ctrl+k", "the controller's ctrl+k");
    check(keyInputText(Action::JUMP, 0x48, 1, 0) == "up" && keyInputText(Action::JUMP, 0x48, 0, 0) == "kp8",
          "the grey up, the keypad's 8");
    check(keyInputText(Action::COMBAT_STEP, 0x25, 0, ctrl) == "k", "a mode's key alone");
    check(keyInputText(Action::USE_ITEM, 0x25, 0, 0) == "k K", "a code's letter in both cases");
    check(keyInputText(Action::USE_ITEM, 0x25, 0, KEY_LEFT_SHIFT | KEY_CAPS_LOCK) == "k K", "with Shift, Caps Lock");
    check(keyInputText(Action::USE_ITEM, 0x33, 0, 0) == ", <", "a code's , and <");
    check(keyInputText(Action::USE_ITEM, 0x3e, 0, 0) == "f4", "a code's f4");
    check(keyInputText(Action::USE_ITEM, 0x3e, 0, KEY_LEFT_SHIFT) == "shift+f4", "a code's shift+f4");
    check(keyInputText(Action::USE_ITEM, 0x20, 0, ctrl) == "ctrl+d", "a code's ctrl+d");
    check(keyInputText(Action::SELECTION_CHOOSE, 0x39, 0, 0) == "space", "the selection's space");

    check(addInputs(Action::MOVE_FORWARD, "k") && boundText(Action::MOVE_FORWARD) == "up mouse_right lstick_up k",
          "k added: " + boundText(Action::MOVE_FORWARD));
    check(addInputs(Action::MOVE_FORWARD, "k up") && boundText(Action::MOVE_FORWARD) == "up mouse_right lstick_up k",
          "added again: once");
    check(addInputs(Action::MOVE_FORWARD, "j") && boundText(Action::JUMP) == "tab pad_a",
          "j taken from jump: " + boundText(Action::JUMP));
    check(addInputs(Action::MOVE_BACK, "pad_a") && boundText(Action::JUMP) == "tab" &&
              boundText(Action::SELECTION_CHOOSE) == "enter space insert pad_a",
          "pad_a taken from jump, not from the selection's: " + boundText(Action::SELECTION_CHOOSE));
    check(addInputs(Action::REWIND, "tab") && boundText(Action::JUMP).empty() && boundText(Action::REWIND) == "f11 tab",
          "a window's key taken from the controller's");
    check(
        !addInputs(Action::REWIND, "wheel_up") && !addInputs(Action::REWIND, "mouse_left") &&
            !addInputs(Action::RUN, "wheel_up") && !addInputs(Action::USE_ITEM, "ctrl+alt+x") &&
            boundText(Action::REWIND) == "f11 tab",
        "inputs of another domain refused (the window's devices are buttons: the mouse's middle and sides, the pad's)");
    check(addInputs(Action::REWIND, "pad_a") && boundText(Action::REWIND) == "f11 tab pad_a" &&
              boundText(Action::MOVE_BACK).find("pad_a") == std::string::npos &&
              boundText(Action::SELECTION_CHOOSE) == "enter space insert",
          "a pad's button for the window, taken from the controller's and the selection's: " +
              boundText(Action::SELECTION_CHOOSE));
    check(addInputs(Action::MOVE_BACK, "pad_a") && boundText(Action::REWIND) == "f11 tab",
          "and back to the controller: " + boundText(Action::REWIND));
    /* 16 an action (the bound ones are a setting of bytes). */
    check(addInputs(Action::MOVE_FORWARD, "f1 f2 f3 f4 f5 f6 f7 f8 f9 f10 f12") &&
              !addInputs(Action::MOVE_FORWARD, "1 f1") && boundText(Action::MOVE_FORWARD).ends_with(" f10 f12"),
          "16 inputs an action, none more: " + boundText(Action::MOVE_FORWARD));
    check(!bindAction(Action::MOVE_FORWARD, "1 2 3 4 5 6 7 8 9 0 q w e r t y u") &&
              boundText(Action::MOVE_FORWARD).ends_with(" f10 f12"),
          "a line of 17 refused");
    clearInputs(Action::MOVE_FORWARD);
    check(boundText(Action::MOVE_FORWARD).empty(), "cleared");
    setKeyPreset(KeyPreset::ORIGINAL);

    check(!captureKey(keyOf(0x25, 0, 0, 1)) && !captureDevice(Device::PAD_B), "no wait: nothing taken");
    startCapture(Action::JUMP);
    check(capturingAction() == Action::JUMP, "waiting for jump");
    check(captureKey(keyOf(0x25, 0, 0, 1)) && capturingAction() == Action::NONE &&
              boundText(Action::JUMP) == "j tab pad_a k",
          "a key ends the wait, added: " + boundText(Action::JUMP));
    check(!captureKey(keyOf(0x25, 0, 0, 0)), "its release the game's");
    startCapture(Action::FIRE);
    check(captureKey(keyOf(0x1d, 0, ctrl, 1)) && capturingAction() == Action::FIRE, "Ctrl down: still waiting");
    check(captureKey(keyOf(0x1d, 0, 0, 0)) && capturingAction() == Action::NONE &&
              boundText(Action::FIRE).ends_with(" lctrl"),
          "Ctrl alone: lctrl for the controller: " + boundText(Action::FIRE));
    startCapture(Action::COMBAT_STEP);
    captureKey(keyOf(0x1d, 1, ctrl, 1));
    captureKey(keyOf(0x1d, 1, 0, 0));
    check(boundText(Action::COMBAT_STEP) == "alt pad_lt ctrl", "the right Ctrl alone: a mode's ctrl");
    startCapture(Action::USE_ITEM);
    captureKey(keyOf(0x1d, 0, ctrl, 1));
    captureKey(keyOf(0x1d, 0, 0, 0));
    check(capturingAction() == Action::USE_ITEM, "Ctrl alone is no code: still waiting");
    check(captureKey(keyOf(0x1d, 0, ctrl, 1)) && captureKey(keyOf(0x20, 0, ctrl, 1)) &&
              boundText(Action::USE_ITEM) == "u U pad_x ctrl+d",
          "Ctrl with D: ctrl+d");
    startCapture(Action::JUMP);
    check(captureDevice(Device::PAD_Y) && boundText(Action::JUMP) == "j tab pad_a k pad_y" &&
              boundText(Action::READY_WEAPON) == "mouse_right_double",
          "a pad's button added, taken from ready_weapon: " + boundText(Action::READY_WEAPON));
    startCapture(Action::REWIND);
    check(captureDevice(Device::WHEEL_UP) && capturingAction() == Action::REWIND,
          "the wheel is no window's button: waiting");
    check(captureKey(keyOf(0x01, 0, 0, 1)) && capturingAction() == Action::NONE && boundText(Action::REWIND).empty(),
          "Esc clears");
    startCapture(Action::REWIND);
    check(captureDevice(Device::MOUSE_X1) && capturingAction() == Action::NONE &&
              boundText(Action::REWIND) == "mouse_x1",
          "the mouse's side button for the window: " + boundText(Action::REWIND));
    startCapture(Action::JUMP);
    stopCapture();
    check(capturingAction() == Action::NONE && boundText(Action::JUMP) == "j tab pad_a k pad_y", "stopped: no change");
    setKeyPreset(KeyPreset::ORIGINAL);

    check(inputsLabel("u U pad_x") == "U, PAD X", "U, PAD X: " + inputsLabel("u U pad_x"));
    check(inputsLabel("b B , <") == "B, ,, <", "B, ,, <: " + inputsLabel("b B , <"));
    check(inputsLabel("B ctrl+d") == "SHIFT+B, CTRL+D", "SHIFT+B, CTRL+D: " + inputsLabel("B ctrl+d"));
    check(inputsLabel("kp8@nonumlock kp8@numlock mouse_left_double") == "KP8 NUM OFF, KP8 NUM ON, MOUSE LEFT DOUBLE",
          "the keypad's and the mouse's: " + inputsLabel("kp8@nonumlock kp8@numlock mouse_left_double"));
    check(inputsLabel("").empty(), "none: empty");
    for (const ActionGroup &group : actionGroups()) {
        for (const Action action : group.actions) {
            check(actionWords(action).english[0] != 0 && actionWords(action).korean[0] != 0,
                  std::string("the words of ") + actionName(action));
        }
    }
    std::size_t grouped = 0;
    for (const ActionGroup &group : actionGroups()) {
        grouped += group.actions.size();
    }
    check(grouped == static_cast<std::size_t>(Action::COUNT) - 1, "every action in a group");
}

/* enhanced.cfg's sound_bits=: the depth read, written when the player
 * chose it, none for another value; the depth the sound's start takes
 * from the DMA channel (initAsylum's) not written. */
void soundDepthText()
{
    chooseSoundBits(0);
    check(enhancedText().find("sound_bits=") == std::string::npos && ASS_GetDepth() == 0, "none chosen: no line");
    ASS_SetDepth(8);
    check(enhancedText().find("sound_bits=") == std::string::npos, "the DMA's depth at the start: no line");
    check(readEnhancedText("sound_bits = 16\r\n").empty() && ASS_GetDepth() == 16, "sound_bits=16 read");
    const std::string text = enhancedText();
    check(text.find("\r\nsound_bits=16\r\n") != std::string::npos, "sound_bits=16 written");
    check(readEnhancedText("sound_bits=8\r\n").empty() && ASS_GetDepth() == 8, "sound_bits=8 read");
    check(readEnhancedText(text).empty() && ASS_GetDepth() == 16 && enhancedText() == text, "read again, the same");
    check(readEnhancedText("sound_bits=32\r\n").empty() && ASS_GetDepth() == 0 &&
              enhancedText().find("sound_bits=") == std::string::npos,
          "sound_bits=32: the DMA's, no line");
    chooseSoundBits(16);
    check(ASS_GetDepth() == 16 && enhancedText().find("\r\nsound_bits=16\r\n") != std::string::npos,
          "the settings' choice: set and written");
    chooseSoundBits(0);
}

/* enhanced.cfg's widescreen=: 1 and 2 read and written, another off, no
 * line when off. */
void wideScreenText()
{
    screenSetWide(0);
    check(enhancedText().find("widescreen=") == std::string::npos, "the wide screen off: no line");
    check(readEnhancedText("widescreen = 2\r\n").empty() && screenWide() == 2, "widescreen=2 read");
    const std::string text = enhancedText();
    check(text.find("\r\nwidescreen=2\r\n") != std::string::npos, "widescreen=2 written");
    check(readEnhancedText("widescreen=1\r\n").empty() && screenWide() == 1, "widescreen=1 read");
    check(readEnhancedText(text).empty() && screenWide() == 2 && enhancedText() == text, "read again, the same");
    check(readEnhancedText("widescreen=3\r\n").empty() && screenWide() == 0, "widescreen=3: off");
}

/* enhanced.cfg's fullscreen=: 1 the full screen, written so; another a
 * window, no line. */
void fullscreenText()
{
    hostSetFullscreen(false);
    check(enhancedText().find("\r\nfullscreen=") == std::string::npos,
          "a window: no line (bind_fullscreen= is the key's)");
    check(readEnhancedText("fullscreen = 1\r\n").empty() && hostFullscreen(), "fullscreen=1 read");
    const std::string text = enhancedText();
    check(text.find("\r\nfullscreen=1\r\n") != std::string::npos, "fullscreen=1 written");
    check(readEnhancedText("fullscreen=0\r\n").empty() && !hostFullscreen(), "fullscreen=0: a window");
    check(readEnhancedText(text).empty() && hostFullscreen() && enhancedText() == text, "read again, the same");
    check(readEnhancedText("fullscreen=yes\r\n").empty() && !hostFullscreen(), "fullscreen=yes: a window");
}

/* The debug features among the actions: the host actions but the full
 * screen, as the window has them (input.h's hostActionIsDebug, in
 * HostAction's order), no other; enhanced.cfg's debug_feature=true kept
 * when it was read, its line gone with another value. */
void debugFeatureText()
{
    const Action hosts[HOST_ACTIONS] = {Action::REWIND,    Action::SAVE_STATE,   Action::LOAD_STATE, Action::FULLSCREEN,
                                        Action::TAKE_OVER, Action::FAST_FORWARD, Action::PAUSE};
    bool same = true;
    for (std::size_t i = 0; i < HOST_ACTIONS; i++) {
        same = same && debugAction(hosts[i]) == hostActionIsDebug(static_cast<HostAction>(i));
    }
    check(same, "the debug actions: the window's host actions but the full screen");
    std::size_t debug = 0;
    for (std::size_t i = 1; i < static_cast<std::size_t>(Action::COUNT); i++) {
        debug += debugAction(static_cast<Action>(i)) ? 1 : 0;
    }
    check(debug == 6, "six debug actions, no game action: " + std::to_string(debug));
    check(readEnhancedText("debug_feature=no\r\n").empty() && enhancedText().find("debug_feature") == std::string::npos,
          "debug_feature=no: no line");
    check(readEnhancedText("Debug_Feature = TRUE\r\n").empty() &&
              enhancedText().find("\r\ndebug_feature=true\r\n") != std::string::npos,
          "debug_feature=true read and written again");
    const std::string text = enhancedText();
    check(readEnhancedText(text).empty() && enhancedText() == text, "read again, the same");
    check(readEnhancedText("debug_feature=0\r\n").empty() && enhancedText().find("debug_feature") == std::string::npos,
          "debug_feature=0: gone");
}

}

int main()
{
    originalControls();
    otherBindings();
    devices();
    modernPreset();
    keyFileText();
    mouseRollText();
    keySettings();
    soundDepthText();
    fullscreenText();
    wideScreenText();
    debugFeatureText();
    std::printf("bindings: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
