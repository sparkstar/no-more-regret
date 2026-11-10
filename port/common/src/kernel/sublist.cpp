/* 1208:0341-0437, file name unknown: the subscriber lists. See sublist.h. */
#include "sublist.h"

void SubscriberList::remove(u16 pid)
{
    u16 removed;
    u16 current;

    removed = MList::remove(pid);
    if (valid()) {
        current = index;
        if (current == removed) {
            index--;
            stop = 1;
        } else if (current > removed) {
            index--;
        }
        total--;
    } else {
        total = 0;
        index = 0;
        stop = 1;
    }
}

u16 SubscriberList::add(u16 pid)
{
    u16 size;
    u16 i;
    u16 result;

    if (valid()) {
        size = count;
        for (i = 0; i < size; i++) {
            if (data[i] == pid) {
                return size;
            }
        }
    }
    result = append(pid);
    total++;
    return result;
}
