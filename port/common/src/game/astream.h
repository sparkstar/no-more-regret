/* ASTREAM.C (Code48, 1178:040f-0602): the animations as they are kept in
 * memory. A shape's ActorAnim holds an AnimAction for each animation number; an
 * AnimAction has a header (AnimInfo) and the frames of each direction.
 * ActorAnim and the animation cache's start and close are the module before it
 * (actanim.h). */
#ifndef PORT_ASTREAM_H
#define PORT_ASTREAM_H
#include "common.h"
#include "animfrm.h"

/* The header of an animation, 4 bytes of bit fields (anim.dat): the frames of
 * each direction, flags and the frame repeat. */
class AnimInfo {
public:
    ui16 frames : 8;
    /* A two-step animation plays one half at a time (NpcData::firstStep). */
    ui16 twoStep : 1;
    /* An attack: the weapon overlay shows, and the frames hit (ANIMPRIM.C
     * checkWeaponHit). */
    ui16 attack : 1;
    ui16 looping : 1;
    /* A move that is blocked does not stop the animation. */
    ui16 unstoppable : 1;
    ui16 unknown0c : 1;
    ui16 unknown0d : 1;
    /* The animation starts again when it ends (AnimPrimitive::loopAtEnd). */
    ui16 endLoop : 1;
    /* The NPC follows its combat target's height, and a blocked move does not
     * test the step; no animation of anim.dat sets it. */
    ui16 flying : 1;
    /* Ticks to wait after each frame. */
    ui16 repeat : 4;
    /* The moves go a quarter turn (4 of 16) off the direction. */
    ui16 rotated : 1;
    ui16 unknown15 : 5;
    ui16 unknown1a : 4;
    /* 16 directions, not 8. */
    ui16 sixteenDirections : 1;
    /* The NPC is destroyed when the animation ends. */
    ui16 destroyActor : 1;

    AnimInfo(); /* 1178:040f */
};
ASSERT_SIZE(AnimInfo, 4);

class AnimAction {
public:
    AnimFrame *frameList;
    AnimInfo info;

    AnimAction() { frameList = nullptr; }
    ~AnimAction() { release(); }

    /* Allocates the frames (1178:0444). */
    void allocate();
    /* The bytes of the frames (1178:04ad). */
    [[nodiscard]] u16 size() const;
    /* Sets info.frames and allocates (1178:04e5). */
    /* UNCALLED: nothing calls it. */
    void setFrames(u8 frames);
    /* The frame at index in direction (1178:0509), and copying one there
     * (0563). */
    [[nodiscard]] AnimFrame *frameAt(char direction, u8 index) const;
    /* UNCALLED: nothing calls it. */
    void setFrame(char direction, u8 index, AnimFrame *frame) const;
    /* Frees the frames (1178:05d5). */
    void release();
};
ASSERT_SIZE(AnimAction, 8);
#endif
