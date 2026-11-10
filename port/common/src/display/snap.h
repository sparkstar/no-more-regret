/* Code14 (1068, file name unknown): the process that keeps the camera on the
 * snap eggs. A snap egg's quality and position give an area; while the
 * controlled NPC is in it, the camera stays on the egg. */
#ifndef PORT_SNAP_H
#define PORT_SNAP_H
#include "common.h"
#include "kprocess.h"
#include "rect.h"

/* The shape of the snap eggs. */
constexpr int SNAP_EGG = 0x4fe;
constexpr int SNAP_EGGS = 64;

class SnapProcess : public Process {
public:
    /* UNCALLED: nothing calls it. */
    explicit SnapProcess(Loading loading) : Process(loading) {}
    /* The snap eggs of the map; 0 in a free slot. */
    u16 eggs[SNAP_EGGS];
    /* The slots in use. */
    i16 eggCount;
    /* The area of the current egg; empty (0,0,-1,-1) without one. */
    Rect area;
    /* The area of nextEgg. */
    Rect nextArea;
    /* The egg to take next. */
    u16 nextEgg;
    u16 currentEgg;
    /* The snap egg holds the camera: it does not follow the Silencer. */
    u8 holdsCamera;

    SnapProcess(); /* 1068:0000 */
    /* Forgets all eggs (1068:0083). */
    void clearEggs();
    /* The area of a snap egg (1068:0137). */
    Rect eggArea(u16 egg);
    void addEgg(u16 egg);    /* 1068:021b */
    void removeEgg(u16 egg); /* 1068:02da */
    /* The controlled NPC is in the current egg's area and within 48 of its
     * level (1068:037e). */
    bool npcInArea();
    /* Takes nextEgg, or the first egg whose area holds the controlled NPC,
     * and moves the camera there (1068:04c6). */
    void findEgg();
    /* Draws the snap eggs' areas, a cheat (1068:0814). */
    void showAreas();
    void run() override;            /* 1068:00d3 */
    void load(File *file) override; /* 1068:07f7 */
};
ASSERT_SIZE(SnapProcess, 0xc9);

extern SnapProcess *&snapProcess; /* 1480:104a */
#endif
