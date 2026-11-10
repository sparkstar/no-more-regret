/* What the system layer shares of the platform's clock, input and sound.
 * The port has no interrupts: time is virtual, counted in the ticks of the
 * original's interval timer, and it passes where the game's loops pass
 * (clockPass) or wait (clockWait, clockDelay). There the events of that
 * time come on their lines, when the hold and the lines' rules let them
 * (interrupts.cpp): the ticks (line 0), the scan codes (1) and the mouse's
 * events (12) to the game's handlers; and the sound's output takes its
 * samples (audio.cpp). */
#ifndef PORT_PC_H
#define PORT_PC_H
#include "common.h"
#include "input.h"

/* The interval timer's input clock: its ticks count the virtual time. */
const std::uint64_t PIT_HZ = 1193182;

/* interrupts.cpp: the hold and the lines. */
/* A line asks: one request a line waits. */
void requestIrq(u8 irq);
/* The requests the hold and the lines' rules let through, to their
 * handlers. */
void deliverInterrupts();
/* The hold is off (the original's interrupt flag set). */
bool interruptsEnabled();
/* The lines that are on. */
bool irqUnmasked(u8 irq);

/* timer.cpp: the virtual time, and how many times as fast as the wall
 * clock it goes now (4 while the window fast forwards, else 1). */
std::uint64_t pcTime();
std::uint64_t clockSpeed();
/* The wall clock taken from now (a pause's end, rewind.cpp): the time it
 * stood is not the game's. */
void restartWallClock();
/* Lets time pass to until, the events of it coming in turn. */
void passTime(std::uint64_t until);
/* The ticks' line (line 0). */
void timerLine();
/* The game's step has ended (end.cpp's playGame): whether a frame passed in
 * it, after which the host's work of the frame runs. Of a step's frames,
 * those of its last passTime passed at its top, and those of a passTime
 * another followed in the same step inside one of the game's waits, which
 * framesInWaits counts for the run: the host's work runs after none of them. */
[[nodiscard]] bool clockStepEnd();
[[nodiscard]] std::uint64_t framesInWaits();
/* Called after each pass of the game's loop (clockPass), when set
 * (statelog.cpp). */
extern void (*portPassHook)();
/* statelog.cpp's count of the passes it was called after (a script's
 * action, run in a pass, may run passes in it), which a snapshot's test
 * puts back with the replay's place (snapshot.h). */
[[nodiscard]] std::uint64_t stateLogPasses();
void putStateLogPasses(std::uint64_t count);

/* keyboard.cpp: the keyboard. */
/* A scan code the keyboard sends. One at a time is on its way to the
 * game, on line 1; keyboardFeed puts the next on its way when none is,
 * true when it did. */
void keyboardScan(u8 scan);
/* An event of the other devices (input.h), which comes on the keys' line
 * after the scan codes sent before it. */
void keyboardDevice(const DeviceEvent &event);
bool keyboardFeed();
/* The keyboard's typematic repeat: the time of the next, or ~0 for none,
 * and the repeats due by time sent. */
std::uint64_t keyboardRepeatTime();
void keyboardRepeat(std::uint64_t time);
/* The keys' line (line 1). */
void keyboardLine();

/* mouse.cpp: the mouse's line (line 12), which moves the cursor and
 * presses the buttons of the next mouse event and calls the game's
 * handler. */
void mouseLine();

/* audio.cpp: the output's samples up to the time out (audio.h), which
 * passTime has it take before the events of a time. */
void audioOutputTime(std::uint64_t now);

/* script.cpp: the events of the keyboard and the mouse, at their times
 * (the scripted input, and the keyboard's repeats). The next one's
 * time, or ~0 for none. */
std::uint64_t nextInputTime();
/* The events due at the time come: scan codes to the keyboard, the
 * mouse's to its driver; then the keyboard's repeats. */
void takeInput(std::uint64_t time);
#endif
