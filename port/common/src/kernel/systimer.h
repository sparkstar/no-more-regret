/* SYSTIMER.C: the system timer, a process on the kernel's
 * timer that counts ticks at 60 Hz, and three processes that wait on it:
 * AccWait counts timer events, WaitProcess compares tick counts, and
 * TimerProcess keeps seconds and tenths. */
#ifndef PORT_SYSTIMER_H
#define PORT_SYSTIMER_H
#include "common.h"
#include "kprocess.h"
#include "waits.h"

class SystemTimer : public PeriodicProcess {
public:
    explicit SystemTimer(Loading loading) : PeriodicProcess(loading) {}
    u32 ticks;
    u8 reserved[6];

    SystemTimer();
    /* The tick count as a value: DISPLAY.C keeps its low word (int now=time())
     * after reading all 32 bits. */
    [[nodiscard]] u32 time() const { return ticks; }
    void run() override {}
    void event(u32 event, EventData data) override;
    void cleanup() override;
    void load(File *file) override;
};

class AccWait : public PeriodicProcess {
public:
    explicit AccWait(Loading loading) : PeriodicProcess(loading) {}
    u32 start;
    u16 remaining;
    u16 count;

    AccWait(u32 ticks, u16 processItem);
    void resume();
    virtual void restart();
    void run() override;
    void event(u32 event, EventData data) override;
    virtual void finish() { terminate(0); }
};

class WaitProcess : public EventProcess {
public:
    explicit WaitProcess(Loading loading) : EventProcess(loading) {}
    u32 start;
    u32 duration;

    WaitProcess(u32 time, u16 processItem);
    void resume();
    virtual void restart();
    void run() override;
    void event(u32 event, EventData data) override;
};

class TimerProcess : public PeriodicProcess {
public:
    explicit TimerProcess(Loading loading) : PeriodicProcess(loading) {}
    u8 running;
    u16 seconds;
    u16 tenths;

    /* UNCALLED: nothing calls it. */
    TimerProcess();
    /* UNCALLED: nothing calls it. */
    void stop();
    /* UNCALLED: nothing calls it. */
    void start();
    void run() override {}
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(SystemTimer, 0x4a);
ASSERT_SIZE(AccWait, 0x48);
ASSERT_SIZE(WaitProcess, 0x3e);
ASSERT_SIZE(TimerProcess, 0x45);

/* No Remorse's PLAYER.C reads it far. */
extern SystemTimer *&systemTimer;

/* The original's SystemTimer::delay as a wait: count ticks of the system
 * timer from now, then code (a lambda as waits.h's giveRest's). */
template <typename Code> void openTimerDelay(u32 count, Code code)
{
    const u32 first = systemTimer->ticks;
    openWaitWhile([first, count]() { return first + count > systemTimer->ticks; }, std::move(code));
}
#endif
