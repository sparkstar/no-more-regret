/* File name unknown: clearing a shape's ActorAnim. See actanim.h. WPNCACHE.C
 * ends before it with its static initializers, and the order of the
 * segment's NE relocations shows a module of its own before ASTREAM.C.
 * No Regret's module also starts and closes the
 * animation cache, which the port does in animinit.cpp, No Remorse's module
 * of those two. */
#include "actanim.h"

void ActorAnim::clear()
{
    memset(static_cast<void *>(this), 0, sizeof(*this));
}

/* UNCALLED: nothing calls it. */
bool ActorAnim::any()
{
    for (i16 i = 0; i < 0x100; i++) {
        if (actions[i] != nullptr) {
            return true;
        }
    }
    return false;
}
