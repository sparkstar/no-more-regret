/* JOYSTICK.ASM in C++ (port/common/src/input/joystick.cpp) on a clock and
 * a joystick of the test's: the axes that time out and leave joystickAxes,
 * the one-shots' times of the stick, the buttons, the reading whose time
 * crosses a tick, the time the readings take, sampleJoystick's first
 * reading. */
#include "joystick.h"
#include "clock.h"
#include "input.h"

#include <cstdio>
#include <string>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH joystick: %s\n", what.c_str());
    }
}

/* The clock: the counts passed and the ticks' divisor. */
std::uint64_t now = 0;
std::uint32_t divisor = 19886;

/* The joystick. */
bool pad = false;
std::int16_t padX = 0, padY = 0;
std::uint8_t padButtons = 0;

const std::uint32_t TIMEOUT = 78196;

/* The time of a one-shot at the stick's value, as the module has it. */
std::uint32_t countsOf(std::int16_t value)
{
    std::uint64_t ohms = (std::uint64_t)(value + 32768) * 100000 / 65535;
    return (std::uint32_t)((24200 + 11 * ohms) * 1193182 / 1000000000);
}

/* A sample from a fresh start of the axes. */
std::uint64_t sample()
{
    std::uint64_t before = now;
    sampleJoystick();
    return now - before;
}

}

void clockPassCounts(std::uint32_t counts)
{
    now += counts;
}

std::uint32_t clockTickPhase()
{
    return (std::uint32_t)(now % divisor);
}

bool inputJoystick(std::int16_t &x, std::int16_t &y, std::uint8_t &buttons)
{
    if (!pad) {
        return false;
    }
    x = padX;
    y = padY;
    buttons = padButtons;
    return true;
}

int main()
{
    /* No joystick: every axis times out once, and no reading takes time
     * after. */
    check(joystickAxes == 0xf, "the four axes at the start");
    std::uint64_t first = sample();
    check(joystickAxes == 0, "no joystick: no axis answers");
    check(first == TIMEOUT, "no joystick: 10000h polls, then none: " + std::to_string(first));
    check(joystickButtons == 0, "no joystick: no button");
    check(sample() == 0, "no axis left: no time");

    /* A joystick: joystick B's axes time out, A's answer. */
    joystickAxes = 0xf;
    pad = true;
    padX = 0;
    padY = -32768;
    padButtons = 0x5;
    now = 100;
    first = sample();
    check(joystickAxes == 3, "a joystick: joystick A's two axes");
    check(joystickFirstX == countsOf(0), "the stick's middle: " + std::to_string(joystickFirstX));
    check(joystickFirstX == 685, "the middle's one-shot is 574 us");
    check(joystickFirstY == 28, "the top's is 24.2 us: " + std::to_string(joystickFirstY));
    check(first == TIMEOUT + countsOf(0), "B's timeout, then the second reading to the longer axis");
    check(joystickButtons == 5, "buttons 1 and 3");
    padX = 32767;
    padButtons = 0xa;
    now = 100;
    check(sample() == 2 * 1341, "two readings to the right end's 1124 us");
    check(joystickFirstX == 1341 && joystickFirstY == 28, "the right end");
    check(joystickButtons == 10, "buttons 2 and 4");

    /* A reading that crosses a tick keeps the axis's last time. */
    padX = 0;
    padY = 0;
    now = divisor - 10;
    sample();
    check(joystickFirstX == 1341, "across a tick: X keeps the last time");
    check(joystickFirstY == 28, "and Y");
    now = divisor + 10;
    sample();
    check(joystickFirstX == 685 && joystickFirstY == 685, "after the tick: the new times");

    /* The joystick gone: A's axes time out. */
    pad = false;
    first = sample();
    check(joystickAxes == 0 && first == TIMEOUT, "the joystick gone: A's axes time out");
    check(joystickButtons == 0, "and no button");

    std::printf("joystick: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
