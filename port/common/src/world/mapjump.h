/* Code7 (file name unknown): moving the game to another map
 * (MapJumpProcess), starting a new game, and the controlled NPC's items
 * carried across. */
#ifndef PORT_MAPJUMP_H
#define PORT_MAPJUMP_H
#include "common.h"
#include "kprocess.h"

class MapJumpProcess : public Process {
public:
    /* UNCALLED: nothing calls it. */
    explicit MapJumpProcess(Loading loading) : Process(loading) {}
    i16 map;

    explicit MapJumpProcess(i16 targetMap);
    /* Clears the map, keeps the controlled NPC's items, loads map and
     * moves the NPC to its egg 30. */
    void run() override;
};
ASSERT_SIZE(MapJumpProcess, 0x34);

/* Moves the controlled NPC, or the camera, to teleport egg egg of map; back
 * where it was when there is none (No Regret only). */
void moveToEgg(u16 map, i16 egg, u8 unused);
/* Shows the loading picture (No Regret only). */
void showLoading();
/* The main menu and a new, loaded or warped game. */
void startGame();
/* Empty. */
void unknown07d2();
/* Starts the jump to map. */
void startMapJump(u16 map);
/* Keeps the controlled NPC, its equipment and its items of shape 476h
 * (1030:07d3), and gives them back on the new map (09fe). */
void keepCarried();
void restoreCarried();
#endif
