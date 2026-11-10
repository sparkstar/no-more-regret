/* The keyboard of the platform's input (input.h): the scan codes of set 1 at
 * their times, one at a time on the keys' line (interrupts.cpp), to the game's
 * handler (inputKeys, the kernel's keyboardInterrupt), which passes the shift
 * keys on to the keyboard's own keeping (inputKeyboardScan); without a handler
 * the keyboard keeps them all: the shift state and a buffer of keys, which the
 * original's BIOS kept at 0040h (INT 9), and the calls of INT 16h (kbhit and
 * getch, the shift state, the typematic rate).
 *
 * The keyboard repeats: the last key to go down sends
 * its make code again after the typematic delay, then at the typematic
 * rate, until it goes up; Pause does not repeat, nor do the shift codes the
 * keyboard sends around extended keys (E0 2A, E0 36). The rate and delay
 * are the keyboard's at power-on, 10.9 a second after 500 ms, until INT 16h
 * AX=0305h sets others: the game has the call (KERNEL.C's applyTypematic)
 * but never makes it. The kernel drops a make code of a key already down
 * (keyRepeats), so the repeats show only through the keyboard's own. The
 * keyboard holds 16 bytes besides the one on its way to the game; a repeat
 * that finds them full is lost.
 *
 * The keys' line also brings the events of the other devices (input.h's
 * DeviceEvent: the mouse's other buttons and wheel, the gamepad), which the
 * original's PC had not: one at a time after the scan codes waiting, to the
 * game's handler of them (the bindings'), so that they come as a key of their
 * time would. The game port has no joystick (inputJoystick). */
#include "input.h"
#include "gamememory.h"
#include "pcstate.h"
#include "intctl.h"
#include "passdump.h"
#include "pc.h"
#include "replay.h"
#include "report.h"
#include "window.h"
#include "clock.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>

namespace {

/* The scan codes the keyboard has sent that are not on their way yet, and
 * the one that is (the original's keyboard controller held one). */
std::deque<u8> sent;
const size_t KEYBOARD_BUFFER = 16;
bool arriving = false;
u8 arrivingScan = 0;
/* The typematic rate (0-31) and delay (0-3), and the key that repeats. */
u8 typematicRate = 0x0b, typematicDelay = 1;
const std::uint64_t NEVER = ~static_cast<std::uint64_t>(0);
u8 repeatCode = 0;
bool repeatExtended = false;
std::uint64_t nextRepeat = NEVER;
/* What the keyboard has sent so far: E0h before an extended code, the
 * rest of Pause's six bytes. */
bool afterE0 = false;
int pauseBytes = 0;
/* getch's second call for a key without a character: its scan code. */
int pendingScan = -1;
/* The game's handler of the scan codes. */
void (*keyHandler)(u8 scan) = nullptr;
/* The other devices' events not on their way yet, the one that is (instead
 * of a scan code), and the game's handler of them. */
std::deque<DeviceEvent> devices;
bool deviceArriving = false;
DeviceEvent arrivingDevice{};
void (*deviceHandler)(const DeviceEvent &event) = nullptr;

/* The keyboard's own state, the BIOS's in the original: the shift state
 * (0040:0017, the shifts, control, alt and the locks), the keys held
 * (0040:0018: the left control and alt, the locks) and the extended state
 * (0040:0096: bit 1 after E0h, bits 2 and 3 the right control and alt). */
u8 flags = 0, held = 0, more = 0;
/* The buffer of keys (the character in the low byte, the scan code in the
 * high), 15 at most, as the BIOS's ring of 16 words held. */
std::deque<u16> stored;
const size_t STORED = 15;

void storeKey(u16 key)
{
    if (stored.size() == STORED) {
        return; /* full: the BIOS beeps */
    }
    stored.push_back(key);
}

/* The US layout's characters of scan codes 00h-39h, plain and shifted. */
const char PLAIN[] = "\0\x1b"
                     "1234567890-=\b\t"
                     "qwertyuiop[]\r\0"
                     "asdfghjkl;'`\0\\"
                     "zxcvbnm,./\0*\0 ";
const char SHIFTED[] = "\0\x1b"
                       "!@#$%^&*()_+\b\t"
                       "QWERTYUIOP{}\r\0"
                       "ASDFGHJKL:\"~\0|"
                       "ZXCVBNM<>?\0*\0 ";

u8 character(u8 scan, bool extended)
{
    if (extended || scan > 0x39) {
        return 0;
    }
    const bool shift = (flags & 3) != 0;
    char c = shift ? SHIFTED[scan] : PLAIN[scan];
    if (((flags & 0x40) != 0) && c >= 'a' && c <= 'z') {
        c = static_cast<char>(c - 'a' + 'A');
    } else if (((flags & 0x40) != 0) && shift && c >= 'A' && c <= 'Z') {
        c = static_cast<char>(c - 'A' + 'a');
    }
    if (((flags & 4) != 0) && c >= 'a' && c <= 'z') {
        return static_cast<u8>(c - 'a' + 1);
    }
    if (((flags & 4) != 0) && c >= 'A' && c <= 'Z') {
        return static_cast<u8>(c - 'A' + 1);
    }
    if ((flags & 8) != 0) {
        return 0;
    }
    return static_cast<u8>(c);
}

/* A shift key down or up: its bit in the state, and for control and alt
 * the either-side bit from both sides. */
void shiftKey(u8 scan, bool extended, bool pressed)
{
    auto set = [&](u8 &byte, u8 bit, bool on) {
        byte = on ? static_cast<u8>(byte | bit) : static_cast<u8>(byte & ~bit);
    };
    switch (scan) {
    case 0x2a:
        if (!extended) {
            set(flags, 2, pressed);
        }
        break;
    case 0x36:
        if (!extended) {
            set(flags, 1, pressed);
        }
        break;
    case 0x1d:
        set(extended ? more : held, extended ? 4 : 1, pressed);
        set(flags, 4, ((held & 1) != 0) || ((more & 4) != 0));
        break;
    case 0x38:
        set(extended ? more : held, extended ? 8 : 2, pressed);
        set(flags, 8, ((held & 2) != 0) || ((more & 8) != 0));
        break;
    }
}

/* Caps, Num and Scroll Lock and Insert toggle when they go down. */
bool lockKey(u8 scan, bool pressed)
{
    static const struct {
        u8 scan, bit;
    } locks[] = {{0x3a, 0x40}, {0x45, 0x20}, {0x46, 0x10}, {0x52, 0x80}};
    for (auto lock : locks) {
        if (lock.scan == scan) {
            if (pressed && ((held & lock.bit) == 0)) {
                flags ^= lock.bit;
            }
            held = pressed ? static_cast<u8>(held | lock.bit) : static_cast<u8>(held & ~lock.bit);
            return scan != 0x52;
        }
    }
    return false;
}

/* 250 ms a step of the delay; the rate's period is (8 + bits 0-2) * 2 to
 * the power of bits 3-4 * 4.17 ms, 33.4 ms (30 a second) to 500 ms. */
std::uint64_t typematicWait()
{
    return (typematicDelay + 1) * PIT_HZ / 4;
}

std::uint64_t typematicPeriod()
{
    const std::uint64_t units = static_cast<std::uint64_t>(8 + (typematicRate & 7)) << (typematicRate >> 3 & 3);
    return units * PIT_HZ * 417 / 100000;
}

/* A key going down becomes the one that repeats; going up, it stops. */
void noteScan(u8 scan)
{
    if (pauseBytes != 0) {
        pauseBytes--;
        return;
    }
    if (scan == 0xe1) {
        pauseBytes = 5;
        return;
    }
    if (scan == 0xe0) {
        afterE0 = true;
        return;
    }
    const bool extended = afterE0;
    afterE0 = false;
    const u8 code = scan & 0x7f;
    if (extended && (code == 0x2a || code == 0x36)) {
        return;
    }
    if ((scan & 0x80) == 0) {
        repeatCode = code;
        repeatExtended = extended;
        nextRepeat = pcTime() + typematicWait();
    } else if (nextRepeat != NEVER && code == repeatCode && extended == repeatExtended) {
        nextRepeat = NEVER;
    }
}

}

void keyboardScan(u8 scan)
{
    sent.push_back(scan);
    noteScan(scan);
}

std::uint64_t keyboardRepeatTime()
{
    return nextRepeat;
}

void keyboardRepeat(std::uint64_t time)
{
    for (; nextRepeat <= time; nextRepeat += typematicPeriod()) {
        if (sent.size() + (repeatExtended ? 2 : 1) <= KEYBOARD_BUFFER) {
            if (repeatExtended) {
                sent.push_back(0xe0);
            }
            sent.push_back(repeatCode);
        }
    }
}

void keyboardDevice(const DeviceEvent &event)
{
    devices.push_back(event);
}

bool keyboardFeed()
{
    if (arriving) {
        return false;
    }
    if (!sent.empty()) {
        arrivingScan = sent.front();
        sent.pop_front();
        deviceArriving = false;
    } else if (!devices.empty()) {
        arrivingDevice = devices.front();
        devices.pop_front();
        deviceArriving = true;
    } else {
        return false;
    }
    arriving = true;
    requestIrq(1);
    return true;
}

/* The keys' line: the scan code on its way to the game's handler, or to the
 * keyboard's own keeping, and the line's end (no time passes in the
 * handler, so it is the same as the original's, which ended it on the
 * way). */
void keyboardLine()
{
    const u8 scan = arrivingScan;
    arriving = false;
    if (deviceArriving) {
        deviceArriving = false;
        passDumpDevice(static_cast<u8>(arrivingDevice.kind));
        if (deviceHandler != nullptr) {
            const GameSide gameSide;
            deviceHandler(arrivingDevice);
        }
    } else if (keyHandler != nullptr) {
        passDumpScan(scan);
        const GameSide gameSide;
        keyHandler(scan);
    } else {
        passDumpScan(scan);
        inputKeyboardScan(scan);
    }
    endInterrupt(1);
}

void inputKeyboardScan(u8 scan)
{
    const SystemSide systemSide;
    if (scan == 0xe0 || scan == 0xe1) {
        more |= 2;
        return;
    }
    const bool extended = (more & 2) != 0;
    more &= static_cast<u8>(~2);
    const bool pressed = (scan & 0x80) == 0;
    scan &= 0x7f;
    if (scan == 0x2a || scan == 0x36 || scan == 0x1d || scan == 0x38) {
        shiftKey(scan, extended, pressed);
    } else if (!lockKey(scan, pressed) && pressed) {
        storeKey(static_cast<u16>(scan << 8 | character(scan, extended)));
    }
}

void inputKeys(void (*handler)(u8 scan))
{
    const SystemSide systemSide;
    keyHandler = handler;
}

void inputDevices(void (*handler)(const DeviceEvent &event))
{
    const SystemSide systemSide;
    deviceHandler = handler;
}

bool inputJoystick(std::int16_t & /*unused*/, std::int16_t & /*unused*/, std::uint8_t & /*unused*/)
{
    const SystemSide systemSide;
    return false;
}

void inputKeysOn()
{
    const SystemSide systemSide;
    unmaskIrq(1);
}

void inputKeysOff()
{
    const SystemSide systemSide;
    maskIrq(1);
}

u16 inputShiftFlags()
{
    const SystemSide systemSide;
    return static_cast<u16>(held << 8 | flags);
}

void inputSetShiftFlags(u16 state)
{
    const SystemSide systemSide;
    flags = static_cast<u8>(state);
    held = static_cast<u8>(state >> 8);
}

/* INT 16h AH=12h: AL the shift state, AH the keys held: the left control
 * and alt, the right ones, the locks and SysRq. */
u16 inputShiftState()
{
    const SystemSide systemSide;
    const u8 high = static_cast<u8>((held & 3) | (more & 0x0c) | (held & 0x70) | (held & 4) << 5);
    return static_cast<u16>(high << 8 | flags);
}

void inputTypematic(u8 rate, u8 delay)
{
    const SystemSide systemSide;
    typematicRate = rate & 0x1f;
    typematicDelay = delay & 3;
}

bool inputKeyWaiting()
{
    const SystemSide systemSide;
    return pendingScan >= 0 || !stored.empty();
}

/* A key from the buffer: its character, or 0 and its scan code at the next
 * call. It waits for one; with no input to come (no script's, no repeat,
 * no window, no replay), it would wait for ever. */
int inputKey()
{
    const SystemSide systemSide;
    if (pendingScan >= 0) {
        const int scan = pendingScan;
        pendingScan = -1;
        return scan;
    }
    while (stored.empty()) {
        if (nextInputTime() == ~static_cast<std::uint64_t>(0) && !keyboardFeed() && !windowRun() && !replaying()) {
            portEnd(3, "the game waits for a key (inputKey), and no input is to come");
        }
        clockWait();
    }
    const u16 key = stored.front();
    stored.pop_front();
    if ((key & 0xff) == 0) {
        pendingScan = key >> 8;
    }
    return key & 0xff;
}

KeyboardState keyboardState()
{
    return {sent,           arriving,      arrivingScan, typematicRate, typematicDelay, repeatCode, repeatExtended,
            nextRepeat,     afterE0,       pauseBytes,   pendingScan,   keyHandler,     devices,    deviceArriving,
            arrivingDevice, deviceHandler, flags,        held,          more,           stored};
}

void putKeyboardState(const KeyboardState &state)
{
    sent = state.sent;
    arriving = state.arriving;
    arrivingScan = state.arrivingScan;
    typematicRate = state.typematicRate;
    typematicDelay = state.typematicDelay;
    repeatCode = state.repeatCode;
    repeatExtended = state.repeatExtended;
    nextRepeat = state.nextRepeat;
    afterE0 = state.afterE0;
    pauseBytes = state.pauseBytes;
    pendingScan = state.pendingScan;
    keyHandler = state.keyHandler;
    devices = state.devices;
    deviceArriving = state.deviceArriving;
    arrivingDevice = state.arrivingDevice;
    deviceHandler = state.deviceHandler;
    flags = state.flags;
    held = state.held;
    more = state.more;
    stored = state.stored;
}
