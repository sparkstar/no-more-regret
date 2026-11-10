/* The platform's clock (clock.h): the virtual time, in the original's interval
 * timer's ticks (1193182 a second), and the game's ticks on line 0
 * (interrupts.cpp) every divisor of them, 65536 until the kernel sets its own.
 *
 * Nothing runs while the game computes: time passes where its loops pass
 * (clockPass: a pass takes what it took on the original's machine, at the
 * machine's speed the player chose) or wait (clockWait, clockDelay), and the
 * timer's interrupts and the input events of that time come there, one after
 * another in their order. The same input gives the same run. The window's
 * frames (window.h) come in the same time, every sixtieth of a second.
 *
 * With a window the virtual time keeps to the wall clock. Ahead of it, a frame
 * waits for it: a game faster than the original's machine takes a pass's time
 * a pass, as there. Behind it, a pass lets time pass as far as the wall clock:
 * on a slow machine the timer still came on time. What the wall clock is ahead
 * by more than a second, a host that stood still (a debugger, a stopped
 * process), is dropped. */
#include "clock.h"
#include "end.h"
#include "gamememory.h"
#include "gamestate.h"
#include "pcstate.h"
#include "intctl.h"
#include "passdump.h"
#include "pc.h"
#include "replay.h"
#include "script.h"
#include "report.h"
#include "window.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <thread>

namespace {

std::uint64_t now = 0;
u32 divisor = 65536;
std::uint64_t nextTick = 65536;
/* The game's handler of the ticks. */
void (*tickHandler)() = nullptr;
/* The passes clockPass has counted, for the sixtieths. */
std::uint64_t frames = 0;
/* The window's frames (window.h), every sixtieth of a second from the
 * start; none again once the first says the run has no window. */
const std::uint64_t NEVER = ~static_cast<std::uint64_t>(0);
std::uint64_t windowFrames = 0;
std::uint64_t nextWindowFrame = 0;
/* The frames of the latest passTime of the game's step, those that passed
 * inside the game's waits (clockStepEnd), and whether one passed in the
 * step. */
std::uint64_t passFrames = 0;
std::uint64_t waitFrames = 0;
bool stepFrame = false;

/* The wall clock, from the window's first frame: virtual time wallVirtual
 * was wall clock wallStart. */
using Clock = std::chrono::steady_clock;
bool live = false;
Clock::time_point wallStart;
std::uint64_t wallVirtual = 0;
const std::uint64_t MOST_BEHIND = PIT_HZ;
/* How many times as fast as the wall clock the virtual time goes: the fast
 * forward's while its key is held (window.h's windowFastForwarding; eight
 * times), else 1. Fast, the wall clock
 * never runs ahead of the virtual time, so that the fast forward makes no
 * pass longer than the game makes it: a host that cannot keep up goes less
 * fast. */
const std::uint64_t FAST_FORWARD_SPEED = 8;
std::uint64_t speed = 1;

/* The speeds' names, in MachineSpeed's order. */
const char *const MACHINE_SPEED_NAMES[] = {"slow", "normal", "fast"};

/* The speed a run starts at: REGRET_GAME_SPEED's, else NORMAL (clock.h). */
MachineSpeed startingSpeed()
{
    MachineSpeed named = MachineSpeed::NORMAL;
    const char *text = std::getenv("REGRET_GAME_SPEED");
    if (text != nullptr && !clockMachineSpeedNamed(text, &named)) {
        named = MachineSpeed::NORMAL;
    }
    return named;
}

/* The machine's speed the player chose (clock.h), a setting outside the
 * arena. */
MachineSpeed &machineSpeed = gameSetting<MachineSpeed>(startingSpeed());
/* NORMAL's ticks of one pass: those whose hash's top 53 bits are below
 * three tenths of all 53 bits' values. */
const std::uint64_t ONE_PASS_TICKS = (std::uint64_t{1} << 53U) / 10 * 3;
/* The ticks slower than this have SLOW's passes at every speed. */
const std::uint64_t SLOWEST_TWO_PASS_DIVISOR = PIT_HZ / 30;

/* Borland's delay counts this many of the timer's counts a millisecond:
 * its value for mode 3, whose counter goes down by 2 a count, which its
 * start-up test (1478:0586) keeps on a PC whose BIOS left mode 3. */
const std::int64_t DELAY_COUNTS_PER_MS = 2386;

/* The counts Borland's delay (1478:05ad) waits for milliseconds from the
 * timer's phase. It reads channel 0's counter, inverted, which rises 2 a
 * count, until it has risen by milliseconds * DELAY_COUNTS_PER_MS, and
 * takes a counter that comes round (its reading drops) for 65536 more. The
 * BIOS's divisor of 65536 comes round every 32768 counts in mode 3, and
 * the delay is right; the kernel's (mode 3, divisor N for its fastest
 * process) every N/2, so the original waits N/65536 of the milliseconds: a
 * fifth at the sound's 90 Hz. */
std::uint64_t borlandDelayCounts(unsigned milliseconds, u32 timerDivisor, u32 phase)
{
    const std::int64_t half = timerDivisor / 2;
    std::int64_t into = phase % half;
    std::int64_t reading = 0xffff - static_cast<std::int64_t>(timerDivisor) + (2 * into);
    std::int64_t target = (static_cast<std::int64_t>(milliseconds) * DELAY_COUNTS_PER_MS) + reading;
    std::uint64_t waited = 0;
    for (;;) {
        // The counts to the counter's reload, where its reading drops.
        const std::int64_t left = half - into;
        if (target < 0x10000) {
            if (reading >= target) {
                return waited;
            }
            // It ends where the reading reaches the target, or at the
            // reload (a drop with nothing more to take).
            const std::int64_t needed = (target - reading + 1) / 2;
            return waited + static_cast<std::uint64_t>(std::min(needed, left));
        }
        waited += static_cast<std::uint64_t>(left);
        target -= 0x10000;
        into = 0;
        reading = 0xffff - static_cast<std::int64_t>(timerDivisor);
    }
}

/* Ticks and nanoseconds, in seconds and the rest so that hours of them
 * stay inside 64 bits. */
std::uint64_t ticksOf(Clock::duration span)
{
    const std::uint64_t ns =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(span).count());
    return (ns / 1000000000 * PIT_HZ) + (ns % 1000000000 * PIT_HZ / 1000000000);
}

std::chrono::nanoseconds spanOf(std::uint64_t ticks)
{
    return std::chrono::nanoseconds((ticks / PIT_HZ * 1000000000) + (ticks % PIT_HZ * 1000000000 / PIT_HZ));
}

/* The virtual time the wall clock has come to (speed times its own), no
 * more than MOST_BEHIND ahead of now, none while fast: the rest is
 * dropped. */
std::uint64_t wallTime()
{
    std::uint64_t wall = wallVirtual + (speed * ticksOf(Clock::now() - wallStart));
    const std::uint64_t most = speed > 1 ? 0 : MOST_BEHIND;
    if (wall > now + most) {
        wallStart += spanOf((wall - now - most) / speed);
        wall = now + most;
    }
    return wall;
}

/* A frame comes when the wall clock comes to its time. */
void waitForWall()
{
    if (wallTime() < now) {
        std::this_thread::sleep_until(wallStart + spanOf((now - wallVirtual) / speed));
    }
}

/* The speed the window asks for, the wall clock taken from where it is. */
void keepSpeed()
{
    const std::uint64_t asked = windowFastForwarding() ? FAST_FORWARD_SPEED : 1;
    if (asked == speed) {
        return;
    }
    if (live) {
        wallVirtual = wallTime();
        wallStart = Clock::now();
    }
    speed = asked;
}

/* The window's frame at now, the first of which starts the wall clock; a
 * replay's frame instead (replay.h), which has no wall clock. */
void windowFrameNow()
{
    if (runEnding()) {
        nextWindowFrame = NEVER;
        return;
    }
    const std::uint64_t frame = windowFrames++;
    if (replaying() && !watching()) {
        replayFrame(frame);
        /* A watched replay before it is shown: the window's frame without
         * its picture, for its close (replay.h). */
        if (replayWatched() && !windowFrameUnshown()) {
            nextWindowFrame = NEVER;
            return;
        }
        nextWindowFrame = windowFrames * PIT_HZ / 60;
        return;
    }
    keepSpeed();
    if (live) {
        waitForWall();
    }
    /* A watched replay's input, else the window's (replay.h). */
    if (replaying()) {
        replayFrame(frame);
    } else {
        recordFrame(frame);
    }
    if (!windowFrame()) {
        nextWindowFrame = NEVER;
        return;
    }
    nextWindowFrame = windowFrames * PIT_HZ / 60;
    if (!live) {
        live = true;
        wallStart = Clock::now();
        wallVirtual = now;
    }
}

/* A frame of the virtual PC: the window's or the replay's. The host's work
 * of the frame runs after the game's step (end.h's playGame). */
void frameNow()
{
    passFrames++;
    stepFrame = true;
    windowFrameNow();
}

}

std::uint64_t pcTime()
{
    return now;
}

std::uint64_t clockSpeed()
{
    return speed;
}

void restartWallClock()
{
    wallStart = Clock::now();
    wallVirtual = now;
}

bool clockStepEnd()
{
    passFrames = 0;
    const bool framed = stepFrame;
    stepFrame = false;
    return framed;
}

std::uint64_t framesInWaits()
{
    return waitFrames;
}

void passTime(std::uint64_t until)
{
    waitFrames += passFrames;
    passFrames = 0;
    for (;;) {
        while (keyboardFeed()) {
            deliverInterrupts();
        }
        const std::uint64_t input = nextInputTime();
        const std::uint64_t next = std::min({nextTick, input, nextWindowFrame});
        if (next > until) {
            break;
        }
        now = next;
        audioOutputTime(now);
        if (now == nextWindowFrame) {
            frameNow();
        }
        if (now == nextTick) {
            nextTick += divisor;
            requestIrq(0);
        }
        if (input <= now) {
            takeInput(now);
        }
        deliverInterrupts();
    }
    /* A handler that waited has let time pass beyond until already. */
    now = std::max(now, until);
    audioOutputTime(now);
    deliverInterrupts();
}

void (*portPassHook)() = nullptr;

/* The clock's own at a tick, the original's BIOS's (INT 8, which BiosTimer
 * passes the ticks on to): its count of the day's ticks, which nothing
 * reads in the port, and the tick's end. */
void clockOwnTick()
{
    const SystemSide systemSide;
    endInterrupt(0);
}

/* The ticks' line: to the game's handler, which ends it itself
 * (clockTickDone), early, as the kernel does; else the clock's own. */
void timerLine()
{
    passDumpTick();
    if (tickHandler != nullptr) {
        const GameSide gameSide;
        tickHandler();
    } else {
        clockOwnTick();
    }
}

void clockTicks(void (*handler)())
{
    const SystemSide systemSide;
    tickHandler = handler;
}

/* A new divisor counts from the time it is set; 0 is 65536. */
void clockTickDivisor(std::uint16_t newDivisor)
{
    const SystemSide systemSide;
    divisor = (newDivisor != 0U) ? newDivisor : 65536;
    nextTick = now + divisor;
}

void clockTicksOn()
{
    const SystemSide systemSide;
    unmaskIrq(0);
}

void clockTicksOff()
{
    const SystemSide systemSide;
    maskIrq(0);
}

void clockTickDone()
{
    const SystemSide systemSide;
    endInterrupt(0);
}

namespace {

/* Whether NORMAL gives the tick one pass: three ticks in ten, by a hash of
 * its number (splitmix64's), so that a walk's frames end on a tick of one
 * pass as often as that whatever their phase. A regular pattern does not:
 * the frames, two ticks each with a tick more in some, fall in step with
 * its period, and a walk went faster at a seventh of the ticks than at a
 * ninth. */
bool onePassTick(std::uint64_t tick)
{
    std::uint64_t mixed = tick + 0x9E3779B97F4A7C15ULL;
    mixed = (mixed ^ (mixed >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    mixed = (mixed ^ (mixed >> 27U)) * 0x94D049BB133111EBULL;
    mixed ^= mixed >> 31U;
    return (mixed >> 11U) < ONE_PASS_TICKS;
}

/* The virtual time the pass that ends at now takes: SLOW's a sixtieth of a
 * second (the passes counted before it give the sixtieths, 60 of them a
 * second to the count); NORMAL's and FAST's to the middle of the tick that
 * nextTick ends or to its end, which its number (one more each tick, the
 * timer's state alone) makes the same after a rewind or a state's load. */
std::uint64_t passLength(MachineSpeed chosen, std::uint64_t pass)
{
    if (chosen == MachineSpeed::SLOW || divisor >= SLOWEST_TWO_PASS_DIVISOR) {
        return ((pass + 1) * PIT_HZ / 60) - (pass * PIT_HZ / 60);
    }
    const bool onePass = chosen == MachineSpeed::NORMAL && onePassTick(nextTick / divisor);
    const std::uint64_t middle = nextTick - (divisor / 2);
    return (!onePass && now < middle) ? middle - now : nextTick - now;
}

}

void clockSetMachineSpeed(MachineSpeed chosen)
{
    const SystemSide systemSide;
    machineSpeed = chosen;
}

MachineSpeed clockMachineSpeed()
{
    const SystemSide systemSide;
    return machineSpeed;
}

const char *clockMachineSpeedName(MachineSpeed named)
{
    return MACHINE_SPEED_NAMES[static_cast<std::size_t>(named)];
}

bool clockMachineSpeedNamed(const char *name, MachineSpeed *named)
{
    for (std::size_t i = 0; i < std::size(MACHINE_SPEED_NAMES); i++) {
        if (std::strcmp(name, MACHINE_SPEED_NAMES[i]) == 0) {
            *named = static_cast<MachineSpeed>(i);
            return true;
        }
    }
    return false;
}

void clockPass()
{
    const SystemSide systemSide;
    const std::uint64_t step = passLength(machineSpeed, frames);
    frames++;
    std::uint64_t until = now + step;
    if (replaying()) {
        until = std::max(until, replayPass(frames));
    } else if (live) {
        const std::uint64_t wall = wallTime();
        if (wall > until) {
            until = wall;
            recordPass(frames, until);
        }
    }
    passDumpTime('P', until);
    passTime(until);
    runScriptActions();
    if (portPassHook != nullptr) {
        portPassHook();
    }
}

void clockWait()
{
    const SystemSide systemSide;
    if (!interruptsEnabled()) {
        portEnd(3, "the game waits with the interrupts off, for ever");
    }
    const std::uint64_t until = std::min(nextTick, nextInputTime());
    passDumpTime('W', until);
    passTime(until);
}

void clockDelay(unsigned milliseconds)
{
    const SystemSide systemSide;
    const std::uint64_t until = now + borlandDelayCounts(milliseconds, divisor, clockTickPhase());
    passDumpTime('D', until);
    passTime(until);
}

void clockPassCounts(std::uint32_t counts)
{
    const SystemSide systemSide;
    passDumpTime('C', now + counts);
    passTime(now + counts);
}

std::uint32_t clockTickPhase()
{
    const SystemSide systemSide;
    return static_cast<std::uint32_t>(divisor - (nextTick - now));
}

ClockDate clockDate()
{
    const SystemSide systemSide;
    using namespace std::chrono;
    if (std::optional<ClockDate> replayed = replayDate()) {
        return *replayed;
    }
    year_month_day day;
    if (windowRun()) {
        const std::time_t host = std::time(nullptr);
        const std::tm local = *std::localtime(&host);
        day = year{local.tm_year + 1900} / (local.tm_mon + 1) / local.tm_mday;
    } else {
        day = sys_days{year{1996} / January / 1} + days{static_cast<long>(now / PIT_HZ / 86400)};
    }
    ClockDate date = {static_cast<std::uint16_t>(static_cast<int>(day.year())),
                      static_cast<std::uint8_t>(static_cast<unsigned>(day.month())),
                      static_cast<std::uint8_t>(static_cast<unsigned>(day.day())),
                      static_cast<std::uint8_t>(weekday{sys_days{day}}.c_encoding())};
    if (windowRun()) {
        recordDate(date);
    }
    return date;
}

std::int64_t clockWallTime()
{
    const SystemSide systemSide;
    return std::time(nullptr);
}

TimerState timerState()
{
    return {now, divisor, nextTick, tickHandler, frames, windowFrames, nextWindowFrame};
}

void putTimerState(const TimerState &state)
{
    now = state.now;
    divisor = state.divisor;
    nextTick = state.nextTick;
    tickHandler = state.tickHandler;
    frames = state.frames;
    windowFrames = state.windowFrames;
    nextWindowFrame = state.nextWindowFrame;
    /* The wall clock goes on from the time put back. */
    wallStart = Clock::now();
    wallVirtual = now;
}
