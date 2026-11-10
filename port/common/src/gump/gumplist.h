/* 12a8:0000-04ce, file name unknown: an intrusive doubly
 * linked list. Objects derive from GumpLink and are chained by next (towards
 * last) and previous (towards first). The GUI's child lists, the sound cache
 * and others use it. */
#ifndef PORT_GUMPLIST_H
#define PORT_GUMPLIST_H
#include "common.h"

class GumpLink {
public:
    GumpLink *next, *previous;
    GumpLink()
    {
        next = nullptr;
        previous = nullptr;
    }
    virtual ~GumpLink();
};
class GumpList {
public:
    GumpLink *first, *last;
    GumpList()
    {
        first = nullptr;
        last = nullptr;
    }
    virtual ~GumpList() { clear(); }
    void prepend(GumpLink *link);
    void append(GumpLink *link);
    /* Puts link after (before) position; a null position means the front
     * (the back). */
    void insertAfter(GumpLink *position, GumpLink *link);
    void insertBefore(GumpLink *position, GumpLink *link);
    void moveAfter(GumpLink *position, GumpLink *link);
    /* UNCALLED: nothing calls it. */
    void moveBefore(GumpLink *position, GumpLink *link);
    /* Moves link to the front (the back). The GUI paints the back last. */
    void lower(GumpLink *link);
    void raise(GumpLink *link);
    void detach(GumpLink *link);
    /* Detaches link and deletes it. */
    void destroy(GumpLink *link);
    /* Deletes every link. */
    void clear();
    /* Steps *link to the next (previous) link, starting from the first (last)
     * when *link is null; 0 past the end. */
    bool forward(GumpLink **link) const;
    bool backward(GumpLink **link) const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u32 count() const;
};
ASSERT_SIZE(GumpLink, 0xa);
ASSERT_SIZE(GumpList, 0xa);
#endif
