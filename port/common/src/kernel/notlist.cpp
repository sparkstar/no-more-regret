/* NOTLIST.C (11f0:0000-0230): the relations list. See notlist.h. */
#include "notlist.h"
#include "kprocess.h"
#include "kernel.h"

void NotifyList::notify(u16 pid)
{
    u16 i;
    MListEntry entry;
    u32 result;
    Process *related;
    Process *dying;
    bool failed;

    if (valid()) {
        dying = findProcess(pid);
        result = dying->result;
        failed = (dying->flags & PROCESS_FAILED) != 0;
        for (i = 0; (entry = get(i)).value != LIST_END; i++) {
            related = findProcess(entry.pid());
            if (entry.has(RELATION_LISTENER) != 0U) {
                related->removeListening(pid);
            }
            if (entry.has(RELATION_WAITING) != 0U) {
                related->removeWaiter(pid);
            }
            if (entry.has(RELATION_WAITER) != 0U) {
                related->setResult(result);
                if (failed) {
                    related->flags |= PROCESS_CHILD_FAILED;
                    if ((related->flags & PROCESS_FAIL_WITH_CHILD) != 0) {
                        related->fail(result);
                        continue;
                    }
                } else {
                    related->flags &= ~PROCESS_CHILD_FAILED;
                }
                related->checkWait(pid);
            }
            if (related->hasFlags(PROCESS_DEAD) != 0U) {
                continue;
            }
            if (entry.has(RELATION_LISTENING) != 0U) {
                related->removeListener(pid);
                related->setResult(result);
                if (failed) {
                    related->flags |= PROCESS_CHILD_FAILED;
                } else {
                    related->flags &= ~PROCESS_CHILD_FAILED;
                }
                if (related->hasFlags(PROCESS_EVENTS) != 0U) {
                    EventProcess *events;

                    events = static_cast<EventProcess *>(related);
                    if ((events->eventMask & EVENT_RELATED_DIED) != 0U) {
                        if ((events->hasFlags(PROCESS_KILLED) == 0U) || events->pid == pid) {
                            events->event(EVENT_RELATED_DIED, pid);
                        }
                    }
                } else {
                    halt();
                }
            }
        }
    }
}
