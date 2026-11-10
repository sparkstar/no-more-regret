/* KDAEMON.C (11d0:0000-02e0): the key daemon, which repeats the keys held
 * down. See kdaemon.h. */
#include "kdaemon.h"
#include "gamestate.h"
#include "devices.h"
#include "kernel.h"
#include "keyboard.h"
#include "systimer.h"

KeyDaemon *&keyDaemon = gameGlobal<KeyDaemon *>(nullptr);
u16 &repeatDelay = gameGlobal<u16>(10);
u16 &repeatRate = gameGlobal<u16>(0);

KeyDaemon::KeyDaemon()
{
    if (keyDaemon != nullptr) {
        halt();
    }
    keyDaemon = this;
    nameProcess(pid, "KeyDaemon");
    setPriority(SYSTEM_LEVEL);
    setType(KEY_DAEMON_TYPE);
    makeSystem();
    pause();
    setRate(45);
    startTimer(1);
    eventMask |= EVENT_TIMER;
    flags |= PROCESS_ACTIVE;
}

void KeyDaemon::run() {}

void KeyDaemon::event(u32 event, EventData /*data*/)
{
    switch (event) {
    case EVENT_TIMER: {
        u16 scan;
        u16 bank;
        u16 queued;
        u16 pending;
        KeyState *states;
        u32 now;
        bool crowded;
        KeyEvent key;

        now = systemTimer->ticks;
        crowded = false;
        queued = queuedKeys();
        pending = pendingKeys;
        if (static_cast<ui16>(queued + pending) >= static_cast<ui16>(KEY_QUEUE / 2)) {
            crowded = true;
        }
        if (!keyboardPaused()) {
            for (bank = 0; bank < 2; bank++) {
                if (bank != 0U) {
                    states = extendedKeys;
                } else {
                    states = plainKeys;
                }
                for (scan = 0; scan < SCAN_CODES; scan++) {
                    if (states[scan].count != 0 && states[scan].time + repeatDelay <= now) {
                        states[scan].count++;
                        key.scan = scan;
                        key.shift = keyShift;
                        key.time = systemTimer->ticks;
                        key.pressed = 1;
                        key.extended = bank;
                        key.repeat = states[scan].count;
                        key.translate();
                        dispatchKey(&key);
                        if (!crowded) {
                            queueKey(&key);
                        }
                    }
                }
            }
            /* The keys of the other devices' inputs held, and the sticks'
             * turns (devices.h). */
            deviceTick();
        }
    }
    }
}

void KeyDaemon::cleanup()
{
    keyDaemon = nullptr;
}

void KeyDaemon::load(File *file)
{
    Process::load(file);
    keyDaemon = this;
}
