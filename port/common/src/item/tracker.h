/* TRACKER.C (Code40, 1138): the items that can be targeted, 100 at most. */
#ifndef PORT_TRACKER_H
#define PORT_TRACKER_H
#include "common.h"
#include "itemdata.h"

class Flex;

/* The list (1480:2d66), its entries (2d6a) and the items in it (2d6c). */
extern u16 *&targets;

/* Makes the list, 100 entries (1138:0000), and frees it (1138:0066). */
void initTracker();
void closeTracker();
void addTarget(u16 item);    /* 1138:0098 */
void removeTarget(u16 item); /* 1138:00fe */
/* The item npc aims at in direction, one of 16 when sixteen is set, else of
 * 8 (1138:0158). */
u16 findTarget(u16 npc, char direction, u8 sixteen);
/* Hurts the items within radius cells of point with damage of type, except
 * except and attacker (unless attacker is the controlled NPC); types 3, 4 and
 * 0Ah weaken with the distance (1138:039c). */
void hurtArea(ItemPoint *point, i16 radius, i16 damage, i16 type, u16 except, u16 attacker);
/* The list in a saved game (1138:0542, 05b7). */
void loadTargets(Flex *flex, u16 index);
void saveTargets(Flex *flex, u16 index);
#endif
