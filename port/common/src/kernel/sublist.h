/* 1208:0341-0437, file name unknown: the subscriber lists of the kernel and of
 * the standard interrupt handler. A module of its own: MOD2AMF.EXE starts a
 * segment with it. */
#ifndef PORT_SUBLIST_H
#define PORT_SUBLIST_H
#include "common.h"
#include "mlist.h"

/* A list of pids that is walked while it may change: remove moves the walk
 * back over the removed entry and stops it when that was the current one. */
class SubscriberList : public MList {
public:
    /* BCC 3.1 made the implicit destructor of a class with virtual functions
     * virtual when it has work to do, and a delete through this class called the
     * object's own. */
    ~SubscriberList() override = default;
    u8 stop;
    u16 index;
    u16 total;
    void remove(u16 pid);
    u16 add(u16 pid);
};
ASSERT_SIZE(SubscriberList, 0xf);
#endif
