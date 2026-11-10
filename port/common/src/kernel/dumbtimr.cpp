/* 1408:0383-04a0, file name unknown: DumbTimer. See dumbtimr.h. */
#include "dumbtimr.h"
#include "control.h"
#include "kernel.h"
#include "systimer.h"

DumbTimer::DumbTimer()
{
    nameProcess(pid, "DumbTimer");
    makeSystem();
    lastTick = 0;
}

void DumbTimer::run()
{
    const u32 now = systemTimer->ticks;

    if (lastTick + 60 < now) {
        controlTick();
        lastTick = now;
    }
}

void DumbTimer::event(u32 /*event*/, EventData /*data*/) {}
