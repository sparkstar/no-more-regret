/* Code47 (file name unknown): No Regret's Bobo boomer, the process of the
 * explosions that follow a death, and the NPC helpers the animations call:
 * firing the weapon at a frame and the sounds of the NPCs. */
#ifndef PORT_BOBOBOOM_H
#define PORT_BOBOBOOM_H
#include "common.h"
#include "kprocess.h"
#include "animfrm.h"
#include "itemdata.h"

/* Process::type */
constexpr int BOBO_BOOMER_TYPE = 0x264;

/* Ten explosions around point, 5-19 ticks apart (No Regret only). */
class BoboBoomer : public Process {
public:
    explicit BoboBoomer(Loading loading) : Process(loading) {}
    i16 count;
    ItemPoint point;

    explicit BoboBoomer(ItemPoint *blastPoint);
    void run() override;
};
ASSERT_SIZE(BoboBoomer, 0x39);

/* npc fires its weapon at the offset of frame. */
void fireAtFrame(u16 npc, AnimFrame frame);
/* npc makes the sound of its shape. */
void makeNpcSound(u16 npc);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
u16 unknown01a7(u16 shape, u16 frame, u16 container);
void unknown0855(u16 /*unused*/);
void unknown085a(u16 /*unused*/);
#endif
