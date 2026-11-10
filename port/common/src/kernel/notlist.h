/* NOTLIST.C (11f0:0000-0230): the relations list of a process, which tells
 * the related processes when the process ends. */
#ifndef PORT_NOTLIST_H
#define PORT_NOTLIST_H
#include "common.h"
#include "mlist.h"

class NotifyList : public MList {
public:
    /* BCC 3.1 made the implicit destructor of a class with virtual functions
     * virtual when it has work to do, and a delete through this class called the
     * object's own. */
    ~NotifyList() override = default;
    /* Tells the related processes that pid ends. */
    void notify(u16 pid);
};
ASSERT_SIZE(NotifyList, 0xa);
#endif
