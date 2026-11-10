/* The bindings of the game's controls. See bindings.h. */
#include "bindings.h"
#include "gamestate.h"
#include "devices.h"
#include "dispatch.h"
#include "kernel.h"
#include "mousectl.h"
#include "newgump.h"
#include "input.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>

namespace {

/* Which code takes an action's inputs: the controller's keys, the world's
 * gump's key codes (in the selection of items, the selection's), the modes
 * held, or the window's for the port's own (host) actions. */
enum class Domain : u8 { CONTROL, WORLD, SELECTION, MODE, HOST };

struct ActionInfo {
    Action action;
    const char *name;
    Domain domain;
};

/* Every action's name and domain, in the enum's order. */
constexpr ActionInfo ACTION_INFOS[] = {
    {Action::NONE, nullptr, Domain::CONTROL},
    {Action::RUN, "run", Domain::MODE},
    {Action::RUN_LOCK, "run_lock", Domain::MODE},
    {Action::COMBAT_STEP, "combat_step", Domain::MODE},
    {Action::ROLL, "roll", Domain::MODE},
    {Action::MOVE_FORWARD, "move_forward", Domain::CONTROL},
    {Action::MOVE_BACK, "move_back", Domain::CONTROL},
    {Action::TURN_LEFT, "turn_left", Domain::CONTROL},
    {Action::TURN_RIGHT, "turn_right", Domain::CONTROL},
    {Action::ADVANCE, "advance", Domain::CONTROL},
    {Action::RETREAT, "retreat", Domain::CONTROL},
    {Action::SLIDE_LEFT, "slide_left", Domain::CONTROL},
    {Action::SLIDE_RIGHT, "slide_right", Domain::CONTROL},
    {Action::ROLL_LEFT, "roll_left", Domain::CONTROL},
    {Action::ROLL_RIGHT, "roll_right", Domain::CONTROL},
    {Action::ROLL_FORWARD, "roll_forward", Domain::CONTROL},
    {Action::JUMP_FORWARD, "jump_forward", Domain::CONTROL},
    {Action::KNEEL, "kneel", Domain::CONTROL},
    {Action::MOVE_NORTHWEST, "move_northwest", Domain::CONTROL},
    {Action::MOVE_SOUTHEAST, "move_southeast", Domain::CONTROL},
    {Action::MOVE_SOUTHWEST, "move_southwest", Domain::CONTROL},
    {Action::MOVE_NORTHEAST, "move_northeast", Domain::CONTROL},
    {Action::MOVE_WEST, "move_west", Domain::CONTROL},
    {Action::MOVE_NORTH, "move_north", Domain::CONTROL},
    {Action::MOVE_SOUTH, "move_south", Domain::CONTROL},
    {Action::MOVE_EAST, "move_east", Domain::CONTROL},
    {Action::NUMPAD_FORWARD, "numpad_forward", Domain::CONTROL},
    {Action::NUMPAD_BACK, "numpad_back", Domain::CONTROL},
    {Action::NUMPAD_TURN_LEFT, "numpad_turn_left", Domain::CONTROL},
    {Action::NUMPAD_TURN_RIGHT, "numpad_turn_right", Domain::CONTROL},
    {Action::NUMPAD_SLIDE_LEFT, "numpad_slide_left", Domain::CONTROL},
    {Action::NUMPAD_SLIDE_RIGHT, "numpad_slide_right", Domain::CONTROL},
    {Action::NUMPAD_ROLL_LEFT, "numpad_roll_left", Domain::CONTROL},
    {Action::NUMPAD_ROLL_RIGHT, "numpad_roll_right", Domain::CONTROL},
    {Action::FIRE, "fire", Domain::CONTROL},
    {Action::JUMP, "jump", Domain::CONTROL},
    {Action::READY_WEAPON, "ready_weapon", Domain::CONTROL},
    {Action::KEYPAD_USE, "keypad_use", Domain::CONTROL},
    {Action::KEYPAD_NEXT_WEAPON, "keypad_next_weapon", Domain::CONTROL},
    {Action::KEYPAD_NEXT_ITEM, "keypad_next_item", Domain::CONTROL},
    {Action::KEYPAD_SELECT, "keypad_select", Domain::CONTROL},
    {Action::CAMERA, "camera", Domain::CONTROL},
    {Action::RETURN_CONTROL, "return_control", Domain::CONTROL},
    {Action::MUSIC_LOUDER, "music_louder", Domain::CONTROL},
    {Action::MUSIC_QUIETER, "music_quieter", Domain::CONTROL},
    {Action::RETICLE_STYLE, "reticle_style", Domain::CONTROL},
    {Action::SELECT_WEAPON_1, "select_weapon_1", Domain::CONTROL},
    {Action::SELECT_WEAPON_2, "select_weapon_2", Domain::CONTROL},
    {Action::SELECT_WEAPON_3, "select_weapon_3", Domain::CONTROL},
    {Action::SELECT_WEAPON_4, "select_weapon_4", Domain::CONTROL},
    {Action::SELECT_WEAPON_5, "select_weapon_5", Domain::CONTROL},
    {Action::SELECT_WEAPON_6, "select_weapon_6", Domain::CONTROL},
    {Action::SELECT_WEAPON_7, "select_weapon_7", Domain::CONTROL},
    {Action::SELECT_WEAPON_8, "select_weapon_8", Domain::CONTROL},
    {Action::SELECT_WEAPON_9, "select_weapon_9", Domain::CONTROL},
    {Action::SELECT_WEAPON_10, "select_weapon_10", Domain::CONTROL},
    {Action::SELECT_WEAPON_11, "select_weapon_11", Domain::CONTROL},
    {Action::SELECT_WEAPON_12, "select_weapon_12", Domain::CONTROL},
    {Action::QUIT, "quit", Domain::WORLD},
    {Action::CANCEL, "cancel", Domain::WORLD},
    {Action::MENU, "menu", Domain::WORLD},
    {Action::HELP, "help", Domain::WORLD},
    {Action::OPTIONS, "options", Domain::WORLD},
    {Action::QUICK_LOAD, "quick_load", Domain::WORLD},
    {Action::QUICK_SAVE, "quick_save", Domain::WORLD},
    {Action::LOAD, "load", Domain::WORLD},
    {Action::SAVE, "save", Domain::WORLD},
    {Action::DETONATE, "detonate", Domain::WORLD},
    {Action::USE_ENERGY_CUBE, "use_energy_cube", Domain::WORLD},
    {Action::USE_MEDKIT, "use_medkit", Domain::WORLD},
    {Action::USE_VIR_IMAGER, "use_vir_imager", Domain::WORLD},
    {Action::USE_MINE_DETECTOR, "use_mine_detector", Domain::WORLD},
    {Action::USE_RADIATION_SHIELD, "use_radiation_shield", Domain::WORLD},
    {Action::TARGETING_RETICLE, "targeting_reticle", Domain::WORLD},
    {Action::DATALINK, "datalink", Domain::WORLD},
    {Action::DROP_WEAPON, "drop_weapon", Domain::WORLD},
    {Action::SHOW_POSITION, "show_position", Domain::WORLD},
    {Action::CALIBRATE_JOYSTICK, "calibrate_joystick", Domain::WORLD},
    {Action::NEXT_SONG, "next_song", Domain::WORLD},
    {Action::VERSION, "version", Domain::WORLD},
    {Action::PREVIOUS_WEAPON, "previous_weapon", Domain::WORLD},
    {Action::NEXT_WEAPON, "next_weapon", Domain::WORLD},
    {Action::USE_ITEM, "use_item", Domain::WORLD},
    {Action::PREVIOUS_ITEM, "previous_item", Domain::WORLD},
    {Action::NEXT_ITEM, "next_item", Domain::WORLD},
    {Action::SELECT_ITEMS, "select_items", Domain::WORLD},
    {Action::GRAB_ITEMS, "grab_items", Domain::WORLD},
    {Action::SELECTION_PREVIOUS, "selection_previous", Domain::SELECTION},
    {Action::SELECTION_NEXT, "selection_next", Domain::SELECTION},
    {Action::SELECTION_CHOOSE, "selection_choose", Domain::SELECTION},
    {Action::SELECTION_USE, "selection_use", Domain::SELECTION},
    {Action::SELECTION_CANCEL, "selection_cancel", Domain::SELECTION},
    {Action::REWIND, "rewind", Domain::HOST},
    {Action::SAVE_STATE, "save_state", Domain::HOST},
    {Action::LOAD_STATE, "load_state", Domain::HOST},
    {Action::FULLSCREEN, "fullscreen", Domain::HOST},
    {Action::TAKE_OVER, "take_over", Domain::HOST},
    {Action::FAST_FORWARD, "fast_forward", Domain::HOST},
    {Action::PAUSE, "pause", Domain::HOST},
};
static_assert(sizeof(ACTION_INFOS) / sizeof(ACTION_INFOS[0]) == static_cast<std::size_t>(Action::COUNT),
              "an action without its name");

/* Whether each action's name and domain are at its place: the table is
 * read by the action's number. */
constexpr bool infosInOrder()
{
    for (std::size_t index = 0; index < std::size(ACTION_INFOS); index++) {
        if (ACTION_INFOS[index].action != static_cast<Action>(index)) {
            return false;
        }
    }
    return true;
}
static_assert(infosInOrder(), "the actions' names in the enum's order");

const std::size_t ACTIONS = static_cast<std::size_t>(Action::COUNT);

std::size_t indexOf(Action action)
{
    return static_cast<std::size_t>(action);
}

Domain domainOf(Action action)
{
    return ACTION_INFOS[indexOf(action)].domain;
}

/* The other devices' inputs' names, in the enum's order. */
const char *const DEVICE_NAMES[] = {
    nullptr,
    "mouse_left",
    "mouse_right",
    "mouse_left_double",
    "mouse_right_double",
    "mouse_middle",
    "mouse_x1",
    "mouse_x2",
    "wheel_up",
    "wheel_down",
    "mouse_move_left",
    "mouse_move_right",
    "pad_a",
    "pad_b",
    "pad_x",
    "pad_y",
    "pad_back",
    "pad_guide",
    "pad_start",
    "pad_ls",
    "pad_rs",
    "pad_lb",
    "pad_rb",
    "pad_up",
    "pad_down",
    "pad_left",
    "pad_right",
    "pad_misc",
    "pad_paddle1",
    "pad_paddle2",
    "pad_paddle3",
    "pad_paddle4",
    "pad_touchpad",
    "pad_lt",
    "pad_rt",
    "lstick_up",
    "lstick_down",
    "lstick_left",
    "lstick_right",
    "rstick_up",
    "rstick_down",
    "rstick_left",
    "rstick_right",
};
static_assert(sizeof(DEVICE_NAMES) / sizeof(DEVICE_NAMES[0]) == static_cast<std::size_t>(Device::COUNT),
              "a device's input without its name");

Device deviceNamed(const std::string &name)
{
    for (std::size_t index = 1; index < static_cast<std::size_t>(Device::COUNT); index++) {
        if (name == DEVICE_NAMES[index]) {
            return static_cast<Device>(index);
        }
    }
    return Device::NONE;
}

/* The inputs that come once, not held: the wheel's notches, the mouse's
 * motion. */
bool pulseDevice(Device device)
{
    return device == Device::WHEEL_UP || device == Device::WHEEL_DOWN || device == Device::MOUSE_MOVE_LEFT ||
           device == Device::MOUSE_MOVE_RIGHT;
}

/* The PC keyboard's keys by name: the scan code and whether E0h comes
 * before it (the keys window.cpp sends). A code of the gumps' keys has the
 * scan code without E0h: the first name of a scan code writes it. */
struct KeyName {
    const char *name;
    u8 scan;
    u8 extended;
};
const KeyName KEY_NAMES[] = {
    {"esc", 0x01, 0},
    {"1", 0x02, 0},
    {"2", 0x03, 0},
    {"3", 0x04, 0},
    {"4", 0x05, 0},
    {"5", 0x06, 0},
    {"6", 0x07, 0},
    {"7", 0x08, 0},
    {"8", 0x09, 0},
    {"9", 0x0a, 0},
    {"0", 0x0b, 0},
    {"minus", 0x0c, 0},
    {"equals", 0x0d, 0},
    {"backspace", 0x0e, 0},
    {"tab", 0x0f, 0},
    {"q", 0x10, 0},
    {"w", 0x11, 0},
    {"e", 0x12, 0},
    {"r", 0x13, 0},
    {"t", 0x14, 0},
    {"y", 0x15, 0},
    {"u", 0x16, 0},
    {"i", 0x17, 0},
    {"o", 0x18, 0},
    {"p", 0x19, 0},
    {"lbracket", 0x1a, 0},
    {"rbracket", 0x1b, 0},
    {"enter", 0x1c, 0},
    {"lctrl", 0x1d, 0},
    {"a", 0x1e, 0},
    {"s", 0x1f, 0},
    {"d", 0x20, 0},
    {"f", 0x21, 0},
    {"g", 0x22, 0},
    {"h", 0x23, 0},
    {"j", 0x24, 0},
    {"k", 0x25, 0},
    {"l", 0x26, 0},
    {"semicolon", 0x27, 0},
    {"apostrophe", 0x28, 0},
    {"grave", 0x29, 0},
    {"lshift", 0x2a, 0},
    {"backslash", 0x2b, 0},
    {"z", 0x2c, 0},
    {"x", 0x2d, 0},
    {"c", 0x2e, 0},
    {"v", 0x2f, 0},
    {"b", 0x30, 0},
    {"n", 0x31, 0},
    {"m", 0x32, 0},
    {"comma", 0x33, 0},
    {"period", 0x34, 0},
    {"slash", 0x35, 0},
    {"rshift", 0x36, 0},
    {"kpstar", 0x37, 0},
    {"lalt", 0x38, 0},
    {"space", 0x39, 0},
    {"capslock", 0x3a, 0},
    {"f1", 0x3b, 0},
    {"f2", 0x3c, 0},
    {"f3", 0x3d, 0},
    {"f4", 0x3e, 0},
    {"f5", 0x3f, 0},
    {"f6", 0x40, 0},
    {"f7", 0x41, 0},
    {"f8", 0x42, 0},
    {"f9", 0x43, 0},
    {"f10", 0x44, 0},
    {"numlock", 0x45, 0},
    {"scrolllock", 0x46, 0},
    {"home", 0x47, 1},
    {"up", 0x48, 1},
    {"pageup", 0x49, 1},
    {"kpminus", 0x4a, 0},
    {"left", 0x4b, 1},
    {"kp5", 0x4c, 0},
    {"right", 0x4d, 1},
    {"kpplus", 0x4e, 0},
    {"end", 0x4f, 1},
    {"down", 0x50, 1},
    {"pagedown", 0x51, 1},
    {"insert", 0x52, 1},
    {"delete", 0x53, 1},
    {"nonusbackslash", 0x56, 0},
    {"f11", 0x57, 0},
    {"f12", 0x58, 0},
    {"kp7", 0x47, 0},
    {"kp8", 0x48, 0},
    {"kp9", 0x49, 0},
    {"kp4", 0x4b, 0},
    {"kp6", 0x4d, 0},
    {"kp1", 0x4f, 0},
    {"kp2", 0x50, 0},
    {"kp3", 0x51, 0},
    {"kp0", 0x52, 0},
    {"kpdot", 0x53, 0},
    {"kpenter", 0x1c, 1},
    {"rctrl", 0x1d, 1},
    {"kpslash", 0x35, 1},
    {"ralt", 0x38, 1},
    {"lwin", 0x5b, 1},
    {"rwin", 0x5c, 1},
    {"menu", 0x5d, 1},
    /* The Pause key, for the window's host actions (platform/input.h): no
     * key sends E0h 45h. */
    {"pause", 0x45, 1},
};

const KeyName *keyNamed(const std::string &name)
{
    for (const KeyName &key : KEY_NAMES) {
        if (name == key.name) {
            return &key;
        }
    }
    return nullptr;
}

/* The name of a key; nullptr for one the table has not. */
const char *nameOfKey(u8 scan, u8 extended)
{
    for (const KeyName &key : KEY_NAMES) {
        if (key.scan == scan && key.extended == extended) {
            return key.name;
        }
    }
    return nullptr;
}

/* The first name of a scan code (a gump's key code has no E0h). */
const char *nameOfScan(u8 scan)
{
    for (const KeyName &key : KEY_NAMES) {
        if (key.scan == scan) {
            return key.name;
        }
    }
    return nullptr;
}

/* The modifiers an input needs held, and the modifier a mode is. */
const u8 MOD_SHIFT = 1;
const u8 MOD_CTRL = 2;
const u8 MOD_ALT = 4;

/* The shift state's bits of each modifier: all of them, and the ones a
 * modifier that is not held gets when a mode or an action needs it. */
const u16 SHIFT_BITS = KEY_LEFT_SHIFT | KEY_RIGHT_SHIFT;
const u16 CTRL_BITS = KEY_CONTROL | KEY_LEFT_CONTROL | KEY_RIGHT_CONTROL;
const u16 ALT_BITS = KEY_ALT | KEY_LEFT_ALT | KEY_RIGHT_ALT;

u16 modifierBits(u8 modifier)
{
    if (modifier == MOD_SHIFT) {
        return SHIFT_BITS;
    }
    if (modifier == MOD_CTRL) {
        return CTRL_BITS;
    }
    return ALT_BITS;
}

/* The shift state's bits of a set of modifiers. */
u16 modifiersBits(u8 modifiers)
{
    u16 bits = 0;
    for (const u8 modifier : {MOD_SHIFT, MOD_CTRL, MOD_ALT}) {
        if ((modifiers & modifier) != 0) {
            bits |= modifierBits(modifier);
        }
    }
    return bits;
}

u16 madeBits(u8 modifier)
{
    if (modifier == MOD_SHIFT) {
        return KEY_LEFT_SHIFT;
    }
    if (modifier == MOD_CTRL) {
        return KEY_CONTROL | KEY_LEFT_CONTROL;
    }
    return KEY_ALT | KEY_LEFT_ALT;
}

/* The modifiers held, as the controller's code asks (KeyEvent::isShifted,
 * isControlled, isAlted). */
u8 heldModifiers(u16 shift)
{
    u8 held = 0;
    if ((shift & (KEY_LEFT_SHIFT | KEY_RIGHT_SHIFT)) != 0) {
        held |= MOD_SHIFT;
    }
    if ((shift & (KEY_LEFT_CONTROL | KEY_RIGHT_CONTROL)) != 0) {
        held |= MOD_CTRL;
    }
    if ((shift & (KEY_LEFT_ALT | KEY_RIGHT_ALT)) != 0) {
        held |= MOD_ALT;
    }
    return held;
}

int modifierCount(u8 modifiers)
{
    return (((modifiers & MOD_SHIFT) != 0) ? 1 : 0) + (((modifiers & MOD_CTRL) != 0) ? 1 : 0) +
           (((modifiers & MOD_ALT) != 0) ? 1 : 0);
}

/* An input of enhanced.cfg (bindings.h). */
enum class InputKind : u8 { KEY, CODE, MODIFIER, LOCK, DEVICE };
enum class NumLock : u8 { ANY, ON, OFF };
struct Input {
    InputKind kind = InputKind::KEY;
    /* KEY: the key, the Num Lock state it needs, the modifiers held with it;
     * MODIFIER: the modifier (one of MOD_*). */
    u8 scan = 0;
    u8 extended = 0;
    NumLock numLock = NumLock::ANY;
    u8 modifiers = 0;
    /* CODE: the gumps' key code. */
    i16 code = 0;
    /* LOCK: the lock's bit of the shift state. */
    u16 lock = 0;
    /* DEVICE: the other device's input. */
    Device device = Device::NONE;
};

/* The most inputs an action has: the games' tables' most is 7
 * (SELECTION_NEXT), the key settings add. */
const std::size_t INPUTS_PER_ACTION = 16;

/* An action's inputs: bytes without the heap, so that the bound ones are a
 * setting outside the arena, which a block of the heap would not be. */
class InputList {
public:
    [[nodiscard]] const Input *begin() const { return inputs_.data(); }
    [[nodiscard]] const Input *end() const { return inputs_.data() + count_; }
    [[nodiscard]] std::size_t size() const { return count_; }
    /* Adds input at the end; false when the list is full. */
    bool add(const Input &input)
    {
        if (count_ == INPUTS_PER_ACTION) {
            return false;
        }
        inputs_[count_] = input;
        count_++;
        return true;
    }
    /* Takes out the inputs test is true of, the others in their order. */
    template <class Test> void removeIf(Test test)
    {
        std::size_t kept = 0;
        for (std::size_t i = 0; i < count_; i++) {
            if (!test(inputs_[i])) {
                inputs_[kept] = inputs_[i];
                kept++;
            }
        }
        count_ = kept;
    }
    void clear() { count_ = 0; }

private:
    std::array<Input, INPUTS_PER_ACTION> inputs_{};
    std::size_t count_ = 0;
};

/* A modifier's name as a prefix (shift+, ctrl+, alt+) or a mode's input. */
u8 modifierNamed(const std::string &name)
{
    if (name == "shift") {
        return MOD_SHIFT;
    }
    if (name == "ctrl") {
        return MOD_CTRL;
    }
    if (name == "alt") {
        return MOD_ALT;
    }
    return 0;
}

u16 lockNamed(const std::string &name)
{
    if (name == "capslock") {
        return KEY_CAPS_LOCK;
    }
    if (name == "numlock") {
        return KEY_NUM_LOCK;
    }
    if (name == "scrolllock") {
        return KEY_SCROLL_LOCK;
    }
    return 0;
}

/* The prefixes of a text and what is left: shift+ctrl+x gives MOD_SHIFT |
 * MOD_CTRL and x. */
u8 takePrefixes(std::string &text)
{
    u8 modifiers = 0;
    for (;;) {
        const std::size_t plus = text.find('+');
        if (plus == std::string::npos || plus == 0 || plus + 1 == text.size()) {
            return modifiers;
        }
        const u8 modifier = modifierNamed(text.substr(0, plus));
        if (modifier == 0U) {
            return modifiers;
        }
        modifiers |= modifier;
        text = text.substr(plus + 1);
    }
}

bool parseKey(std::string text, Input *input)
{
    input->kind = InputKind::KEY;
    input->modifiers = takePrefixes(text);
    input->numLock = NumLock::ANY;
    const std::size_t at = text.find('@');
    if (at != std::string::npos) {
        const std::string state = text.substr(at + 1);
        if (state == "numlock") {
            input->numLock = NumLock::ON;
        } else if (state == "nonumlock") {
            input->numLock = NumLock::OFF;
        } else {
            return false;
        }
        text = text.substr(0, at);
    }
    const KeyName *key = keyNamed(text);
    if (key == nullptr) {
        return false;
    }
    input->scan = key->scan;
    input->extended = key->extended;
    return true;
}

/* The ASCII codes that have names (a space ends an input in enhanced.cfg). */
struct CodeName {
    const char *name;
    i16 code;
};
const CodeName CODE_NAMES[] = {
    {"esc", ASCII_ESCAPE}, {"enter", '\r'}, {"tab", '\t'}, {"backspace", '\b'}, {"space", ' '},
};

bool parseCode(std::string text, Input *input)
{
    input->kind = InputKind::CODE;
    if (text.size() == 1 && text[0] > ' ' && text[0] < 0x7f) {
        input->code = static_cast<i16>(static_cast<u8>(text[0]));
        return true;
    }
    for (const CodeName &named : CODE_NAMES) {
        if (text == named.name) {
            input->code = named.code;
            return true;
        }
    }
    const u8 modifiers = takePrefixes(text);
    const KeyName *key = keyNamed(text);
    if ((key == nullptr) || modifierCount(modifiers) > 1) {
        return false;
    }
    i16 kind = KEYCODE_SCAN;
    if (modifiers == MOD_SHIFT) {
        kind = KEYCODE_SHIFT;
    } else if (modifiers == MOD_CTRL) {
        kind = KEYCODE_CONTROL;
    } else if (modifiers == MOD_ALT) {
        kind = KEYCODE_ALT;
    }
    input->code = static_cast<i16>(kind | key->scan);
    return true;
}

bool parseMode(const std::string &text, Input *input)
{
    const u8 modifier = modifierNamed(text);
    if (modifier != 0U) {
        input->kind = InputKind::MODIFIER;
        input->modifiers = modifier;
        return true;
    }
    const u16 lock = lockNamed(text);
    if (lock != 0U) {
        input->kind = InputKind::LOCK;
        input->lock = lock;
        return true;
    }
    return parseKey(text, input) && input->modifiers == 0 && input->numLock == NumLock::ANY;
}

/* A device's button the window can take for a host action
 * (platform/input.h): the mouse's middle and side buttons, the gamepad's
 * buttons; its event's kind and which. Not the left and right buttons (the
 * mouse driver's), the wheel, the motion, the triggers and the sticks. */
bool hostButtonOf(Device device, DeviceKind *kind, u8 *which)
{
    const auto at = static_cast<int>(device);
    if (at >= static_cast<int>(Device::MOUSE_MIDDLE) && at <= static_cast<int>(Device::MOUSE_X2)) {
        *kind = DeviceKind::MOUSE_BUTTON;
        *which = static_cast<u8>(DEVICE_MOUSE_MIDDLE + (at - static_cast<int>(Device::MOUSE_MIDDLE)));
        return true;
    }
    if (at >= static_cast<int>(Device::PAD_A) && at < static_cast<int>(Device::PAD_A) + DEVICE_PAD_BUTTONS) {
        *kind = DeviceKind::PAD_BUTTON;
        *which = static_cast<u8>(at - static_cast<int>(Device::PAD_A));
        return true;
    }
    return false;
}

bool parseInput(Domain domain, const std::string &text, Input *input)
{
    const Device device = deviceNamed(text);
    DeviceKind kind;
    u8 which = 0;
    if (device != Device::NONE && domain == Domain::HOST && !hostButtonOf(device, &kind, &which)) {
        /* The window takes the host actions' keys and buttons alone. */
        return false;
    }
    if (device != Device::NONE) {
        input->kind = InputKind::DEVICE;
        input->device = device;
        return domain != Domain::MODE || !pulseDevice(device);
    }
    if (domain == Domain::WORLD || domain == Domain::SELECTION) {
        return parseCode(text, input);
    }
    if (domain == Domain::MODE) {
        return parseMode(text, input);
    }
    return parseKey(text, input);
}

std::string prefixes(u8 modifiers)
{
    std::string text;
    if ((modifiers & MOD_SHIFT) != 0) {
        text += "shift+";
    }
    if ((modifiers & MOD_CTRL) != 0) {
        text += "ctrl+";
    }
    if ((modifiers & MOD_ALT) != 0) {
        text += "alt+";
    }
    return text;
}

std::string inputText(const Input &input)
{
    if (input.kind == InputKind::DEVICE) {
        return DEVICE_NAMES[static_cast<std::size_t>(input.device)];
    }
    if (input.kind == InputKind::MODIFIER) {
        return prefixes(input.modifiers).substr(0, prefixes(input.modifiers).size() - 1);
    }
    if (input.kind == InputKind::LOCK) {
        if (input.lock == KEY_CAPS_LOCK) {
            return "capslock";
        }
        if (input.lock == KEY_NUM_LOCK) {
            return "numlock";
        }
        return "scrolllock";
    }
    if (input.kind == InputKind::CODE) {
        for (const CodeName &named : CODE_NAMES) {
            if (input.code == named.code) {
                return named.name;
            }
        }
        if (input.code > ' ' && input.code < 0x7f) {
            return std::string(1, static_cast<char>(input.code));
        }
        u8 modifier = 0;
        if ((input.code & KEYCODE_SHIFT) != 0) {
            modifier = MOD_SHIFT;
        } else if ((input.code & KEYCODE_CONTROL) != 0) {
            modifier = MOD_CTRL;
        } else if ((input.code & KEYCODE_ALT) != 0) {
            modifier = MOD_ALT;
        }
        const char *name = nameOfScan(static_cast<u8>(input.code & 0xff));
        return prefixes(modifier) + ((name != nullptr) ? name : "?");
    }
    const char *name = nameOfKey(input.scan, input.extended);
    std::string text = prefixes(input.modifiers) + ((name != nullptr) ? name : "?");
    if (input.numLock == NumLock::ON) {
        text += "@numlock";
    } else if (input.numLock == NumLock::OFF) {
        text += "@nonumlock";
    }
    return text;
}

/* The inputs of a text, separated by spaces; false when one is not an
 * input of the domain or there are more than an action has. */
bool parseInputs(Domain domain, const char *text, InputList *inputs)
{
    InputList read;
    const std::string all(text);
    std::size_t at = 0;
    while (at < all.size()) {
        std::size_t end = all.find_first_of(" \t", at);
        if (end == std::string::npos) {
            end = all.size();
        }
        if (end > at) {
            Input input;
            if (!parseInput(domain, all.substr(at, end - at), &input) || !read.add(input)) {
                return false;
            }
        }
        at = end + 1;
    }
    *inputs = read;
    return true;
}

/* The original's inputs, the modern layout's (for the actions it changes)
 * and the bound ones, by action; read from the game's tables at the first
 * use, the program's start. The bound ones and the preset are the player's
 * settings, outside the arena; the tables' are the same in every snapshot. */
InputList (&originals)[ACTIONS] = gameGlobal<InputList[ACTIONS]>();
InputList (&moderns)[ACTIONS] = gameGlobal<InputList[ACTIONS]>();
bool (&modernized)[ACTIONS] = gameGlobal<bool[ACTIONS]>();
InputList (&bound)[ACTIONS] = gameSetting<InputList[ACTIONS]>();
bool (&present)[ACTIONS] = gameGlobal<bool[ACTIONS]>();
bool &loaded = gameGlobal<bool>(false);
KeyPreset &preset = gameSetting<KeyPreset>(KeyPreset::ORIGINAL);
/* Whether the mouse rolls (mouseRoll), the player's setting. */
bool &mouseRolls = gameSetting<bool>(true);
/* The key settings' wait (startCapture): the action, and a modifier key
 * pressed while it waits, which let go alone is the input. */
Action &waiting = gameGlobal<Action>(Action::NONE);
bool &modifierDown = gameGlobal<bool>(false);
u8 &modifierScan = gameGlobal<u8>(0);
u8 &modifierExtended = gameGlobal<u8>(0);

/* The host actions' keys to the window (platform/input.h), as the
 * bindings have them now; none while the key settings wait for an input,
 * which may be one of them. */
void publishHostKeys()
{
    const Action hosts[HOST_ACTIONS] = {Action::REWIND,    Action::SAVE_STATE,   Action::LOAD_STATE, Action::FULLSCREEN,
                                        Action::TAKE_OVER, Action::FAST_FORWARD, Action::PAUSE};
    for (std::size_t i = 0; i < HOST_ACTIONS; i++) {
        std::vector<HostKey> keys;
        for (const Input &input : bound[indexOf(hosts[i])]) {
            if (waiting != Action::NONE) {
                continue;
            }
            DeviceKind kind;
            u8 which = 0;
            if (input.kind == InputKind::KEY) {
                keys.push_back({input.scan, input.extended != 0U, input.modifiers});
            } else if (input.kind == InputKind::DEVICE && hostButtonOf(input.device, &kind, &which)) {
                keys.push_back({0, false, 0, true, kind, which});
            }
        }
        inputHostKeys(static_cast<HostAction>(i), keys.data(), keys.size());
    }
}

void load()
{
    if (loaded) {
        return;
    }
    loaded = true;
    for (std::size_t i = 0; i < GAME_ACTION_DEFAULT_COUNT; i++) {
        const ActionDefault &entry = GAME_ACTION_DEFAULTS[i];
        const std::size_t index = indexOf(entry.action);
        if (!parseInputs(domainOf(entry.action), entry.inputs, &originals[index])) {
            halt();
        }
        bound[index] = originals[index];
        present[index] = true;
    }
    for (std::size_t i = 0; i < GAME_MODERN_BINDING_COUNT; i++) {
        const ActionDefault &entry = GAME_MODERN_BINDINGS[i];
        const std::size_t index = indexOf(entry.action);
        if (!present[index] || !parseInputs(domainOf(entry.action), entry.inputs, &moderns[index])) {
            halt();
        }
        modernized[index] = true;
    }
    publishHostKeys();
}

bool numLockFits(NumLock need, u16 shift)
{
    if (need == NumLock::ON) {
        return (shift & KEY_NUM_LOCK) != 0;
    }
    if (need == NumLock::OFF) {
        return (shift & KEY_NUM_LOCK) == 0;
    }
    return true;
}

/* Whether a key input gives the key: the key, its Num Lock state and the
 * modifiers it needs held. */
bool keyFits(const Input &input, const KeyEvent &key, u8 held)
{
    return input.kind == InputKind::KEY && input.scan == key.scan && input.extended == key.extended &&
           numLockFits(input.numLock, key.shift) && (input.modifiers & ~held) == 0;
}

/* The controller's action whose inputs (the bound ones or the original's)
 * give the key, the one that needs most modifiers; its input in found. */
Action keyAction(const InputList *table, const KeyEvent &key, const Input **found)
{
    const u8 held = heldModifiers(key.shift);
    Action best = Action::NONE;
    int bestModifiers = -1;
    for (std::size_t index = 1; index < ACTIONS; index++) {
        if (ACTION_INFOS[index].domain != Domain::CONTROL) {
            continue;
        }
        for (const Input &input : table[index]) {
            if (keyFits(input, key, held) && modifierCount(input.modifiers) > bestModifiers) {
                best = ACTION_INFOS[index].action;
                bestModifiers = modifierCount(input.modifiers);
                *found = &input;
            }
        }
    }
    return best;
}

/* Whether the key is one of a controller's action's original inputs. */
bool reservedKey(u8 scan, u8 extended)
{
    for (std::size_t index = 1; index < ACTIONS; index++) {
        if (ACTION_INFOS[index].domain != Domain::CONTROL) {
            continue;
        }
        for (const Input &input : originals[index]) {
            if (input.kind == InputKind::KEY && input.scan == scan && input.extended == extended) {
                return true;
            }
        }
    }
    return false;
}

/* The run's lock turned round by its inputs that are not a lock (a key, a
 * device's), at each press. */
bool &runLocked = gameGlobal<bool>(false);

/* Whether the mouse's left or right button is down as the driver last told
 * it: before the world's gump gets its press, which the mouse's process
 * holds back while a second click may come (mousectl.h's PRESS_DELAY). */
bool mouseButtonDown(Device device)
{
    if (mouseCtl == nullptr) {
        return false;
    }
    if (device == Device::MOUSE_LEFT) {
        return (mouseCtl->buttons & MOUSE_BUTTON_LEFT) != 0;
    }
    if (device == Device::MOUSE_RIGHT) {
        return (mouseCtl->buttons & MOUSE_BUTTON_RIGHT) != 0;
    }
    return false;
}

/* Whether a mode is held: one of its inputs is (a modifier, a lock that is
 * on, a key or a device's input that is down; the mouse's buttons from the
 * press on, mouseButtonDown). The run's lock is its locks and modifiers
 * turned round by runLocked. */
bool modeHeld(Action mode, u16 shift)
{
    bool held = false;
    for (const Input &input : bound[indexOf(mode)]) {
        if (input.kind == InputKind::MODIFIER && ((shift & modifierBits(input.modifiers)) != 0)) {
            held = true;
        }
        if (input.kind == InputKind::LOCK && ((shift & input.lock) != 0)) {
            held = true;
        }
        if (mode == Action::RUN_LOCK) {
            continue;
        }
        if (input.kind == InputKind::KEY && (keyRepeats(input.scan, input.extended) != 0U)) {
            held = true;
        }
        if (input.kind == InputKind::DEVICE && (deviceDown(input.device) || mouseButtonDown(input.device))) {
            held = true;
        }
    }
    if (mode == Action::RUN_LOCK) {
        return held != runLocked;
    }
    return held;
}

/* Whether the key pressed is an input of the run's lock that is a key. */
bool runLockKey(const KeyEvent &key)
{
    for (const Input &input : bound[indexOf(Action::RUN_LOCK)]) {
        if (input.kind == InputKind::KEY && input.scan == key.scan && input.extended == key.extended) {
            return true;
        }
    }
    return false;
}

/* Whether the mode is bound to its own modifier (the original's). */
bool modeIsModifier(Action mode, u8 modifier)
{
    for (const Input &input : bound[indexOf(mode)]) {
        if (input.kind == InputKind::MODIFIER && input.modifiers == modifier) {
            return true;
        }
    }
    return false;
}

/* The shift state for the controller: the modifiers' bits are the modes
 * held (run Shift, roll Ctrl, the combat step Alt; Caps Lock the run's lock),
 * as the shift state had them when the mode is its modifier, and the
 * modifiers the action needs; the Num Lock bit as the action's input needs
 * it. The other bits stay. */
u16 controlShift(u16 shift, u8 needed, NumLock numLock)
{
    struct ModeBits {
        Action mode;
        u8 modifier;
    };
    const ModeBits modes[] = {
        {Action::RUN, MOD_SHIFT},
        {Action::ROLL, MOD_CTRL},
        {Action::COMBAT_STEP, MOD_ALT},
    };
    u16 made = shift & static_cast<u16>(~(SHIFT_BITS | CTRL_BITS | ALT_BITS | KEY_CAPS_LOCK));
    for (const ModeBits &entry : modes) {
        const u16 bits = modifierBits(entry.modifier);
        const bool held = modeHeld(entry.mode, shift) || ((needed & entry.modifier) != 0);
        if (!held) {
            continue;
        }
        if (((shift & bits) != 0) && (modeIsModifier(entry.mode, entry.modifier) || ((needed & entry.modifier) != 0))) {
            made |= shift & bits;
        } else {
            made |= madeBits(entry.modifier);
        }
    }
    if (modeHeld(Action::RUN_LOCK, shift)) {
        made |= KEY_CAPS_LOCK;
    }
    if (numLock == NumLock::ON) {
        made |= KEY_NUM_LOCK;
    } else if (numLock == NumLock::OFF) {
        made &= static_cast<u16>(~KEY_NUM_LOCK);
    }
    return made;
}

/* The original key of the action that the key is, if it is one of them;
 * else the action's first key; nullptr for an action without one. */
const Input *ownInput(Action action, const KeyEvent &key)
{
    const u8 held = heldModifiers(key.shift);
    for (const Input &input : originals[indexOf(action)]) {
        if (keyFits(input, key, held)) {
            return &input;
        }
    }
    for (const Input &input : originals[indexOf(action)]) {
        if (input.kind == InputKind::KEY) {
            return &input;
        }
    }
    return nullptr;
}

/* The code an input of a world's or a selection's action gives: the
 * action's original code that it is, else the action's first. */
bool codeOf(Domain domain, i16 code, i16 *result)
{
    for (std::size_t index = 1; index < ACTIONS; index++) {
        if (ACTION_INFOS[index].domain != domain) {
            continue;
        }
        for (const Input &input : bound[index]) {
            if (input.kind != InputKind::CODE || input.code != code) {
                continue;
            }
            *result = actionCode(ACTION_INFOS[index].action);
            for (const Input &original : originals[index]) {
                if (original.kind == InputKind::CODE && original.code == code) {
                    *result = code;
                }
            }
            return true;
        }
    }
    return false;
}

/* The world's or the selection's action of an original code. */
Action codeAction(Domain domain, i16 code)
{
    for (std::size_t index = 1; index < ACTIONS; index++) {
        if (ACTION_INFOS[index].domain != domain) {
            continue;
        }
        for (const Input &input : originals[index]) {
            if (input.kind == InputKind::CODE && input.code == code) {
                return ACTION_INFOS[index].action;
            }
        }
    }
    return Action::NONE;
}

}

const char *actionName(Action action)
{
    return ACTION_INFOS[indexOf(action)].name;
}

Action actionNamed(const char *name)
{
    for (std::size_t index = 1; index < ACTIONS; index++) {
        if (std::strcmp(ACTION_INFOS[index].name, name) == 0) {
            return ACTION_INFOS[index].action;
        }
    }
    return Action::NONE;
}

bool debugAction(Action action)
{
    return action == Action::REWIND || action == Action::SAVE_STATE || action == Action::LOAD_STATE ||
           action == Action::TAKE_OVER || action == Action::FAST_FORWARD || action == Action::PAUSE;
}

bool gameHasAction(Action action)
{
    load();
    return present[indexOf(action)];
}

bool bindAction(Action action, const char *text)
{
    load();
    if (!present[indexOf(action)]) {
        return false;
    }
    InputList inputs;
    if (!parseInputs(domainOf(action), text, &inputs)) {
        return false;
    }
    bound[indexOf(action)] = inputs;
    if (domainOf(action) == Domain::HOST) {
        publishHostKeys();
    }
    return true;
}

std::string boundText(Action action)
{
    load();
    std::string text;
    for (const Input &input : bound[indexOf(action)]) {
        if (!text.empty()) {
            text += ' ';
        }
        text += inputText(input);
    }
    return text;
}

void setKeyPreset(KeyPreset chosen)
{
    load();
    preset = chosen;
    for (std::size_t index = 0; index < ACTIONS; index++) {
        if (preset == KeyPreset::MODERN && modernized[index]) {
            bound[index] = moderns[index];
        } else {
            bound[index] = originals[index];
        }
    }
    publishHostKeys();
}

void loadBindings()
{
    load();
}

KeyPreset keyPreset()
{
    return preset;
}

const char *keyPresetName(KeyPreset named)
{
    return named == KeyPreset::MODERN ? "modern" : "original";
}

bool keyPresetNamed(const char *name, KeyPreset *named)
{
    if (std::strcmp(name, "original") == 0) {
        *named = KeyPreset::ORIGINAL;
        return true;
    }
    if (std::strcmp(name, "modern") == 0) {
        *named = KeyPreset::MODERN;
        return true;
    }
    return false;
}

void resetBindings()
{
    setKeyPreset(preset);
}

std::string bindingsText()
{
    std::string text = std::string("keys=") + keyPresetName(keyPreset()) + "\r\n";
    text += std::string("mouse_roll=") + (mouseRolls ? "1" : "0") + "\r\n";
    for (std::size_t entry = 0; entry < GAME_ACTION_DEFAULT_COUNT; entry++) {
        const Action action = GAME_ACTION_DEFAULTS[entry].action;
        text += std::string("bind_") + actionName(action) + "=" + boundText(action) + "\r\n";
    }
    return text;
}

namespace {

/* The text without the spaces, tabs and returns at its ends. */
std::string trimmed(const std::string &text)
{
    const std::size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

std::string lowered(std::string text)
{
    for (char &c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

}

std::vector<std::string> readBindingsText(const std::string &text)
{
    std::vector<std::string> unread;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        const std::string line = trimmed(text.substr(start, end - start));
        start = end + 1;
        const std::size_t equals = line.find('=');
        if (line.empty() || line[0] == '#' || line[0] == '/' || equals == std::string::npos) {
            continue;
        }
        const std::string key = lowered(trimmed(line.substr(0, equals)));
        const std::string value = trimmed(line.substr(equals + 1));
        if (key == "keys") {
            KeyPreset named;
            if (keyPresetNamed(lowered(value).c_str(), &named)) {
                setKeyPreset(named);
            } else {
                unread.push_back(line);
            }
        } else if (key == "mouse_roll") {
            if (value == "1" || value == "0") {
                mouseRolls = value == "1";
            } else {
                unread.push_back(line);
            }
        } else if (key.starts_with("bind_")) {
            if (!bindAction(actionNamed(key.c_str() + 5), value.c_str())) {
                unread.push_back(line);
            }
        }
    }
    return unread;
}

bool controlKey(const KeyEvent &pressed, ControlKey *result)
{
    load();
    if ((pressed.pressed != 0U) && pressed.repeat == 1 && runLockKey(pressed)) {
        runLocked = !runLocked;
    }
    const Input *input = nullptr;
    const Action action = keyAction(bound, pressed, &input);
    if (action != Action::NONE) {
        const Input *own = ownInput(action, pressed);
        result->action = action;
        result->key = pressed;
        /* The modifiers the input needs that the action's own key does not
         * (Ctrl+A's Ctrl, the roll to the left's key Insert) are the
         * input's: they hold no mode. */
        const u8 ownModifiers = (own != nullptr) ? own->modifiers : 0;
        const u16 shift = pressed.shift & static_cast<u16>(~modifiersBits(input->modifiers & ~ownModifiers));
        if (own != nullptr) {
            result->key.scan = own->scan;
            result->key.extended = own->extended;
            result->key.shift = controlShift(shift, own->modifiers, own->numLock);
        } else {
            result->key.shift = controlShift(shift, 0, NumLock::ANY);
        }
        result->key.translate();
        return true;
    }
    if (reservedKey(pressed.scan, pressed.extended)) {
        return false;
    }
    result->action = Action::NONE;
    result->key = pressed;
    result->key.shift = controlShift(pressed.shift, 0, NumLock::ANY);
    result->key.translate();
    return true;
}

Action controlAction(const KeyEvent *key)
{
    load();
    const Input *input = nullptr;
    return keyAction(originals, *key, &input);
}

bool keyMode(const KeyEvent *key, Action mode)
{
    if (mode == Action::RUN) {
        return (key->shift & (KEY_LEFT_SHIFT | KEY_RIGHT_SHIFT)) != 0;
    }
    if (mode == Action::RUN_LOCK) {
        return (key->shift & KEY_CAPS_LOCK) != 0;
    }
    if (mode == Action::COMBAT_STEP) {
        return (key->shift & (KEY_LEFT_ALT | KEY_RIGHT_ALT)) != 0;
    }
    if (mode == Action::ROLL) {
        return (key->shift & (KEY_LEFT_CONTROL | KEY_RIGHT_CONTROL)) != 0;
    }
    return false;
}

u16 modeShift(u16 shift)
{
    load();
    return controlShift(shift, 0, NumLock::ANY);
}

void setMouseRoll(bool rolls)
{
    mouseRolls = rolls;
}

bool mouseRoll()
{
    return mouseRolls;
}

u16 deviceTurnShift(u16 shift)
{
    const u16 held = modeShift(shift);
    /* Without the roll's and the combat step's bits, Ctrl's and Alt's (the
     * run's are Shift's and Caps Lock: controlShift). */
    return mouseRolls ? held : static_cast<u16>(held & ~(CTRL_BITS | ALT_BITS));
}

u16 actionHeld(Action action)
{
    load();
    u16 most = 0;
    for (const Input &input : bound[indexOf(action)]) {
        u16 repeats = 0;
        if (input.kind == InputKind::KEY) {
            repeats = keyRepeats(input.scan, input.extended);
        } else if (input.kind == InputKind::DEVICE) {
            repeats = deviceRepeats(input.device, action);
        }
        if (repeats > most) {
            most = repeats;
        }
    }
    return most;
}

bool worldCode(i16 code, i16 *result)
{
    load();
    if (codeOf(Domain::WORLD, code, result)) {
        return true;
    }
    if (worldAction(code) != Action::NONE) {
        return false;
    }
    *result = code;
    return true;
}

Action worldAction(i16 code)
{
    load();
    return codeAction(Domain::WORLD, code);
}

bool selectionCode(i16 code, i16 *result)
{
    load();
    if (codeOf(Domain::SELECTION, code, result)) {
        return true;
    }
    if (selectionAction(code) != Action::NONE) {
        return false;
    }
    *result = code;
    return true;
}

Action selectionAction(i16 code)
{
    load();
    return codeAction(Domain::SELECTION, code);
}

const char *deviceName(Device device)
{
    return DEVICE_NAMES[static_cast<std::size_t>(device)];
}

Action deviceAction(Device device, bool selecting)
{
    load();
    for (std::size_t index = 1; index < ACTIONS; index++) {
        const Domain domain = ACTION_INFOS[index].domain;
        if (domain == Domain::MODE || (domain == Domain::SELECTION) != selecting) {
            continue;
        }
        for (const Input &input : bound[index]) {
            if (input.kind == InputKind::DEVICE && input.device == device) {
                return ACTION_INFOS[index].action;
            }
        }
    }
    return Action::NONE;
}

bool runLockInput(Device device)
{
    load();
    for (const Input &input : bound[indexOf(Action::RUN_LOCK)]) {
        if (input.kind == InputKind::DEVICE && input.device == device) {
            return true;
        }
    }
    return false;
}

void toggleRunLock()
{
    runLocked = !runLocked;
}

KeyEvent actionKey(Action action, u8 pressed, u16 repeat)
{
    load();
    KeyEvent key;
    key.clear();
    key.pressed = pressed;
    key.repeat = repeat;
    key.time = 0;
    const Input *own = nullptr;
    for (const Input &input : originals[indexOf(action)]) {
        if (input.kind == InputKind::KEY) {
            own = &input;
            break;
        }
    }
    if (own != nullptr) {
        key.scan = own->scan;
        key.extended = own->extended;
        key.shift = controlShift(keyShift, own->modifiers, own->numLock);
    } else {
        key.shift = controlShift(keyShift, 0, NumLock::ANY);
    }
    key.translate();
    return key;
}

i16 actionCode(Action action)
{
    load();
    for (const Input &input : originals[indexOf(action)]) {
        if (input.kind == InputKind::CODE) {
            return input.code;
        }
    }
    return 0;
}

bool controlActionOf(Action action)
{
    return action != Action::NONE && domainOf(action) == Domain::CONTROL;
}

bool codeActionOf(Action action)
{
    return action != Action::NONE && (domainOf(action) == Domain::WORLD || domainOf(action) == Domain::SELECTION);
}

namespace {

bool sameInput(const Input &a, const Input &b)
{
    return a.kind == b.kind && a.scan == b.scan && a.extended == b.extended && a.numLock == b.numLock &&
           a.modifiers == b.modifiers && a.code == b.code && a.lock == b.lock && a.device == b.device;
}

/* Where an input gives its action: the keys of the controller and the
 * window alike (the window takes its own first), the codes of the world's
 * gump, of the selection, the modes' keys; the other devices' inputs in the
 * world (the controller's, the world's and the modes' actions) or in the
 * selection, or the window's buttons, which it takes before both. */
enum class Reach : u8 {
    KEYS,
    WORLD_CODES,
    SELECTION_CODES,
    MODE_KEYS,
    WORLD_DEVICES,
    SELECTION_DEVICES,
    HOST_DEVICES,
    NONE
};

/* Whether two reaches give one input to two actions: the same, or the
 * window's buttons and the game's. */
bool reachesMeet(Reach one, Reach other)
{
    if (one == Reach::NONE || other == Reach::NONE) {
        return false;
    }
    const auto devices = [](Reach reach) {
        return reach == Reach::WORLD_DEVICES || reach == Reach::SELECTION_DEVICES || reach == Reach::HOST_DEVICES;
    };
    if (one == Reach::HOST_DEVICES || other == Reach::HOST_DEVICES) {
        return devices(one) && devices(other);
    }
    return one == other;
}

Reach reachOf(Domain domain, const Input &input)
{
    if (input.kind == InputKind::DEVICE) {
        if (domain == Domain::SELECTION) {
            return Reach::SELECTION_DEVICES;
        }
        if (domain == Domain::HOST) {
            return Reach::HOST_DEVICES;
        }
        return Reach::WORLD_DEVICES;
    }
    switch (domain) {
    case Domain::CONTROL:
    case Domain::HOST:
        return Reach::KEYS;
    case Domain::WORLD:
        return Reach::WORLD_CODES;
    case Domain::SELECTION:
        return Reach::SELECTION_CODES;
    case Domain::MODE:
        return Reach::MODE_KEYS;
    }
    return Reach::NONE;
}

/* Whether a code is a character's, which has two cases. */
bool characterCode(i16 code)
{
    return code > ' ' && code < 0x7f;
}

/* The code text of the world's and the selection's actions for a key
 * (keyInputText). */
std::string codeInputText(u8 scan, u8 extended, u16 shift)
{
    KeyEvent key;
    key.clear();
    key.scan = scan;
    key.extended = extended;
    if ((shift & (CTRL_BITS | ALT_BITS)) != 0) {
        key.shift = shift;
        key.translate();
        return inputText(Input{InputKind::CODE, 0, 0, NumLock::ANY, 0, keyCode(key), 0, Device::NONE});
    }
    key.shift = static_cast<u16>(shift & ~(SHIFT_BITS | KEY_CAPS_LOCK));
    key.translate();
    const i16 lower = keyCode(key);
    key.shift = static_cast<u16>((shift & ~KEY_CAPS_LOCK) | KEY_LEFT_SHIFT);
    key.translate();
    const i16 upper = keyCode(key);
    if (!characterCode(lower) && lower != ' ') {
        /* A key without a character: its code as it was pressed (shift+f4
         * with Shift held). */
        key.shift = shift;
        key.translate();
        return inputText(Input{InputKind::CODE, 0, 0, NumLock::ANY, 0, keyCode(key), 0, Device::NONE});
    }
    std::string text = inputText(Input{InputKind::CODE, 0, 0, NumLock::ANY, 0, lower, 0, Device::NONE});
    if (characterCode(upper) && upper != lower) {
        text += " " + inputText(Input{InputKind::CODE, 0, 0, NumLock::ANY, 0, upper, 0, Device::NONE});
    }
    return text;
}

/* Whether a key is one of the modifiers (either Shift, Ctrl or Alt). */
bool modifierKey(u8 scan)
{
    return scan == SCAN_LEFT_SHIFT || scan == SCAN_RIGHT_SHIFT || scan == SCAN_CONTROL || scan == SCAN_ALT;
}

/* The input a modifier key let go alone gives the action: a mode's modifier,
 * the controller's and the window's key; empty for the codes. */
std::string modifierInputText(Action action, u8 scan, u8 extended)
{
    const Domain domain = domainOf(action);
    if (domain == Domain::MODE) {
        if (scan == SCAN_CONTROL) {
            return "ctrl";
        }
        if (scan == SCAN_ALT) {
            return "alt";
        }
        return "shift";
    }
    if (domain == Domain::CONTROL || domain == Domain::HOST) {
        const char *name = nameOfKey(scan, extended);
        return (name != nullptr) ? name : "";
    }
    return "";
}

/* The wait ends with the input text, when it is one of the action's
 * domain; else it goes on. */
void finishCapture(const std::string &text)
{
    InputList inputs;
    if (text.empty() || !parseInputs(domainOf(waiting), text.c_str(), &inputs)) {
        return;
    }
    const Action action = waiting;
    stopCapture();
    addInputs(action, text.c_str());
}

}

bool addInputs(Action action, const char *text)
{
    load();
    const std::size_t index = indexOf(action);
    InputList inputs;
    if (!present[index] || !parseInputs(domainOf(action), text, &inputs)) {
        return false;
    }
    /* None added when they do not all fit. */
    InputList &own = bound[index];
    std::size_t adding = 0;
    for (const Input &input : inputs) {
        if (std::ranges::none_of(own, [&input](const Input &held) { return sameInput(held, input); })) {
            adding++;
        }
    }
    if (own.size() + adding > INPUTS_PER_ACTION) {
        return false;
    }
    for (const Input &input : inputs) {
        const Reach reach = reachOf(domainOf(action), input);
        for (std::size_t other = 1; other < ACTIONS; other++) {
            if (other == index || !reachesMeet(reach, reachOf(ACTION_INFOS[other].domain, input))) {
                continue;
            }
            bound[other].removeIf([&input](const Input &held) { return sameInput(held, input); });
        }
        if (std::ranges::none_of(own, [&input](const Input &held) { return sameInput(held, input); })) {
            own.add(input);
        }
    }
    publishHostKeys();
    return true;
}

void clearInputs(Action action)
{
    load();
    bound[indexOf(action)].clear();
    publishHostKeys();
}

void startCapture(Action action)
{
    load();
    waiting = action;
    modifierDown = false;
    publishHostKeys();
}

void stopCapture()
{
    waiting = Action::NONE;
    modifierDown = false;
    publishHostKeys();
}

Action capturingAction()
{
    return waiting;
}

bool captureKey(const KeyEvent &key)
{
    if (waiting == Action::NONE) {
        return false;
    }
    if (key.pressed == 0U) {
        if (modifierDown && key.scan == modifierScan && key.extended == modifierExtended) {
            modifierDown = false;
            finishCapture(modifierInputText(waiting, key.scan, key.extended));
        }
        return true;
    }
    if (key.repeat > 1) {
        return true;
    }
    if (key.scan == SCAN_ESCAPE && key.extended == 0U) {
        const Action action = waiting;
        stopCapture();
        clearInputs(action);
        return true;
    }
    if (modifierKey(key.scan)) {
        modifierDown = true;
        modifierScan = key.scan;
        modifierExtended = key.extended;
        return true;
    }
    modifierDown = false;
    finishCapture(keyInputText(waiting, key.scan, key.extended, key.shift));
    return true;
}

bool captureDevice(Device device)
{
    if (waiting == Action::NONE) {
        return false;
    }
    finishCapture(deviceName(device));
    return true;
}

std::string keyInputText(Action action, u8 scan, u8 extended, u16 shift)
{
    const Domain domain = domainOf(action);
    if (domain == Domain::WORLD || domain == Domain::SELECTION) {
        return codeInputText(scan, extended, shift);
    }
    const char *name = nameOfKey(scan, extended);
    if (name == nullptr) {
        return "";
    }
    if (domain == Domain::MODE) {
        return name;
    }
    return prefixes(heldModifiers(shift)) + name;
}
