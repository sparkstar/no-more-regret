/* STDINT.C (1208:0000-0340): the standard interrupt handler. See stdinter.h. */
#include "stdinter.h"
#include "gamestate.h"
#include "kernel.h"

StdIntHandler *&stdIntHandler = gameGlobal<StdIntHandler *>(nullptr);

StdIntHandler::StdIntHandler()
{
    if (stdIntHandler != nullptr) {
        halt();
    }
    stdIntHandler = this;
    setType(STDINT_TYPE);
    nameProcess(pid, "StdIntHandler");
    makeSystem();
}

void StdIntHandler::run()
{
    EventProcess *process;
    u32 mask;
    u32 bit;
    EventData data;
    i16 i;

    subscribers.total = subscribers.count;
    if (subscribers.total != 0U) {
        subscribers.total--;
        for (subscribers.index = 0; subscribers.index < subscribers.total; subscribers.index++) {
            pollingPid = subscribers.get(subscribers.index);
            subscribers.stop = 0;
            process = static_cast<EventProcess *>(findProcess(pollingPid));
            mask = process->eventMask;
            if (mask != 0U) {
                if (process->hasFlags(PROCESS_BLOCKED) != 0U) {
                    continue;
                }
                for (i = 8; i <= 31; i++) {
                    /* An int shift: bit 15 becomes FFFF8000h and the
                     * higher bits 0, as in the original. */
                    bit = static_cast<i16>(1 << i);
                    if ((mask & bit) != 0U) {
                        if (process->poll(bit, &data)) {
                            process->event(bit, data);
                            if (subscribers.stop != 0U) {
                                break;
                            }
                        }
                    }
                }
            }
            pollingPid = 0;
        }
    }
}

void StdIntHandler::cleanup()
{
    Process::cleanup();
    if (subscribers.valid()) {
        subscribers.clear();
    }
    stdIntHandler = nullptr;
}

void StdIntHandler::load(File *file)
{
    Process::load(file);
    subscribers.load(file);
}

void StdIntHandler::save(File *file)
{
    Process::save(file);
    subscribers.save(file);
}
