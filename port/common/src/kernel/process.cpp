/* PROCESS.C (11f8:0000-1b6b): the kernel's processes. See kprocess.h. */
#include "kprocess.h"
#include "kernel.h"
#include "stdinter.h"
#include "systimer.h"
#include "cprotmem.h"
#include "clock.h"
#include "trace.h"

Process::Process()
{
    /* The original halts on a process on the stack, whose selector is SS. A
     * process of the heap comes from operator new, which has left its size
     * for the kernel (setNewProcessSize, which halts on a second one before
     * this constructor takes it): the port halts without one. */
    if (newProcessSize == 0U) {
        halt();
    }
    item = 0;
    flags = 0;
    status = STATUS_STARTED;
    result = 0;
    pid = 0;
    type = DEFAULT_TYPE;
    allocations = 0;
    messageMask = 0;
    priority = 0;
    timeoutStart = 0;
    timeout = 0;
    setPriority(currentPriority);
    registerProcess(this);
    if (kernelActive != 0U) {
        u16 creator;
        Process *parent;

        creator = currentPid();
        if (creator != 0U) {
            parent = findProcess(creator);
            if (parent->hasStatus(STATUS_WATCHED) != 0U) {
                parent->status |= STATUS_CREATED_CHILD;
            }
        }
    }
    wakeProcess(this);
}

void *Process::operator new(size_t size)
{
    void *memory;

    memory = ::operator new(size);
    if (memory == nullptr) {
        halt();
    }
    /* The original's heap held what it held; the port's is 0, so that
     * what a process leaves unset (a usecode stack below its top) is the
     * same each run, in its saved image too. */
    memset(memory, 0, size);
    setNewProcessSize(size);
    return memory;
}

void Process::operator delete(Process *process, std::destroying_delete_t /*unused*/)
{
    if (process->hasFlags(PROCESS_KILLED) == 0U) {
        halt();
    }
    process->~Process();
}

/* Only when a constructor throws, which none does. */
void Process::operator delete(void *process)
{
    ::operator delete(process);
}

void *Process::operator new(size_t size, Loading /*unused*/)
{
    void *memory = ::operator new(size);
    if (memory == nullptr) {
        halt();
    }
    /* The fields the save has come after; the others are 0. */
    memset(memory, 0, size);
    return memory;
}

/* Only when a Loading constructor throws, which none does. */
/* UNCALLED: nothing calls it. */
void Process::operator delete(void *process, Loading /*unused*/)
{
    ::operator delete(process);
}

void Process::setItem(u16 newItem)
{
    u16 old;
    i16 kind;

    old = item;
    item = newItem;
    markItemProcess(newItem);
    if (old != 0U) {
        updateItemProcess(old);
    }
    kind = type;
    if (IS_ITEM_TYPE(kind)) {
        if (newItem != 0U) {
            markItemType(newItem, kind);
        }
        if (old != 0U) {
            updateItemType(old, kind);
        }
    }
}

void Process::setType(i16 newType)
{
    i16 old;

    if (item == 0) {
        type = newType;
    } else {
        old = type;
        type = newType;
        if (IS_ITEM_TYPE(newType)) {
            markItemType(item, newType);
        }
        if (IS_ITEM_TYPE(old)) {
            updateItemType(item, old);
        }
    }
    if (type > 0xfff) {
        halt();
    }
}

void Process::setPriority(u8 newLevel)
{
    if (newLevel == 0 || newLevel > 31) {
        halt();
    }
    if ((level() != 0U) && level() == newLevel) {
        return;
    }
    if (level() != 0U) {
        removeFromLevel(level());
    }
    priority = newLevel;
    addToLevel(level());
}

void Process::setTimeout(u32 delay)
{
    status |= STATUS_TIMEOUT;
    timeoutStart = systemTimer->ticks;
    timeout = delay;
}

void Process::clearTimeout()
{
    status &= ~STATUS_TIMEOUT;
    timeoutStart = 0;
    timeout = 0;
}

void Process::checkTimeout()
{
    u32 now;
    EventProcess *events;

    now = systemTimer->ticks;
    if (timeoutStart + timeout >= now) {
        return;
    }
    if (hasFlags(PROCESS_EVENTS) != 0U) {
        events = static_cast<EventProcess *>(this);
        if ((events->eventMask & EVENT_TIMEOUT) != 0U) {
            events->event(EVENT_TIMEOUT, 0);
        }
    }
    clearTimeout();
}

/* UNCALLED: nothing calls it. */
void Process::raisePriority()
{
    u8 newLevel;

    newLevel = level() + 1;
    if (newLevel >= ALWAYS_LEVEL) {
        halt();
    }
    if (currentPriority != static_cast<i16>(newLevel - 1)) {
        halt();
    }
    setPriority(newLevel);
    setCurrentPriority(newLevel);
}

void Process::detach()
{
    Process *next;

    if (hasFlags(PROCESS_QUEUED) != 0U) {
        findItemProcess(item, type);
        next = nextItemProcess();
        unqueue();
        if (isValid(next)) {
            next->queue();
        }
    }
    relations.notify(pid);
}

void Process::unlink()
{
    if (hasFlags(PROCESS_TIMED) != 0U) {
        stopTimer();
    }
    if (hasFlags(PROCESS_SUBSCRIBED) != 0U) {
        unsubscribe();
    }
    if (hasStatus(STATUS_MOUSE) != 0U) {
        ignoreMouse();
    }
    if (hasStatus(STATUS_KEYBOARD) != 0U) {
        ignoreKeyboard();
    }
    releasePid(pid);
}

void Process::shutdown()
{
    detach();
    cleanup();
}

void Process::clearEvents()
{
    if (hasFlags(PROCESS_TIMED) != 0U) {
        (static_cast<EventProcess *>(this))->eventMask &= ~EVENT_TIMER;
    }
    if (hasStatus(STATUS_MOUSE) != 0U) {
        (static_cast<EventProcess *>(this))->eventMask &= ~EVENT_MOUSE;
    }
    if (hasStatus(STATUS_KEYBOARD) != 0U) {
        (static_cast<EventProcess *>(this))->eventMask &= ~EVENT_KEYBOARD;
    }
}

void Process::kill()
{
    if (hasFlags(PROCESS_DEAD) != 0U) {
        return;
    }
    if (hasFlags(PROCESS_KILLED) != 0U) {
        return;
    }
    if ((hasFlags(PROCESS_DEAD) != 0U) || (hasFlags(PROCESS_KILLED) != 0U)) {
        halt();
    }
    traceProcessKilled(pid, type, item, processNames[pid]);
    clearEvents();
    flags |= PROCESS_KILLED;
    shutdown();
    destroyProcess(this);
    if (allocations != 0U) {
        halt();
    }
    unlink();
}

void Process::checkWait(u16 finished)
{
    Process *process;
    EventProcess *events;
    u16 i;
    u16 entry;
    u16 first;
    u16 second;
    MListEntry waited;
    u16 me;
    /* The assembly's CPU stack. */
    u16 stack[LIST_MAX];
    u16 depth = 0;

    if (!waitList.valid()) {
        return;
    }
    me = pid;
    if (finished != 0U) {
        process = findProcess(finished);
        process->removeWaiter(me);
        removeWaitingFor(finished);
    }
    for (i = 0; waitList.get(i) != LIST_END; i++) {
        if (waitList.get(i) == finished) {
            waitList.put(i, LIST_TRUE);
        }
    }
    for (i = 0;; i++) {
        entry = waitList.get(i);
        switch (entry) {
        case LIST_TRUE:
            stack[depth++] = 1;
            break;
        case LIST_AND:
            first = stack[--depth];
            second = stack[--depth];
            if ((first != 0U) && (second != 0U)) {
                stack[depth++] = 1;
            } else {
                stack[depth++] = 0;
            }
            break;
        case LIST_OR:
            first = stack[--depth];
            second = stack[--depth];
            if ((first != 0U) || (second != 0U)) {
                stack[depth++] = 1;
            } else {
                stack[depth++] = 0;
            }
            break;
        case LIST_END:
            entry = stack[--depth];
            if (entry != 0U) {
                for (i = 0; (waited = waitList.get(i)).value != LIST_END; i++) {
                    if (waited.isPid()) {
                        process = findProcess(waited.value);
                        process->removeWaiter(me);
                        removeWaitingFor(process->pid);
                    }
                }
                wakeProcess(this);
                waitList.clear();
                if (hasFlags(PROCESS_EVENTS) != 0U) {
                    events = static_cast<EventProcess *>(this);
                    if ((events->eventMask & EVENT_WAIT_OVER) != 0U) {
                        events->event(EVENT_WAIT_OVER, 0);
                    }
                }
            }
            return;
        default:
            stack[depth++] = 0;
        }
    }
}

void Process::removeListener(u16 other)
{
    MListEntry entry;
    u16 index;

    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value &= ~RELATION_LISTENER;
        relations.put(index, entry.value);
        if ((hasFlags(PROCESS_KILLED) == 0U) && !entry.isRelated()) {
            relations.remove(entry.value);
        }
    }
}

void Process::removeListening(u16 other)
{
    MListEntry entry;
    u16 index;

    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value &= ~RELATION_LISTENING;
        relations.put(index, entry.value);
        if ((hasFlags(PROCESS_KILLED) == 0U) && !entry.isRelated()) {
            relations.remove(entry.value);
        }
    }
}

void Process::removeWaitingFor(u16 other)
{
    MListEntry entry;
    u16 index;

    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value &= ~RELATION_WAITING;
        relations.put(index, entry.value);
        if ((hasFlags(PROCESS_KILLED) == 0U) && !entry.isRelated()) {
            relations.remove(entry.value);
        }
    }
}

void Process::removeWaiter(u16 other)
{
    MListEntry entry;
    u16 index;

    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value &= ~RELATION_WAITER;
        relations.put(index, entry.value);
        if ((hasFlags(PROCESS_KILLED) == 0U) && !entry.isRelated()) {
            relations.remove(entry.value);
        }
    }
}

void Process::addListener(u16 other)
{
    MListEntry entry;
    u16 index;

    if (hasFlags(PROCESS_KILLED) != 0U) {
        halt();
    }
    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value |= RELATION_LISTENER;
        relations.put(index, entry.value);
        return;
    }
    entry = other;
    entry.value |= RELATION_LISTENER;
    relations.append(entry.value);
}

void Process::addListening(u16 other)
{
    MListEntry entry;
    u16 index;

    if (hasFlags(PROCESS_KILLED) != 0U) {
        halt();
    }
    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value |= RELATION_LISTENING;
        relations.put(index, entry.value);
        return;
    }
    entry = other;
    entry.value |= RELATION_LISTENING;
    relations.append(entry.value);
}

void Process::addWaitingFor(u16 other)
{
    MListEntry entry;
    u16 index;

    if (hasFlags(PROCESS_KILLED) != 0U) {
        halt();
    }
    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value |= RELATION_WAITING;
        relations.put(index, entry.value);
        return;
    }
    entry = other;
    entry.value |= RELATION_WAITING;
    relations.append(entry.value);
}

void Process::addWaiter(u16 other)
{
    MListEntry entry;
    u16 index;

    if (hasFlags(PROCESS_KILLED) != 0U) {
        halt();
    }
    entry = relations.findPid(other, &index);
    if (entry.value != 0U) {
        entry.value |= RELATION_WAITER;
        relations.put(index, entry.value);
        return;
    }
    entry = other;
    entry.value |= RELATION_WAITER;
    relations.append(entry.value);
}

/* UNCALLED: nothing calls it. */
Process *Process::waitFor(Process *process)
{
    bool waiting;

    if (process->item == 0) {
        process->setItem(item);
    } else if (item == 0) {
        setItem(process->item);
    }
    process->addWaiter(pid);
    addWaitingFor(process->pid);
    waiting = waitList.valid();
    waitList.append(process->pid);
    if (waiting) {
        waitList.append(LIST_AND);
    }
    return process;
}

/* UNCALLED: nothing calls it. */
Process *Process::waitForChild(Process *child)
{
    addFlags(PROCESS_FAIL_WITH_CHILD);
    return waitFor(child);
}

Process *Process::block(Process *waiter)
{
    bool waiting;

    if (waiter->item == 0) {
        waiter->setItem(item);
    } else if (item == 0) {
        setItem(waiter->item);
    }
    addWaiter(waiter->pid);
    waiter->addWaitingFor(pid);
    waiting = waiter->waitList.valid();
    waiter->waitList.append(pid);
    if (waiting) {
        waiter->waitList.append(LIST_AND);
    }
    return waiter;
}

/* UNCALLED: nothing calls it. */
Process *Process::blockChild(Process *waiter)
{
    waiter->addFlags(PROCESS_FAIL_WITH_CHILD);
    return block(waiter);
}

/* UNCALLED: nothing calls it. */
u16 Process::waitFor(u16 other)
{
    Process *process;

    process = findProcess(other);
    waitFor(process);
    return other;
}

void Process::terminate(u32 value)
{
    flags &= ~PROCESS_FAILED;
    if (value != 0U) {
        result = value;
    }
    kill();
}

void Process::fail(u32 value)
{
    flags |= PROCESS_FAILED;
    if (result == 0U) {
        result = value;
    }
    kill();
}

/* UNCALLED: nothing calls it. */
void Process::broadcast(u16 message, u16 filter, u16 mask, void *data) const
{
    broadcastMessage(message, filter, mask, pid, data);
}

/* UNCALLED: nothing calls it. */
void Process::send(u16 message, u16 other, void *data) const
{
    Process *process;

    process = findProcess(other);
    if (process->hasFlags(PROCESS_RECEIVES) != 0U) {
        process->receive(message, pid, data);
    }
}

/* UNCALLED: nothing calls it. */
void Process::suspend()
{
    i16 saved;

    if (hasFlags(PROCESS_SYSTEM) != 0U) {
        halt();
    }
    flags |= PROCESS_SUSPENDED;
    if (type == DEFAULT_TYPE) {
        halt();
    }
    saved = type;
    waitList.tag = saved;
    if (hasFlags(PROCESS_QUEUED) == 0U) {
        suspendProcess(this);
    }
}

/* UNCALLED: nothing calls it. */
void Process::resume()
{
    resumeProcess(pid);
    if (nextItemProcess() != nullptr) {
        halt();
    }
    flags &= ~PROCESS_SUSPENDED;
    waitList.tag = 0;
}

void Process::makeSystem()
{
    if (hasFlags(PROCESS_SUSPENDED) != 0U) {
        halt();
    }
    flags |= PROCESS_SYSTEM;
}

void Process::queue()
{
    if (hasFlags(PROCESS_SUSPENDED) == 0U) {
        halt();
    }
    if (hasFlags(PROCESS_SYSTEM) != 0U) {
        halt();
    }
    flags |= PROCESS_QUEUED;
}

void Process::unqueue()
{
    flags &= ~PROCESS_QUEUED;
}

void Process::markDead()
{
    if (hasStatus(STATUS_CREATED_CHILD) != 0U) {
        halt();
    }
    if (hasFlags(PROCESS_DEAD) != 0U) {
        halt();
    }
    flags |= PROCESS_DEAD;
    deadProcesses++;
}

void Process::cleanup()
{
    if (waitList.valid()) {
        waitList.clear();
    }
    if (relations.valid()) {
        relations.clear();
    }
}

void Process::load(File *file)
{
    waitList.load(file);
    relations.load(file);
}

void Process::save(File *file)
{
    waitList.save(file);
    relations.save(file);
}

/* UNCALLED: nothing calls it. */
void *Process::allocate(u32 size)
{
    void *memory;

    memory = allocateMemory(size, pid, ANY_FRAGMENT);
    if (memory != nullptr) {
        allocations++;
        return memory;
    }
    halt();
}

/* UNCALLED: nothing calls it. */
void Process::release(void *memory)
{
    if (allocations != 0U) {
        releaseMemory(memory, pid);
        allocations--;
        return;
    }
    halt();
}

EventProcess::EventProcess()
{
    flags |= PROCESS_EVENTS;
    eventMask = 0;
}

/* UNCALLED: nothing calls it. */
void Process::stopListening(u16 other)
{
    Process *process;

    process = findProcess(other);
    if (process->hasFlags(PROCESS_EVENTS) == 0U) {
        halt();
    }
    removeListening(other);
    process->removeListener(pid);
}

void Process::listenTo(u16 other)
{
    Process *process;

    process = findProcess(other);
    if (process->hasFlags(PROCESS_EVENTS) == 0U) {
        halt();
    }
    addListening(other);
    process->addListener(pid);
}

/* UNCALLED: nothing calls it. */
void Process::subscribe()
{
    if (KERNEL_SAFE()) {
        stdIntHandler->subscribers.add(pid);
        flags |= PROCESS_SUBSCRIBED;
        return;
    }
    halt();
}

void Process::unsubscribe()
{
    if (KERNEL_SAFE()) {
        stdIntHandler->subscribers.remove(pid);
        flags &= ~PROCESS_SUBSCRIBED;
        return;
    }
    halt();
}

/* UNCALLED: nothing calls it. */
void Process::listenMouse()
{
    if (KERNEL_SAFE()) {
        addMouseListener(this);
        return;
    }
    halt();
}

void Process::ignoreMouse()
{
    if (KERNEL_SAFE()) {
        removeMouseListener(this);
        return;
    }
    halt();
}

void Process::listenKeyboard()
{
    if (KERNEL_SAFE()) {
        addKeyboardListener(this);
        return;
    }
    halt();
}

void Process::ignoreKeyboard()
{
    if (KERNEL_SAFE()) {
        removeKeyboardListener(this);
        return;
    }
    halt();
}

void Process::hold()
{
    if (KERNEL_SAFE()) {
        status |= STATUS_HOLD;
        return;
    }
    halt();
}

/* UNCALLED: nothing calls it. */
void Process::unhold()
{
    if (KERNEL_SAFE()) {
        status &= ~STATUS_HOLD;
        return;
    }
    halt();
}

PeriodicProcess::PeriodicProcess()
{
    addFlags(PROCESS_PERIODIC);
    rate = 0;
    elapsed = 0;
    period = 0;
}

void PeriodicProcess::setRate(u16 hertz)
{
    maskTimer();
    rate = hertz;
    period = static_cast<s32>(1000000L) / hertz;
    elapsed = 0;
    setTimerStep(0);
    restoreTimer();
}

void PeriodicProcess::setPeriod(u32 microseconds)
{
    maskTimer();
    rate = static_cast<s32>(1000000L) / microseconds;
    period = microseconds;
    elapsed = 0;
    setTimerStep(0);
    restoreTimer();
}

void PeriodicProcess::tick()
{
    u32 total;

    total = elapsed;
    total += timerStep;
    if (period > total) {
        elapsed = total;
        return;
    }
    total -= period;
    elapsed = total;
    if (hasStatus(STATUS_TICKING) == 0U) {
        status |= STATUS_TICKING;
        event(EVENT_TIMER, 0);
        status &= ~STATUS_TICKING;
    }
}

void Process::startTimer(u8 run) const
{
    addTimer(pid, run);
}

void Process::stopTimer() const
{
    removeTimer(pid);
}

void Process::pause()
{
    if (hasFlags(PROCESS_PAUSED) == 0U) {
        clockPushHold();
        clockHold();
        addFlags(PROCESS_PAUSED);
        if (hasFlags(PROCESS_TIMED) != 0U) {
            runningTimers--;
            pausedTimers++;
        }
        clockPopHold();
    }
}

/* UNCALLED: nothing calls it. */
void Process::unpause()
{
    if (hasFlags(PROCESS_PAUSED) != 0U) {
        clockPushHold();
        clockHold();
        removeFlags(PROCESS_PAUSED);
        if (hasFlags(PROCESS_TIMED) != 0U) {
            pausedTimers--;
            runningTimers++;
        }
        clockPopHold();
    }
}
