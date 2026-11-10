/* 12a8:0000-04ce, file name unknown: an intrusive doubly linked list. See
 * gumplist.h. */
#include "common.h"
#include "gumplist.h"

GumpLink::~GumpLink() = default;

void GumpList::prepend(GumpLink *link)
{
    link->previous = nullptr;
    link->next = first;
    if (first != nullptr) {
        first->previous = link;
    } else {
        last = link;
    }
    first = link;
}

void GumpList::append(GumpLink *link)
{
    link->previous = last;
    link->next = nullptr;
    if (last != nullptr) {
        last->next = link;
    } else {
        first = link;
    }
    last = link;
}

void GumpList::insertAfter(GumpLink *position, GumpLink *link)
{
    if (link != nullptr) {
        if (position == nullptr) {
            prepend(link);
        } else {
            if (last != position) {
                link->next = position->next;
                link->previous = position;
                position->next->previous = link;
            } else {
                last = link;
                link->next = nullptr;
                link->previous = position;
            }
            position->next = link;
        }
    }
}

void GumpList::insertBefore(GumpLink *position, GumpLink *link)
{
    if (link != nullptr) {
        if (position == nullptr) {
            append(link);
        } else {
            if (first != position) {
                link->next = position;
                link->previous = position->previous;
                position->previous->next = link;
            } else {
                first = link;
                link->previous = nullptr;
                link->next = position;
            }
            position->previous = link;
        }
    }
}

void GumpList::moveAfter(GumpLink *position, GumpLink *link)
{
    detach(link);
    insertAfter(position, link);
}

/* UNCALLED: nothing calls it. */
void GumpList::moveBefore(GumpLink *position, GumpLink *link)
{
    detach(link);
    insertBefore(position, link);
}

void GumpList::lower(GumpLink *link)
{
    detach(link);
    insertAfter(nullptr, link);
}

void GumpList::raise(GumpLink *link)
{
    detach(link);
    insertBefore(nullptr, link);
}

void GumpList::detach(GumpLink *link)
{
    GumpLink *previous;

    if (link == nullptr) {
        return;
    }
    previous = link->previous;
    if (previous != nullptr) {
        previous->next = link->next;
    } else {
        first = link->next;
    }
    if (link->next != nullptr) {
        link->next->previous = previous;
    } else {
        if (previous != nullptr) {
            previous->next = nullptr;
        } else {
            first = nullptr;
        }
        last = previous;
    }
}

void GumpList::destroy(GumpLink *link)
{
    detach(link);
    delete link;
}

void GumpList::clear()
{
    GumpLink *next;

    while (first != nullptr) {
        next = first->next;
        delete first;
        first = next;
    }
    first = last = nullptr;
}

bool GumpList::forward(GumpLink **link) const
{
    if (*link == nullptr) {
        *link = first;
    } else {
        *link = (*link)->next;
    }
    return *link != nullptr;
}

bool GumpList::backward(GumpLink **link) const
{
    if (*link == nullptr) {
        *link = last;
    } else {
        *link = (*link)->previous;
    }
    return *link != nullptr;
}

/* UNCALLED: nothing calls it. */
u32 GumpList::count() const
{
    u32 links = 0;
    GumpLink *link;

    for (link = first; link != nullptr; link = link->next) {
        links++;
    }
    return links;
}
