/* The platform's clock and input (clock.h, input.h, port/system/pc.h),
 * which have no oracle: the ticks at the divisor the game sets, over
 * virtual time; the hold, the lines on and off and their ends; the clock's
 * own ticks and the keyboard's own keeping; the input's times; the
 * keyboard's typematic repeat; the mouse's events to the game's handler;
 * the time of a pass at each machine's speed and of delay. */
#include "clock.h"
#include "gamestate.h"
#include "conio.h"
#include "dos.h"
#include "input.h"
#include "intctl.h"
#include "mousedrv.h"
#include "pc.h"
#include "script.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH pc: %s\n", what.c_str());
    }
}

int ticks = 0;
bool acknowledge = true;

void timerHandler()
{
    ticks++;
    if (acknowledge) {
        clockTickDone();
    }
}

struct Key {
    u8 scan;
    std::uint64_t time;
};
std::vector<Key> &keys = gameGlobal<std::vector<Key>>();

/* The passes of each of count ticks at a machine's speed: those that ended
 * before the tick came, the tick's passes. The first passes come into step
 * with the ticks first. */
std::vector<int> passesEachTick(MachineSpeed speed, int count)
{
    clockSetMachineSpeed(speed);
    for (int seen = ticks; ticks == seen;) {
        clockPass();
    }
    std::vector<int> each;
    int passes = 0;
    while (static_cast<int>(each.size()) < count) {
        const int seen = ticks;
        clockPass();
        passes++;
        if (ticks != seen) {
            each.push_back(ticks - seen == 1 ? passes : -1);
            passes = 0;
        }
    }
    return each;
}

int countOf(const std::vector<int> &each, int passes)
{
    int count = 0;
    for (int tick : each) {
        count += tick == passes ? 1 : 0;
    }
    return count;
}

void keyboardHandler(u8 scan)
{
    keys.push_back({scan, pcTime()});
}

struct MouseCall {
    u16 events, buttons, x, y;
};
std::vector<MouseCall> mouseCalls;

void mouseHandler(u16 events, u16 buttons, u16 x, u16 y)
{
    mouseCalls.push_back({events, buttons, x, y});
}

}

int main()
{
    /* The clock's own ticks, before anything is hooked, end themselves: the
     * kernel's come after them. */
    passTime(pcTime() + PIT_HZ);

    /* The kernel's timer at 60 Hz (clock.h). */
    clockTicks(timerHandler);
    clockTickDivisor(19886);
    std::uint64_t start = pcTime();
    passTime(start + PIT_HZ);
    check(ticks == 60, "60 Hz for a second: " + std::to_string(ticks));

    /* Off, a tick waits, one of them. */
    clockTicksOff();
    ticks = 0;
    passTime(pcTime() + PIT_HZ / 10);
    check(ticks == 0, "off");
    clockTicksOn();
    passTime(pcTime());
    check(ticks == 1, "one tick kept while off");

    /* Held, the same; the push and pop of the hold keep it. */
    ticks = 0;
    clockPushHold();
    clockHold();
    passTime(pcTime() + PIT_HZ / 10);
    check(ticks == 0 && !interruptsEnabled(), "held");
    clockPopHold();
    passTime(pcTime());
    check(ticks == 1 && interruptsEnabled(), "the pop lets it go");

    /* Until the handler lets the next come, none does. */
    ticks = 0;
    acknowledge = false;
    passTime(pcTime() + PIT_HZ / 10);
    check(ticks == 1, "one tick until clockTickDone");
    clockTickDone();
    acknowledge = true;
    passTime(pcTime() + PIT_HZ / 60);
    check(ticks >= 2, "after it");

    /* The pass of a loop at each machine's speed (clock.h): SLOW's a
     * sixtieth of a second; FAST's two every tick; NORMAL's two but one in
     * three ticks of ten, which a tick's number picks; under the BIOS's
     * divisor SLOW's at any speed. */
    MachineSpeed starting = MachineSpeed::NORMAL;
    if (const char *named = std::getenv("REGRET_GAME_SPEED")) {
        check(clockMachineSpeedNamed(named, &starting), std::string("REGRET_GAME_SPEED a speed: ") + named);
    }
    check(clockMachineSpeed() == starting, "the speed at the start REGRET_GAME_SPEED's, else NORMAL");
    clockSetMachineSpeed(MachineSpeed::SLOW);
    start = pcTime();
    for (int i = 0; i < 60; i++) {
        clockPass();
    }
    check(pcTime() - start == PIT_HZ, "SLOW: 60 passes are a second");
    const std::vector<int> fast = passesEachTick(MachineSpeed::FAST, 600);
    check(countOf(fast, 2) == 600, "FAST: two passes each tick: " + std::to_string(countOf(fast, 2)));
    const std::vector<int> normal = passesEachTick(MachineSpeed::NORMAL, 1000);
    const int onePass = countOf(normal, 1);
    check(onePass + countOf(normal, 2) == 1000 && onePass > 250 && onePass < 350,
          "NORMAL: one pass in three ticks of ten, the others two: " + std::to_string(onePass) + " of 1000");
    clockTickDivisor(0);
    clockSetMachineSpeed(MachineSpeed::FAST);
    start = pcTime();
    for (int i = 0; i < 60; i++) {
        clockPass();
    }
    check(pcTime() - start == PIT_HZ, "FAST under the BIOS's divisor: 60 passes are a second");
    clockTickDivisor(19886);
    clockSetMachineSpeed(MachineSpeed::SLOW);
    /* Borland's delay as the original's (timer.cpp): under the kernel's
     * divisor a part of the milliseconds, divisor/65536 of them within half
     * the divisor's counts; under the BIOS's (65536) the milliseconds at its
     * 2386 counts each, 2 a count of the clock. */
    start = pcTime();
    clockDelay(250);
    std::uint64_t delayed = pcTime() - start;
    const std::uint64_t underKernel = (std::uint64_t)PIT_HZ / 4 * 19886 / 65536;
    check(delayed + 19886 / 2 >= underKernel && delayed <= underKernel + 19886 / 2,
          "clockDelay(250) under the divisor 19886: " + std::to_string(delayed) + " counts");
    clockTickDivisor(0);
    start = pcTime();
    clockDelay(250);
    check(pcTime() - start == 250 * 2386 / 2, "clockDelay(250) under the BIOS's divisor");
    clockTickDivisor(19886);
    ticks = 0;
    start = pcTime();
    clockWait();
    check(ticks == 1 && pcTime() - start <= 19886, "clockWait goes on to the next tick");
    /* Without a window the day is the virtual time's, from 1 January 1996,
     * a Monday. */
    ClockDate date = clockDate();
    check(date.year == 1996 && date.month == 1 && date.day == 1 && date.weekday == 1, "the virtual time's day");

    /* The ticks back to the clock's own. */
    clockTicks(nullptr);
    ticks = 0;
    passTime(pcTime() + PIT_HZ);
    check(ticks == 0, "the handler none");

    /* The keyboard's own: the shift state and the buffer. */
    auto key = [](std::uint64_t time, u8 scan) { addInput({time, InputEvent::SCAN, scan, 0, 0, 0, {}, 0, {}}); };
    std::uint64_t t = pcTime();
    key(t + 10, 0x2a);
    key(t + 20, 0x1e);
    key(t + 30, 0xaa);
    key(t + 40, 0x1e);
    key(t + 50, 0xe0);
    key(t + 50, 0x48);
    passTime(t + 100);
    check(!(inputShiftFlags() & 2), "the shift up again");
    check(inputKeyWaiting() && inputKey() == 'A' && inputKey() == 'a', "A, then a");
    check(inputKey() == 0 && inputKey() == 0x48 && !inputKeyWaiting(), "the up arrow: 0, then its scan code");
    key(t + 200, 0x1d);
    passTime(t + 300);
    check((inputShiftState() & 0x0104) == 0x0104, "control down in INT 16h's state");
    key(t + 310, 0x9d);
    passTime(t + 400);
    check((inputShiftState() & 0x0104) == 0, "control up");

    /* The kernel's keyboard: the scan codes at their times, in order. */
    inputKeys(keyboardHandler);
    t = pcTime();
    key(t + 1000, 0x1c);
    key(t + 1000, 0x9c);
    key(t + 5000, 0x01);
    passTime(t + 10000);
    check(keys.size() == 3 && keys[0].scan == 0x1c && keys[1].scan == 0x9c && keys[2].scan == 0x01, "three scan codes");
    check(keys.size() == 3 && keys[0].time == t + 1000 && keys[1].time == t + 1000 && keys[2].time == t + 5000,
          "at their times");
    keys.clear();
    inputKeysOff();
    key(t + 20000, 0x39);
    passTime(t + 30000);
    check(keys.empty(), "the keys off");
    inputKeysOn();
    passTime(t + 30000);
    check(keys.size() == 1 && keys[0].scan == 0x39, "its key after");
    key(t + 30000, 0xb9);
    passTime(t + 40000);

    /* The keyboard repeats the last key down: at power-on after 500 ms,
     * then 10.9 a second (91.7 ms), until it goes up. A second down: the
     * make code and six repeats. */
    keys.clear();
    t = pcTime();
    const std::uint64_t period = 22 * PIT_HZ * 417 / 100000;
    key(t + 100, 0x1e);
    key(t + 100 + PIT_HZ, 0x9e);
    passTime(t + 2 * PIT_HZ);
    bool repeats = keys.size() == 8 && keys[7].scan == 0x9e && keys[7].time == t + 100 + PIT_HZ;
    for (size_t i = 0; repeats && i < 7; i++) {
        repeats = keys[i].scan == 0x1e && keys[i].time == (i ? t + 100 + PIT_HZ / 2 + (i - 1) * period : t + 100);
    }
    check(repeats, "a key held a second: its make, six repeats 91.7 ms apart after 500 ms, its break");
    /* INT 16h AX=0305h: 250 ms, then 30 a second; an extended key repeats
     * with its E0h. */
    inputTypematic(0, 0);
    keys.clear();
    t = pcTime();
    key(t, 0xe0);
    key(t, 0x48);
    key(t + PIT_HZ / 2, 0xe0);
    key(t + PIT_HZ / 2, 0xc8);
    passTime(t + PIT_HZ);
    repeats = keys.size() == 20 && keys[18].scan == 0xe0 && keys[19].scan == 0xc8;
    for (size_t i = 0; repeats && i < 18; i += 2) {
        repeats = keys[i].scan == 0xe0 && keys[i + 1].scan == 0x48 &&
                  keys[i].time == (i ? t + PIT_HZ / 4 + (i / 2 - 1) * (8 * PIT_HZ * 417 / 100000) : t);
    }
    check(repeats, "the up arrow half a second at the fastest rate: E0 48 and eight repeats");
    inputTypematic(0x0b, 1);
    /* The last key down repeats; the other going up does not stop it. */
    keys.clear();
    t = pcTime();
    key(t, 0x1e);
    key(t + PIT_HZ / 10, 0x30);
    key(t + PIT_HZ / 5, 0x9e);
    key(t + PIT_HZ, 0xb0);
    passTime(t + 2 * PIT_HZ);
    repeats = keys.size() == 9 && keys[0].scan == 0x1e && keys[1].scan == 0x30 && keys[2].scan == 0x9e &&
              keys[8].scan == 0xb0;
    for (size_t i = 3; repeats && i < 8; i++) {
        repeats = keys[i].scan == 0x30 && keys[i].time == t + PIT_HZ / 10 + PIT_HZ / 2 + (i - 3) * period;
    }
    check(repeats, "B down after A repeats, A going up or not");
    /* The keyboard holds 16 bytes while the controller is not read. */
    keys.clear();
    inputKeysOff();
    t = pcTime();
    key(t, 0x39);
    passTime(t + 5 * PIT_HZ);
    inputKeysOn();
    key(t + 5 * PIT_HZ, 0xb9);
    passTime(t + 6 * PIT_HZ);
    check(keys.size() == 18 && keys[16].scan == 0x39 && keys[17].scan == 0xb9,
          "held five seconds, off: the controller's byte and 16 more, then the break");

    /* The keyboard's own again: its shift state and buffer (the BIOS's). */
    inputKeys(nullptr);
    t = pcTime();
    key(t + 10, 0x2a);
    key(t + 20, 0x1e);
    key(t + 30, 0xaa);
    passTime(t + 100);
    check(keys.size() == 18 && inputKey() == 'A', "the keyboard's own after the game's handler");

    /* The mouse: the game's handler. */
    inputMouseStart(mouseHandler);
    inputMouseXRange(0, 1278);
    inputMouseYRange(0, 479);
    auto mouse = [](std::uint64_t time, i16 x, i16 y, u16 buttons) {
        addInput({time, InputEvent::MOUSE, 0, x, y, buttons, {}, 0, {}});
    };
    t = pcTime();
    mouse(t + 10, 100, 50, 0);
    mouse(t + 20, 100, 50, 1);
    mouse(t + 30, 2000, 600, 0);
    passTime(t + 100);
    check(mouseCalls.size() == 3, "three mouse events");
    if (mouseCalls.size() == 3) {
        check(mouseCalls[0].events == 1 && mouseCalls[0].x == 100 && mouseCalls[0].y == 50, "moved");
        check(mouseCalls[1].events == 2 && mouseCalls[1].buttons == 1, "the left button down");
        check(mouseCalls[2].events == 5 && mouseCalls[2].x == 1278 && mouseCalls[2].y == 479,
              "moved to the limits, the button up");
    }
    inputMouseStop();
    mouse(t + 400, 20, 20, 0);
    passTime(t + 500);
    check(mouseCalls.size() == 3 && mouseDriver.x == 20, "no handler after inputMouseStop, the pointer moves");

    std::printf("pc: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
