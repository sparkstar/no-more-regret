/* A ring buffer of N items, the GUI's input queues (MOUSECTL.C's mouse
 * records, DISPATCH.C's keys). File name unknown. The members of an instance
 * are out-of-line copies: a module that defines a Queue object makes them
 * virtual segments, and a module compiled with -Jgd makes them public
 * definitions where a typedef names the instance, as Borland's TMPLINST.CPP
 * does. The linker keeps the public
 * ones, two modules of their own: the mouse queue's after DraggableGump
 * (1328:0652, 06ab, mouseque.cpp), the key queue's after GUMPSHAP.C
 * (1338:02ed, 0342, keyqueue.cpp). */
#ifndef PORT_QUEUE_H
#define PORT_QUEUE_H
#include "common.h"
template <class T, i16 N> class Queue {
public:
    /* Private in the original; the saved game moves them one by one
     * (savefields.cpp). */
    T items[N];
    s16 head, tail, count;

    Queue() { clear(); }
    ~Queue() = default;
    void clear() { head = tail = count = 0; }
    bool isFull() { return count == N; }
    bool isEmpty() { return count == 0; }
    void push(const T &item);
    T pop();
};
/* Emission order is the reverse of definition order: push precedes pop. */
template <class T, i16 N> T Queue<T, N>::pop()
{
    T item;
    if (isEmpty()) {
        return item;
    }
    --count;
    item = items[tail++];
    if (tail == N) {
        tail = 0;
    }
    return item;
}
template <class T, i16 N> void Queue<T, N>::push(const T &item)
{
    if (isFull()) {
        return;
    }
    ++count;
    items[head++] = item;
    if (head == N) {
        head = 0;
    }
}
#endif
