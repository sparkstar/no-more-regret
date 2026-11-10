/* KERNEL.C (No Regret's 11e8:0000-42a8, No Remorse's 11d0:0000-4287): the
 * process table, the scheduler, the timer and keyboard interrupts, the
 * subscriber lists and the item maps; both games', the addresses No
 * Regret's. See kernel.h. */
#include "gamestate.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <typeinfo>
#include <vector>
#include "devices.h"
#include "kernel.h"
#include "waits.h"
#include "kprocess.h"
#include "savefields.h"
#include "stdinter.h"
#include "systimer.h"
#include "keyboard.h"
#include "keyio.h"
#include "kdaemon.h"
#include "biostimr.h"
#include "mouse.h"
#include "input.h"
#include "flex.h"
#include "clock.h"
#include "host.h"
#include "trace.h"

void updatePriority();
void trimTable();
void resizeTable(u16 size);
void clearSlot(u16 pid);
bool runsLater(Process *process);
void allocateTables();
void freeTables();
void programTimer(u16 divisor);
void setTimerPeriod(u32 period);
void hookTimer();
void unhookTimer();
void enableTimer();
void disableTimer();
void timerInterrupt();
void timerTick();
/* UNCALLED: nothing calls it. */
u16 readBiosShift();
void writeBiosShift(u16 shift);
void readShiftState();
void hookKeyboard();
void unhookKeyboard();
bool keyQueueFull();
KeyEvent nextKey();
void deliverKeys();
/* UNCALLED: nothing calls it. */
void applyTypematic();
bool itemHasProcess(u16 item);
void clearTypeCounts();

/* The process table: its size at start, how much it grows when it is full,
 * how much trimTable takes off, and how many runs of trimTable pass before it
 * tries again. */
constexpr int TABLE_SIZE = 100;
constexpr int TABLE_GROWTH = 30;
constexpr int TABLE_SHRINK = 20;
constexpr int TRIM_DELAY = 150;
constexpr int NO_TRIM = 0xffff;
/* initKernel registers StdIntHandler, BiosTimer and SystemTimer first; the
 * searches and kills start after them. */
constexpr int FIRST_PID = 3;
/* The most types killItemTypes and killItemExcept take. */
constexpr int MAX_TYPES = 10;
/* saveGuard is the last variable saveKernel writes and saveEnd the first it
 * does not; while it writes they hold the SAVING values. loadKernel wants
 * GUARD_SAVING in the block it reads and saveEnd untouched. */
constexpr int GUARD_IDLE = 0xec;
constexpr int GUARD_SAVING = 0xed;
constexpr int END_IDLE = 0xee;
constexpr int END_SAVING = 0xef;
/* The first of the two objects of each process in a saved kernel. */
constexpr int PROCESS_OBJECTS = 15;

u8 &kernelStarted = gameGlobal<u8>(0);
u8 &kernelReady = gameGlobal<u8>(0);
u8 &keyOverflow = gameGlobal<u8>(0);
u8 &keyboardDepth = gameGlobal<u8>(0);
u16 &pendingKeys = gameGlobal<u16>(0);
u16 &keyShift = gameGlobal<u16>(0);
u16 &insertDown = gameGlobal<u16>(0);
/* The make and break codes of the shift, control, alt and lock keys, which
 * the keyboard interrupt passes on to the BIOS. */
constexpr int MODIFIER_SCANS = 14;
u16 (&modifierScans)[MODIFIER_SCANS] = gameGlobal<u16[MODIFIER_SCANS]>({0x36, 0xb6, 0x2a, 0xaa, 0x1d, 0x9d, 0x38, 0xb8,
                                                                        0x46, 0xc6, 0x45, 0xc5, 0x3a, 0xba});
u8 &extendedScan = gameGlobal<u8>(0);
u8 &killingAll = gameGlobal<u8>(0);
u8 &keyboardHooked = gameGlobal<u8>(0);
u16 &keyRead = gameGlobal<u16>(0);
u16 &keyWrite = gameGlobal<u16>(0);
/* Zeroed as static storage, which KeyEvent's empty constructor leaves: the
 * original's array is initialized data that no static constructor touches
 * (1480:3b72). */
KeyEvent (&keyQueue)[KEY_QUEUE_CAPACITY] = gameGlobal<KeyEvent[KEY_QUEUE_CAPACITY]>();
u8 &typematicDelay = gameGlobal<u8>(0);
u8 &typematicRate = gameGlobal<u8>(0);
KeyState (&plainKeys)[SCAN_CODES] = gameGlobal<KeyState[SCAN_CODES]>();
KeyState (&extendedKeys)[SCAN_CODES] = gameGlobal<KeyState[SCAN_CODES]>();
u8 &keyboardReady = gameGlobal<u8>(0);
u8 (&reservedVectors)[12] = gameGlobal<u8[12]>({0});
u8 &timerStateChanged = gameGlobal<u8>(0);
u8 &inTimer = gameGlobal<u8>(0);
u8 &interruptNested = gameGlobal<u8>(0);
u8 &reservedTimer = gameGlobal<u8>(0);
u16 &itemCursor = gameGlobal<u16>(0);
u16 &newProcessSize = gameGlobal<u16>(0);
u8 &killing = gameGlobal<u8>(0);
u16 &markedProcesses = gameGlobal<u16>(0);
u16 &searchItem = gameGlobal<u16>(0);
i16 &searchType = gameGlobal<i16>(ANY_TYPE);
u16 &deadProcesses = gameGlobal<u16>(0);
u16 &pollingPid = gameGlobal<u16>(0);
u8 &priorityChanged = gameGlobal<u8>(0);
u8 &reservedPriority = gameGlobal<u8>(0);
u16 &tickingPid = gameGlobal<u16>(0);
u16 &nestedTickingPid = gameGlobal<u16>(0);
u8 &inInterrupt = gameGlobal<u8>(0);
u8 &timerEnabled = gameGlobal<u8>(0);
u8 &timerHooked = gameGlobal<u8>(0);
u8 &timerWanted = gameGlobal<u8>(0);
u8 &kernelLoaded = gameGlobal<u8>(0);
u16 &mouseHeld = gameGlobal<u16>(0);
u8 &debugKeys = gameGlobal<u8>(0);
Process **&processes = gameGlobal<Process **>(nullptr); /* 1480:4416 */
u16 *&processSizes = gameGlobal<u16 *>(nullptr);
u8 &timerMasked = gameGlobal<u8>(0);
u16 &processLimit = gameGlobal<u16>(TABLE_SIZE);
u16 &processCount = gameGlobal<u16>(0);
u16 &trimCountdown = gameGlobal<u16>(NO_TRIM);
u16 &runningPid = gameGlobal<u16>(0);
u8 &tableGrows = gameGlobal<u8>(1);
u8 &kernelActive = gameGlobal<u8>(0);
u16 &keyboardListeners = gameGlobal<u16>(0);
u16 *&priorityCounts = gameGlobal<u16 *>(nullptr);
u8 &currentPriority = gameGlobal<u8>(BASE_LEVEL);
const char **&processNames = gameGlobal<const char **>(nullptr);
u16 &timerDivisor = gameGlobal<u16>(0);
u32 &timerStep = gameGlobal<u32>(0);
SubscriberList &timerList = gameGlobal<SubscriberList>();
SubscriberList &keyboardList = gameGlobal<SubscriberList>();
u16 &timerRate = gameGlobal<u16>(0);
u16 &pausedTimers = gameGlobal<u16>(0);
u16 &runningTimers = gameGlobal<u16>(0);
u8 &timerDeferred = gameGlobal<u8>(0);
u8 &nestedInterrupts = gameGlobal<u8>(0);
u16 &searchPid = gameGlobal<u16>(0);
u32 &frameStart = gameGlobal<u32>(0);
u32 &frameTicks = gameGlobal<u32>(0);
u8 *&itemProcesses = gameGlobal<u8 *>(nullptr);
u8 *&itemTypes = gameGlobal<u8 *>(nullptr);
u32 *&typeCounts = gameGlobal<u32 *>(nullptr);
SubscriberList &mouseList = gameGlobal<SubscriberList>();
u16 &mouseListeners = gameGlobal<u16>(0);
u8 &saveGuard = gameGlobal<u8>(GUARD_IDLE);
u8 &saveEnd = gameGlobal<u8>(END_IDLE);

/* UNCALLED: nothing calls it. */
void startKernel()
{
    initKernel();
}

/* UNCALLED: nothing calls it. */
void stopKernel()
{
    exitKernel();
}

void initKernel()
{
    const char *debug;
    BiosTimer *biosTimer;

    if (kernelStarted != 0U) {
        halt();
    }
    kernelStarted = 1;
    kernelActive = 1;
    debug = hostEnvironment("CRUDEBUG");
    if ((debug != nullptr) && (stricmp(debug, "TRUE") == 0)) {
        debugKeys = 1;
    }
    allocateTables();
    timerRate = 0;
    /* The original took two real-mode stacks for the timer interrupt
     * (DosAllocRealSeg) and DOS's InDOS flag (INT 21h AH=34h): the port's
     * ticks come to timerInterrupt on the game's stack, and it has no DOS to
     * be busy. */
    clockRelease();
    new StdIntHandler;
    clockTicksOff();
    hookTimer();
    biosTimer = new BiosTimer;
    biosTimer->setVectors();
    biosTimer->startTimer(1);
    new SystemTimer;
    inputKeysOff();
    hookKeyboard();
    inputKeysOn();
    new KeyboardProcess;
    new KeyDaemon;
    keyboardReady = 1;
    inputMouseStart(mouseCallback);
    kernelReady = 1;
    kernelActive = 0;
}

void exitKernel()
{
    if (kernelStarted != 0U) {
        kernelStarted = 0;
        unhookKeyboard();
        disableTimer();
        unhookTimer();
        setTimerPeriod(BIOS_TICK);
        clockTicksOn();
        inputMouseStop();
        freeTables();
    }
}

/* Gives the timer and keyboard back to DOS, for a shell. */
/* UNCALLED: nothing calls it. */
void releaseInterrupts()
{
    if (kernelStarted != 0U) {
        unhookKeyboard();
        disableTimer();
        unhookTimer();
        setTimerPeriod(BIOS_TICK);
        clockTicksOn();
    }
}

/* Where in a pid's turn runProcesses goes on after a wait (the port's): a
 * wait opened in a process's run, in the timer's interrupt, in the keys'
 * or the other devices' delivery ends the pass there, and its rest goes
 * on from the next part when the wait is over. */
enum class TurnPart : u8 { RUN, TIMER, KEYS, DEVICE_KEYS, END };

namespace {
void runProcessesFrom(u16 pid, TurnPart part);

/* While a wait is opened in part of pid's turn: the pass from the part
 * after it, as the rest. */
void giveTurnRest(u16 pid, TurnPart next)
{
    giveRest([pid, next]() {
        runProcessesFrom(pid, next);
        return true;
    });
}
}

void runProcesses()
{
    kernelActive = 1;
    runProcessesFrom(0, TurnPart::RUN);
}

namespace {
/* runProcesses' loop from part of pid's turn on, and the pass's end. The
 * kernel stays active (kernelActive) and the process running (runningPid)
 * while a wait opened in its turn lasts, as in the original, where the
 * wait was inside the pass. */
void runProcessesFrom(u16 pid, TurnPart part)
{
    Process *process;
    u8 level;

    for (; pid < processLimit; pid++, part = TurnPart::RUN) {
        if (part == TurnPart::RUN) {
            process = processes[pid];
            if (isValid(process)) {
                if (process->hasFlags(PROCESS_DEAD) == 0U) {
                    if (process->hasStatus(STATUS_TIMEOUT) != 0U) {
                        process->checkTimeout();
                    }
                    if (process->hasFlags(PROCESS_BLOCKED) != 0U) {
                        process->flags &= ~PROCESS_BLOCKED;
                    } else if ((process->hasStatus(STATUS_TIMEOUT) == 0U) && !process->waitList.valid() &&
                               (process->hasFlags(PROCESS_ACTIVE) == 0U)) {
                        level = process->level();
                        if ((process->hasFlags(PROCESS_SYSTEM) != 0U) || level == currentPriority ||
                            level == SYSTEM_LEVEL || level == ALWAYS_LEVEL) {
                            runningPid = pid;
                            process->run();
                            if (waitOpened()) {
                                giveTurnRest(pid, TurnPart::TIMER);
                                return;
                            }
                            runningPid = 0;
                        }
                    }
                }
            }
            part = TurnPart::TIMER;
        } else if (part == TurnPart::TIMER) {
            /* The rest of a process's run that waited. */
            runningPid = 0;
        }
        if (part == TurnPart::TIMER) {
            if ((timerDeferred != 0U) && (timerEnabled != 0U)) {
                clockPushHold();
                clockHold();
                nestedInterrupts = 1;
                timerInterrupt();
                if (waitOpened()) {
                    giveRest([pid]() {
                        nestedInterrupts = 0;
                        clockPopHold();
                        runProcessesFrom(pid, TurnPart::KEYS);
                        return true;
                    });
                    return;
                }
                nestedInterrupts = 0;
                clockPopHold();
            }
            part = TurnPart::KEYS;
        }
        if (part == TurnPart::KEYS) {
            if (keysQueued()) {
                deliverKeys();
                if (waitOpened()) {
                    giveTurnRest(pid, TurnPart::DEVICE_KEYS);
                    return;
                }
            }
            part = TurnPart::DEVICE_KEYS;
        }
        if (part == TurnPart::DEVICE_KEYS) {
            /* The other devices' keys for the controller (devices.h). */
            deliverDeviceKeys();
            if (waitOpened()) {
                giveTurnRest(pid, TurnPart::END);
                return;
            }
        }
        freeDeadProcesses();
        if (priorityChanged != 0U) {
            priorityChanged = 0;
            break;
        }
    }
    if (priorityCounts[currentPriority] == 0) {
        updatePriority();
    }
    trimTable();
    kernelActive = 0;
    /* The pass's time on the original's machine, and its interrupts. */
    clockPass();
}
}

void updatePriority()
{
    u8 highest;
    u16 pid;
    Process *process;

    highest = 1;
    for (pid = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process)) {
            u8 level;

            level = process->level();
            if (level > highest && level != SYSTEM_LEVEL && level != ALWAYS_LEVEL) {
                highest = level;
            }
        }
    }
    setCurrentPriority(highest);
    priorityChanged = 0;
}

void trimTable()
{
    u16 last;

    if (trimCountdown == NO_TRIM) {
        return;
    }
    trimCountdown--;
    if (trimCountdown != 0) {
        return;
    }
    for (last = processLimit - 1; last >= TABLE_SIZE; last--) {
        if (isValid(processes[last])) {
            break;
        }
    }
    if (last == static_cast<ui16>(TABLE_SIZE - 1) && static_cast<ui16>(processCount + TABLE_GROWTH) < TABLE_SIZE) {
        resizeTable(TABLE_SIZE);
    } else {
        u16 limit;

        limit = processLimit - TABLE_SHRINK;
        if (limit > last && static_cast<ui16>(processCount + TABLE_GROWTH) < limit) {
            resizeTable(limit);
        } else {
            trimCountdown = TRIM_DELAY;
        }
    }
}

namespace {
/* A process's end: its C++ destructor (the members': its kill ran the
 * destructors' work, onKill) and the storage Process::operator new took. A
 * process freed without a kill (the kernel's end) has no onKill, as the
 * original frees it without its destructor. */
void freeProcess(Process *process)
{
    process->~Process();
    ::operator delete(process);
}
}

void resizeTable(u16 size)
{
    u16 pid;
    Process **newProcesses;
    Process **oldProcesses;
    u16 *newSizes;
    u16 *oldSizes;
    const char **newNames;
    const char **oldNames;

    newSizes = new u16[size];
    newProcesses = new Process *[size];
    newNames = new const char *[size];
    for (pid = 0; pid < size; pid++) {
        newProcesses[pid] = processes[pid];
        newSizes[pid] = processSizes[pid];
        newNames[pid] = processNames[pid];
    }
    oldProcesses = processes;
    oldSizes = processSizes;
    oldNames = processNames;
    clockPushHold();
    clockHold();
    processes = newProcesses;
    processSizes = newSizes;
    processNames = newNames;
    processLimit = size;
    if (size != TABLE_SIZE) {
        trimCountdown = TRIM_DELAY;
    } else {
        trimCountdown = NO_TRIM;
    }
    clockPopHold();
    delete[] oldProcesses;
    delete[] oldSizes;
    delete[] oldNames;
}

namespace {
/* UNCALLED: nothing calls it. */
bool growTable()
{
    u16 pid;
    u16 size;
    Process **newProcesses;
    Process **oldProcesses;
    u16 *newSizes;
    u16 *oldSizes;
    const char **newNames;
    const char **oldNames;

    size = processLimit + TABLE_GROWTH;
    if (tableGrows == 0U) {
        halt();
    }
    if (size > PID_MASK) {
        halt();
    }
    newProcesses = new Process *[size];
    newSizes = new u16[size];
    newNames = new const char *[size];
    memset(newNames, 0, size * sizeof(char *));
    for (pid = 0; pid < processLimit; pid++) {
        newProcesses[pid] = processes[pid];
        newSizes[pid] = processSizes[pid];
        newNames[pid] = processNames[pid];
    }
    for (pid = processLimit; pid < size; pid++) {
        newProcesses[pid] = nullptr;
        newSizes[pid] = 0;
        newNames[pid] = nullptr;
    }
    oldProcesses = processes;
    oldSizes = processSizes;
    oldNames = processNames;
    clockPushHold();
    clockHold();
    processes = newProcesses;
    processSizes = newSizes;
    processNames = newNames;
    processLimit += TABLE_GROWTH;
    trimCountdown = TRIM_DELAY;
    clockPopHold();
    delete[] oldProcesses;
    delete[] oldSizes;
    delete[] oldNames;
    return true;
}
}

u16 countItemProcesses(u16 item, i16 type)
{
    u16 pid;
    u16 count;
    Process *process;

    for (pid = 0, count = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && process->item == item && (type == ANY_TYPE || process->type == type) &&
            (process->hasFlags(PROCESS_DEAD) == 0U)) {
            count++;
        }
    }
    return count;
}

u16 countProcesses(u16 item, i16 type)
{
    u16 pid;
    u16 count;
    Process *process;

    for (pid = 0, count = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && process->item == item && (type == ANY_TYPE || process->type == type) &&
            (process->hasFlags(PROCESS_DEAD) == 0U)) {
            count++;
        }
    }
    return count;
}

namespace {
/* UNCALLED: nothing calls it. */
u16 allocatePid()
{
    u16 pid;
    Process **slot;

    do {
        slot = processes;
        for (pid = 0; pid < processLimit; pid++) {
            if (!isValid(*slot++)) {
                return pid;
            }
        }
    } while (growTable());
    return 0xffff;
}
}

/* UNCALLED: nothing calls it. */
u16 registerProcess(Process *process)
{
    u16 pid;

    if (kernelStarted == 0U) {
        halt();
    }
    if (!isValid(process)) {
        halt();
    }
    pid = allocatePid();
    processes[pid] = process;
    process->setPid(pid);
    setProcessSize(pid, newProcessSize);
    newProcessSize = 0;
    processCount++;
    if (killing != 0U) {
        markedProcesses++;
        process->status |= STATUS_NEW;
    }
    processNames[pid] = "Default";
    traceProcessMade(pid, currentPid());
    return pid;
}

/* UNCALLED: nothing calls it. */
Process *processAt(u16 pid)
{
    Process *process;

    if (pid >= processLimit) {
        return nullptr;
    }
    process = processes[pid];
    return process;
}

Process *findProcess(u16 pid)
{
    Process *process;

    if (pid == 0) {
        return nullptr;
    }
    if (pid >= processLimit) {
        return nullptr;
    }
    process = processes[pid];
    if (!isValid(process)) {
        return nullptr;
    }
    /* The KERNEL.C of MOD2AMF.EXE halts here, as it does for the three tests
     * above; this one keeps only the test. */
    if (process->hasFlags(PROCESS_DEAD) != 0U) {
        /* empty in the original */
    }
    return process;
}

void killItem(u16 item, i16 type)
{
    killItemProcesses(item, type, Priority(PRIORITY_EQUAL | BASE_LEVEL));
}

namespace {
void unmarkNewProcesses()
{
    u16 pid;
    u16 unmarked;
    Process *process;

    unmarked = 0;
    if (killing == 0U) {
        halt();
    }
    if (markedProcesses != 0U) {
        for (pid = FIRST_PID; pid < processLimit; pid++) {
            process = processes[pid];
            if (process->hasStatus(STATUS_NEW) != 0U) {
                process->status &= ~STATUS_NEW;
                unmarked++;
                if (unmarked == markedProcesses) {
                    break;
                }
            }
        }
    }
}
}

void killItemProcesses(u16 item, i16 type, Priority condition)
{
    Process *process;
    u16 pid;

    if (killing != 0U) {
        halt();
    }
    killing = 1;
    markedProcesses = 0;
    if (!condition.isSet()) {
        halt();
    }
    if (itemHasProcess(item)) {
        for (pid = FIRST_PID; pid < processLimit; pid++) {
            process = processes[pid];
            if (isValid(process) && (item == 0 || process->item == item) &&
                (type == ANY_TYPE || process->type == type) && (process->hasFlags(PROCESS_SYSTEM) == 0U) &&
                (process->hasFlags(PROCESS_KILLED) == 0U) && condition.holds(process->level()) &&
                (process->hasStatus(STATUS_NEW) == 0U)) {
                process->fail(0);
            }
        }
    }
    unmarkNewProcesses();
    killing = 0;
}

void killItemTypes(u16 item, Priority condition, ...)
{
    i16 kind;
    Process *process;
    u16 pid;
    u16 i;
    u16 count;
    va_list arguments;
    i16 type;
    i16 types[MAX_TYPES];

    if (killing != 0U) {
        halt();
    }
    killing = 1;
    markedProcesses = 0;
    if (!condition.isSet()) {
        halt();
    }
    va_start(arguments, condition);
    for (count = 0; count < MAX_TYPES; count++) {
        type = va_arg(arguments, int);
        types[count] = type;
        if (type == TYPE_END) {
            break;
        }
    }
    if (count == MAX_TYPES) {
        halt();
    }
    if (itemHasProcess(item)) {
        for (pid = FIRST_PID; pid < processLimit; pid++) {
            process = processes[pid];
            if (isValid(process) && process->item == item && (process->hasFlags(PROCESS_SYSTEM) == 0U) &&
                (process->hasFlags(PROCESS_KILLED) == 0U) && condition.holds(process->level())) {
                kind = process->type;
                for (i = 0; i < count; i++) {
                    if (types[i] == kind) {
                        if (process->hasStatus(STATUS_NEW) == 0U) {
                            process->fail(0);
                        }
                        break;
                    }
                }
            }
        }
    }
    unmarkNewProcesses();
    killing = 0;
}

void killItemExcept(u16 item, Priority condition, ...)
{
    i16 kind;
    Process *process;
    u16 pid;
    u16 i;
    u16 count;
    va_list arguments;
    i16 type;
    i16 types[MAX_TYPES];

    if (killing != 0U) {
        halt();
    }
    killing = 1;
    markedProcesses = 0;
    if (!condition.isSet()) {
        halt();
    }
    va_start(arguments, condition);
    for (count = 0; count < MAX_TYPES; count++) {
        type = va_arg(arguments, int);
        types[count] = type;
        if (type == TYPE_END) {
            break;
        }
    }
    if (count == MAX_TYPES) {
        halt();
    }
    if (itemHasProcess(item)) {
        for (pid = FIRST_PID; pid < processLimit; pid++) {
            process = processes[pid];
            if (isValid(process) && process->item == item && (process->hasFlags(PROCESS_SYSTEM) == 0U) &&
                (process->hasFlags(PROCESS_KILLED) == 0U) && condition.holds(process->level())) {
                kind = process->type;
                for (i = 0; i < count; i++) {
                    if (types[i] == kind) {
                        break;
                    }
                }
                if (i == count && (process->hasStatus(STATUS_NEW) == 0U)) {
                    process->fail(0);
                }
            }
        }
    }
    unmarkNewProcesses();
    killing = 0;
}

void killAll()
{
    Process *process;
    EventProcess *events;
    u16 pid;

    killingAll = 1;
    for (pid = FIRST_PID; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process)) {
            if ((process->hasFlags(PROCESS_KILLED) != 0U) && (process->hasFlags(PROCESS_DEAD) == 0U)) {
                halt();
            }
            if (process->hasFlags(PROCESS_EVENTS) != 0U) {
                events = static_cast<EventProcess *>(process);
                if ((events->eventMask & EVENT_KILL_ALL) != 0U) {
                    events->event(EVENT_KILL_ALL, 0);
                }
            }
            if ((process->hasFlags(PROCESS_SYSTEM) == 0U) && (process->hasFlags(PROCESS_KILLED) == 0U)) {
                process->fail(0);
            }
        }
    }
    killingAll = 0;
}

void addWait(u16 waiter, u16 pid)
{
    const MListEntry entry = pid;
    Process *process;

    if ((waiter != 0U) && (pid != 0U)) {
        process = findProcess(waiter);
        process->waitList.append(pid);
        if (entry.isPid()) {
            findProcess(pid)->addWaiter(waiter);
            process->addWaitingFor(pid);
        }
    }
}

Process *firstProcess(u16 item, i16 type)
{
    Process *process;

    searchItem = item;
    searchType = type;
    for (searchPid = FIRST_PID; searchPid < processLimit; searchPid++) {
        process = processes[searchPid];
        if (isValid(process) && (item == 0 || process->item == item) && (type == ANY_TYPE || process->type == type) &&
            (process->hasFlags(PROCESS_DEAD) == 0U)) {
            searchPid++;
            return process;
        }
    }
    return nullptr;
}

Process *nextProcess()
{
    Process *process;
    u16 item;
    i16 type;

    item = searchItem;
    type = searchType;
    if (searchPid >= processLimit) {
        return nullptr;
    }
    for (; searchPid < processLimit; searchPid++) {
        process = processes[searchPid];
        if (isValid(process) && (item == 0 || process->item == item) && (type == ANY_TYPE || process->type == type) &&
            (process->hasFlags(PROCESS_DEAD) == 0U)) {
            searchPid++;
            return process;
        }
    }
    return nullptr;
}

Process *findItemProcess(u16 item, i16 type)
{
    u16 pid;
    Process *process;

    /* NOLINTNEXTLINE(readability-simplify-boolean-expr): IS_ITEM_TYPE is the original's range test, negated */
    if (!IS_ITEM_TYPE(type) || itemHasType(item, type)) {
        for (pid = FIRST_PID; pid < processLimit; pid++) {
            process = processes[pid];
            if (isValid(process) && process->item == item && process->type == type &&
                (process->hasFlags(PROCESS_QUEUED) != 0U) && (process->hasFlags(PROCESS_DEAD) == 0U)) {
                itemCursor = pid;
                return process;
            }
        }
    }
    itemCursor = processLimit;
    return nullptr;
}

Process *nextItemProcess()
{
    Process *process;
    u16 pid;
    u16 item;
    i16 type;
    u16 i;
    u16 count;

    if (itemCursor == processLimit) {
        return nullptr;
    }
    process = processes[itemCursor];
    item = process->item;
    type = process->type;
    for (pid = FIRST_PID; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && (process->hasFlags(PROCESS_SUSPENDED) != 0U) && process->item == item &&
            process->type == type && (process->hasFlags(PROCESS_DEAD) == 0U) && process->waitList.valid()) {
            count = process->waitList.count - 1;
            for (i = 0; i < count; i++) {
                if (process->waitList.get(i) == itemCursor) {
                    itemCursor = pid;
                    return process;
                }
            }
        }
    }
    itemCursor = processLimit;
    return nullptr;
}

/* UNCALLED: nothing calls it. */
void resumeProcess(u16 pid)
{
    Process *process;

    process = findProcess(pid);
    if (process->hasFlags(PROCESS_SUSPENDED) == 0U) {
        halt();
    }
    itemCursor = pid;
}

u16 currentPid()
{
    Process *process;

    if (runningPid >= processLimit) {
        return 0;
    }
    process = processes[runningPid];
    if (!isValid(process) || (process->hasFlags(PROCESS_DEAD) != 0U)) {
        return 0;
    }
    return runningPid;
}

/* UNCALLED: nothing calls it. */
void suspendProcess(Process *process)
{
    Process *last;
    Process *next;

    if (process->hasFlags(PROCESS_SUSPENDED) == 0U) {
        halt();
    }
    last = findItemProcess(process->item, process->type);
    if (last == nullptr) {
        process->queue();
        return;
    }
    for (next = nextItemProcess(); next != nullptr; next = nextItemProcess()) {
        last = next;
    }
    last->addWaiter(process->pid);
    process->addWaitingFor(last->pid);
    process->waitList.add(last->pid);
}

void releasePid(u16 pid)
{
    Process *process;
    u16 item;
    i16 type;

    process = findProcess(pid);
    item = processes[pid]->item;
    type = processes[pid]->type;
    priorityCounts[process->level()]--;
    if (process->hasStatus(STATUS_CREATED_CHILD) != 0U) {
        clearSlot(pid);
        freeProcess(process);
    } else {
        process->markDead();
    }
    processCount--;
    if (item != 0U) {
        updateItemProcess(item);
    }
    if ((item != 0U) && IS_ITEM_TYPE(type)) {
        updateItemType(item, type);
    }
}

void clearSlot(u16 pid)
{
    processes[pid] = nullptr;
    processSizes[pid] = 0;
    processNames[pid] = nullptr;
}

void setCurrentPriority(u8 level)
{
    if (currentPriority != level) {
        currentPriority = level;
        priorityChanged = 1;
    }
}

void wakeProcess(Process *process)
{
    if (runsLater(process)) {
        process->flags |= PROCESS_BLOCKED;
    }
}

bool runsLater(Process *process)
{
    return (kernelActive != 0U) && process->pid > runningPid;
}

/* UNCALLED: nothing calls it. */
void broadcastMessage(u16 message, u16 filter, u16 mask, u16 from, void *data)
{
    u16 pid;
    Process *process;

    for (pid = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && ((filter == 0U) || ((process->messageMask & mask) != 0))) {
            process->receive(message, from, data);
        }
    }
}

void setNewProcessSize(u16 size)
{
    if (newProcessSize != 0U) {
        halt();
    }
    newProcessSize = size;
}

void allocateTables()
{
    processes = new Process *[processLimit];
    processSizes = new u16[processLimit];
    priorityCounts = new u16[LEVELS];
    memset(processes, 0, processLimit * sizeof(Process *));
    memset(processSizes, 0, processLimit * sizeof(u16));
    memset(priorityCounts, 0, LEVELS * sizeof(u16));
    itemProcesses = new u8[KERNEL_ITEMS / 8];
    memset(itemProcesses, 0, KERNEL_ITEMS / 8);
    itemTypes = new u8[KERNEL_ITEMS];
    memset(itemTypes, 0, KERNEL_ITEMS);
    typeCounts = new u32[TYPES];
    clearTypeCounts();
    processNames = new const char *[processLimit];
    memset(processNames, 0, processLimit * sizeof(char *));
}

namespace {
void freeProcesses()
{
    u16 pid;
    Process *process;

    for (pid = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && (process->hasFlags(PROCESS_DEAD) == 0U) && (process->hasFlags(PROCESS_SYSTEM) == 0U)) {
            process->cleanup();
            if (process->allocations != 0U) {
                halt();
            }
            freeProcess(process);
            processes[pid] = nullptr;
        }
    }
}

void freeSystemProcesses()
{
    u16 pid;
    Process *process;

    for (pid = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && (process->hasFlags(PROCESS_SYSTEM) != 0U) && (process->hasFlags(PROCESS_DEAD) == 0U)) {
            process->cleanup();
            if (process->allocations != 0U) {
                halt();
            }
            freeProcess(process);
            processes[pid] = nullptr;
        }
    }
}
}

void freeDeadProcesses()
{
    u16 pid;
    Process *process;

    if (deadProcesses != 0U) {
        for (pid = FIRST_PID; pid < processLimit && (deadProcesses != 0U); pid++) {
            process = processes[pid];
            if (isValid(process) && (process->hasFlags(PROCESS_DEAD) != 0U)) {
                if (process->allocations != 0U) {
                    halt();
                }
                clearSlot(pid);
                freeProcess(process);
                deadProcesses--;
            }
        }
    }
}

void freeTables()
{
    if (timerEnabled != 0U) {
        hostDebugBreak();
        halt();
    }
    freeProcesses();
    freeSystemProcesses();
    freeDeadProcesses();
    delete[] processes;
    delete[] processSizes;
    delete[] priorityCounts;
    processes = nullptr;
    processSizes = nullptr;
    priorityCounts = nullptr;
    if (timerList.valid()) {
        timerList.clear();
    }
    if (mouseList.valid()) {
        mouseList.clear();
    }
    if (keyboardList.valid()) {
        keyboardList.clear();
    }
    delete[] itemProcesses;
    itemProcesses = nullptr;
    delete[] itemTypes;
    itemTypes = nullptr;
    delete[] typeCounts;
    typeCounts = nullptr;
    delete[] processNames;
    processNames = nullptr;
}

namespace {
void clearKeys()
{
    u16 i;

    for (i = 0; i < SCAN_CODES; i++) {
        plainKeys[i].time = 0;
        plainKeys[i].count = 0;
        extendedKeys[i].time = 0;
        extendedKeys[i].count = 0;
    }
    for (i = 0; i < KEY_QUEUE; i++) {
        keyQueue[i].clear();
    }
    keyRead = 0;
    keyWrite = 0;
    pendingKeys = 0;
}
}

/* The block of globals the original's kernel writes, and a far pointer
 * in its table of names. */
constexpr int KERNEL_BLOCK = 114;
constexpr int FAR_POINTER = 4;

namespace {
/* A saved process: its bytes, the original's memory of it; an object of the
 * class of their vtable, made with its Loading constructor (the original
 * ran no constructor); its fields out of the bytes (savefields.h) and what
 * the loads make again of the pointers. */
Process *loadProcess(Flex *flex, i16 object, u16 size)
{
    u8 *bytes = new u8[size];
    flex->readObject(object, bytes, size, 0);
    const SaveClass *saved = saveClassOfVtable(static_cast<u16>(bytes[0] | bytes[1] << 8));
    if ((saved == nullptr) || saved->size != size) {
        hostUnimplemented("a saved process of a class or a size the port does not know");
    }
    Process *process = saved->make();
    SaveImage image(SaveImage::READ, bytes, size, process);
    saved->fields(image, *process);
    delete[] bytes;
    saveLoaded(process);
    return process;
}
}

/* Reads what saveKernel wrote to the objects of flex from index on. */
void loadKernel(Flex *flex, i16 index)
{
    u16 pid;
    i16 object;
    u16 name;
    Process *process;
    Process *timer;
    bool found;

    object = index;
    kernelLoaded = 1;
    if ((inInterrupt != 0U) || (kernelStarted == 0U)) {
        halt();
    }
    inputKeysOff();
    unhookKeyboard();
    disableTimer();
    programTimer(0);
    unhookTimer();
    clockTicksOn();
    freeTables();
    object++;
    /* The block of globals the original reads over its data segment, field
     * by field; the tables' pointers in it are allocated again. saveEnd's
     * check below is the original's for a block too long, which the port
     * reads to its end. */
    {
        u8 block[KERNEL_BLOCK];
        if (flex->objectSize(object) != KERNEL_BLOCK) {
            saveEnd = END_SAVING;
        }
        flex->readObject(object++, block, KERNEL_BLOCK, 0);
        SaveImage image(SaveImage::READ, block, KERNEL_BLOCK, nullptr);
        saveGlobals(image, KERNEL_GLOBALS, KERNEL_GLOBAL_COUNT);
    }
    allocateTables();
    flex->seekObject(object++);
    timerList.load(flex);
    flex->seekObject(object++);
    keyboardList.load(flex);
    flex->seekObject(object++);
    mouseList.load(flex);
    flex->readObject(object++, processSizes, -1, 0);
    flex->readObject(object++, itemProcesses, -1, 0);
    flex->readObject(object++, itemTypes, -1, 0);
    flex->readObject(object++, priorityCounts, -1, 0);
    /* The names are literals of the program, whose far pointers the
     * original makes near ones of the new DS; the port finds them by the
     * literals' offsets (savefields.h). */
    {
        u8 *names = new u8[processLimit * FAR_POINTER];
        flex->readObject(object++, names, static_cast<ui16>(processLimit * FAR_POINTER), 0);
        for (pid = 0; pid < processLimit; pid++) {
            u8 *at = names + (pid * FAR_POINTER);
            name = static_cast<u16>(at[0] | at[1] << 8);
            processNames[pid] = (name != 0U) || (at[2] != 0U) || (at[3] != 0U) ? saveNameAt(name) : nullptr;
        }
        delete[] names;
    }
    object = index + PROCESS_OBJECTS;
    for (pid = 0; pid < processLimit; pid++) {
        if (processSizes[pid] == 0) {
            object += 2;
        } else {
            process = loadProcess(flex, object++, processSizes[pid]);
            processes[pid] = process;
            flex->seekObject(object++);
            process->load(flex);
        }
    }
    for (pid = 0; pid < processLimit; pid++) {
        if (processes[pid] != nullptr) {
            processes[pid]->afterLoad();
        }
    }
    found = false;
    for (pid = 0; pid < processLimit; pid++) {
        timer = processes[pid];
        if (isValid(timer) && timer->type == BIOS_TIMER_TYPE) {
            if (found) {
                halt();
            }
            found = true;
            (static_cast<BiosTimer *>(timer))->setVectors();
        }
    }
    clockTicksOff();
    hookTimer();
    programTimer(timerDivisor);
    restoreTimer();
    hookKeyboard();
    if (saveGuard != GUARD_SAVING) {
        halt();
    }
    saveGuard = GUARD_IDLE;
    if (saveEnd == END_SAVING) {
        halt();
    }
    saveEnd = END_IDLE;
    flushKeys();
    inputKeysOn();
}

/* The globals saveKernel writes whole: the original's data segment from
 * processes (1480:4416) to saveGuard, 114 bytes, which the port's globals
 * are not laid out as. Each with its size in the original; the save's
 * tables are checked against the original's offsets. */
const SaveGlobal KERNEL_GLOBALS[] = {
    SAVE_GLOBAL(processes, 4, GLOBAL_TABLE),      SAVE_GLOBAL(processSizes, 4, GLOBAL_TABLE),
    SAVE_GLOBAL(timerMasked, 1, GLOBAL_AS_IS),    SAVE_GLOBAL(processLimit, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(processCount, 2, GLOBAL_AS_IS),   SAVE_GLOBAL(trimCountdown, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(runningPid, 2, GLOBAL_AS_IS),     SAVE_GLOBAL(tableGrows, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(kernelActive, 1, GLOBAL_AS_IS),   SAVE_GLOBAL(keyboardListeners, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(priorityCounts, 4, GLOBAL_TABLE), SAVE_GLOBAL(currentPriority, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(processNames, 4, GLOBAL_TABLE),   SAVE_GLOBAL(timerDivisor, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(timerStep, 4, GLOBAL_AS_IS),      SAVE_GLOBAL(timerList, 15, GLOBAL_LIST),
    SAVE_GLOBAL(keyboardList, 15, GLOBAL_LIST),   SAVE_GLOBAL(timerRate, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(pausedTimers, 2, GLOBAL_AS_IS),   SAVE_GLOBAL(runningTimers, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(timerDeferred, 1, GLOBAL_AS_IS),  SAVE_GLOBAL(nestedInterrupts, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(searchPid, 2, GLOBAL_AS_IS),      SAVE_GLOBAL(frameStart, 4, GLOBAL_AS_IS),
    SAVE_GLOBAL(frameTicks, 4, GLOBAL_AS_IS),     SAVE_GLOBAL(itemProcesses, 4, GLOBAL_TABLE),
    SAVE_GLOBAL(itemTypes, 4, GLOBAL_TABLE),      SAVE_GLOBAL(typeCounts, 4, GLOBAL_TABLE),
    SAVE_GLOBAL(mouseList, 15, GLOBAL_LIST),      SAVE_GLOBAL(mouseListeners, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(saveGuard, 1, GLOBAL_AS_IS)};
const u16 KERNEL_GLOBAL_COUNT = sizeof(KERNEL_GLOBALS) / sizeof(KERNEL_GLOBALS[0]);

namespace {
/* The offset of a global of KERNEL_GLOBALS in the kernel's block. */
u16 blockOffsetOf(const void *place)
{
    u16 at = 0;
    for (const SaveGlobal &global : KERNEL_GLOBALS) {
        if (global.place() == place) {
            return at;
        }
        at = static_cast<u16>(at + global.size);
    }
    return KERNEL_BLOCK;
}

/* An object of a FLEX's bytes (UNITFILE.C: the count at 54h, each
 * object's offset and size at 80h + 8n), none past the count or the
 * file's end. */
struct FileObject {
    bool there = false;
    const u8 *bytes = nullptr;
    u32 size = 0;
};

u32 dwordAt(const std::vector<u8> &file, std::size_t at)
{
    return static_cast<u32>(file[at]) | static_cast<u32>(file[at + 1]) << 8 | static_cast<u32>(file[at + 2]) << 16 |
           static_cast<u32>(file[at + 3]) << 24;
}

FileObject fileObject(const std::vector<u8> &file, u32 count, u32 number)
{
    if (number >= count) {
        return {};
    }
    const u32 offset = dwordAt(file, 0x80 + (8 * static_cast<std::size_t>(number)));
    const u32 size = dwordAt(file, 0x80 + (8 * static_cast<std::size_t>(number)) + 4);
    if (offset > file.size() || size > file.size() - offset) {
        return {};
    }
    return {true, file.data() + offset, size};
}
}

bool kernelSaveLoadable(const std::vector<u8> &file, i16 index)
{
    const std::size_t header = 0x80;
    if (file.size() < header) {
        return false;
    }
    const u32 count = dwordAt(file, 0x54);
    if (count > (file.size() - header) / 8) {
        return false;
    }
    const auto at = static_cast<u32>(index);
    /* The block of globals, its size the original's (a block of another
     * ends loadKernel) and its guard the one saveKernel wrote. */
    const FileObject block = fileObject(file, count, at + 1);
    if (!block.there || block.size != KERNEL_BLOCK || block.bytes[blockOffsetOf(&saveGuard)] != GUARD_SAVING) {
        return false;
    }
    const u16 limitAt = blockOffsetOf(&processLimit);
    const u16 limit = static_cast<u16>(block.bytes[limitAt] | block.bytes[limitAt + 1] << 8);
    /* The tables loadKernel reads whole into tables of its sizes. */
    const FileObject sizes = fileObject(file, count, at + 5);
    if (limit == 0U || !sizes.there || sizes.size != limit * sizeof(u16) ||
        fileObject(file, count, at + 6).size != static_cast<u32>(KERNEL_ITEMS / 8) ||
        fileObject(file, count, at + 7).size != static_cast<u32>(KERNEL_ITEMS) ||
        fileObject(file, count, at + 8).size != LEVELS * sizeof(u16) ||
        fileObject(file, count, at + 9).size != limit * FAR_POINTER) {
        return false;
    }
    /* Each saved process: a class the port knows by its vtable, of the
     * size the table says, and the objects of its bytes and its load. */
    for (u32 pid = 0; pid < limit; pid++) {
        const u16 size = static_cast<u16>(sizes.bytes[2 * pid] | sizes.bytes[(2 * pid) + 1] << 8);
        if (size == 0U) {
            continue;
        }
        const u32 object = at + PROCESS_OBJECTS + (2 * pid);
        const FileObject bytes = fileObject(file, count, object);
        if (!bytes.there || bytes.size < size || size < 2 || !fileObject(file, count, object + 1).there) {
            return false;
        }
        const SaveClass *saved = saveClassOfVtable(static_cast<u16>(bytes.bytes[0] | bytes.bytes[1] << 8));
        if (saved == nullptr || saved->size != size) {
            return false;
        }
    }
    return true;
}

namespace {
/* The class of a process to save, which the port must know. */
const SaveClass *savedClass(Process *process)
{
    const SaveClass *saved = saveClassOf(process);
    if (saved == nullptr) {
        char text[120];
        snprintf(text, sizeof(text), "a process to save of a class the saved games do not know (%s)",
                 typeid(*process).name());
        hostUnimplemented(text);
    }
    return saved;
}

/* A process as the original's memory of it: BCC's layout of its class,
 * its vtable, its fields (savefields.h). */
void saveProcess(Flex *flex, i16 object, Process *process)
{
    const SaveClass *saved = savedClass(process);
    u8 *bytes = new u8[saved->size];
    memset(bytes, 0, saved->size);
    bytes[0] = static_cast<u8>(saved->vtable);
    bytes[1] = static_cast<u8>(saved->vtable >> 8);
    SaveImage image(SaveImage::WRITE, bytes, saved->size, process);
    saved->fields(image, *process);
    flex->writeObject(object, bytes, saved->size, 0, 1);
    delete[] bytes;
}
}

/* Writes the kernel to the objects of flex from index on: a line naming the
 * build, the kernel's variables, the subscriber lists, the tables and, from
 * index+PROCESS_OBJECTS, two objects for each process. */
void saveKernel(Flex *flex, i16 index)
{
    u16 pid;
    Process *process;

    if ((inInterrupt != 0U) || (kernelStarted == 0U)) {
        halt();
    }
    for (pid = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && (process->hasFlags(PROCESS_DEAD) == 0U) && (process->hasStatus(STATUS_STARTED) == 0U)) {
            process->fail(0);
        }
    }
    freeDeadProcesses();
    unhookKeyboard();
    maskTimer();
    programTimer(0);
    unhookTimer();
    clockTicksOn();
    saveGuard = GUARD_SAVING;
    saveEnd = END_SAVING;
    writeKernel(flex, index);
    saveGuard = GUARD_IDLE;
    saveEnd = END_IDLE;
    clockTicksOff();
    hookTimer();
    programTimer(timerDivisor);
    restoreTimer();
    flushKeys();
    hookKeyboard();
}

/* saveKernel's objects as the kernel is, without its work before and after
 * them (the port's, for the pass dump): the processes not started yet have
 * no objects, as those the save fails first. */
void writeKernel(Flex *flex, i16 index)
{
    u16 pid;
    i16 object = index;
    char version[] = "< Kernel Ver. 2.00 - July 8, 1995 - Tony Zurovec >";
    u16 defines = 0;
    FlexEntry entry;
    Process *process;
    char text[100];

    defines |= 0x1;
    defines |= 0x2;
    defines |= 0x4;
    defines |= 0x10;
    defines |= 0x20;
    defines |= 0x40;
    defines |= 0x80;
    defines |= 0x100;
    defines |= 0x8000;
    snprintf(text, sizeof(text), "Defines=%x Id=%s", defines, version);
    flex->writeObject(object++, text, static_cast<ui16>(strlen(text) + 1), 0, 1);
    /* The variables from processes to saveGuard, which the original writes
     * from its data segment: field by field. */
    {
        u8 block[KERNEL_BLOCK];
        SaveImage image(SaveImage::WRITE, block, KERNEL_BLOCK, nullptr);
        saveGlobals(image, KERNEL_GLOBALS, KERNEL_GLOBAL_COUNT);
        flex->writeObject(object++, block, KERNEL_BLOCK, 0, 1);
    }
    flex->readEntry(object, &entry);
    flex->seek(0, SEEK_END);
    entry.offset = flex->size();
    timerList.save(flex);
    entry.size = flex->size() - entry.offset;
    flex->writeEntry(object++, &entry);
    flex->readEntry(object, &entry);
    flex->seek(0, SEEK_END);
    entry.offset = flex->size();
    keyboardList.save(flex);
    entry.size = flex->size() - entry.offset;
    flex->writeEntry(object++, &entry);
    flex->readEntry(object, &entry);
    flex->seek(0, SEEK_END);
    entry.offset = flex->size();
    mouseList.save(flex);
    entry.size = flex->size() - entry.offset;
    flex->writeEntry(object++, &entry);
    /* The sizes are the original's of the classes: the port's objects are
     * larger (savefields.h). */
    {
        u16 *sizes = new u16[processLimit];
        for (pid = 0; pid < processLimit; pid++) {
            process = processes[pid];
            sizes[pid] =
                (isValid(process)) && (process->hasStatus(STATUS_STARTED) != 0U) ? savedClass(process)->size : 0;
        }
        flex->writeObject(object++, sizes, static_cast<ui16>(processLimit * sizeof(u16)), 0, 1);
        delete[] sizes;
    }
    flex->writeObject(object++, itemProcesses, KERNEL_ITEMS / 8L, 0, 1);
    flex->writeObject(object++, itemTypes, KERNEL_ITEMS, 0, 1);
    flex->writeObject(object++, priorityCounts, static_cast<ui16>(LEVELS * sizeof(u16)), 0, 1);
    /* The names as the original's far pointers: its DS and the literal's
     * offset; a usecode class's name, which lies in the unit file, that
     * file's selector (savefields.h). */
    {
        u8 *names = new u8[processLimit * FAR_POINTER];
        for (pid = 0; pid < processLimit; pid++) {
            const char *name = processNames[pid];
            const u16 offset = saveNameOffset(name);
            u16 selector = 0;
            if (name != nullptr) {
                selector = (offset != 0U) ? ORIGINAL_DS : ORIGINAL_UNIT_FILE;
            }
            u8 *at = names + (pid * FAR_POINTER);
            at[0] = static_cast<u8>(offset);
            at[1] = static_cast<u8>(offset >> 8);
            at[2] = static_cast<u8>(selector);
            at[3] = static_cast<u8>(selector >> 8);
        }
        flex->writeObject(object++, names, static_cast<ui16>(processLimit * FAR_POINTER), 0, 1);
        delete[] names;
    }
    object = index + PROCESS_OBJECTS;
    for (pid = 0; pid < processLimit; pid++) {
        process = processes[pid];
        if (isValid(process) && (process->hasStatus(STATUS_STARTED) != 0U)) {
            saveProcess(flex, object++, process);
            flex->readEntry(object, &entry);
            flex->seek(0, SEEK_END);
            entry.offset = flex->size();
            process->save(flex);
            entry.size = flex->size() - entry.offset;
            flex->writeEntry(object++, &entry);
        } else {
            object += 2;
        }
    }
}

/* Sets the divisor of channel 0 of the interval timer. */
void programTimer(u16 divisor)
{
    clockPushHold();
    clockHold();
    clockTickDivisor(divisor);
    clockPopHold();
}

/* Makes the timer interrupt come every period microseconds. */
void setTimerPeriod(u32 period)
{
    if (period == 0) {
        halt();
    }
    clockPushHold();
    clockHold();
    timerRate = static_cast<s32>(1000000L) / period;
    timerStep = period;
    /* In counts of the 1.19318 MHz timer, 0.838 microseconds each. */
    if (period <= BIOS_TICK) {
        period = period * 10000 / 8380;
    } else {
        halt();
    }
    timerDivisor = period;
    programTimer(timerDivisor);
    clockPopHold();
}

namespace {
/* Ticks the periodic processes on the timer list. From the timer interrupt
 * only the paused ones run while DOS or the kernel is busy; the others wait
 * for runProcesses. */
void runTimers()
{
    PeriodicProcess *process;
    u32 events;
    u16 pid;
    bool dosBusy;
    bool runAll;
    /* Saved and put back under interruptNested, which a nested tick sets
     * and clears inside this walk. */
    u8 stop = 0;
    u16 walked = 0;
    u16 total = 0;
    u16 running;
    u8 level;

    runAll = true;
    if (interruptNested != 0U) {
        runAll = false;
    } else if (runningTimers != 0U) {
        dosBusy = false; /* the original's InDOS flag: the port has no DOS */
        if (timerDeferred != 0U) {
            if (nestedInterrupts != 0U) {
                if (dosBusy) {
                    return;
                }
                timerDeferred = 0;
            } else if (dosBusy || (kernelActive != 0U)) {
                if (pausedTimers == 0U) {
                    return;
                }
                runAll = false;
            } else {
                timerDeferred = 0;
            }
        } else if (dosBusy || (kernelActive != 0U)) {
            timerDeferred = 1;
            if (pausedTimers == 0U) {
                return;
            }
            runAll = false;
        }
    }
    if (interruptNested != 0U) {
        stop = timerList.stop;
        walked = timerList.index;
        total = timerList.total;
    }
    timerList.total = timerList.count;
    if (timerList.total != 0U) {
        timerList.total--;
        for (timerList.index = 0; timerList.index < timerList.total; timerList.index++) {
            pid = timerList.get(timerList.index);
            process = static_cast<PeriodicProcess *>(findProcess(pid));
            events = process->eventMask;
            if ((events & EVENT_TIMER) != 0U) {
                level = process->level();
                if (level == currentPriority || level == SYSTEM_LEVEL || level == ALWAYS_LEVEL) {
                    if (interruptNested != 0U) {
                        if (process->hasFlags(PROCESS_PAUSED) != 0U) {
                            nestedTickingPid = pid;
                            process->tick();
                            nestedTickingPid = 0;
                        }
                    } else if (nestedInterrupts != 0U) {
                        if (process->hasFlags(PROCESS_PAUSED) == 0U) {
                            running = runningPid;
                            runningPid = pid;
                            tickingPid = pid;
                            process->tick();
                            tickingPid = 0;
                            runningPid = running;
                        }
                    } else if (process->hasFlags(PROCESS_PAUSED) != 0U) {
                        clockPushHold();
                        clockHold();
                        tickingPid = pid;
                        process->tick();
                        tickingPid = 0;
                        clockPopHold();
                    } else if (runAll) {
                        running = runningPid;
                        runningPid = pid;
                        tickingPid = pid;
                        process->tick();
                        tickingPid = 0;
                        runningPid = running;
                    }
                }
            }
        }
    } else {
        halt();
    }
    if (interruptNested != 0U) {
        timerList.stop = stop;
        timerList.index = walked;
        timerList.total = total;
    }
}
}

void addTimer(u16 pid, u8 run)
{
    PeriodicProcess *process;
    u32 period;

    process = static_cast<PeriodicProcess *>(processes[pid]);
    if ((process->hasFlags(PROCESS_EVENTS) == 0U) || (process->hasFlags(PROCESS_PERIODIC) == 0U)) {
        halt();
    }
    maskTimer();
    process->flags |= PROCESS_TIMED;
    if (process->hasFlags(PROCESS_PAUSED) != 0U) {
        pausedTimers++;
    } else {
        runningTimers++;
    }
    timerList.add(pid);
    period = process->period;
    if (period < timerStep) {
        setTimerStep(period);
    }
    if (timerMasked != 0U) {
        restoreTimer();
    } else if (run != 0U) {
        enableTimer();
    }
}

void removeTimer(u16 pid)
{
    PeriodicProcess *process;
    u32 period;

    process = static_cast<PeriodicProcess *>(processes[pid]);
    if ((process->hasFlags(PROCESS_EVENTS) == 0U) || (process->hasFlags(PROCESS_PERIODIC) == 0U)) {
        halt();
    }
    maskTimer();
    process->flags &= ~PROCESS_TIMED;
    if (process->hasFlags(PROCESS_PAUSED) != 0U) {
        pausedTimers--;
    } else {
        runningTimers--;
        if (runningTimers == 0) {
            timerDeferred = 0;
        }
    }
    timerList.remove(pid);
    if (timerList.valid()) {
        period = process->period;
        if (period < timerStep) {
            setTimerStep(period);
        }
        restoreTimer();
    } else {
        timerDeferred = 0;
        disableTimer();
    }
}

void hookTimer()
{
    if (timerHooked != 0U) {
        halt();
        return;
    }
    clockTicks(timerInterrupt);
    timerHooked = 1;
}

void unhookTimer()
{
    if (timerHooked != 0U) {
        clockTicks(nullptr);
        timerHooked = 0;
    }
}

/* From the timer interrupt the change waits for its end (timerTick). */
void enableTimer()
{
    if (inInterrupt != 0U) {
        timerWanted = 1;
        timerStateChanged = 1;
        return;
    }
    if (timerList.valid()) {
        clockPushHold();
        clockHold();
        timerEnabled = 1;
        clockTicksOn();
        clockPopHold();
    }
}

void disableTimer()
{
    if (inInterrupt != 0U) {
        timerWanted = 0;
        timerStateChanged = 1;
        return;
    }
    clockPushHold();
    clockHold();
    clockTicksOff();
    timerEnabled = 0;
    clockPopHold();
}

/* Sets the timer to step microseconds, or with 0 to the shortest period of
 * the processes on the timer list. */
void setTimerStep(u32 step)
{
    u16 pid;
    PeriodicProcess *process;
    u16 i;
    u16 count;
    u32 period;
    u32 shortest;
    u32 old;

    if (step != 0U) {
        if (step == timerStep) {
            return;
        }
        setTimerPeriod(step);
        return;
    }
    old = timerStep;
    clockPushHold();
    clockHold();
    if (timerList.valid()) {
        count = timerList.count - 1;
        for (i = 0, shortest = 1000000L; i < count; i++) {
            pid = timerList.get(i);
            process = static_cast<PeriodicProcess *>(findProcess(pid));
            period = process->period;
            if (period < shortest) {
                shortest = period;
            }
        }
        if (shortest != old) {
            setTimerPeriod(shortest);
        }
    }
    clockPopHold();
}

namespace {
/* runKernel's after its processes' pass. */
void kernelPassEnd(u32 ticks, void (*idle)())
{
    if (idle != nullptr) {
        idle();
    }
    while (frameStart + ticks > systemTimer->ticks) {
        clockWait();
    }
    frameTicks = systemTimer->ticks - frameStart;
}
}

void runKernel(u32 ticks, void (*idle)())
{
    frameStart = systemTimer->ticks;
    runProcesses();
    if (waitOpened()) {
        giveRest([ticks, idle]() {
            kernelPassEnd(ticks, idle);
            return false;
        });
        return;
    }
    kernelPassEnd(ticks, idle);
}

void restoreTimer()
{
    if (timerMasked != 0U) {
        clockTicksOn();
    }
}

/* Masks the timer interrupt; restoreTimer unmasks it again if it was on. */
void maskTimer()
{
    timerMasked = timerEnabled;
    if (timerEnabled != 0U) {
        clockTicksOff();
    }
}

/* The clock's ticks (clock.h): the original's timer interrupt (IRQ 0),
 * which ran timerTick on a real-mode stack of its own, the second of two
 * for a tick that came while the first ran. The tick lets the next come
 * early (the end of the interrupt), but for runProcesses' call of it. */
void timerInterrupt()
{
    if (inTimer != 0U) {
        interruptNested = 1;
    } else {
        inTimer = 1;
    }
    if (nestedInterrupts != 0U) {
        if (interruptNested != 0U) {
            clockTickDone();
        }
    } else {
        clockTickDone();
    }
    timerTick();
    if (interruptNested != 0U) {
        interruptNested = 0;
    } else {
        inTimer = 0;
    }
}

void timerTick()
{
    [[maybe_unused]] u16 paused;

    CLEAR_DIRECTION();
    if (interruptNested != 0U) {
        clockTicksOff();
        runTimers();
        clockTicksOn();
        return;
    }
    paused = pausedTimers;
    timerStateChanged = 0;
    inInterrupt = 1;
    clockRelease();
    runTimers();
    clockHold();
    inInterrupt = 0;
    if (timerStateChanged != 0U) {
        if (timerWanted != 0U) {
            enableTimer();
        } else {
            disableTimer();
        }
    }
}

/* Empties the key queue, No Remorse's: the shift state stays as the keys
 * held are (No Regret's original released the shift, control and alt keys
 * in the BIOS's state, which kept them after a key went up unseen: the
 * port's keyboard sees every key, and a key still held lost its state). */
void flushKeys()
{
    while (keysQueued()) {
        nextKey();
    }
    clearKeys();
    flushDevices();
}

/* Scroll Lock pauses the keyboard when CRUDEBUG=TRUE. */
bool keyboardPaused()
{
    return ((keyShift & KEY_SCROLL_LOCK) != 0) && (debugKeys != 0U);
}

/* The keyboard's shift state: the BIOS's flags at 0040:0017 in the
 * original, the input's (input.h) in the port. */
/* UNCALLED: nothing calls it. */
u16 readBiosShift()
{
    return inputShiftFlags();
}

void writeBiosShift(u16 shift)
{
    inputSetShiftFlags(shift);
}

/* Hands the key to the keyboard listeners that hold it (STATUS_HOLD) right
 * away, from the keyboard interrupt. */
void dispatchKey(KeyEvent *key)
{
    EventProcess *process;
    u8 stop;
    u16 walked;
    u16 total;
    KeyEvent copy;

    stop = keyboardList.stop;
    walked = keyboardList.index;
    total = keyboardList.total;
    keyboardList.total = keyboardList.count;
    if (keyboardList.total != 0U) {
        keyboardList.total--;
        for (keyboardList.index = 0; keyboardList.index < keyboardList.total; keyboardList.index++) {
            keyboardList.stop = 0;
            process = static_cast<EventProcess *>(findProcess(keyboardList.get(keyboardList.index)));
            if (((process->eventMask & EVENT_KEYBOARD) != 0U) && (process->hasStatus(STATUS_HOLD) != 0U)) {
                copy = *key;
                process->event(EVENT_KEYBOARD, &copy);
            }
        }
    }
    keyboardList.stop = stop;
    keyboardList.index = walked;
    keyboardList.total = total;
}

void queueKey(KeyEvent *key)
{
    if (keyQueueFull()) {
        keyOverflow = 1;
        return;
    }
    keyQueue[keyWrite] = *key;
    keyWrite++;
    if (keyWrite == KEY_QUEUE) {
        keyWrite = 0;
    }
}

namespace {
/* The keyboard's scan codes, which the input brings (input.h): the
 * original's keyboard interrupt (IRQ 1), which read them from the
 * controller (64h, 60h), sent the controller its enable (AEh) and ended the
 * interrupt; the input does that part. Keys the kernel does not take (the
 * shift keys, Scroll Lock, keys while paused) go on to the keyboard's own
 * keeping, as the original passed them to the BIOS's handler. */
void keyboardInterrupt(u8 scan)
{
    bool passOn;
    bool nested;
    u8 pressed;
    u16 i;
    u16 code;
    u32 time;
    KeyEvent key;

    passOn = false;
    nested = false;
    time = systemTimer->ticks;
    CLEAR_DIRECTION();
    if (keyboardDepth != 0U) {
        passOn = true;
        nested = true;
    }
    keyboardDepth++;
    code = scan;
    if (extendedScan != 0U) {
        code |= 0x100;
    }
    for (i = 0; i < MODIFIER_SCANS; i++) {
        if (modifierScans[i] == code) {
            passOn = true;
            break;
        }
    }
    /* 70h-7Fh and F0h-FFh, among them the keyboard's replies (FAh, FEh). */
    if ((scan & SCAN_MASK) >= 0x70) {
        passOn = true;
        nested = true;
    }
    if (passOn || keyboardPaused()) {
        inputKeyboardScan(scan);
        readShiftState();
        if ((scan & SCAN_MASK) == SCAN_SCROLL_LOCK || nested) {
            extendedScan = 0;
            keyboardDepth--;
            return;
        }
    }
    /* The shift codes the keyboard sends around an extended key. */
    if ((extendedScan != 0U) && (scan == SCAN_LEFT_SHIFT || scan == (SCAN_LEFT_SHIFT | SCAN_RELEASED))) {
        extendedScan = 0;
        keyboardDepth--;
        return;
    }
    if (scan == SCAN_PREFIX) {
        extendedScan = 1;
    } else {
        if ((scan & SCAN_RELEASED) != 0) {
            pressed = 0;
        } else {
            pressed = 1;
        }
        scan &= SCAN_MASK;
        if (keyboardPaused()) {
            if (pressed != 0U) {
                extendedScan = 0;
                keyboardDepth--;
                return;
            }
            if (keyRepeats(scan, extendedScan) == 0U) {
                extendedScan = 0;
                keyboardDepth--;
                return;
            }
        } else {
            if (scan == SCAN_ALT) {
                if (extendedScan != 0U) {
                    if (pressed != 0U) {
                        keyShift |= KEY_RIGHT_ALT;
                        keyShift |= KEY_ALT;
                    } else {
                        keyShift &= ~KEY_RIGHT_ALT;
                        if ((keyShift & KEY_LEFT_ALT) == 0) {
                            keyShift &= ~KEY_ALT;
                        }
                    }
                }
            } else if (scan == SCAN_CONTROL) {
                if (extendedScan != 0U) {
                    if (pressed != 0U) {
                        keyShift |= KEY_RIGHT_CONTROL;
                        keyShift |= KEY_CONTROL;
                    } else {
                        keyShift &= ~KEY_RIGHT_CONTROL;
                        if ((keyShift & KEY_LEFT_CONTROL) == 0) {
                            keyShift &= ~KEY_CONTROL;
                        }
                    }
                }
            } else if (scan == SCAN_INSERT) {
                if (pressed != 0U) {
                    if (insertDown == 0U) {
                        insertDown = 1;
                        if ((keyShift & KEY_INSERT) != 0) {
                            keyShift &= ~KEY_INSERT;
                        } else {
                            keyShift |= KEY_INSERT;
                        }
                    }
                } else {
                    insertDown = 0;
                }
            }
            if (((scan == SCAN_ALT || scan == SCAN_CONTROL) && (extendedScan != 0U)) || scan == SCAN_INSERT) {
                writeBiosShift(keyShift);
            }
        }
        if ((pressed != 0U) && (keyRepeats(scan, extendedScan) != 0U)) {
            extendedScan = 0;
            keyboardDepth--;
            return;
        }
        key.clear();
        key.scan = scan;
        key.pressed = pressed;
        key.shift = keyShift;
        key.time = time;
        key.extended = extendedScan;
        if (pressed != 0U) {
            pendingKeys++;
            if (extendedScan != 0U) {
                extendedKeys[scan].count++;
                extendedKeys[scan].time = time;
                key.repeat = extendedKeys[scan].count;
            } else {
                plainKeys[scan].count++;
                plainKeys[scan].time = time;
                key.repeat = plainKeys[scan].count;
            }
        } else {
            if (pendingKeys != 0U) {
                pendingKeys--;
            }
            if (extendedScan != 0U) {
                extendedKeys[scan].time = 0;
                extendedKeys[scan].count = 0;
            } else {
                plainKeys[scan].time = 0;
                plainKeys[scan].count = 0;
            }
            key.repeat = 0;
        }
        key.translate();
        if (keyboardReady != 0U) {
            queueKey(&key);
            dispatchKey(&key);
        }
        extendedScan = 0;
    }
    keyboardDepth--;
}
}

/* How many times the key has come since it went down. */
u16 keyRepeats(u8 scan, u8 extended)
{
    if (extended != 0U) {
        return extendedKeys[scan].count;
    }
    return plainKeys[scan].count;
}

/* The shift state from the BIOS's INT 16h AH=12h, No Remorse's: AL the flags,
 * AH the left and right control and alt keys held (KEY_LEFT_CONTROL to
 * KEY_RIGHT_ALT). No Regret's original read the word at 0040:0017, whose high
 * byte has SysReq and Pause where the right keys' bits are. */
void readShiftState()
{
    keyShift = inputShiftState();
}

void hookKeyboard()
{
    if (keyboardHooked != 0U) {
        halt();
        return;
    }
    clockPushHold();
    clockHold();
    inputKeys(keyboardInterrupt);
    keyboardHooked = 1;
    readShiftState();
    clockPopHold();
}

void unhookKeyboard()
{
    if (keyboardHooked != 0U) {
        inputKeys(nullptr);
        keyboardHooked = 0;
    }
}

u8 queuedKeys()
{
    u16 count;

    if (keyWrite > keyRead) {
        count = keyWrite - keyRead;
    } else if (keyRead > keyWrite) {
        count = KEY_QUEUE - (keyRead - keyWrite);
    } else {
        count = 0;
    }
    return count;
}

bool keyQueueFull()
{
    if (static_cast<ui16>(keyRead - 1) == keyWrite) {
        return true;
    }
    if (keyWrite == static_cast<ui16>(KEY_QUEUE - 1) && keyRead == 0) {
        return true;
    }
    return false;
}

KeyEvent nextKey()
{
    KeyEvent key;

    if (!keysQueued()) {
        halt();
    }
    key = keyQueue[keyRead];
    clockPushHold();
    clockHold();
    keyRead++;
    if (keyRead == KEY_QUEUE) {
        keyRead = 0;
    }
    keyOverflow = 0;
    clockPopHold();
    return key;
}

void addKeyboardListener(Process *process)
{
    if (inInterrupt != 0U) {
        halt();
    }
    if (process->hasFlags(PROCESS_EVENTS) == 0U) {
        halt();
    }
    if (process->hasStatus(STATUS_KEYBOARD) != 0U) {
        return;
    }
    process->status |= STATUS_KEYBOARD;
    keyboardListeners++;
    keyboardList.add(process->pid);
}

void removeKeyboardListener(Process *process)
{
    if (inInterrupt != 0U) {
        halt();
    }
    if ((process->hasFlags(PROCESS_EVENTS) == 0U) || (process->hasStatus(STATUS_KEYBOARD) == 0U)) {
        return;
    }
    process->status &= ~STATUS_KEYBOARD;
    keyboardListeners--;
    keyboardList.remove(process->pid);
}

namespace {
/* Hands the queued keys to the keyboard listeners at the current priority
 * or above that did not take them from the interrupt. */
void deliverKeyFrom(KeyEvent key);
}

void deliverKeys()
{
    KeyEvent key;

    while (keysQueued()) {
        key = nextKey();
        keyboardList.total = keyboardList.count;
        if (keyboardList.total != 0U) {
            keyboardList.total--;
            keyboardList.index = 0;
            deliverKeyFrom(key);
            if (waitOpened()) {
                return;
            }
        }
    }
}

namespace {
/* deliverKeys' delivery of key from the process at keyboardList.index on
 * (the port's): a wait opened in a process's event (a cheat's box) ends
 * it there, and its rest goes on with the next process, then the next
 * keys. */
void deliverKeyFrom(KeyEvent key)
{
    EventProcess *process;
    u8 level;
    KeyEvent copy;

    for (; keyboardList.index < keyboardList.total; keyboardList.index++) {
        keyboardList.stop = 0;
        process = static_cast<EventProcess *>(findProcess(keyboardList.get(keyboardList.index)));
        if (((process->eventMask & EVENT_KEYBOARD) != 0U) && (process->hasStatus(STATUS_HOLD) == 0U)) {
            level = process->level();
            if (level >= currentPriority || level == ALWAYS_LEVEL || level == SYSTEM_LEVEL) {
                copy = key;
                process->event(EVENT_KEYBOARD, &copy);
                if (waitOpened()) {
                    giveRest([key]() {
                        keyboardList.index++;
                        deliverKeyFrom(key);
                        if (!waitOpened()) {
                            deliverKeys();
                        }
                        return false;
                    });
                    return;
                }
            }
        }
    }
}
}

/* The typematic delay (0-3) and rate (0-31) of the keyboard, through INT 16h
 * AX=0305h. */
/* UNCALLED: nothing calls it. */
void setTypematicDelay(u8 delay)
{
    typematicDelay = delay & 3;
    applyTypematic();
}

/* UNCALLED: nothing calls it. */
void setTypematicRate(u8 rate)
{
    typematicRate = rate & 0x1f;
    applyTypematic();
}

/* UNCALLED: nothing calls it. */
void applyTypematic()
{
    u8 rate;
    u8 delay;

    rate = typematicRate;
    delay = typematicDelay;
    inputTypematic(rate, delay);
}

/* Every process is in its own slot. */
/* UNCALLED: nothing calls it. */
void checkTable()
{
    u16 pid;

    for (pid = 0; pid < processLimit; pid++) {
        if (isValid(processes[pid]) && processes[pid]->pid != pid) {
            halt();
        }
    }
}

/* The item bitmap: which items have processes. */
void markItemProcess(u16 item)
{
    u8 bits;
    u8 bit;
    u16 index;

    index = item >> 3;
    bits = itemProcesses[index];
    bit = item & 7;
    bits |= 1 << bit;
    itemProcesses[index] = bits;
}

namespace {
void unmarkItemProcess(u16 item)
{
    u8 bits;
    u8 bit;
    u16 index;

    index = item >> 3;
    bits = itemProcesses[index];
    bit = item & 7;
    bits &= ~(1 << bit);
    itemProcesses[index] = bits;
}
}

void updateItemProcess(u16 item)
{
    Process *process;

    process = firstProcess(item, ANY_TYPE);
    if (process != nullptr) {
        markItemProcess(item);
    } else {
        unmarkItemProcess(item);
    }
}

bool itemHasProcess(u16 item)
{
    u8 bits;
    u8 bit;

    bits = itemProcesses[item >> 3];
    bit = item & 7;
    return (static_cast<i16>(bits & 1 << bit)) != 0;
}

/* The item types: which of the types F0h-F7h each item has processes of. */
void markItemType(u16 item, i16 type)
{
    u8 *types;
    u16 bit;

    types = &itemTypes[item];
    bit = type - FIRST_ITEM_TYPE;
    bit = 1 << bit;
    *types |= bit;
}

namespace {
void unmarkItemType(u16 item, i16 type)
{
    u8 *types;
    u16 bit;

    types = &itemTypes[item];
    bit = type - FIRST_ITEM_TYPE;
    bit = 1 << bit;
    *types &= ~bit;
}
}

void updateItemType(u16 item, i16 type)
{
    Process *process;

    process = firstProcess(item, type);
    if (process != nullptr) {
        markItemType(item, type);
    } else {
        unmarkItemType(item, type);
    }
}

bool itemHasType(u16 item, i16 type)
{
    u8 *types;
    u16 bit;

    types = &itemTypes[item];
    bit = type - FIRST_ITEM_TYPE;
    bit = 1 << bit;
    return (*types & bit) != 0;
}

void clearTypeCounts()
{
    memset(typeCounts, 0, TYPES * sizeof(u32));
}

/* Counts a run of the current process under its type. */
/* UNCALLED: nothing calls it. */
void countType()
{
    u16 pid;

    pid = currentPid();
    if (pid != 0U) {
        Process *process;

        process = findProcess(pid);
        typeCounts[process->type] = typeCounts[process->type] + 1;
    }
}

extern "C" void mouseEvent(u16 events, u16 buttons, u16 x, u16 y)
{
    EventProcess *process;
    MouseEvent event;

    event.events = events;
    event.buttons = buttons;
    event.x = x;
    event.y = y;
    if (mouseHeld != 0U) {
        event.events &= ~MOUSE_MOVED;
        if (event.events == 0U) {
            return;
        }
    }
    mouseHeld++;
    mouseList.total = mouseList.count;
    CLEAR_DIRECTION();
    if ((kernelReady != 0U) && (mouseList.total != 0U)) {
        mouseList.total--;
        for (mouseList.index = 0; mouseList.index < mouseList.total; mouseList.index++) {
            mouseList.stop = 0;
            process = (EventProcess *)findProcess(mouseList.get(mouseList.index));
            if ((process->eventMask & EVENT_MOUSE) != 0U) {
                MouseEvent copy = event;

                clockPushHold();
                clockRelease();
                process->event(EVENT_MOUSE, &copy);
                clockPopHold();
            }
        }
    }
    mouseHeld--;
}

/* MOUSECB.ASM (11d0:0514): the handler the game gives the mouse, which
 * passes the events on unless the kernel holds them. */
void mouseCallback(u16 events, u16 buttons, u16 x, u16 y)
{
    if (mouseHeld == 0U) {
        mouseEvent(events, buttons, x, y);
    }
}

/* UNCALLED: nothing calls it. */
void addMouseListener(Process *process)
{
    if (inInterrupt != 0U) {
        halt();
    }
    if (process->hasFlags(PROCESS_EVENTS) == 0U) {
        halt();
    }
    if (process->hasStatus(STATUS_MOUSE) != 0U) {
        return;
    }
    process->status |= STATUS_MOUSE;
    mouseListeners++;
    mouseList.add(process->pid);
}

void removeMouseListener(Process *process)
{
    if (inInterrupt != 0U) {
        halt();
    }
    if ((process->hasFlags(PROCESS_EVENTS) == 0U) || (process->hasStatus(STATUS_MOUSE) == 0U)) {
        return;
    }
    process->status &= ~STATUS_MOUSE;
    mouseListeners--;
    mouseList.remove(process->pid);
}
