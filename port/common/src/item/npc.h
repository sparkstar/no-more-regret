/* ..\\ITEM\\NPC.C (Code32, No Regret's 10f8, No Remorse's 10e8; both games',
 * the addresses No Regret's): the NPCs, items 1-255 with a record of
 * NPC_DATA_SIZE bytes each, their activities and animations, and the controlled
 * NPC. Most members are usecode intrinsics. */
#ifndef PORT_NPC_H
#define PORT_NPC_H
#include <array>
#include "common.h"
#include "item.h"

class Flex;

/* The number of NPCs, item 0 included. */
constexpr int NPCS = 0x100;
/* A range of equipment slots, first to last. */
struct SlotRange {
    i16 first;
    i16 last;
};
/* The game's values: EQUIPMENT_SLOTS (NpcData's equipment), NPC_DATA_SIZE,
 * the slots' ranges, BASE_MAP and the shapes' lists. */
#include "npc_game.h"

/* The activities (Npc::setActivity). 70h-72h stand for the NPC's default
 * activities. */
constexpr int ACTIVITY_STAND = 1;
constexpr int ACTIVITY_LOITER = 2;
constexpr int ACTIVITY_PACE = 3;
constexpr int ACTIVITY_ATTACK = 5;
constexpr int ACTIVITY_SURRENDER = 7;
constexpr int ACTIVITY_GUARD = 8;
constexpr int ACTIVITY_KNEEL_ATTACK = 9;
/* On difficulty 4 half of the attacks and kneeling attacks become these: the
 * idle NPC acts more often (Attack::aggressive). */
constexpr int ACTIVITY_AGGRESSIVE_ATTACK = 0xa;
constexpr int ACTIVITY_AGGRESSIVE_KNEEL_ATTACK = 0xb;
/* An attack that starts with a feint (Attack::startFeint). An NPC in combat
 * changes its activity only to or from this one. */
constexpr int ACTIVITY_FEINT_ATTACK = 0xc;
constexpr int ACTIVITY_ROLLING_THUNDER = 0xd;
constexpr int ACTIVITY_DEFAULT0 = 0x70;
constexpr int ACTIVITY_DEFAULT1 = 0x71;
constexpr int ACTIVITY_DEFAULT2 = 0x72;
/* Npc::doAnim's maxBusy: start the animation only when the NPC is not
 * busy, or whatever animations it runs. */
constexpr int ONLY_IF_IDLE = 0;
constexpr int EVEN_IF_BUSY = 10000;

/* An NPC's record (1480:8988). */
struct __attribute__((packed)) NpcData {
    u16 hp;
    u16 activity;
    /* The activity before the current one. */
    u16 lastActivity;
    u16 defaultActivities[3];
    u8 direction;
    /* The animation last started (Npc::doAnim). */
    u16 lastAnimSet;
    /* The high byte of the NPC's frame (Item::frame). */
    u8 frameHigh;
    /* The frame the animation is at: Npc::busyAnim compares it with the
     * animation's frames, and it picks the weapon overlay's frame. */
    u8 animFrame;
    /* The NPC's height when it began to fall (GravityProcess). */
    u8 fallStart;
    u8 unknown12;
    ui16 dead : 1;
    ui16 immortal : 1;
    ui16 kneeling : 1;
    ui16 unknown13 : 5;
    u8 unknown14;
    /* The equipment slots of the weapon in hand and two others. */
    u16 activeSlots[3];
    u16 equipment[EQUIPMENT_SLOTS_CAPACITY];
    u16 mana;
    /* The record belongs to an NPC. */
    ui16 used : 1;
    ui16 falling : 1;
    /* A two-step animation (AnimInfo::twoStep) has done its first half. */
    ui16 firstStep : 1;
    ui16 inCombat : 1;
    ui16 unknown77_4 : 1;
    ui16 unknown77_5 : 1;
    /* Npc::doAnim starts nothing. */
    ui16 stopped : 1;
    ui16 unknown77_7 : 1;
    /* Npc::setActivity changes nothing. */
    ui16 activityLocked : 1;
    /* The weapon is drawn. */
    ui16 weaponDrawn : 1;
    /* A woman (Npc::create sets it for shape 597h): other sounds. */
    ui16 female : 1;
    /* Made armed by its egg (Item::createNpc). */
    ui16 armed : 1;
    /* The NPC was hurt (Npc::playHurtSound). */
    ui16 hurt : 1;
    ui16 unknown78_5 : 3;
    u8 unknown79;
    u16 combatTactic;
    ItemPoint home;
    /* The quality of the waypoints (shape 33Ah) the NPC walks between, 0 for
     * any: the egg's low quality (combat.dat command A6h). */
    u8 waypointGroup;
    /* The NPC it attacks (Npc::setAttackTarget). */
    u16 attackTarget;
    /* The Silencer's shield, 0-3: none, SHAPE_IONIC_SHIELD, SHAPE_PLASMA_SHIELD
     * or SHAPE_GRAVITON_SHIELD. */
    u16 shieldType;
    /* The NPC fires a long burst (four times burstSize) whose shots spread half
     * as much. */
    u8 aimedBurst;
    /* When the NPC was last hit (Npc::recentlyHit). */
    u32 hitTime;
    /* The last hurt sound it made. */
    i16 hurtSound;
    u8 unknown8d[0xa];
};
/* The bytes besides the equipment's: the program's record holds the most
 * slots of the two games', and saveNpcs and loadNpcs change it to the game's
 * NPC_DATA_SIZE. */
constexpr int NPC_DATA_OTHER_BYTES = 0x3d;
ASSERT_FILE_SIZE(NpcData, NPC_DATA_OTHER_BYTES + (2 * EQUIPMENT_SLOTS_CAPACITY));

extern NpcData *&npcs; /* 1480:8988 */

/* A process id in a structure, returned in AX (Npc::setInCombat). */
struct ProcessId {
    u16 pid;
};
ASSERT_SIZE(ProcessId, 2);

class Npc : public Item {
public:
    explicit Npc(u16 itemId) { this->id = itemId; }
    /* An item number becomes an Item temporary, not an Npc (DLITEM.C's
     * weapon). */
    Npc &operator=(Item item)
    {
        id = item.id;
        return *this;
    }
    u8 frameHigh() { return npcs[id].frameHigh; }
    void setFrameHigh(u8 high) { npcs[id].frameHigh = high; }
    u8 isActivityLocked() { return npcs[id].activityLocked; }
    u8 isWeaponDrawn() { return npcs[id].weaponDrawn; }
    u8 isHurt() { return npcs[id].hurt; }
    u8 isFemale() { return npcs[id].female; }
    void setHurtSound(i16 sound) { npcs[id].hurtSound = sound; }
    /* As Process::addFlags: the time goes in through a local. */
    void setHitTime(u32 time)
    {
        const u32 value = time;
        npcs[id].hitTime = value;
    }
    u8 isFirstStep() { return npcs[id].firstStep; }
    void setLastAnim(u16 anim) { npcs[id].lastAnimSet = anim; }
    void setAnimFrame(u8 frame) { npcs[id].animFrame = frame; }
    /* The animation the NPC is busy with, when it is one that ends by
     * itself in time; 40h otherwise (10f8:011a). */
    u16 busyAnim();
    /* An animation of the NPC runs (10f8:0261). */
    bool isBusy();
    /* On the base's map, BASE_MAP (10f8:0277). */
    bool onBaseMap();
    /* Gives the NPC the weapon and the default activities of npc.dat's
     * type (10f8:029a). */
    void applyType(i16 type);
    /* Starts animation anim in direction; with maxBusy ONLY_IF_IDLE only
     * when no other animation runs, else when at most maxBusy do
     * (EVEN_IF_BUSY: whatever runs); kill ends the others first. The new
     * process's pid, or 0 (10f8:039a). */
    u16 doAnim(u16 anim, u16 direction, u16 maxBusy, u8 kill);
    /* The NPC attacks value: an NPC out of combat stops what it does and
     * starts ACTIVITY_ATTACK. Not the dead, the Silencer or the controlled
     * NPC (10f8:2107). */
    void setAttackTarget(u16 value);
    u8 isFalling();  /* 10f8:2198 */
    u8 isInCombat(); /* 10f8:21b7 */
    /* The last animation is not standing, walking, a quick jump or putting
     * a weapon away (10f8:21d7). */
    bool isActing();
    bool isSliding(); /* 10f8:220b */
    /* The NPC was hit in the last 78h ticks (10f8:2231). */
    bool recentlyHit();
    u8 isDead();      /* 10f8:2278 */
    void setDead();   /* 10f8:22a8 */
    void clearDead(); /* 10f8:22cf */
    /* Starts a PathFinder to x,y,z or to item; its pid (10f8:22f6, 234e). */
    /* UNCALLED: nothing calls it. */
    u16 pathfind(u16 x, u16 y, u8 z, u16 steps);
    /* UNCALLED: nothing calls it. */
    u16 pathfind(u16 item, u16 steps);
    u8 isImmortal();      /* 10f8:23e4 */
    void setImmortal();   /* 10f8:2403 */
    void clearImmortal(); /* 10f8:241e */
    bool isRunning();     /* 10f8:2439 */
    bool isStartingRun(); /* 10f8:246e */
    u8 isKneeling();      /* 10f8:2499 */
    /* The Silencer kneels and stands: his shape gets lower and higher
     * (10f8:24b9, 250b). */
    void kneel();
    void standUp();
    u8 unknown77Bit5();        /* 10f8:255d */
    void setUnknown77Bit5();   /* 10f8:257d */
    void clearUnknown77Bit5(); /* 10f8:2598 */
    u8 isStopped();            /* 10f8:25b3 */
    void setStopped();         /* 10f8:25d3 */
    void clearStopped();       /* 10f8:25ee */
    /* A new NPC item of shape and frame into id; without a free number it
     * takes an NPC outside the fast area (10f8:2609). */
    bool allocate(u16 shape, u16 frame);
    /* Makes a new NPC of shape and frame into id (10f8:27b9). */
    bool create(u16 shape, i16 frame);
    /* create with the type of the shape (10f8:280e). */
    bool createActor(u16 shape, u16 frame);
    void playHurtSound(); /* 10f8:2865 */
    /* Clears the record but the frame and sets the type's hp (10f8:2cce). */
    void reset();
    /* createActor at point when the shape fits there (10f8:2d44, 2dd6). */
    /* UNCALLED: nothing calls it. */
    bool createAtPoint(u16 shape, u16 frame, ItemPoint *point);
    /* UNCALLED: nothing calls it. */
    bool createAtCoords(u16 shape, u16 frame, u16 x, u16 y, u8 z);
    /* The item in the NPC's equipment slot, 0 for none (10f8:2e86). */
    u16 equipment(i16 slot);
    /* Puts item in the NPC's equipment slot (10f8:2eb9). */
    bool setEquip(u16 slot, u16 item);
    void clearSlot(i16 slot); /* 10f8:2fce */
    /* Takes item out of the NPC's slots (10f8:2ff1). */
    void removeEquip(u16 item);
    /* The slot of item, EQUIPMENT_SLOTS for none (10f8:3039). */
    i16 slotOf(u16 item);
    /* Empties every slot (10f8:307e). */
    void clearEquip();
    /* Puts the NPC at x,y,z of map (10f8:30b8). */
    void teleport(u16 x, u16 y, u8 z, u8 map);
    /* A free slot among 1-12, 13-20 and 21-44, 0 for none; noSlot is always
     * 0 (10f8:31c1-321a). */
    i16 weaponSlot();
    i16 utilitySlot();
    i16 noSlot();
    i16 packSlot();
    /* Usecode event 3 with activity; setActivity when there is none
     * (10f8:325f). */
    /* UNCALLED: nothing calls it. */
    u16 activityEvent(u32 activity);
    void setDefaultActivity0(u16 activity); /* 10f8:329d */
    void setDefaultActivity1(u16 activity); /* 10f8:32ba */
    void setDefaultActivity2(u16 activity); /* 10f8:32d7 */
    void setUnknown12(u8 value);            /* 10f8:32f4 */
    u8 unknown12();                         /* 10f8:3311 */
    u16 defaultActivity0();                 /* 10f8:332b */
    u16 defaultActivity1();                 /* 10f8:3345 */
    u16 defaultActivity2();                 /* 10f8:335f */
    void setCurrentActivity(u16 activity);  /* 10f8:3379 */
    /* Starts the activity's process (10f8:33b5). */
    void setActivity(u16 activity);
    bool isActivity(u16 activity); /* 10f8:3725 */
    /* Usecode event 8, schedule (10f8:378a). */
    /* UNCALLED: nothing calls it. */
    u16 schedule(u32 time);
    /* The damage of the weapon in hand, or of the second active one
     * (10f8:37fc). */
    i16 weaponDamage(u8 second);
    ProcessId setInCombat(); /* 10f8:3879 */
    /* Leaves combat: puts the weapon away and ends the combat process
     * (10f8:389f). */
    void clrInCombat();
    /* The original's is void, but leaves turnNpc's process in AX, and the
     * usecode that calls it waits for that process. */
    u16 turnToward(u8 direction, u8 speed); /* 10f8:3961 */
    /* setInCombat and clrInCombat that the player asked for
     * (10f8:397e, 399e). */
    void enterCombat();
    void leaveCombat();
    u16 hp();               /* 10f8:39ea */
    u16 activity();         /* 10f8:3a03 */
    u16 activeWeaponSlot(); /* 10f8:3a1d */
    u16 activeSlot2();      /* 10f8:3a37 */
    u16 activeSlot3();      /* 10f8:3a51 */
    u16 maxHp();            /* 10f8:3a6b */
    /* The items in the active slots (10f8:3aae, 3ae2, 3b16). */
    u16 activeWeapon();
    u16 activeItem2();
    u16 activeItem3();
    u16 mana();             /* 10f8:3b4a */
    void setMana(u16 mana); /* 10f8:3b64 */
    /* The Silencer's energy with his battery (10f8:3b81). */
    u16 maxEnergy();
    void recharge(u8 unknown); /* 10f8:3bc0 */
    /* WORLD.C compares the shield signed (10f8:3be9, 3c04). */
    i16 shieldType();
    void setShieldType(u16 type);
    u16 batteryType();                /* 10f8:3c2c */
    void setBatteryType(u16 type);    /* 10f8:3c34 */
    void setHp(u16 hp);               /* 10f8:3c56 */
    u16 combatTactic();               /* 10f8:3c72 */
    void setCombatTactic(u16 tactic); /* 10f8:3c8c */
    void setHome(ItemPoint home);     /* 10f8:3ca9 */
    ItemPoint home();                 /* 10f8:3cd9 */
    void setActiveWeapon(u16 slot);   /* 10f8:3d2d */
    void setActiveSlot2(u16 slot);    /* 10f8:3d54 */
    void setActiveSlot3(u16 slot);    /* 10f8:3d71 */
    void setDirection(u8 direction);  /* 10f8:3def */
    char direction();                 /* 10f8:3e0c */
    u16 lastAnimSet();                /* 10f8:3e26 */
    /* Adds to hp and mana up to the most; 0 when it was full
     * (10f8:3e40, 3e92). */
    bool addHp(i16 delta);
    bool addMana(i16 delta);
    /* Constants (10f8:4040, 4047, 404f). */
    bool unknown4040();
    i16 unknown4047();
    bool unknown404f();
    /* The first item of shape in the Silencer's slots, whatever the NPC
     * (10f8:4056). */
    u16 findItem(u16 shape);
    /* The ammunition for the controlled NPC's weapon (10f8:40b9). */
    u16 findAmmo();
    /* The rounds or shots left in the weapon in hand (10f8:4117). */
    u16 ammo();
    void useAmmo(); /* 10f8:41aa */
    /* 1 when the weapon in hand is small (smallWeapon) or there is none: the
     * NPC takes the _SMALL animations, else the _LARGE ones (10f8:424f). */
    bool usesSmallAnims();
    /* Reloads the weapon in hand from the ammunition (10f8:429b). */
    void reload();
    /* The credits of the controlled NPC's credit card (10f8:446c, 44b7). */
    s32 credits();
    void setCredits(u32 credits);
    /* NpcData::waypointGroup (10f8:44f4, 4511). */
    u16 waypointGroup();
    void setWaypointGroup(u16 value);
    u16 lastActivity(); /* 10f8:452f */
};
ASSERT_SIZE(Npc, 2);

/* The controlled NPC, the Silencer at first, and the game's settings
 * (1480:898c). Packed with the pragma: __attribute__((packed)) leaves the
 * base's alignment of 2, which would round the saved 23 bytes up to 24. */
#pragma pack(push, 1)
class ControlledNpc : public Npc {
public:
    /* Set while a movie or gump holds the NPC still. */
    u16 held;
    /* The random seed, kept with the saved game. */
    u32 randomSeed;
    u8 unknown08;
    u8 unknown09[2];
    /* The game's difficulty, 1-4. */
    i16 difficulty;
    /* One bit a keycard. */
    u32 keycards;
    /* The Silencer's battery, 0-3 (Npc::maxEnergy). */
    u16 battery;
    u8 unknown13[4];

    ControlledNpc(); /* 10f8:3ef2 */
    /* The settings of a new game (10f8:3f54). */
    void reset();
    bool setUnknown08(u8 value); /* 10f8:3fbc */
    /* Reads and writes the object in flex's entry index; the last two
     * bytes are left out (10f8:3fd5, 4005). */
    void load(Flex *flex, i16 index);
    i16 save(Flex *flex, i16 index);
    /* save's object with the random seed of now, which it leaves unset here
     * (the port's, for the pass dump). */
    void writeState(Flex *flex, i16 index) const;
};
#pragma pack(pop)
ASSERT_FILE_SIZE(ControlledNpc, 0x17);

/* Frees npc's record, when the NPC is destroyed (10f8:23c9). */
void freeNpcNumber(Item *npc);

/* MusicProcess declared it far. */
extern ControlledNpc &controlledNpc;

/* The NPC table (10f8:0000-00f3). */
void allocateNpcs();
void freeNpcs();
void saveNpcs(Flex *flex, i16 index);
void loadNpcs(Flex *flex, i16 index);
/* The map of the NPC npc points to (10f8:3243). Its callers pass an NPC
 * number's address. */
u16 npcMap(u16 *npc);
/* The controlled NPC is held still (10f8:3d96). */
void setHeld(u16 held);
/* Hold the controlled NPC still, and let it go (10f8:3dd3, 3de1). */
/* UNCALLED: nothing calls it. */
void holdControlledNpc();
void releaseControlledNpc();
/* WORLD.C passes an int (pickUp pushes the word it compared). */
void setKeycard(i16 card);

u16 isHeld();
i16 gameDifficulty();
bool hasKeycard(u8 card);

/* Nothing calls these, in the original either. */
void initNpcs();
void closeNpcs();
void clearNpcs();
u16 newNpcNumber();
void controlledEnterCombat();
/* UNCALLED: nothing calls it. */
void controlledLeaveCombat();
u16 controlledInCombat();
s32 controlledCredits();
void setControlledCredits(u16 credits);
/* Takes all the keycards away (npc.cpp; the usecode's intrinsic). */
void clearKeycards();
#endif
