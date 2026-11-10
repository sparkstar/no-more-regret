/* Code51 (1190, file name unknown): a frame of an animation, 8 bytes of bit
 * fields (anim.dat): the shape frame, a sound and the moves the frame makes. */
#ifndef PORT_ANIMFRM_H
#define PORT_ANIMFRM_H
#include "common.h"
#include "animfrm_game.h"

class __attribute__((packed)) AnimFrame {
public:
    /* The frame of the NPC's shape; flipped mirrors it. */
    ui16 frame : 12;
    /* The NPC makes its sound (makeNpcSound). */
    ui16 npcSound : 1;
    ui16 unknown0d : 1;
    /* The frame calls the item's usecode (Item::calledFromAnim). */
    ui16 usecode : 1;
    ui16 flipped : 1;
    /* How far the frame moves the NPC up or down. */
    i16 deltaZ : 8;
    /* The low 8 bits of the sound (sound()); the high 2 are soundHigh, which
     * No Remorse's sounds have not. */
    ui16 soundLow : 8;
    /* How far the frame moves the NPC along its direction, in halves of a
     * unit. */
    i16 deltaDir : 9;
    /* The move keeps to the floor. */
    ui16 onGround : 1;
    /* How far the frame's attack reaches. */
    ui16 attackRange : 3;
    ui16 unknown2d : 1;
    /* The moves the frame makes (moves()). */
    i16 dx : 5;
    i16 dy : 5;
    ui16 dz : 4;
    ui16 unknown3c : 2;
    ui16 soundHigh : 2;

    /* UNCALLED: nothing calls it. */
    void clear();               /* 1190:0000 */
    void copy(AnimFrame *from); /* 1190:0013 */
    /* 1 when the frame moves the NPC (1190:002e). */
    [[nodiscard]] bool moves() const;
    /* The sound, of the game's bits (animfrm_game.h; No Regret's 10:
     * 1190:0063, 008e; No Remorse's 8, which reads soundLow itself). */
    [[nodiscard]] u16 sound() const;
    /* UNCALLED: nothing calls it. */
    void setSound(i16 sound);
};
ASSERT_FILE_SIZE(AnimFrame, 8);
#endif
