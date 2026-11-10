/* 1178:03ba-040e, file name unknown: a shape's animations (ActorAnim) and the
 * start and close of the animation cache. A module of its own before
 * ASTREAM.C: the order of the segment's NE relocations shows it. */
#ifndef PORT_ACTANIM_H
#define PORT_ACTANIM_H
#include "common.h"
#include "astream.h"

/* The animations of a shape, by animation number: in memory the actions, in
 * anim.dat their offsets in the file. The file holds the offsets alone, 400h
 * bytes, which AnimFile reads and writes: the port's pointers are wider. */
class ActorAnim {
public:
    union {
        AnimAction *actions[0x100];
        s32 offsets[0x100];
    };

    ActorAnim() { clear(); }
    void setAction(i16 index, AnimAction *action) { actions[index] = action; }
    /* UNCALLED: nothing calls it. */
    void setOffset(i16 index, s32 offset) { offsets[index] = offset; }
    void clear(); /* 1178:03ba */
    /* 1 when it has an animation (1178:03cd). */
    /* UNCALLED: nothing calls it. */
    bool any();
};
ASSERT_SIZE(ActorAnim, 0x400);

/* Make the animation cache and free it (1178:0403, 0409). */
void initAnims();
void closeAnims();
#endif
