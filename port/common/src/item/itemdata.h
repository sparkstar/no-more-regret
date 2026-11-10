/* ..\\ITEM\\ITEMDATA.C (Code25, 10c0): the item tables, indexed by item
 * number, and the item records of the map files. */
#ifndef PORT_ITEMDATA_H
#define PORT_ITEMDATA_H
#include "common.h"
#include "kernel.h"

/* A place on the map: z is the height, 0-FAh. */
struct __attribute__((packed)) ItemPoint {
    u16 x;
    u16 y;
    u8 z;

    ItemPoint() = default;
    ItemPoint(u16 newX, u16 newY, u8 newZ)
    {
        this->x = newX;
        this->y = newY;
        this->z = newZ;
    }
    void set(u16 newX, u16 newY, u8 newZ)
    {
        this->x = newX;
        this->y = newY;
        this->z = newZ;
    }
    /* Nothing calls it (Code20 1098:0784). */
    bool operator==(const ItemPoint &other) const;
};
ASSERT_FILE_SIZE(ItemPoint, 5);

/* An item as the map files hold it (fixed.dat, nonfixed.dat). It has no
 * constructor of its own: the copy for new[] (Code7 1030:0c72) is the
 * implicit one, which does not allocate. */
struct __attribute__((packed)) ItemRecord : public ItemPoint {
    u16 shape;
    u8 frame;
    u16 flags;
    u16 quality;
    u8 npc;
    u8 map;
    u16 next;
};
ASSERT_FILE_SIZE(ItemRecord, 0x10);

/* Where each item is on the map (1480:8957, 895b, 895f). */
extern u16 *&itemX;
extern u16 *&itemY;
extern u8 *&itemZ;
extern u16 *&itemShape;
extern u8 *&itemFrame;
/* Item::flags (ITEM_*). */
extern u16 *&itemFlags;
extern u16 *&itemQuality;
extern u8 *&itemNpc;
extern u8 *&itemMap;
/* The next item in the same map chunk or container. */
extern u16 *&itemNext;
/* The items a search found, ending with 0 (fastfind.asm). */
extern u16 *&foundItems;
/* What damage the item takes before it is destroyed (ITEMDAM.C). */
extern u8 *&itemDamagePoints;

void allocateItems(); /* 10c0:0000 */
void freeItems();     /* 10c0:01c7 */
/* Copies the item into record (10c0:0330). */
void readItem(u16 item, ItemRecord *record);
/* Sets the item from record, with the damage points of its shape
 * (10c0:03ef). */
void writeItem(u16 item, ItemRecord *record);
/* Sets the item's place, shape, frame and NPC from record, and clears its
 * flags and quality (10c0:04df). */
void resetItem(u16 item, ItemRecord *record);
/* A new item from record (10c0:0574). */
u16 createItem(ItemRecord *record);
#endif
