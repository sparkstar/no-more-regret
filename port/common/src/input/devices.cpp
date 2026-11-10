/* The controls' other devices. See devices.h. */
#include "devices.h"
#include "gamestate.h"
#include "control.h"
#include "dispatch.h"
#include "input.h"
#include "kdaemon.h"
#include "kernel.h"
#include "systimer.h"
#include "world.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace {

const std::size_t DEVICES = static_cast<std::size_t>(Device::COUNT);

/* An input: down, and held as the game sees it (a mode's, a key's that
 * repeats: not after the keys are flushed, as a key's count); the
 * controller's action its press gave as a key, which repeats while it is
 * held (or, turn, a stick's turn at each tick); the key's repeats and the
 * time it went down (as the kernel's KeyState). */
struct Held {
    bool down = false;
    bool live = false;
    Action action = Action::NONE;
    bool turn = false;
    u16 count = 0;
    u32 time = 0;
};
Held (&held)[DEVICES] = gameGlobal<Held[DEVICES]>();

/* The keys for the controller, oldest first, with the turn of a stick's
 * (mouseTurnAmount, 0 for none). */
struct DeviceKey {
    KeyEvent key;
    Action action;
    i16 turn;
};
/* A vector, which makes nothing until the first key comes: a deque makes
 * its map as it starts, from the host's heap, before the game's memory is
 * the game's. */
std::vector<DeviceKey> &keys = gameGlobal<std::vector<DeviceKey>>();
/* The keys waiting that leave out the repeats' keys and the sticks' turns
 * (the repeats still count), as the key daemon leaves its repeats out of a
 * crowded queue: the kernel's loop has not run for a while. */
const std::size_t CROWDED = 32;

/* The gamepad's axes, the direction each stick leans (left, right; NONE in
 * its dead zone) and the triggers pulled. */
i16 (&axes)[DEVICE_AXES] = gameGlobal<i16[DEVICE_AXES]>();
Device (&leans)[2] = gameGlobal<Device[2]>({Device::NONE, Device::NONE});
bool (&pulled)[2] = gameGlobal<bool[2]>({false, false});

/* The codes for the world's gump, oldest first. */
std::vector<i16> &codes = gameGlobal<std::vector<i16>>();

/* A direction or a trigger is on beyond half its travel, off below two
 * fifths. */
const s32 ON = 16384;
const s32 OFF = 13107;
const s32 TRAVEL = 32767;

std::size_t indexOf(Device device)
{
    return static_cast<std::size_t>(device);
}

Device offset(Device device, int by)
{
    return static_cast<Device>(static_cast<int>(device) + by);
}

/* Whether the world's gump is selecting items, where the inputs give the
 * selection's actions. */
bool selecting()
{
    return modalTop == 0 && (selectingItems != 0U);
}

/* The action's key, down (repeat 1 or more) or up, at the time, for the
 * controller. */
void sendKey(Action action, u8 pressed, u16 repeat)
{
    KeyEvent key = actionKey(action, pressed, repeat);
    key.time = systemTimer->ticks;
    keys.push_back({key, action, 0});
}

/* Whether the input is a stick's direction. */
bool stickInput(Device device)
{
    return device >= Device::LSTICK_UP && device <= Device::RSTICK_RIGHT;
}

/* How far a stick leans the input's way, 0 to 32767 (its axis's place, the
 * up and left of which are below 0). */
s32 leanOf(Device device)
{
    const int at = static_cast<int>(device) - static_cast<int>(Device::LSTICK_UP);
    const int stick = at / 4;
    const int way = at % 4; /* up, down, left, right */
    const s32 value = way < 2 ? axes[(stick * 2) + 1] : axes[stick * 2];
    const s32 lean = (way == 0 || way == 2) ? -value : value;
    return std::clamp<s32>(lean, 0, TRAVEL);
}

void press(Device device, Action action)
{
    Held &entry = held[indexOf(device)];
    entry = Held();
    entry.down = true;
    entry.live = true;
    if (runLockInput(device)) {
        toggleRunLock();
    }
    if (stickInput(device) && (action == Action::TURN_LEFT || action == Action::TURN_RIGHT)) {
        entry.action = action;
        entry.turn = true;
        return;
    }
    if (controlActionOf(action)) {
        entry.action = action;
        entry.count = 1;
        entry.time = systemTimer->ticks;
        sendKey(action, 1, 1);
        return;
    }
    if (codeActionOf(action)) {
        codes.push_back(actionCode(action));
    }
}

void release(Device device)
{
    Held &entry = held[indexOf(device)];
    if (!entry.down) {
        return;
    }
    const Held was = entry;
    entry = Held();
    if (!was.turn && controlActionOf(was.action)) {
        sendKey(was.action, 0, 0);
    }
}

/* An input of the devices' events down or up: the action of the place the
 * world is in; a press dropped while the keyboard is paused. */
void input(Device device, bool down)
{
    if (!down) {
        release(device);
        return;
    }
    /* The key settings' wait for an input takes it (bindings.h). */
    if (captureDevice(device)) {
        return;
    }
    if (held[indexOf(device)].down || keyboardPaused()) {
        return;
    }
    press(device, deviceAction(device, selecting()));
}

void pulse(Action action)
{
    if (controlActionOf(action)) {
        sendKey(action, 1, 1);
        sendKey(action, 0, 0);
    } else if (codeActionOf(action)) {
        codes.push_back(actionCode(action));
    }
}

/* The way a stick leans now: the axis it leans most, which takes the place
 * of the way it leant when that is a quarter more; NONE in its dead zone. */
Device leaning(int stick)
{
    const Device up = (stick != 0) ? Device::RSTICK_UP : Device::LSTICK_UP;
    const s32 x = axes[stick * 2];
    const s32 y = axes[(stick * 2) + 1];
    Device most;
    if (std::abs(x) > std::abs(y)) {
        most = offset(up, x < 0 ? 2 : 3);
    } else {
        most = offset(up, y < 0 ? 0 : 1);
    }
    const Device was = leans[stick];
    if (was != Device::NONE && leanOf(was) >= OFF) {
        if (most != was && leanOf(most) >= ON && leanOf(most) * 4 > leanOf(was) * 5) {
            return most;
        }
        return was;
    }
    return leanOf(most) >= ON ? most : Device::NONE;
}

/* An axis's new place: the sticks' ways and the triggers. */
void axisMoved(u8 axis, i16 value)
{
    axes[axis] = value;
    if (axis == DEVICE_AXIS_LEFT_TRIGGER || axis == DEVICE_AXIS_RIGHT_TRIGGER) {
        const int trigger = axis - DEVICE_AXIS_LEFT_TRIGGER;
        const bool on = pulled[trigger] ? value >= OFF : value >= ON;
        if (on != pulled[trigger]) {
            pulled[trigger] = on;
            input((trigger != 0) ? Device::PAD_RT : Device::PAD_LT, on);
        }
        return;
    }
    const int stick = axis / 2;
    const Device way = leaning(stick);
    if (way != leans[stick]) {
        const Device was = leans[stick];
        leans[stick] = way;
        if (was != Device::NONE) {
            input(was, false);
        }
        if (way != Device::NONE) {
            input(way, true);
        }
    }
}

/* The gamepad taken away: all of it up. */
void padGone()
{
    for (std::size_t index = indexOf(Device::PAD_A); index <= indexOf(Device::RSTICK_RIGHT); index++) {
        release(static_cast<Device>(index));
    }
    for (i16 &axis : axes) {
        axis = 0;
    }
    leans[0] = leans[1] = Device::NONE;
    pulled[0] = pulled[1] = false;
}

/* The game's handler of the devices' events (input.h), on the keys' line. */
void deviceEvent(const DeviceEvent &event)
{
    switch (event.kind) {
    case DeviceKind::MOUSE_BUTTON:
        if (event.which <= DEVICE_MOUSE_X2) {
            input(offset(Device::MOUSE_MIDDLE, event.which), event.value != 0);
        }
        break;
    case DeviceKind::WHEEL: {
        const Device device = event.value > 0 ? Device::WHEEL_UP : Device::WHEEL_DOWN;
        if (captureDevice(device)) {
            break;
        }
        const int notches = std::abs(static_cast<int>(event.value));
        for (int notch = 0; notch < notches && !keyboardPaused(); notch++) {
            pulse(deviceAction(device, selecting()));
        }
        break;
    }
    case DeviceKind::PAD_BUTTON:
        if (event.which < DEVICE_PAD_BUTTONS) {
            input(offset(Device::PAD_A, event.which), event.value != 0);
        }
        break;
    case DeviceKind::PAD_AXIS:
        if (event.which < DEVICE_AXES) {
            axisMoved(event.which, event.value);
        }
        break;
    case DeviceKind::PAD_GONE:
        padGone();
        break;
    }
}

/* The handler from the start (the devices' events come before the game
 * hooks the keyboard, and after). */
const bool HOOKED = (inputDevices(deviceEvent), true);

/* A stick's turn by its lean: the mouse's turn of 2 to 10 (a key's turn of
 * 1 to 5 steps of the reticle), from two fifths of its travel to all of
 * it; with the modes held unless the mouse does not roll
 * (deviceTurnShift). */
void stickTurn(Device device, Action action)
{
    const s32 lean = leanOf(device);
    const s32 steps = 1 + (lean > OFF ? (lean - OFF) * 4 / (TRAVEL - OFF) : 0);
    KeyEvent key;
    key.clear();
    key.scan = action == Action::TURN_LEFT ? SCAN_LEFT : SCAN_RIGHT;
    key.shift = deviceTurnShift(keyShift);
    key.time = 0;
    key.pressed = 1;
    key.extended = 1;
    key.repeat = 1;
    key.translate();
    keys.push_back({key, action, static_cast<i16>(steps * 2)});
}

}

bool deviceDown(Device device)
{
    return held[indexOf(device)].live;
}

u16 deviceRepeats(Device device, Action action)
{
    const Held &entry = held[indexOf(device)];
    if (!entry.live || entry.turn || entry.action != action) {
        return 0;
    }
    return entry.count;
}

void devicePress(Device device, Action action)
{
    press(device, action);
}

void deviceRelease(Device device)
{
    release(device);
}

void devicePulse(Device /*unused*/, Action action)
{
    pulse(action);
}

void deviceTick()
{
    static_cast<void>(HOOKED);
    const u32 now = systemTimer->ticks;
    for (std::size_t index = 1; index < DEVICES; index++) {
        Held &entry = held[index];
        if (!entry.live || entry.action == Action::NONE) {
            continue;
        }
        const bool crowded = keys.size() >= CROWDED;
        if (entry.turn) {
            if (!crowded) {
                stickTurn(static_cast<Device>(index), entry.action);
            }
        } else if (entry.time + repeatDelay <= now) {
            entry.count++;
            if (!crowded) {
                sendKey(entry.action, 1, entry.count);
            }
        }
    }
}

void deliverDeviceKeys()
{
    while (!keys.empty()) {
        DeviceKey next = keys.front();
        keys.erase(keys.begin());
        if ((controller == nullptr) || (controller->hasStatus(STATUS_KEYBOARD) == 0U) ||
            (controller->hasStatus(STATUS_HOLD) != 0U) || ((controller->eventMask & EVENT_KEYBOARD) == 0U)) {
            continue;
        }
        const u8 level = controller->level();
        if (level < currentPriority && level != ALWAYS_LEVEL && level != SYSTEM_LEVEL) {
            continue;
        }
        if (next.turn != 0) {
            mouseTurnAmount = next.turn;
            mouseTurn = 1;
        }
        controller->actionKey(&next.key, next.action);
        mouseTurn = 0;
    }
}

void flushDevices()
{
    keys.clear();
    for (Held &entry : held) {
        entry.live = false;
        entry.count = 0;
    }
}

bool deviceCode(i16 *code)
{
    if (codes.empty()) {
        return false;
    }
    *code = codes.front();
    codes.erase(codes.begin());
    return true;
}
