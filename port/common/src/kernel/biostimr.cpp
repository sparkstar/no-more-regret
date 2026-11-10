/* 11e0:02d1-044a, file name unknown: BiosTimer, which passes the timer's
 * ticks on to the BIOS at its 18.2 Hz, in the original's order, over the
 * platform's clock (clock.h): the clock's own tick is what the original's
 * BIOS did. See biostimr.h. */
#include "biostimr.h"
#include "clock.h"
#include "kernel.h"

BiosTimer::BiosTimer()
{
    realVector = 0;
    protectedVector = nullptr;
    nameProcess(pid, "BiosTimer");
    setPriority(SYSTEM_LEVEL);
    setPeriod(BIOS_TICK);
    eventMask |= EVENT_TIMER;
    setType(BIOS_TIMER_TYPE);
    pause();
    flags |= PROCESS_ACTIVE;
    makeSystem();
}

void BiosTimer::setVectors()
{
    realVector = BIOS_TIMER_HANDLER;
    protectedVector = nullptr;
}

/* A far call to the BIOS's handler with the flags pushed, as an interrupt
 * makes it (DosRealFarCall): the clock's own tick. */
void BiosTimer::event(u32 /*event*/, EventData /*data*/)
{
    clockPushHold();
    CLEAR_DIRECTION();
    clockOwnTick();
    clockPopHold();
}
