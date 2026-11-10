/* SYSTIMER.C (11d8:0000-079a): the system timer and the processes that wait
 * on it. See systimer.h. */
#include "systimer.h"
#include "gamestate.h"
#include "kernel.h"

SystemTimer *&systemTimer = gameGlobal<SystemTimer *>(nullptr);

SystemTimer::SystemTimer()
{
    nameProcess(pid, "SystemTimer");
    ticks = 0;
    if (systemTimer != nullptr) {
        halt();
    }
    systemTimer = this;
    setPriority(SYSTEM_LEVEL);
    setRate(60);
    eventMask |= EVENT_TIMER;
    setType(SYSTEM_TIMER_TYPE);
    pause();
    flags |= PROCESS_ACTIVE;
    makeSystem();
    startTimer(1);
}

void SystemTimer::event(u32 /*event*/, EventData /*data*/)
{
    ticks++;
}

void SystemTimer::cleanup()
{
    systemTimer = nullptr;
}

void SystemTimer::load(File *file)
{
    Process::load(file);
    systemTimer = this;
}

AccWait::AccWait(u32 ticks, u16 processItem)
{
    nameProcess(pid, "AccWait");
    setItem(processItem);
    start = systemTimer->ticks;
    count = ticks;
    setType(ACC_WAIT_TYPE);
    remaining = ticks;
    setRate(60);
    flags |= PROCESS_ACTIVE;
    pause();
    eventMask |= EVENT_TIMER;
}

void AccWait::resume()
{
    start = systemTimer->ticks;
    startTimer(1);
}

void AccWait::restart()
{
    start = systemTimer->ticks;
    remaining = count;
    flags |= PROCESS_ACTIVE;
    eventMask = EVENT_TIMER;
}

void AccWait::run()
{
    finish();
}

void AccWait::event(u32 event, EventData /*data*/)
{
    switch (event) {
    case EVENT_TIMER:
        if (--remaining == 0) {
            flags &= ~PROCESS_ACTIVE;
            eventMask &= ~EVENT_TIMER;
        }
        break;
    case EVENT_WAIT_OVER:
        if (hasFlags(PROCESS_TIMED) == 0U) {
            resume();
        }
        break;
    }
}

WaitProcess::WaitProcess(u32 time, u16 processItem)
{
    nameProcess(pid, "Wait");
    setItem(processItem);
    start = systemTimer->ticks;
    duration = time;
    setType(WAIT_TYPE);
    eventMask |= EVENT_WAIT_OVER;
    flags |= PROCESS_ACTIVE;
}

void WaitProcess::resume()
{
    start = systemTimer->ticks;
    flags &= ~PROCESS_ACTIVE;
}

void WaitProcess::restart()
{
    start = systemTimer->ticks;
}

void WaitProcess::run()
{
    u32 now;

    now = systemTimer->ticks;
    if (start + duration <= now) {
        terminate(0);
    }
}

void WaitProcess::event(u32 event, EventData /*data*/)
{
    switch (event) {
    case EVENT_WAIT_OVER:
        if (hasFlags(PROCESS_ACTIVE) != 0U) {
            resume();
        }
        break;
    }
}

/* UNCALLED: nothing calls it. */
TimerProcess::TimerProcess()
{
    nameProcess(pid, "Timer");
    pause();
    makeSystem();
    setType(TIMER_TYPE);
    seconds = 0;
    tenths = 0;
    running = 0;
    setRate(10);
    addEvents(EVENT_TIMER);
}

/* UNCALLED: nothing calls it. */
void TimerProcess::stop()
{
    running = 0;
    stopTimer();
}

/* UNCALLED: nothing calls it. */
void TimerProcess::start()
{
    running = 1;
    seconds = 0;
    tenths = 0;
    startTimer(1);
}

void TimerProcess::event(u32 event, EventData /*data*/)
{
    switch (event) {
    case EVENT_TIMER:
        tenths++;
        if (tenths == 10) {
            seconds++;
            tenths = 0;
        }
        break;
    }
}
