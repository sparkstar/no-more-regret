/* Code30 (No Regret's 10e8:0000-09d1, No Remorse's Code28 10d8:0000-0788,
 * file name unknown, among the..\ITEM modules; the addresses are No
 * Regret's): the alarm and the monster eggs, the games' differences as each
 * game's values (alert_game.h). */
#include "alert.h"
#include "gamestate.h"
#include "alert_game.h"
#include "itemfind.h"
#include "scrit.h"
#include "item.h"
#include "npc.h"
#include "itemfind.h"
#include "scrit.h"
#include "dtable.h"
#include "shapes.h"

/* Not read (1480:16bc): a list of 1, 3, 4 and 14h, ended by FFFFh. */
u16 (&unknown16bc)[5] = gameGlobal<u16[5]>({1, 3, 4, 0x14, 0xffff});
u8 &alertOn = gameGlobal<u8>(0);

/* Turn the alarm on and off: the alarm lights and the doors of shapes 662h
 * and 663h change (10e8:0000, 0247, intrinsics 14Ch and 141h). */
void setAlertActive()
{
    if (ALERT_ONLY_ON_CHANGE && alertOn != 0U) {
        return;
    }
    alertOn = 1;
    AreaSearch search;
    search.init(0, 0, 0x7fff, 0x7fff,
                LoopScript(SCRIPT_SHAPE, SCRIPT_INT, SHAPE_ALARM_NS, SCRIPT_EQUAL, SCRIPT_SHAPE, SCRIPT_INT,
                           SHAPE_ALARM_EW, SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_SHAPE, SCRIPT_INT, SHAPE_MALRM_NS,
                           SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_SHAPE, SCRIPT_INT, SHAPE_MALRM_EW, SCRIPT_EQUAL, SCRIPT_OR,
                           SCRIPT_SHAPE, SCRIPT_INT, SHAPE_ALARM_BOX, SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_END),
                -1);
    while (search.isValid()) {
        const u16 shape = itemShape[search.id];
        if (shape == SHAPE_ALARM_BOX) {
            if (search.frame() < 2) {
                search.setFrame(search.frame() + 2);
            }
        } else if (ALERT_SWAPS_LIGHTS && shape == SHAPE_ALARM_LIGHT_A_OFF) {
            /* The script does not find these shapes (No Regret's). */
            search.setShape(SHAPE_ALARM_LIGHT_A_ON);
            search.setFrame(0);
        } else if (ALERT_SWAPS_LIGHTS && shape == SHAPE_ALARM_LIGHT_B_OFF) {
            search.setShape(SHAPE_ALARM_LIGHT_B_ON);
            search.setFrame(0);
        } else if (search.frame() == 0) {
            search.setFrame(1);
        }
        search.next();
    }
    if (ALERT_SWAPS_LIGHTS) {
        search.init(0, 0, 0x7fff, 0x7fff,
                    LoopScript(SCRIPT_SHAPE, SCRIPT_INT, SHAPE_ALARM_LIGHT_A_OFF, SCRIPT_EQUAL, SCRIPT_SHAPE,
                               SCRIPT_INT, SHAPE_ALARM_LIGHT_B_OFF, SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_END),
                    -1);
        while (search.isValid()) {
            const u16 shape = itemShape[search.id];
            if (shape == SHAPE_ALARM_LIGHT_A_OFF) {
                search.setShape(SHAPE_ALARM_LIGHT_A_ON);
                search.setFrame(0);
            } else if (shape == SHAPE_ALARM_LIGHT_B_OFF) {
                search.setShape(SHAPE_ALARM_LIGHT_B_ON);
                search.setFrame(0);
            }
            search.next();
        }
    }
}

void clearAlertActive()
{
    if (ALERT_ONLY_ON_CHANGE && alertOn == 0U) {
        return;
    }
    alertOn = 0;
    AreaSearch search;
    search.init(0, 0, 0x7fff, 0x7fff,
                LoopScript(SCRIPT_SHAPE, SCRIPT_INT, SHAPE_ALARM_NS, SCRIPT_EQUAL, SCRIPT_SHAPE, SCRIPT_INT,
                           SHAPE_ALARM_EW, SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_SHAPE, SCRIPT_INT, SHAPE_MALRM_NS,
                           SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_SHAPE, SCRIPT_INT, SHAPE_MALRM_EW, SCRIPT_EQUAL, SCRIPT_OR,
                           SCRIPT_SHAPE, SCRIPT_INT, SHAPE_ALARM_BOX, SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_END),
                -1);
    while (search.isValid()) {
        const u16 shape = itemShape[search.id];
        if (shape == SHAPE_ALARM_BOX) {
            if (search.frame() > 1) {
                search.setFrame(search.frame() - 2);
            }
        } else if (ALERT_SWAPS_LIGHTS && shape == SHAPE_ALARM_LIGHT_A_ON) {
            /* The script does not find these shapes (No Regret's). */
            search.setShape(SHAPE_ALARM_LIGHT_A_OFF);
            search.setFrame(0);
        } else if (ALERT_SWAPS_LIGHTS && shape == SHAPE_ALARM_LIGHT_B_ON) {
            search.setShape(SHAPE_ALARM_LIGHT_B_OFF);
            search.setFrame(0);
        } else if (search.frame() == 1) {
            search.setFrame(0);
        }
        search.next();
    }
    if (ALERT_SWAPS_LIGHTS) {
        search.init(0, 0, 0x7fff, 0x7fff,
                    LoopScript(SCRIPT_SHAPE, SCRIPT_INT, SHAPE_ALARM_LIGHT_A_ON, SCRIPT_EQUAL, SCRIPT_SHAPE, SCRIPT_INT,
                               SHAPE_ALARM_LIGHT_B_ON, SCRIPT_EQUAL, SCRIPT_OR, SCRIPT_END),
                    -1);
        while (search.isValid()) {
            const u16 shape = itemShape[search.id];
            if (shape == SHAPE_ALARM_LIGHT_A_ON) {
                search.setShape(SHAPE_ALARM_LIGHT_A_OFF);
                search.setFrame(0);
            } else if (shape == SHAPE_ALARM_LIGHT_B_ON) {
                search.setShape(SHAPE_ALARM_LIGHT_B_OFF);
                search.setFrame(0);
            }
            search.next();
        }
    }
}

/* 10e8:048d */
u8 isAlertActive()
{
    return alertOn;
}

void MonsterEgg::hatch()
{
    if (frame() == 1) {
        return;
    }
    setFrame(1);
    Npc npc(0);
    u16 shape;
    const u16 type = npcType();
    {
        const NpcType info = npcTypes[type];
        shape = info.shape;
    }
    const Item content(contents());
    const char direction = lowNpc();
    const u8 unknown12 = highNpc();
    u16 weapon1 = npcTypes[type].weaponType;
    u16 weapon2 = npcTypes[type].weaponType2;
    const i16 level = controlledNpc.difficulty;
    const u8 armed = ((settings() & 4) != 0) ? 1 : 0;
    const i16 difficulty = (settings() & 3) + 1;
    /* Left 0 for an empty egg, where the original has what its stack held; no
     * shape of the game is a monster egg (TYPEFLAG.DAT), so nothing hatches. */
    u16 activity0 = 0;
    u16 activity1 = 0;
    u16 activity2 = 0;
    u16 x;
    u16 y;
    u16 z;
    i16 facing;
    u16 tactic;

    if (difficulty > level) {
        return;
    }
    if (controlledNpc.difficulty == 4) {
        weapon1 = strongerWeaponType(shape);
        weapon2 = weapon1;
    }
    /* The activities are not set when the egg is empty. */
    if (content.isValid()) {
        activity0 = content.qualityHigh();
        activity1 = content.qualityLow();
        activity2 = content.map();
    }
    x = itemX[id];
    y = itemY[id];
    z = itemZ[id];
    facing = (direction + direction + 4) & 0xf;
    if (npc.create(shape, facing)) {
        tactic = 0;
        npc.setCombatTactic(tactic);
        npc.setDirection(direction * 2);
        npc.popToCoords(x, y, z);
        npc.setHome(ItemPoint(x, y, z));
        npc.setUnknown12(unknown12);
        npc.setDefaultActivity0(activity0);
        npc.setDefaultActivity1(activity1);
        npc.setDefaultActivity2(activity2);
        npc.setActivity(ACTIVITY_DEFAULT0);
        npc.orFlags(ITEM_DISPOSABLE);
        npc.setWaypointGroup(0);
        if ((armed != 0U) && (weapon2 != 0U)) {
            const i16 slot = npc.weaponSlot();
            if (slot != 0) {
                Item weapon(0);
                weapon.create(weaponTypes[weapon2].shape, 0);
                weapon.popToCoords(0, 0, 0);
                npc.setEquip(slot, weapon.id);
                npc.setActiveWeapon(slot);
                return;
            }
        } else if (weapon1 != 0U) {
            const i16 slot = npc.weaponSlot();
            if (slot != 0) {
                Item weapon(0);
                weapon.create(weaponTypes[weapon1].shape, 0);
                weapon.popToCoords(0, 0, 0);
                npc.setEquip(slot, weapon.id);
                npc.setActiveWeapon(slot);
                return;
            }
        }
    } else if (isAlertActive() != 0U) {
        /* The original turns the alarm on, which it already is, and off. */
        setAlertActive();
        clearAlertActive();
    }
}

u8 MonsterEgg::settings()
{
    return map() & 0xf;
}

void MonsterEgg::setSettings(u8 settings)
{
    setMap((map() & 0xf0) | settings);
}

i16 MonsterEgg::highMap()
{
    return (map() & 0xf0) >> 4;
}

void MonsterEgg::setHighMap(i16 value)
{
    const i16 newMap = (map() & 0xff0f) | (value << 4);

    setMap(newMap);
}

u16 MonsterEgg::npcType()
{
    const Item content(contents());

    if (content.isValid()) {
        return itemNpc[content.id];
    }
    return 0;
}

void MonsterEgg::setNpcType(u8 type)
{
    const Item content(contents());

    if (content.isValid()) {
        itemNpc[content.id] = type;
    }
}

i16 MonsterEgg::highNpc()
{
    return itemNpc[id] >> 4;
}

i16 MonsterEgg::lowNpc()
{
    return itemNpc[id] & 0xf;
}

void MonsterEgg::setHighNpc(u8 value)
{
    itemNpc[id] &= 0xf;
    itemNpc[id] |= value << 4;
}

void MonsterEgg::setLowNpc(u8 value)
{
    itemNpc[id] &= 0xf0;
    itemNpc[id] |= value & 0xf;
}

void MonsterEgg::unknown09b4() {}

void MonsterEgg::unknown09b9() {}

void MonsterEgg::unknown09be() {}

void MonsterEgg::unknown09c3() {}

void MonsterEgg::unknown09c8() {}

void MonsterEgg::unknown09cd() {}
