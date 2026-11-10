/* CONTROL.C (Code42; No Regret's 1148, No Remorse's 1130; the addresses are
 * No Regret's): the controlled NPC, the player's keys and the targeting
 * reticle, the cheats and No Regret's screenshots. memoryProfiler (1480:2fef)
 * is CONTROL.C's too; it is declared in cprotmem.h, which the memory library
 * reads it through (No Remorse never sets it). The saved block's size and
 * frameSkip's type are each game's (control_game.h). */
#ifndef PORT_CONTROL_H
#define PORT_CONTROL_H
#include <array>
#include "common.h"
#include "kprocess.h"
#include "bindings.h"

/* Process::type */
constexpr int CONTROLLER_TYPE = 0x247;
constexpr int AUTO_FIRER_TYPE = 0x260;

class KeyEvent;
class AutoFirer;
class Npc;
/* The AutoFirer while it runs (1480:302b). */
extern AutoFirer *&autoFirer;

/* The process that moves the controlled NPC with the player's keys. */
class Controller : public PeriodicProcess {
public:
    explicit Controller(Loading loading) : PeriodicProcess(loading) {}
    Controller(); /* 1148:0121 */
    /* Fires shots more times in the timer events; nothing calls it
     * (1148:01e3). */
    void setShots(u16 shots);
    /* Turns the NPC toward the arrow key's direction, or makes it step
     * aside (1148:1768); action is the key's (bindings.h). */
    void turn(Action action, char *direction, u16 *anim, u8 walk);
    /* The arrow keys: 0 when the key is not one of them (1148:1a03). */
    bool move(KeyEvent *key, Action action);
    /* The fire key goes down (pressed 1) or up; 1 when a shot went off
     * (1148:30f1). */
    bool fire(u8 pressed);
    /* Stops the firing and the AutoFirer (1148:3dc3), or the firing only
     * (3df8). */
    void stopAllFiring();
    void stopFiring();
    void run() override;
    void cleanup() override;
    void load(File *file) override;
    void save(File *file) override;
    void afterLoad() override;
    void event(u32 event, EventData data) override;
    /* A key the game makes itself (the mouse's, the joystick's, a mouse
     * shot's): an action's key already, which does not go through the
     * bindings (bindings.h). */
    void syntheticKey(KeyEvent *key);
    /* An action's key from an input of the other devices (devices.h): the
     * action is the input's, the key its own (bindings.h's actionKey); it
     * types no cheat. */
    void actionKey(KeyEvent *key, Action action);

private:
    /* A key for the controller (the original's EVENT_KEYBOARD): pressed as
     * it came, which the cheat codes are typed with (nullptr for an input of
     * the other devices), and key the action's key (nullptr for a key bound
     * to nothing) with its action. */
    void keyboard(KeyEvent *pressed, KeyEvent *bound, Action action);
    /* keyboard's after the cheats' codes typed (the port's): it goes on
     * after a cheat's box. */
    void keyboardRest(KeyEvent *pressed, KeyEvent *bound, Action action, Npc npc, u16 lastAnim, u8 standing);
};
ASSERT_SIZE(Controller, 0x40);

/* Keeps the controlled NPC firing. */
class AutoFirer : public PeriodicProcess {
public:
    explicit AutoFirer(Loading loading) : PeriodicProcess(loading) {}
    AutoFirer(); /* 1148:3e1f */
    void run() override {}
    void cleanup() override;
    void load(File *file) override;
    void save(File *file) override;
    void afterLoad() override { autoFirer = this; }
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(AutoFirer, 0x40);

/* Puts the reticle in front of the controlled NPC (common/src/game/control.cpp). */
void placeReticle();

/* The Controller (1480:2f5a). */
extern Controller *&controller;
/* The NPC the player controls, 1 (the Silencer) at first (1480:2f79). */
extern u16 &controlledActor;
/* The controller takes keys only while it is set (1480:2f7c); WORLD.C's
 * freezeWorld clears it and thawWorld sets it. */
extern u8 &keysEnabled;
/* The NPC the controller moves, turns and fires with; setControlledNpc sets
 * it with controlledActor, and the item cache compares it with the followed
 * item (1480:2f81). */
extern u16 &movedActor;
/* The reticle's sprite (1480:2f87) and whether it shows (2f8a). */
extern u16 &reticleSprite;
extern u8 &reticleShown;
/* Set when the controlled NPC starts a move (NPC.C doAnim: walking, running,
 * sliding, rolling): the time, how long it lasts in ticks and the dodge factor,
 * which divides the spread of the shots at it meanwhile (other shots divide it
 * by 8; 1480:2f90, 2f94, 2f9e). */
extern u32 &dodgeStart;
extern u32 &dodgeLength;
extern i16 &dodgeFactor;
/* The sprite that moves with the Silencer (1480:2f9c). */
extern u16 &silencerSprite;
/* The targeting reticle is on (1480:2fae), its sprite (2fb1) and the item it
 * is on (2fc6). */
extern u8 &targeting;
/* The animations that keep the reticle away (AnimPrimitive::holdReticle,
 * 1480:2faf). */
extern u16 &reticleHolds;
extern u16 &targetSprite;
/* The sprite process that shows the target sprite (1480:2fb3). */
extern u16 &targetSpritePid;
/* No Regret's boss (shape 5B1h) fires a GL-303 grenade at every fifth and
 * sixth shot: its count (1480:2fc0). */
extern i16 &bossShots;
/* The look of the reticle and the target sprite, 0-3: No Regret's Ctrl+R
 * cycles it and it picks six frames of shape 59Ah (1480:2fc2). No Remorse
 * has one, 0 (FIRST_RETICLE_STYLE). */
extern i16 &reticleStyle;
/* The boss's shield (No Remorse's shape 3ACh): it takes the boss's damage
 * until it is gone (takeHit; 1480:2fc4); No Remorse's resetBossShield sets
 * it back to 500. */
extern i16 &bossShield;
/* The rest of the controller's state Controller::save writes
 * (CONTROLLER_GLOBALS, savefields_game.h). */
extern u8 &cannotWalk;
extern u8 &fireHeld;
extern u8 &firing;
extern u8 &controllerInTimer;
extern u32 &lastAlert;
extern char &lastFacing;
extern i16 &reticleAngle;
extern u8 &reticleMoved;
extern u16 &shotsLeft;
extern u32 &targetTime;
extern i16 &unknown2f83;
extern u16 &targetItem;
/* crusader.cfg's frameskip: from 2 the other NPCs' looping animations skip
 * frames, at 3 the controlled NPC's too; fire shapes show every other frame
 * (No Regret's word 1480:2fc8; No Remorse's is a byte, 1478:27fd, whose save
 * holds the low byte, CONTROLLER_GLOBALS). */
extern i16 &frameSkip;
/* crusader.cfg's limitblasts: no new blast within 60 pixels of a young one
 * (createSpriteProcess, 1480:2fca). */
extern u8 &limitBlasts;
/* The fire key is down: the PL-1's shot starts an AutoFirer and stops it
 * when the key comes up (1480:302f). */
extern u8 &fireDown;
/* Set while the AutoFirer fires (1480:3030). */
extern u8 &autoFiring;

/* controlledActor, intrinsic 16h (1148:0048). */
u16 controlledNpcNumber();
/* Intrinsic 54h: setControlledNpc (1148:0038, 3a26). */
void setControlledNpcNumber(u16 npc);
void setControlledNpc(u16 npc);
/* The controlled NPC's last animation is a peaceful one (standing, walking,
 * jumping, unreadying, electrocuted, teleporting in) (1148:004c); it is in a
 * combat stance: not walking, standing, jumping, running, unreadying or
 * electrocuted (1148:00a8). The arrow keys turn it with ANIM_STAND in the
 * one and with ANIM_COMBAT_STAND_SMALL in the other. */
bool inPeacefulStance();
bool inCombatStance();
/* Shows the reticle when the controlled NPC readies a weapon (1148:02d7) and
 * takes it away (06ec). */
void showReticle();
void hideReticle();
/* No Regret's anchor process calls it to move the controlled NPC (1148:033f);
 * No Remorse's DumbTimer's run, to look for the target again from time to
 * time (1430:0346; 1130:02f3). */
void controlTick();
/* Whether a step of anim may start now (1148:1624): with mouseTurn set the
 * running and walking steps come 60/(mouseSpeed*3+3) ticks apart
 * (lastStep, the last it allowed; Controller::move). */
bool stepAllowed(u16 anim);
extern u32 &lastStep;
/* The shotgun was pumped and a shot fired since the fire key went down;
 * when the fire key went down, and how long it was held. */
extern u8 &shotgunPumped;
extern u8 &firedSincePress;
extern u32 &firePressed;
extern u32 &fireHeldFor;
/* Puts the reticle on the target again (1148:041c). */
void updateTarget();
/* Moves the reticle to item, 0 for none (1148:04a5). */
void setTarget(u16 item);
/* The NPCs were alerted less than 240 ticks ago (1148:3b96). */
bool recentlyAlerted();
/* A shot alerts the NPCs on the screen (1148:3bd3). */
void alertNpcs();
/* The controlled NPC gets an item of shape, the cheats' (1148:01fb). */
void giveItem(u16 shape);
/* Takes the reticle and the target away and stops the firing
 * (1148:3fb5). */
void resetTargeting(Controller *activeController);
/* Puts npc in the first frame of anim, facing its direction (1148:40dc). */
void setFirstFrame(u16 npc, u16 anim);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void unknown0000();
i16 unknown3b65();
/* An item the cheat gives up to count of (CHEAT_GIFTS, control_game.h). */
struct CheatGift {
    u16 shape;
    u16 count;
};
#include "control_game.h"
/* The cheat's first list of weapons (control_game.h), whose first word No
 * Remorse's saved game holds (CONTROLLER_GLOBALS). */
extern std::array<i16, 6> &weaponKeys1;
#endif
