/* KERNEL.C (No Regret's 11e8:0000-42a8, No Remorse's 11d0:0000-4287; the
 * addresses No Regret's): the process table, the scheduler, the timer and
 * keyboard interrupts and the subscriber lists of the timer, keyboard and
 * mouse. */
#ifndef PORT_KERNEL_H
#define PORT_KERNEL_H
#include "common.h"

#include <vector>
#include "priority.h"

class Process;
class KeyEvent;
class Flex;

/* The game's sizes: the key queue (KEY_QUEUE) and the items the kernel
 * keeps maps for (KERNEL_ITEMS, which may use ITEMS below). */
#include "kernel_game.h"
/* Scan codes the key states are kept for. */
constexpr int SCAN_CODES = 128;
/* The item numbers and process types the kernel keeps maps of. Item 0 is
 * none; the NPCs are 1-255. */
constexpr int ITEMS = 0x2800;
constexpr int TYPES = 0x1000;
/* Microseconds between two ticks of the BIOS timer (18.2 Hz). */
constexpr s32 BIOS_TICK = (static_cast<s32>(54920L));

/* A key held down, for KeyDaemon's repeat: when it went down and how many
 * repeats it has had. */
struct KeyState {
    u32 time;
    u16 count;
};

/* MouseEvent::events: the mouse driver's event mask. */
constexpr int MOUSE_MOVED = 0x1;
constexpr int MOUSE_LEFT_DOWN = 0x2;
constexpr int MOUSE_LEFT_UP = 0x4;
constexpr int MOUSE_RIGHT_DOWN = 0x8;
constexpr int MOUSE_RIGHT_UP = 0x10;
constexpr int MOUSE_MIDDLE_DOWN = 0x20;
constexpr int MOUSE_MIDDLE_UP = 0x40;
/* MouseControl records both buttons pressed or released together as these
 * (a chord, with the middle button's bit). */
constexpr int MOUSE_BOTH_DOWN = (MOUSE_LEFT_DOWN | MOUSE_RIGHT_DOWN | MOUSE_MIDDLE_DOWN);
constexpr int MOUSE_BOTH_UP = (MOUSE_LEFT_UP | MOUSE_RIGHT_UP | MOUSE_MIDDLE_UP);
/* MouseEvent::buttons: the buttons down. */
constexpr int MOUSE_BUTTON_LEFT = 0x1;
constexpr int MOUSE_BUTTON_RIGHT = 0x2;

/* What EventProcess::event gets for EVENT_MOUSE (a far pointer as data). */
class MouseEvent {
public:
    u16 events;
    u16 buttons;
    u16 x;
    u16 y;
    MouseEvent() = default;
};
ASSERT_SIZE(KeyState, 6);
ASSERT_SIZE(MouseEvent, 8);

extern u16 &pendingKeys; /* 1480:3b48 */
/* The keyboard interrupt's queue (keyQueue in KERNEL.C): the next key to
 * read and the next free place (1480:3b6e, 3b70). */
extern u16 &keyRead;
extern u16 &keyWrite;
extern u16 &keyShift;                        /* 1480:3b4a: the shift state of the next key event */
extern u16 &timerDivisor;                    /* 1480:4434: the PIT divisor of the kernel's timer */
extern KeyState (&plainKeys)[SCAN_CODES];    /* 1480:3db4: by scan code */
extern KeyState (&extendedKeys)[SCAN_CODES]; /* 1480:40b4: the E0h-prefixed keys */
/* killAll is killing the processes (1480:3b6b). */
extern u8 &killingAll;
/* CRUDEBUG=TRUE: Ctrl+Alt+Esc breaks into the debugger (1480:4415). */
extern u8 &debugKeys;
extern u8 &interruptNested;        /* 1480:43f7: the timer interrupt came in again */
extern u16 &deadProcesses;         /* 1480:4404 */
extern u16 &pollingPid;            /* 1480:4406: the pid StdIntHandler::run is polling */
extern u8 &inInterrupt;            /* 1480:440e: running processes from the timer interrupt */
extern u16 &mouseHeld;             /* 1480:4413: mouse events are not passed on */
extern u16 *&processSizes;         /* 1480:441a: the size of each process, for saveKernel */
extern u8 &kernelActive;           /* 1480:4428 */
extern u16 *&priorityCounts;       /* 1480:442b: processes at each priority level */
extern u8 &currentPriority;        /* 1480:442f */
extern const char **&processNames; /* 1480:4430 */
extern u32 &timerStep;             /* 1480:4436: microseconds per timer interrupt */
extern u16 &pausedTimers;          /* 1480:445a */
extern u16 &runningTimers;         /* 1480:445c */
extern u8 &nestedInterrupts;       /* 1480:445f: the timer interrupt may run nested */
extern u32 &frameTicks;            /* 1480:4466: the ticks the last frame took */
/* initKernel sets it, exitKernel clears it: the processes are there. */
extern u8 &kernelStarted;

/* Whether the keyboard interrupt has queued keys. */
inline bool keysQueued()
{
    return keyRead != keyWrite;
}

inline bool isValid(void *pointer)
{
    return pointer != nullptr;
}
inline void nameProcess(u16 pid, const char *name)
{
    processNames[pid] = name;
}
/* UNCALLED: nothing calls it. */
inline void setProcessSize(u16 pid, u16 size)
{
    processSizes[pid] = size;
}
inline void addToLevel(u8 level)
{
    priorityCounts[level]++;
}
inline void removeFromLevel(u8 level)
{
    priorityCounts[level]--;
}

/* A process may be changed from the timer interrupt only when it cannot have
 * interrupted another. */
#define KERNEL_SAFE() (!inInterrupt || (nestedInterrupts && !interruptNested))

void initKernel();
void exitKernel();
void runProcesses();
u16 countItemProcesses(u16 item, i16 type);
u16 countProcesses(u16 item, i16 type);
/* UNCALLED: nothing calls it. */
u16 registerProcess(Process *process);
Process *findProcess(u16 pid);
void killItem(u16 item, i16 type);
void killItemProcesses(u16 item, i16 type, Priority condition);
/* The types end with TYPE_END. */
void killItemTypes(u16 item, Priority condition, ...);
void killItemExcept(u16 item, Priority condition, ...);
void killAll();
void addWait(u16 waiter, u16 pid);
Process *firstProcess(u16 item, i16 type);
Process *nextProcess();
Process *findItemProcess(u16 item, i16 type);
Process *nextItemProcess();
/* UNCALLED: nothing calls it. */
void resumeProcess(u16 pid);
u16 currentPid();
/* UNCALLED: nothing calls it. */
void suspendProcess(Process *process);
void releasePid(u16 pid);
void setCurrentPriority(u8 level);
void wakeProcess(Process *process);
/* UNCALLED: nothing calls it. */
void broadcastMessage(u16 message, u16 filter, u16 mask, u16 from, void *data);
void setNewProcessSize(u16 size);
/* The size Process::operator new has left for the process its constructor
 * registers, 0 when there is none. */
extern u16 &newProcessSize; /* 1480:43fb */
void freeDeadProcesses();
void loadKernel(Flex *flex, i16 index);
void saveKernel(Flex *flex, i16 index);
/* The port's: whether a saved game's file (its bytes) has the kernel's
 * objects from index as loadKernel can take them: the FLEX's objects in
 * the file, the block of globals of the original's size with the guard of
 * a save, the tables of their sizes, and each saved process of a class the
 * port knows, of its size. Where it has not, loadKernel would end the run
 * (or the original read the bytes as they were). */
[[nodiscard]] bool kernelSaveLoadable(const std::vector<u8> &file, i16 index);
/* saveKernel's objects without its work before and after them: the port's,
 * for the pass dump. */
void writeKernel(Flex *flex, i16 index);
void addTimer(u16 pid, u8 run);
void removeTimer(u16 pid);
void setTimerStep(u32 step);
/* A pass of the kernel: runs the processes, calls idle and waits until
 * ticks timer ticks have passed since the pass began. The original's ran
 * its passes for ever (the game ended by exit); the port's game step calls
 * it once a step (gamemain.cpp's gameStep). */
void runKernel(u32 ticks, void (*idle)());
/* The process table may grow (1480:4427); main sets it. */
extern u8 &tableGrows;
/* The kernel runs its mouse list; cleared while a game is saved or loaded
 * (1480:3b45). */
extern u8 &kernelReady;
void restoreTimer();
void maskTimer();
void flushKeys();
bool keyboardPaused();
void dispatchKey(KeyEvent *key);
void queueKey(KeyEvent *key);
u16 keyRepeats(u8 scan, u8 extended);
u8 queuedKeys();
void addKeyboardListener(Process *process);
void removeKeyboardListener(Process *process);
void markItemProcess(u16 item);
void updateItemProcess(u16 item);
void markItemType(u16 item, i16 type);
void updateItemType(u16 item, i16 type);
bool itemHasType(u16 item, i16 type);
/* Called by the mouse driver's event handler (mousecb.asm). */
extern "C" void mouseEvent(u16 events, u16 buttons, u16 x, u16 y);
/* UNCALLED: nothing calls it. */
void addMouseListener(Process *process);
void removeMouseListener(Process *process);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void startKernel();
/* UNCALLED: nothing calls it. */
void stopKernel();
/* UNCALLED: nothing calls it. */
void releaseInterrupts();
/* UNCALLED: nothing calls it. */
Process *processAt(u16 pid);
/* UNCALLED: nothing calls it. */
void setTypematicDelay(u8 delay);
/* UNCALLED: nothing calls it. */
void setTypematicRate(u8 rate);
/* UNCALLED: nothing calls it. */
void checkTable();
/* UNCALLED: nothing calls it. */
void countType();
#endif
