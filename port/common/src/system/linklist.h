/* 12b8:0000-0372, file name unknown: an intrusive singly linked list. Objects
 * derive from ListLink and are chained by next from first to last. The sound
 * cache keeps its slots in one, least recently used first. gumplist.h has the
 * doubly linked list. */
#ifndef PORT_LINKLIST_H
#define PORT_LINKLIST_H
#include "common.h"

class ListLink {
public:
    ListLink *next;
    ListLink() { next = nullptr; }
    virtual ~ListLink();
};
class LinkList {
public:
    ListLink *first, *last;
    LinkList() { first = last = nullptr; }
    virtual ~LinkList() { clear(); }
    /* UNCALLED: nothing calls it. */
    void prepend(ListLink *link);
    void append(ListLink *link);
    /* Moves link to the back. */
    void raise(ListLink *link);
    /* Detaches link and deletes it; previous as for detach. */
    /* UNCALLED: nothing calls it. */
    void destroy(ListLink *link, ListLink *previous);
    /* Puts the chain that starts at link after position. */
    /* UNCALLED: nothing calls it. */
    void insertAfter(ListLink *position, ListLink *link);
    /* Takes link out. previous is the link before it, or 0 to look it up. */
    void detach(ListLink *link, ListLink *previous);
    /* Deletes every link. */
    void clear();
    /* Steps *link to the next link, starting from the first when *link is
     * null; 0 past the end. */
    /* UNCALLED: nothing calls it. */
    bool forward(ListLink **link) const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u32 count() const;
};
ASSERT_SIZE(ListLink, 0x6);
ASSERT_SIZE(LinkList, 0xa);
#endif
