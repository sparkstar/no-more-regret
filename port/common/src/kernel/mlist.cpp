/* MLIST.C (1200:0000-085e): lists of process ids. See mlist.h. */
#include "mlist.h"
#include <cstdarg>
#include "file.h"
#include "kernel.h"
#include "kprocess.h"

u16 MList::build(u16 owner, ...)
{
    u16 i;
    u16 size;
    MListEntry entry;
    u16 *argument;
    /* Set and read under the same owner; with an owner and no pid in the
     * list, checkWait would run on nothing, but no list is built with an
     * owner (Process::waitOn has no callers). */
    Process *process = nullptr;
    Process *ownerProcess = nullptr;

    va_list list;
    u16 words[LIST_MAX];

#pragma GCC diagnostic push
/* The original's last named parameter is a 16-bit value, which the call
 * promotes: ISO C++ leaves va_start after it undefined, GCC's and Clang's
 * va_start do not use it. */
#pragma GCC diagnostic ignored "-Wvarargs"
    va_start(list, owner);
#pragma GCC diagnostic pop
    for (size = 0; size < LIST_MAX;) {
        if ((words[size++] = static_cast<u16>(va_arg(list, int))) == LIST_END) {
            break;
        }
    }
    va_end(list);
    if (words[size - 1] != LIST_END) {
        halt();
    }
    argument = words;
    if (owner != 0U) {
        ownerProcess = findProcess(owner);
    }
    if (data != nullptr) {
        delete[] data;
    }
    data = new u16[size];
    for (i = 0; i < size; i++) {
        entry = *argument++;
        data[i] = entry.value;
        if ((owner != 0U) && entry.isPid()) {
            process = findProcess(entry.value);
            process->addWaiter(owner);
            ownerProcess->addWaitingFor(entry.value);
            if (process->item == 0) {
                process->setItem(ownerProcess->item);
            } else if (ownerProcess->item == 0) {
                ownerProcess->setItem(process->item);
            }
        }
    }
    count = size;
    if (owner != 0U) {
        process->checkWait(0);
    }
    return count;
}

void MList::clear()
{
    if (data == nullptr) {
        halt();
    }
    delete[] data;
    count = 0;
    data = nullptr;
}

MList::~MList()
{
    if (valid()) {
        clear();
    }
}

u16 MList::add(u16 entry)
{
    u16 size;
    u16 i;

    if (valid()) {
        size = count;
        for (i = 0; i < size; i++) {
            if (data[i] == entry) {
                return size;
            }
        }
    }
    return append(entry);
}

u16 MList::append(u16 entry)
{
    u16 *copy;
    u16 i;
    u16 size;

    if (valid()) {
        size = count;
        copy = new u16[size + 1];
        size--;
        for (i = 0; i < size; i++) {
            copy[i] = data[i];
        }
        copy[i] = entry;
        i++;
        copy[i] = LIST_END;
        delete[] data;
        data = copy;
        count = size + 2;
        return count;
    }
    return build(0, entry, LIST_END);
}

u16 MList::remove(u16 entry)
{
    u16 size;
    u16 kept;
    u16 i;
    /* Returned for an entry not in the list, where the original returns what
     * its stack held; no caller looks at it. */
    u16 index = 0;
    u16 *copy;
    u16 j;

    if (valid()) {
        size = count;
        for (i = 0, kept = 0; i < size; i++) {
            if (get(i) != entry) {
                kept++;
            }
        }
        if (kept == 1) {
            clear();
            return 0;
        }
        copy = new u16[kept];
        for (i = 0, j = 0; i < size; i++) {
            if (get(i) != entry) {
                copy[j++] = get(i);
            } else {
                index = i;
            }
        }
        delete[] data;
        data = copy;
        count = kept;
        return index;
    }
    halt();
}

u16 MList::get(u16 index) const
{
    if (index >= count) {
        halt();
    }
    return data[index];
}

void MList::put(u16 index, u16 entry) const
{
    if (index >= count) {
        halt();
    }
    data[index] = entry;
}

/* UNCALLED: nothing calls it. */
u16 MList::find(u16 entry) const
{
    u16 i;

    if (valid()) {
        for (i = 0; i < count; i++) {
            if (get(i) == entry) {
                return i;
            }
        }
    }
    return 0;
}

/* UNCALLED: nothing calls it. */
MListEntry MList::findPid(u16 pid) const
{
    u16 i;
    MListEntry entry;

    if (valid()) {
        for (i = 0; i < count; i++) {
            entry = get(i);
            if (entry.isPid() && entry.pid() == pid) {
                return entry;
            }
        }
    }
    return MListEntry(0);
}

MListEntry MList::findPid(u16 pid, u16 *index) const
{
    u16 i;
    MListEntry entry;

    if (valid()) {
        for (i = 0; i < count; i++) {
            entry = get(i);
            if (entry.isPid() && entry.pid() == pid) {
                *index = i;
                return entry;
            }
        }
    }
    return MListEntry(0);
}

void MList::allocate(u16 size)
{
    if (valid()) {
        halt();
    }
    data = new u16[size];
    count = size;
}

void MList::load(File *file)
{
    u16 size;

    size = count;
    if (valid()) {
        data = nullptr;
        allocate(size);
        file->read(data, static_cast<ui16>(size * sizeof(u16)));
    }
}

void MList::save(File *file)
{
    if (valid()) {
        file->write(data, static_cast<ui16>(count * sizeof(u16)));
    }
}
