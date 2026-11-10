/* 12b8:0000-0372, file name unknown: an intrusive singly linked list. See
 * linklist.h. */
#include "common.h"
#include "linklist.h"

ListLink::~ListLink() = default;

/* UNCALLED: nothing calls it. */
void LinkList::prepend(ListLink *link)
{
    if (link == nullptr) {
        return;
    }
    link->next = first;
    first = link;
    if (last == nullptr) {
        last = first;
    }
}

void LinkList::append(ListLink *link)
{
    if (link == nullptr) {
        return;
    }
    if (last != nullptr) {
        last->next = link;
    } else {
        first = link;
    }
    last = link;
    link->next = nullptr;
}

void LinkList::raise(ListLink *link)
{
    detach(link, nullptr);
    append(link);
}

/* UNCALLED: nothing calls it. */
void LinkList::destroy(ListLink *link, ListLink *previous)
{
    detach(link, previous);
    delete link;
}

/* UNCALLED: nothing calls it. */
void LinkList::insertAfter(ListLink *position, ListLink *link)
{
    ListLink *end;
    ListLink *next;

    if (position == nullptr || link == nullptr) {
        return;
    }
    end = link;
    next = link->next;
    while (next != nullptr) {
        end = next;
        next = next->next;
    }
    if (last != position) {
        end->next = position->next;
        position->next = link;
        return;
    }
    last = end;
    end->next = nullptr;
    position->next = link;
}

void LinkList::detach(ListLink *link, ListLink *previous)
{
    ListLink *current;

    if (previous == nullptr) {
        for (current = first; current != nullptr && current != link; current = current->next) {
            previous = current;
        }
        if (current == nullptr) {
            return;
        }
    }
    if (previous == nullptr) {
        first = first->next;
    }
    if (last != link) {
        if (previous != nullptr) {
            previous->next = link->next;
        }
        if (first == link) {
            first = link->next;
        }
    } else {
        last = previous;
        if (previous != nullptr) {
            previous->next = nullptr;
        }
    }
}

void LinkList::clear()
{
    ListLink *next;

    while (first != nullptr) {
        next = first->next;
        delete first;
        first = next;
    }
    first = last = nullptr;
}

/* UNCALLED: nothing calls it. */
bool LinkList::forward(ListLink **link) const
{
    if (*link == nullptr) {
        *link = first;
    } else {
        *link = (*link)->next;
    }
    return *link != nullptr;
}

/* UNCALLED: nothing calls it. */
u32 LinkList::count() const
{
    u32 links = 0;
    ListLink *link;

    for (link = first; link != nullptr; link = link->next) {
        links++;
    }
    return links;
}
