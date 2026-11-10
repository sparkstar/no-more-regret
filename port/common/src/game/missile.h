/* Code11 (1050:00a6-, file name unknown): thrown and shot items. The tracker
 * works out the speeds that carry an item from one point to another under
 * gravity. */
#ifndef PORT_MISSILE_H
#define PORT_MISSILE_H
#include "common.h"
#include "itemdata.h"

class MissileTracker {
public:
    i16 gravity;
    /* How far the item goes, and how far it could go. */
    i16 range;
    i16 maxRange;
    i16 speedX;
    i16 speedY;
    i16 speedZ;
    /* The item gets there. */
    u8 reaches;

    MissileTracker(ItemPoint from, ItemPoint to, i16 speed, i16 missileGravity); /* 1050:00a6 */
    /* Throws item with the speeds (1050:0290). */
    void launch(u16 item) const;
    /* Starts item's fall with the speeds and runs it once; 1 when that puts
     * item within 50 of target (1050:02c0). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] bool test(u16 item, ItemPoint target) const;
};
ASSERT_SIZE(MissileTracker, 13);

#endif
