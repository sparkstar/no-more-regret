/* ATTACK.C (Code37; No Regret's 1120, No Remorse's 1108; the addresses are
 * No Regret's): an NPC's attack, the activity Npc::setActivity starts for the
 * combat activities. It reads the tactics of static\combat.dat. The members of
 * one game only say so; the BCC size is each game's (attack_game.h). */
#ifndef PORT_ATTACK_H
#define PORT_ATTACK_H
#include "common.h"
#include "activity.h"
#include "attack_game.h"

class AnimInfo;
class Item;
class Npc;

/* Process::type */
constexpr int ATTACK_TYPE = 0x259;

/* An entry of static\combat.dat: a tactic, a script of byte commands and
 * their word arguments. */
struct CombatTactic {
    char name[16];
    /* The script's start for each block, from the entry's start. */
    u16 blocks[4];
};
ASSERT_SIZE(CombatTactic, 0x18);

/* Script arguments at or past DATA_BASE name a data word (Attack::data). The
 * constant is decimal and does not fit an int, so it is a long. */
constexpr int DATA_BASE = 33000;

class Attack : public Activity {
public:
    explicit Attack(Loading loading) : Activity(loading) {}
    u8 unknown48;
    /* The NPC's direction when the attack began. */
    i16 homeDirection;
    CombatTactic *tactic;
    /* The tactic's entry as bytes, and where its script is read. */
    u8 *script;
    u16 position;
    i16 block;
    /* The tactic, 0 for the generic attack. */
    i16 tacticNumber;
    u16 unknown59;
    /* Where the block's script starts. */
    u16 blockStart;
    /* A script loop: the times left and where it starts. */
    i16 loopCount;
    u16 loopStart;
    u16 data[10];
    /* When the NPC last got ready to fire (SystemTimer::ticks). */
    u32 readyTime;
    /* When sightTarget found a line of fire (1120:2473). */
    u32 sightedTime;
    /* No Regret's 4 bytes here, which nothing reads; No Remorse's feintEnd. */
    u8 unknown7d[4];
    /* When the feint ends (No Remorse's startFeint): until then the NPC
     * stands, turns, readies and unreadies its weapon and fires at random. */
    u32 feintEnd;
    /* ACTIVITY_KNEEL_ATTACK or ACTIVITY_AGGRESSIVE_KNEEL_ATTACK. */
    u8 kneeling;
    /* ACTIVITY_AGGRESSIVE_*: the idle NPC acts at 4 rolls in 10, not 3. */
    u8 aggressive;
    /* The waits are twice as long. */
    u8 doubleDelay;
    /* The NPC found a line of fire to its target; it goes after 60 ticks
     * (sightExpired) or at random. */
    u8 targetSighted;
    /* The path finder stopped the NPC's walk where it can hit its target. */
    u8 stoppedToFire;
    /* ACTIVITY_FEINT_ATTACK, or a hit on some shapes (CRUSADER.C): No
     * Regret's NPC walks and retreats, or readies and unreadies its weapon,
     * once (feint), then attacks as ACTIVITY_AGGRESSIVE_ATTACK (feinted, No
     * Regret's); No Remorse's feints until feintEnd, then attacks as
     * ACTIVITY_ATTACK. */
    u8 feint;
    u8 feinted;
    /* The shots left in a burst (event). */
    i16 shots;
    /* No Remorse's speech while attacking (readyToSpeak): the NPC said its
     * first line when it got ready to fire, when it last spoke, and the line
     * it speaks, -1 when none (it fires when the line is over). */
    u8 greeted;
    u32 lastSpoke;
    i16 speech;
    /* readyTime as stillReady last saw it, and the ticks the NPC waits after it
     * before the first shot: 3Ch with the UV-9, GL-303 and AR-7, else 1Eh. */
    u32 lastReady;
    i16 readyDelay;
    /* When the NPC last fired, and the ticks it waits between shots by the
     * difficulty. */
    u32 lastFire;
    i16 fireDelay;
    /* The NPC walked (chase, or wandering without a target): it acts at
     * once when it arrives. */
    u8 justMoved;
    /* When the NPC last looked for another target (No Regret's searchDue). */
    u32 lastSearch;
    /* The NPC was guarding (activity 8). */
    u8 fromGuard;

    explicit Attack(u16 ownerNpc); /* 1120:0000 */
    ~Attack() override;
    /* No Remorse's: 1 at most every 480 ticks, the NPC may speak again
     * (1108:0334). */
    bool readyToSpeak();
    /* 1 while the NPC stays ready to fire after readyTime (1120:0321). */
    bool stillReady();
    /* The next script word, or the data word it names (1120:03b2). */
    i16 nextValue();
    /* The next script word as it is (1120:03f3). */
    u16 nextWord();
    void setData(u16 at, u16 value); /* 1120:0417 */
    i16 getData(u16 at);             /* 1120:044b */
    /* Starts block of the tactic's script (1120:047f). */
    void setBlock(i16 newBlock);
    void setTactic(i16 number); /* 1120:04ff */
    /* Reads static\combat.dat into tactics (1120:0579). */
    void loadTactics();
    /* A loaded Attack's tactic: the original keeps the saved far pointer,
     * whose selector is another run's (the tactics load when the first
     * Attack is made); the port takes it from tacticNumber again, as
     * setTactic has it. */
    void tacticLoaded();
    /* The NPC can fire at its target in direction: it is still ready, has
     * sighted it or has a line of fire (1120:1330). */
    u8 canFire(char direction);
    /* Walks to the target (1120:1396). */
    void chase();
    /* No Regret's: 1 at most every 90 ticks, time to look for another target
     * (1120:1403). */
    bool searchDue();
    /* Starts the feint: No Regret's unless the NPC feinted (1120:1455), No
     * Remorse's for 8 to 17 seconds (1108:1485); No Regret's ends it (146e). */
    void startFeint();
    void endFeint();
    /* The attack without a tactic (1120:1485), and its steps (the port's):
     * the NPC without a target walks off at random (wander), readies its
     * weapon (drawWeapon), fires a burst (fireBurst, four times the shots
     * by chance: aimedBurstByChance), feints (feintNoRegret,
     * feintNoRemorse), speaks (No Remorse's stillSpeaking,
     * speakBeforeFiring), fires at the target it faces (fireWhenReady) or
     * cannot (waitOrChase), turns to it (turnToTarget). */
    void attack();
    void wander(Npc &self);
    void drawWeapon(Npc &self, u32 now);
    void aimedBurstByChance(Npc &self);
    void fireBurst(Npc &self, u32 now);
    bool stillSpeaking(Npc &self);
    bool speakBeforeFiring(Npc &self, u16 shape);
    void feintNoRegret(Npc &self);
    bool feintNoRemorse(Npc &self, Npc &target, const Item &gun, const AnimInfo &info, u16 stance, u32 now);
    void fireWhenReady(Npc &self, u16 shape, u16 weapon, Item &gun, u32 now);
    void waitOrChase(Npc &self, Npc &target);
    void turnToTarget(Npc &self, Npc &target, const AnimInfo &info, u8 direction, u32 now);
    /* run's steps (the port's): a tactic's jump (jumpIf), its fire when the
     * target is in the line of fire, its walks to a waypoint and halfway to
     * the target. */
    void jumpIf(bool jump);
    void fireIfInRange(Npc &self, Npc &target, i16 anim);
    void walkToWaypoint(Npc &self, i16 frame);
    void walkHalfway(Npc &self, Npc &target);
    /* Sets targetSighted and its time (1120:2473); 1 when 60 ticks (120 with
     * doubleDelay) went by since (2495). */
    void sightTarget();
    [[nodiscard]] bool sightExpired() const;
    /* Sets stoppedToFire (1120:24fd). */
    void setStoppedToFire();
    void run() override;
    void event(u32 event, EventData eventData) override;
};
ASSERT_SIZE(Attack, ATTACK_SIZE);
/* Deletes the tactics Attack::loadTactics read; the next Attack reads them
 * again (the port's, for gameRelease). */
void freeTactics();
#endif
