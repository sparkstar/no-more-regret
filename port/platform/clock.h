/* The game's clock: the port's time, which the game lets pass where its
 * loops pass or wait and nowhere else, so that the same input gives the same
 * run. What comes in the time that passes (the timer's ticks, the input's
 * events) comes there, in its order. A run with a window keeps the time to
 * the wall clock; a run a script drives has the virtual time alone. The
 * system layer's timer (port/system/timer.cpp) is the clock; its ticks are
 * the game's timer, which the original had from the interval timer's
 * interrupt. */
#ifndef PORT_PLATFORM_CLOCK_H
#define PORT_PLATFORM_CLOCK_H
#include <cstdint>

/* A pass of the game's loops (the kernel's runProcesses, a modal gump's)
 * ends: the time a pass takes on the original's machine passes, as fast as
 * the machine's speed has it (MachineSpeed). */
void clockPass();
/* The speed of the original's machine the passes give, the settings' GAME
 * SPEED. A process that waits goes on in the pass after its wait ends, so
 * the passes a tick of the game's timer has set how long the waits of the
 * walks, the people and the usecode's steps take; the timer's own times
 * (the usecode's TIMER) stay:
 *     SLOW    a pass a sixtieth of a second, about one a tick: an
 *             animation's frame of two ticks takes three
 *     NORMAL  two passes a tick, at its start and its middle, but one in
 *             three ticks of ten, picked by a hash of the tick's number:
 *             the walks at about 90% of FAST's
 *     FAST    two passes every tick: the walks of a fast machine, which
 *             more passes do not make faster
 * A run's speed is a setting the player chose (platform/gamestate.h's
 * gameSetting). A run starts at the speed REGRET_GAME_SPEED names (the
 * tests' slow, at whose passes their results came), else NORMAL, and
 * enhanced.cfg's game_speed= comes after it. While the ticks come fewer
 * than 30 a second (the BIOS's, before the kernel sets its own) a pass is
 * SLOW's at every speed. */
enum class MachineSpeed : std::uint8_t { SLOW, NORMAL, FAST };
void clockSetMachineSpeed(MachineSpeed chosen);
[[nodiscard]] MachineSpeed clockMachineSpeed();
/* A speed's name (slow, normal, fast), and the speed of a name: false for
 * one it does not know. */
[[nodiscard]] const char *clockMachineSpeedName(MachineSpeed named);
[[nodiscard]] bool clockMachineSpeedNamed(const char *name, MachineSpeed *named);
/* A loop waits for what a tick or the input changes: the time goes on to
 * the next of them. While the game holds the clock nothing can come, which
 * in the original waits for ever: the run ends. */
void clockWait();
/* Borland's delay of milliseconds: the time it took in the original, which
 * reads the interval timer's counter as the BIOS set it, and under the
 * kernel's faster timer waits a part of them. */
void clockDelay(unsigned milliseconds);
/* counts of the interval timer's input clock pass (1193182 a second): a
 * part of the game that times itself by the timer's counter
 * (JOYSTICK.ASM's readings of the game port). */
void clockPassCounts(std::uint32_t counts);
/* The interval timer's counts since its last tick, 0 to the divisor less
 * 1: its counter as the original read it back. */
std::uint32_t clockTickPhase();

/* The game's timer: its handler at each tick while the ticks are on and the
 * game does not hold them (the original's timer interrupt, IRQ 0), or
 * without one the clock's own (the BIOS's count of the day's ticks). The
 * ticks come every divisor counts of the interval timer (1193182 a second;
 * 0 for 65536, the BIOS's 18.2 a second). A tick that comes while they are
 * off waits for them on. */
void clockTicks(void (*handler)());
void clockTickDivisor(std::uint16_t divisor);
void clockTicksOn();
void clockTicksOff();
/* The handler lets the next tick come, inside it if the time passes there
 * (the original's end of interrupt, which the kernel sends early). */
void clockTickDone();
/* The clock's own at a tick, what the original's BIOS did (INT 8), which
 * BiosTimer passes the game's ticks on to: the tick's end. (The day's count
 * of ticks the BIOS kept, nothing reads.) */
void clockOwnTick();
/* The game's hold on what the clock brings (the original's interrupt flag):
 * while it holds, no tick and no input comes, and clockWait never ends (the
 * run ends). A handler runs held until it lets go. Push and pop keep the
 * hold around a part that holds (pushf, cli ... popf). */
void clockHold();
void clockRelease();
void clockPushHold();
void clockPopHold();

/* A day as DOS gives it (_dos_getdate); weekday 0 is Sunday. */
struct ClockDate {
    std::uint16_t year;
    std::uint8_t month;
    std::uint8_t day;
    std::uint8_t weekday;
};
/* Today: the host's in a run with a window; in a run a script drives, the
 * virtual time's day from 1 January 1996, so that the run is the same on
 * any day (the music is Christmas's from 24 to 31 December). */
ClockDate clockDate();
/* The host's wall clock: the seconds since 1970 (C's time), the time stamps
 * TAGFLEX.C gives the sound archive's names. Not the game's time, which is
 * the virtual time. */
std::int64_t clockWallTime();
#endif
