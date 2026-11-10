/* Code30 (10e8, file name unknown, among the ..\ITEM modules): the monster
 * eggs, which hatch NPCs, and the alarm. */
#ifndef PORT_ALERT_H
#define PORT_ALERT_H
#include "common.h"
#include "item.h"

/* An egg that hatches an NPC. Its map number holds the settings (bit 2:
 * armed, bits 0-1: the lowest difficulty less 1) and its NPC number the
 * NPC's direction (low nibble) and a value for Npc::setUnknown12 (high
 * nibble). The item in it gives the NPC's default activities. */
class MonsterEgg : public Item {
public:
    explicit MonsterEgg(u16 itemId) : Item(itemId) {}
    /* Makes the egg's NPC, once (10e8:0491). */
    void hatch();
    /* The low and high nibbles of the map number (10e8:0818-086a). */
    u8 settings();
    void setSettings(u8 settings);
    i16 highMap();
    void setHighMap(i16 value);
    /* The NPC type: the NPC number of the item in the egg (10e8:0899,
     * 08d4). */
    u16 npcType();
    void setNpcType(u8 type);
    /* The high and low nibbles of the NPC number (10e8:090c-097f). */
    i16 highNpc();
    i16 lowNpc();
    void setHighNpc(u8 value);
    void setLowNpc(u8 value);
    /* Empty; nothing calls them (10e8:09b4-09cd). */
    void unknown09b4();
    void unknown09b9();
    void unknown09be();
    void unknown09c3();
    void unknown09c8();
    void unknown09cd();
};
ASSERT_SIZE(MonsterEgg, 2);

/* The alarm is on (1480:16c6). */
extern u8 &alertOn;

void setAlertActive();
void clearAlertActive();
u8 isAlertActive();
#endif
