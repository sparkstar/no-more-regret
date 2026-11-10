/* CONTROL.C (Code42, No Regret's 1148, No Remorse's 1130; the addresses are
 * No Regret's): the controlled NPC, the player's keys and the targeting
 * reticle, the functions both games have, their values and rules each game's
 * control_game.h (the keys, the moves and the cheats). The saved globals'
 * layout is each game's (<game>/src/game/control_game.cpp). */
#include <cstdio>
#include "gamestate.h"
#include "control.h"
#include "control_game.h"
#include "animcach.h"
#include "animprim.h"
#include "asylum.h"
#include "boboboom.h"
#include "camera.h"
#include "cprotmem.h"
#include "crusader.h"
#include "crusys.h"
#include "cycle.h"
#include "dispatch.h"
#include "distance.h"
#include "dsfxman.h"
#include "dtable.h"
#include "explode.h"
#include "file.h"
#include "gamepal.h"
#include "gumpshap.h"
#include "itemdata.h"
#include "kernel.h"
#include "keyboard.h"
#include "mapjump.h"
#include "msgbox.h"
#include "mousectl.h"
#include "music.h"
#include "npc.h"
#include "palette.h"
#include "pfloat.h"
#include "priority.h"
#include "spmgr.h"
#include "sprite.h"
#include "status.h"
#include "systimer.h"
#include "savefields.h"
#include "targa.h"
#include "tracker.h"
#include "type.h"
#include "u9mouse.h"
#include "utility.h"
#include "viewport.h"
#include "world.h"
#include "movie.h"
#include "host.h"
#include "sounds.h"
#include "shapes.h"
#include "screen.h"

Controller *&controller = gameGlobal<Controller *>(nullptr);
u16 &controlledActor = gameGlobal<u16>(1);
/* The controller's state from here goes into the saved game
 * (CONTROLLER_GLOBALS, <game>/src/game/control_game.cpp; Controller::save). */
u8 &fireHeld = gameGlobal<u8>(0);

u8 &firing = gameGlobal<u8>(0);

/* Set while the timer's shots fire (CONTROL.C's static inTimer; the
 * kernel's inTimer is another). */
u8 &controllerInTimer = gameGlobal<u8>(0);

u16 &shotsLeft = gameGlobal<u16>(0);
u16 &movedActor = gameGlobal<u16>(1);

i16 &unknown2f83 = gameGlobal<i16>(0x40);
/* The reticle's angle in degrees; over 360 takes the NPC's direction. */
i16 &reticleAngle = gameGlobal<i16>(1000);
u16 &reticleSprite = gameGlobal<u16>(0xffff);
/* The reticle is to be placed again. */
u8 &reticleMoved = gameGlobal<u8>(0);
u8 &reticleShown = gameGlobal<u8>(0);

u8 &cannotWalk = gameGlobal<u8>(0);

/* When alertNpcs last alerted the NPCs. */
u32 &lastAlert = gameGlobal<u32>(0);
u16 &silencerSprite = gameGlobal<u16>(0);

u8 &targeting = gameGlobal<u8>(1);
u16 &reticleHolds = gameGlobal<u16>(0);
u16 &targetSprite = gameGlobal<u16>(0xffff);
u16 &targetSpritePid = gameGlobal<u16>(0);
/* The direction the target was last looked for. */
char &lastFacing = gameGlobal<char>(NO_DIRECTION);

u32 &targetTime = gameGlobal<u32>(0);

u16 &targetItem = gameGlobal<u16>(0);
/* The look of the reticle and the target sprite (control.h): No Regret's
 * Ctrl+R cycles it, No Remorse's one look is 0. */
i16 &reticleStyle = gameGlobal<i16>(FIRST_RETICLE_STYLE);
/* The shotgun was pumped and a shot fired since the fire key went down;
 * when it went down and how long it was held (control.h). */
u8 &shotgunPumped = gameGlobal<u8>(0);
u8 &firedSincePress = gameGlobal<u8>(0);
u32 &firePressed = gameGlobal<u32>(0);
u32 &fireHeldFor = gameGlobal<u32>(0);
/* The last step stepAllowed allowed (control.h). */
u32 &lastStep = gameGlobal<u32>(0);
namespace {
/* When the reticle last turned the busy NPC (No Regret's Controller::turn). */
u32 &lastTurn = gameGlobal<u32>(0);
}
/* No Regret's boss's shots (bossShots, control.h). */
i16 &bossShots = gameGlobal<i16>(0);

AutoFirer *&autoFirer = gameGlobal<AutoFirer *>(nullptr);
u8 &fireDown = gameGlobal<u8>(0);

/* No Regret's MEMORY cheat turns the memory profiler's drawing on and off
 * (cprotmem.h); nothing sets it in No Remorse. */
u8 &memoryProfiler = gameGlobal<u8>(0);

/* Nothing calls it (1148:0000, 3b65). */
/* UNCALLED: nothing calls it. */
void unknown0000()
{
    hideReticle();
}

void setControlledNpcNumber(u16 npc)
{
    setControlledNpc(npc);
}

u16 controlledNpcNumber()
{
    return controlledActor;
}

bool inPeacefulStance()
{
    Npc self(controlledActor);
    const u16 anim = self.lastAnimSet();
    return anim == ANIM_STAND || anim == ANIM_WALK || anim == ANIM_QUICK_JUMP || anim == ANIM_ELECTROCUTED ||
           anim == ANIM_UNREADY_SMALL || anim == ANIM_UNREADY_LARGE || anim == ANIM_TELEPORT_IN ||
           anim == ANIM_JUMP_LANDING;
}

/* NON_COMBAT_ANIMS are each game's. */
bool inCombatStance()
{
    Npc self(controlledActor);
    const u16 anim = self.lastAnimSet();
    return !(isOneOf(anim, NON_COMBAT_ANIMS) || self.isStartingRun());
}

/* The controlled NPC gets an item of shape with quality CHEAT_ITEM_QUALITY
 * (No Regret's 99, No Remorse's 100: the cheats); an item it has already
 * gets the quality (1148:01fb). */
void giveItem(u16 shape)
{
    const u16 found = controlledNpc.findItem(shape);
    u16 slot;
    if (found != 0U) {
        Item item(found);
        item.setQuality(CHEAT_ITEM_QUALITY);
    } else {
        Item item;
        slot = controlledNpc.utilitySlot();
        item.create(shape, 0);
        item.popToCoords(controlledNpc.x(), controlledNpc.y(), controlledNpc.z());
        item.setFamilyQuality(CHEAT_ITEM_QUALITY);
        if ((slot != 0U) && (item.id != 0U)) {
            item.push();
            item.popToEnd(controlledNpc.id);
            controlledNpc.setEquip(slot, item.id);
        }
    }
}

Controller::Controller()
{
    controller = this;
    nameProcess(pid, "Controller");
    setType(CONTROLLER_TYPE);
    makeSystem();
    setRate(60);
    startTimer(1);
    listenKeyboard();
    eventMask |= EVENT_RELATED_DIED | EVENT_WAIT_OVER | EVENT_KEYBOARD | EVENT_TIMEOUT;
}

/* Nothing calls it. */
void Controller::setShots(u16 shots)
{
    shotsLeft = shots;
    eventMask |= EVENT_TIMER;
}

void controlTick()
{
    if (targeting != 0U) {
        if (reticleShown != 0U) {
            const u32 now = systemTimer->time();
            if (targetTime + 60 <= now) {
                targetTime = now;
                Npc self(controlledActor);
                const Npc target(findTarget(controlledActor, self.direction(), 1));
                if (target.isValid()) {
                    if (target.id != targetItem && (target.isOccluding() != 0U)) {
                        setTarget(target.id);
                        return;
                    }
                } else {
                    setTarget(0);
                    return;
                }
            }
        }
    } else {
        setTarget(0);
    }
}

void updateTarget()
{
    if (targeting != 0U) {
        if (targetItem != 0U) {
            setTarget(0);
        }
        if (reticleShown != 0U) {
            Npc self(controlledActor);
            const Npc target(findTarget(controlledActor, self.direction(), 1));
            if (target.isValid() && (target.isOccluding() != 0U)) {
                setTarget(target.id);
            }
        }
    }
}

void hideReticle()
{
    Process *process;
    if (reticleShown != 0U) {
        freeSprite(reticleSprite);
        reticleSprite = 0xffff;
        reticleAngle = 1000;
        reticleShown = 0;
        if (targetSpritePid != 0U) {
            process = findProcess(targetSpritePid);
            process->terminate(0);
        }
        if (targetSprite != 0xffff) {
            freeSprite(targetSprite);
            targetSprite = 0xffff;
        }
        targetItem = 0;
    }
}

void showReticle()
{
    const Npc self(controlledActor);
    const bool held = RETICLE_HIDDEN_WHILE_HELD && controlledNpc.held != 0U && controlledActor == 1;
    if ((reticleHolds == 0U) && !held && self.shape() != SHAPE_BURNING) {
        reticleMoved = 1;
        if ((reticleShown == 0U) && (cannotWalk == 0U)) {
            lastFacing = NO_DIRECTION;
            placeReticle();
            reticleShown = 1;
            updateTarget();
        }
    }
}

void setTarget(u16 item)
{
    Process *process;
    if (targeting == 0U) {
        return;
    }
    if (targetItem == item) {
        return;
    }
    if (item == 0U) {
        lastFacing = NO_DIRECTION;
    }
    targetItem = 0;
    if (targetSpritePid != 0U) {
        freeSprite(targetSprite);
        targetSprite = 0xffff;
        process = findProcess(targetSpritePid);
        process->terminate(0);
    } else if (targetSprite != 0xffff) {
        freeSprite(targetSprite);
        targetSprite = 0xffff;
    }
    if (item != 0U) {
        Npc self(controlledActor);
        const char facing = self.direction();
        if (lastFacing == facing) {
            return;
        }
        targetTime = systemTimer->time();
        if (fireDistance(&self, item, facing, 0, 0, 0) != 0U) {
            const Item target(item);
            const ShapeInfo info = typeFlags.info(target.shape());
            i16 z = info.z;
            const u16 x = target.x();
            const u16 y = target.y();
            u16 spriteX;
            u16 spriteY;
            if (z >= 4) {
                z >>= 1;
                z++;
            } else if (z >= 2) {
                z >>= 1;
            }
            if (!target.isNpc()) {
                if (z >= 2) {
                    z -= 2;
                } else if (z == 1) {
                    z = 0;
                }
            }
            z *= 8;
            z += target.z();
            if (info.x > info.y) {
                spriteX = info.x >> 1;
                spriteX *= 16;
                spriteX = x - spriteX;
                spriteY = y;
            } else if (info.y > info.x) {
                spriteY = info.y >> 1;
                spriteY *= 16;
                spriteY = y - spriteY;
                spriteX = x;
            } else {
                spriteX = x;
                spriteY = y;
            }
            createSpriteProcess(SHAPE_RETICLE, reticleStyle * 6, (reticleStyle * 6) + 5, 10, spriteX, spriteY, z);
            targetItem = item;
        }
        lastFacing = facing;
    }
}

/* Puts the reticle in front of the controlled NPC (1148:0762). The reticle goes
 * 200 units out from movedActor at reticleAngle. The firing frame's offset is
 * read into x twice (original: the y pointer is x's). */
void placeReticle()
{
    Npc npc(movedActor);
    u16 x = npc.centreX();
    u16 y = npc.centreY();
    u8 z = itemZ[npc.id];
    const char facing = npc.direction();
    u16 dx;
    u16 dz;
    fireOffset(facing, &dx, &dx, &dz);
    z += dz;
    if (reticleAngle > 360) {
        PFloat angle(22, 5);
        angle *= facing;
        angle.value += PFloat(5, 6).value;
        const PFloat sum = angle;
        reticleAngle = static_cast<i16>(static_cast<i16>(sum.value >> 16) + 270) % 360;
    }
    PFloat degrees(reticleAngle, 0);
    const PFloat across = cosine(degrees) * 200;
    const PFloat down = sine(degrees) * 200;
    x += static_cast<i16>(across.value >> 16);
    y += static_cast<i16>(down.value >> 16);
    if (reticleSprite == 0xffff) {
        reticleSprite = createSprite(0x4cc, 0, 0, x, y, z);
    } else {
        showSprite(reticleSprite, x, y, z);
    }
    reticleMoved = 0;
}

/* The left and right keys turn the reticle, or the NPC in steps when there
 * is none; direction gets the reticle's direction and anim the stance. */
void Controller::turn(Action action, char *direction, u16 *anim, u8 walk)
{
    bool turned = false;
    i16 step;
    Npc npc(movedActor);
    const char facing = npc.direction();
    /* A key other than left and right leaves it as it is: the original's
     * busy NPC then turns to what its stack held ([bp-9]), which the port
     * cannot know, so it keeps its facing. */
    char toward = facing;
    u32 now;
    if (reticleShown == 0U) {
        if (cannotWalk == 0) {
            return;
        }
        if (npc.isBusy()) {
            now = systemTimer->time();
            if (BUSY_NPC_TURNS && lastTurn + 12 <= now) {
                AnimPrimitive *process = static_cast<AnimPrimitive *>(findItemProcess(movedActor, ANIM_TYPE));
                if ((action == Action::TURN_LEFT || action == Action::MOVE_SOUTHWEST ||
                     action == Action::NUMPAD_TURN_LEFT)) {
                    toward = (facing - 1) & 0xf;
                } else if ((action == Action::TURN_RIGHT || action == Action::MOVE_NORTHEAST ||
                            action == Action::NUMPAD_TURN_RIGHT)) {
                    toward = (facing + 1) & 0xf;
                }
                process->turnTo(toward);
                lastTurn = now;
            }
            return;
        }
        if ((action == Action::TURN_LEFT || action == Action::MOVE_SOUTHWEST || action == Action::NUMPAD_TURN_LEFT)) {
            toward = (facing - 1) & 0xf;
        } else if ((action == Action::TURN_RIGHT || action == Action::MOVE_NORTHEAST ||
                    action == Action::NUMPAD_TURN_RIGHT)) {
            toward = (facing + 1) & 0xf;
        } else {
            return;
        }
    }
    if (cannotWalk != 0U) {
        if (npc.isKneeling() != 0U) {
            *anim = ANIM_KNEELING_SMALL;
        } else {
            *anim = ANIM_COMBAT_STAND_SMALL;
        }
    } else {
        if (mouseTurn == 0U) {
            if (walk != 0U) {
                step = 2;
            } else {
                step = 5;
            }
        } else {
            step = mouseTurnAmount >> 1;
            if (step > 20) {
                step = 20;
            }
        }
        if ((action == Action::TURN_LEFT || action == Action::MOVE_SOUTHWEST || action == Action::NUMPAD_TURN_LEFT)) {
            turned = true;
            reticleAngle -= step;
        } else if ((action == Action::TURN_RIGHT || action == Action::MOVE_NORTHEAST ||
                    action == Action::NUMPAD_TURN_RIGHT)) {
            turned = true;
            reticleAngle += step;
        }
        if (turned) {
            placeReticle();
            toward = fineDirection(npc.centreX(), npc.centreY(), spriteX(reticleSprite), spriteY(reticleSprite));
            *direction = toward;
        }
        if (!turned || toward != facing) {
            if (npc.isKneeling() != 0U) {
                *anim = ANIM_KNEELING_SMALL;
            } else {
                *anim = ANIM_COMBAT_STAND_SMALL;
            }
        }
    }
    if ((targeting != 0U) && turned && toward != facing) {
        const Npc target(findTarget(controlledActor, toward, 1));
        if (target.isValid()) {
            if (target.id != targetItem) {
                if (target.isOccluding() != 0U) {
                    setTarget(target.id);
                    return;
                }
                setTarget(0);
                return;
            }
        } else {
            setTarget(0);
        }
    }
}

/* Whether a step of anim may start now (1148:1624): with mouseTurn set the
 * running and walking steps come 60/(mouseSpeed*3+3) ticks apart. */
bool stepAllowed(u16 anim)
{
    const u32 now = systemTimer->time();
    if ((mouseTurn != 0U) &&
        (anim == ANIM_WALK || anim == ANIM_RUN || anim == ANIM_COMBAT_RUN_SMALL || anim == ANIM_COMBAT_RUN_LARGE)) {
        if ((lastStep == 0U) || lastStep + 60 / static_cast<i16>((mouseSpeed * 3) + 3) <= now) {
            lastStep = now;
            return true;
        }
        return false;
    }
    if (anim == ANIM_WALK || anim == ANIM_RUN || anim == ANIM_START_RUN ||
        (ROLL_FORWARD_STEPS && anim == ANIM_ROLL_FORWARD) || anim == ANIM_COMBAT_STAND_SMALL ||
        anim == ANIM_COMBAT_STAND_LARGE || anim == ANIM_BRIGHT_FIRE_LARGE || anim == ANIM_BRIGHT_KNEEL_FIRE ||
        anim == ANIM_KNEELING_SMALL || anim == ANIM_KNEELING_LARGE || anim == ANIM_FIRE_SMALL ||
        anim == ANIM_KNEEL_FIRE_SMALL || anim == ANIM_FIRE_LARGE || anim == ANIM_KNEEL_FIRE_LARGE ||
        anim == ANIM_RELOAD_SMALL || anim == ANIM_RELOAD_LARGE || anim == ANIM_COMBAT_RUN_SMALL ||
        anim == ANIM_COMBAT_RUN_LARGE || anim == ANIM_START_RUN_SMALL || anim == ANIM_START_RUN_LARGE ||
        anim == ANIM_ADVANCE_SMALL || anim == ANIM_ADVANCE_LARGE || anim == ANIM_RETREAT_SMALL ||
        anim == ANIM_RETREAT_LARGE || anim == ANIM_READY_SMALL || anim == ANIM_READY_LARGE ||
        anim == ANIM_STOP_RUN_SMALL || anim == ANIM_STOP_RUN_LARGE) {
        return true;
    }
    return false;
}

/* The fire key: pressed fires the weapon (a burst goes on in the timer
 * events), released lets it go; an empty weapon reloads, or clicks. */
bool Controller::fire(u8 pressed)
{
    i16 delay;
    i16 burst;
    u16 weaponId;
    u16 animPid;
    bool fired = false;
    Npc npc(movedActor);
    u8 sound;
    const u16 shape = npc.shape();
    u16 weaponShape;
    if (pressed == 0U) {
        fireHeldFor = systemTimer->time() - firePressed;
        firePressed = 0;
    }
    weaponId = npc.activeWeapon();
    const Item weapon(weaponId);
    if (weaponId != 0U) {
        weaponShape = weapon.shape();
    } else {
        weaponShape = 0;
    }
    fireDown = pressed;
    if (AUTO_FIRE_STOPS_EMPTY && (autoFirer != nullptr) && (npc.ammo() == 0U)) {
        return false;
    }
    if ((shotsLeft != 0U) && (controllerInTimer == 0U)) {
        return false;
    }
    if (npc.isBusy() && (pressed != 0U)) {
        return false;
    }
    if ((fireHeld != 0U) && ((pressed != 0U) || (firedSincePress == 0U)) &&
        ((weaponId != 0U) || isOneOf(shape, UNARMED_SHOOTERS))) {
        if (!npc.isSilencer() || (npc.ammo() != 0U)) {
            if (npc.isSilencer() && (npc.isKneeling() == 0U) && weapon.shape() == SHAPE_SG_A1 &&
                (shotgunPumped == 0U)) {
                sound = reloadSound(weapon.shape());
                if (sound != 0U) {
                    npc.restartSound(sound);
                }
                if (npc.isSilencer()) {
                    npc.doAnim(ANIM_RELOAD_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
                }
                shotgunPumped = 1;
            } else {
                if (weaponShape == SHAPE_SG_A1) {
                    shotgunPumped = 0;
                }
                firedSincePress = 1;
                delay = shotDelay(weaponShape);
                burst = burstSize(weaponShape);
                if (npc.isKneeling() != 0U) {
                    animPid = npc.doAnim(ANIM_KNEEL_FIRE_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
                } else {
                    animPid = npc.doAnim(ANIM_FIRE_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
                }
                if (animPid != 0U) {
                    if (npc.isSilencer()) {
                        npc.useAmmo();
                    }
                    if (!recentlyAlerted()) {
                        alertNpcs();
                    }
                    firing = 1;
                    setTimeout(delay);
                    if (controllerInTimer != 0U) {
                        shotsLeft--;
                    } else if (burst >= 2) {
                        setShots(burst - 1);
                    }
                    fired = true;
                }
            }
        } else if (npc.findAmmo() != 0U) {
            /* The RP-32 branch below joins the reload here. */
        reload:
            stopFiring();
            if (weapon.shape() == SHAPE_SG_A1) {
                shotgunPumped = 1;
            }
            npc.reload();
            firing = 1;
            setTimeout(15);
        } else if (RP_32_NEVER_DRY && weaponShape == SHAPE_RP_32 &&
                   (controlledNpc.difficulty == 1 || controlledNpc.difficulty == 2)) {
            /* On difficulty 1 and 2 the RP-32 never runs dry: giveItem hands
             * over a clip and the reload goes on without looking for
             * ammunition again (original). */
            giveItem(SHAPE_RP_32_CLIP);
            goto reload;
        } else {
            fireHeld = 0;
            npc.restartSound(SOUND_DRY_FIRE);
            firing = 1;
            setTimeout(20);
        }
    }
    if (controllerInTimer != 0U) {
        if (shotsLeft == 0U) {
            mouseFiring = 0;
            eventMask &= ~EVENT_TIMER;
            if (actionHeld(Action::FIRE) != 0U) {
                fireHeld = 1;
            } else {
                fireHeld = 0;
            }
        }
    } else if (shotsLeft == 0U) {
        mouseFiring = 0;
        fireHeld = pressed;
    }
    return fired;
}

void Controller::run()
{
    if ((reticleShown != 0U) && (reticleMoved != 0U)) {
        placeReticle();
    }
}

void Controller::event(u32 event, EventData data)
{
    Npc npc(movedActor);
    const Npc unused(0);
    switch (event) {
    case EVENT_RELATED_DIED:
        silencerSprite = 0;
        break;
    case EVENT_TIMER:
        controllerInTimer = 1;
        if (npc.isValid() && (npc.isDead() == 0U)) {
            fire(1);
        }
        controllerInTimer = 0;
        break;
    case EVENT_WAIT_OVER:
    case EVENT_TIMEOUT:
        firing = 0;
        break;
    case EVENT_KEYBOARD: {
        /* A key the player pressed: the key of the action it is bound to
         * (bindings.h), or none. */
        KeyEvent pressed = *data.pointer<KeyEvent>();
        ControlKey bound;
        if (controlKey(pressed, &bound)) {
            keyboard(&pressed, &bound.key, bound.action);
        } else {
            keyboard(&pressed, nullptr, Action::NONE);
        }
        break;
    }
    }
}

void Controller::syntheticKey(KeyEvent *key)
{
    keyboard(key, key, controlAction(key));
    /* The mouse's, the joystick's and the explosions' keys (arrows, space,
     * J) end no cheat's code (6, X, T, Y) and are not cheatKey's (Escape,
     * F10): no box of a cheat is opened by them, which their callers do not
     * go on after. */
    if (waitOpened()) {
        halt();
    }
}

void Controller::actionKey(KeyEvent *key, Action action)
{
    keyboard(nullptr, key, action);
}

void Controller::afterLoad()
{
    controlledActor = movedActor;
    controller = this;
}

void Controller::cleanup()
{
    Process::cleanup();
    controller = nullptr;
}

/* Intrinsic 54h: the player controls npc from now on. */
void setControlledNpc(u16 npc)
{
    if (controlledActor != npc) {
        Npc old(controlledActor);
        Npc next(npc);
        reticleAngle = 1000;
        if (npc != 0U) {
            if (hasAnim(next.shape(), ANIM_WALK)) {
                cannotWalk = 0;
            } else {
                cannotWalk = 1;
            }
        }
        if (controlledActor != 0U) {
            if ((old.isDead() == 0U) && (old.isInCombat() != 0U)) {
                old.clrInCombat();
            }
            hideReticle();
        }
        setFollowedItem(movedActor = controlledActor = npc);
        if (npc != 0U) {
            if (!next.isSilencer()) {
                killItemProcesses(npc, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
                if (next.isInCombat() != 0U) {
                    next.clrInCombat();
                }
            }
            moveCamera(next.x(), next.y(), next.z(), 0);
        }
        gameMouse->setShape(0);
    }
}

/* Nothing calls it. */
i16 unknown3b65()
{
    const Npc self(controlledActor);
    i16 z = self.z();
    z += unknown2f83;
    return z;
}

/* The NPCs were alerted less than 240 ticks ago. */
bool recentlyAlerted()
{
    const u32 now = systemTimer->time();
    const u32 elapsed = now - lastAlert;
    return elapsed < 0xf0;
}

/* Stops the firing, and the AutoFirer. */
void Controller::stopAllFiring()
{
    stopFiring();
    if (autoFirer != nullptr) {
        autoFirer->terminate(0);
    }
}

/* Stops the firing. */
void Controller::stopFiring()
{
    fireHeld = 0;
    firing = 0;
    controllerInTimer = 0;
    shotsLeft = 0;
    eventMask &= ~EVENT_TIMER;
}

/* A shot alerts the NPCs on the screen: those in combat turn on the
 * controlled NPC, the others start fighting, WATCHFUL_SHAPES' only when they
 * can see it. */
void alertNpcs()
{
    Npc npc(0);
    const Npc self(movedActor);
    i16 i;
    lastAlert = systemTimer->time();
    for (i = 2; i < 0x100; i++) {
        npc = Npc(i);
        if (npc.isValid() && !npc.inVoid() && static_cast<ui16>(i) != controlledActor && npc.isOnScreen()) {
            if (npc.isInCombat() != 0U) {
                if (npc.activity() == ACTIVITY_GUARD) {
                    npc.setActivity(ACTIVITY_ATTACK);
                }
                npc.setAttackTarget(controlledActor);
            } else if (npc.activity() != npc.defaultActivity2() && (npc.isActivityLocked() == 0U)) {
                u8 alerted;
                const u16 shape = npc.shape();
                if (isOneOf(shape, WATCHFUL_SHAPES)) {
                    alerted = rangeIfVisible(&npc, controlledActor);
                } else {
                    alerted = 1;
                }
                if (alerted != 0U) {
                    npc.setActivity(ACTIVITY_DEFAULT2);
                    if (npc.isInCombat() != 0U) {
                        npc.setAttackTarget(controlledActor);
                    }
                }
            } else if (npc.activity() == ACTIVITY_GUARD) {
                npc.setActivity(ACTIVITY_ATTACK);
                npc.setAttackTarget(controlledActor);
            }
        }
    }
}

/* AUTO_FIRER_RATE shots a second: No Regret's 12, No Remorse's 6. */
AutoFirer::AutoFirer()
{
    nameProcess(pid, "AutoFirer");
    setType(AUTO_FIRER_TYPE);
    setItem(controlledActor);
    autoFirer = this;
    setRate(AUTO_FIRER_RATE);
    startTimer(1);
    eventMask |= EVENT_TIMER;
}

void AutoFirer::event(u32 /*event*/, EventData /*data*/)
{
    Npc npc(controlledActor);
    const Item weapon(npc.activeWeapon());
    AnimFrame *frame;
    if (weapon.isValid() && weapon.shape() == SHAPE_PL_1) {
        if (npc.ammo() != 0U) {
            frame = controlledFrame();
            if (frame != nullptr) {
                autoFiring = 1;
                if (AUTO_FIRER_USES_AMMO) {
                    npc.useAmmo();
                }
                fireAtFrame(controlledActor, *frame);
                autoFiring = 0;
            }
        }
    } else if (autoFirer != nullptr) {
        autoFirer->terminate(0);
    }
}

void AutoFirer::cleanup()
{
    Process::cleanup();
    autoFirer = nullptr;
}

/* Takes the reticle and the target away, and stops the firing. */
void resetTargeting(Controller *activeController)
{
    if (activeController != nullptr) {
        hideReticle();
        activeController->stopAllFiring();
        targetSpritePid = 0;
        targetItem = 0;
        targetSprite = 0xffff;
        lastFacing = NO_DIRECTION;
        reticleHolds = 0;
    }
}

/* The original reads and writes its globals from fireHeld to the word after
 * targetItem (CONTROLLER_BLOCK bytes: (u16)&targetItem-(u16)&fireHeld+4,
 * 79 in No Regret and 75 in No Remorse) as they lie in its data segment; the
 * port's are laid out otherwise (CONTROLLER_GLOBALS). */
void Controller::load(File *file)
{
    u8 block[CONTROLLER_BLOCK_CAPACITY];
    Process::load(file);
    file->read(block, CONTROLLER_BLOCK, -1);
    SaveImage image(SaveImage::READ, block, CONTROLLER_BLOCK, nullptr);
    saveGlobals(image, tableStart(CONTROLLER_GLOBALS), CONTROLLER_GLOBAL_COUNT);
}

void Controller::save(File *file)
{
    u8 block[CONTROLLER_BLOCK_CAPACITY];
    Process::save(file);
    SaveImage image(SaveImage::WRITE, block, CONTROLLER_BLOCK, nullptr);
    saveGlobals(image, tableStart(CONTROLLER_GLOBALS), CONTROLLER_GLOBAL_COUNT);
    file->write(block, CONTROLLER_BLOCK, -1);
}

void AutoFirer::load(File *file)
{
    Process::load(file);
    file->read(&fireDown, 2, -1);
}

void AutoFirer::save(File *file)
{
    Process::save(file);
    file->write(&fireDown, 2, -1);
}

void setFirstFrame(u16 npc, u16 anim)
{
    AnimFrame frame;
    Npc self(npc);
    const u16 shape = itemShape[npc];
    u16 value;
    getAnimFrame(shape, anim, self.direction(), 0, &frame);
    self.setFrame(frame.frame);
    if (frame.flipped) {
        self.setFlags(self.flags() | ITEM_FLIPPED);
    }
    value = anim;
    npcs[self.id].lastAnimSet = value;
    npcs[self.id].animFrame = 0;
}

namespace {
/* Backspace saves the screen (No Regret's PIX cheat). */
u8 &pictures = gameGlobal<u8>(0);
/* The weapons of the number keys and of the cheat (control_game.h). */
std::array<i16, 13> &weaponKeys = gameGlobal<std::array<i16, 13>>(NUMBER_KEY_WEAPONS);
}
std::array<i16, 6> &weaponKeys1 = gameGlobal<std::array<i16, 6>>(WEAPON_LIST_1);
namespace {
std::array<i16, 6> &weaponKeys2 = gameGlobal<std::array<i16, 6>>(WEAPON_LIST_2);
std::array<i16, 4> &weaponKeys3 = gameGlobal<std::array<i16, 4>>(WEAPON_LIST_3);
}

u8 &keysEnabled = gameGlobal<u8>(1);

u32 &dodgeStart = gameGlobal<u32>(0);
u32 &dodgeLength = gameGlobal<u32>(0);

i16 &dodgeFactor = gameGlobal<i16>(1);
i16 &frameSkip = gameSetting<i16>(0);
u8 &limitBlasts = gameSetting<u8>(0);
i16 &bossShield = gameGlobal<i16>(BOSS_SHIELD_START);
namespace {
/* The animations a kneeling NPC keeps (No Regret's). */
i16 kneelAnims[16] = {ANIM_KNEEL_START,
                      ANIM_KNEELING_SMALL,
                      ANIM_KNEELING_LARGE,
                      ANIM_KNEEL_FIRE_SMALL,
                      ANIM_KNEEL_FIRE_LARGE,
                      ANIM_COMBAT_ROLL_LEFT,
                      ANIM_COMBAT_ROLL_RIGHT,
                      ANIM_KNEEL_ROLL_LEFT,
                      ANIM_KNEEL_ROLL_RIGHT,
                      ANIM_BRIGHT_KNEEL_FIRE,
                      5,
                      ANIM_KNEELING_RETREAT,
                      ANIM_KNEELING_ADVANCE,
                      ANIM_KNEELING_SLIDE_LEFT,
                      0x1b,
                      -1};
/* The cheats, typed as scan codes, and how much of each has been typed:
 * JASSICA16, and No Regret's LOOSECANNON16, PIX, VIDTEST, PRO (nothing reads
 * it) and MEMORY. */
u8 jassicaCheat[10] = {SCAN_J, SCAN_A, SCAN_S, SCAN_S, SCAN_I, SCAN_C, SCAN_A, SCAN_1, SCAN_6, 0};
i16 &jassicaTyped = gameGlobal<i16>(0);
u8 looseCannonCheat[14] = {SCAN_L, SCAN_O, SCAN_O, SCAN_S, SCAN_E, SCAN_C, SCAN_A,
                           SCAN_N, SCAN_N, SCAN_O, SCAN_N, SCAN_1, SCAN_6, 0};
i16 &looseCannonTyped = gameGlobal<i16>(0);
u8 pixCheat[4] = {SCAN_P, SCAN_I, SCAN_X, 0};
i16 &pixTyped = gameGlobal<i16>(0);
u8 vidtestCheat[8] = {SCAN_V, SCAN_I, SCAN_D, SCAN_T, SCAN_E, SCAN_S, SCAN_T, 0};
i16 &vidtestTyped = gameGlobal<i16>(0);
[[maybe_unused]] u8 proCheat[4] = {SCAN_P, SCAN_R, SCAN_O, 0};
[[maybe_unused]] i16 proTyped = 0;
u8 memoryCheat[7] = {SCAN_M, SCAN_E, SCAN_M, SCAN_O, SCAN_R, SCAN_Y, 0};
i16 &memoryTyped = gameGlobal<i16>(0);
}

u8 &autoFiring = gameGlobal<u8>(0);

void typeCheat(KeyEvent *key);

namespace {
/* anim is one a kneeling NPC keeps (1148:0006). */
bool isKneelAnim(u16 anim)
{
    i16 i;
    for (i = 0;; i++) {
        if (kneelAnims[i] == -1) {
            break;
        }
        if (static_cast<ui16>(kneelAnims[i]) == anim) {
            return true;
        }
    }
    return false;
}

/* Saves the screen to the first free cru%04d.tga (1148:08f0). */
void screenshot()
{
    Targa *targa = nullptr;
    i16 number = 0;
    char name[40];
    do {
        snprintf(name, sizeof(name), "cru%04d.tga", number++);
    } while (fileExists(name) != 0);
    targa = new Targa(name, SCREEN_WIDTH, SCREEN_HEIGHT, 8);
    targa->viewport->blit(screenViewport);
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    targa->palette->set(gamePalette->data.get(), 0, 256);
    targa->save(nullptr, 0);
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 0;
    }
    if (targa != nullptr) {
        delete targa;
    }
}

/* The cheats' and the debugger's keys of 1148:09fd (handleKey; No Remorse's 1130:0896) as the
 * player pressed them: they are not bound (bindings.h). 1 when the key is
 * done with; with Ctrl and Alt, Escape goes no further in the original
 * either. */
/* cheatKey's F10 after its box of the immortality (the port's, which goes on
 * after the box has closed): the silencer's gifts and the status. */
void cheatGifts(Npc npc, KeyEvent key)
{
    if (npc.isSilencer()) {
        u16 slot;
        i16 i;
        i16 shape;
        if (CHEAT_GIVES_CREDITS) {
            controlledNpc.setCredits(CHEAT_CREDITS);
        }
        Item item;
        if (item.create(SHAPE_FUSION_BATTERY, 0)) {
            item.popToCoords(0, 0, 0);
            if (!pickUp(item.id, 1)) {
                item.destroy();
            }
        }
        if (item.create(SHAPE_GRAVITON_SHIELD, 0)) {
            item.popToCoords(0, 0, 0);
            if (!pickUp(item.id, 1)) {
                item.destroy();
            }
        }
        npc.recharge(0);
        for (i = 0;; i++) {
            if (!WEAPON_LISTS) {
                shape = weaponKeys[i];
            } else if (key.isControlled()) {
                shape = weaponKeys2[i];
            } else if (key.isAlted()) {
                shape = weaponKeys3[i];
            } else {
                shape = weaponKeys1[i];
            }
            if (shape == -1) {
                break;
            }
            slot = controlledNpc.weaponSlot();
            if (slot == 0U) {
                break;
            }
            if (controlledNpc.findItem(shape) == 0U) {
                item.create(shape, 0);
                item.popToCoords(controlledNpc.x(), controlledNpc.y(), controlledNpc.z());
                controlledNpc.setEquip(slot, item.id);
                if (controlledNpc.activeWeapon() == 0U) {
                    controlledNpc.setActiveWeapon(slot);
                }
            }
        }
        /* Up to its count of each gift the controlled NPC has none of. */
        for (const CheatGift &gift : CHEAT_GIFTS) {
            if (controlledNpc.findItem(gift.shape) == 0U) {
                for (i = 0; i < gift.count; i++) {
                    if (item.create(gift.shape, 0)) {
                        item.popToCoords(0, 0, 0);
                        if (!pickUp(item.id, 1)) {
                            item.destroy();
                            break;
                        }
                    }
                }
            }
        }
        for (const u16 clip : CHEAT_CLIPS) {
            giveItem(clip);
        }
    }
    updateStatus(STATUS_ENERGY);
    updateStatus(1);
    updateStatus(0);
}

bool cheatKey(KeyEvent *key)
{
    Npc npc(movedActor);
    if (key->pressed != 0U) {
        switch (key->scan) {
        case SCAN_ESCAPE:
            if (((key->shift & KEY_CONTROL) != 0) && ((key->shift & KEY_ALT) != 0)) {
                if (DEBUG_KEY_FLUSHES) {
                    flushKeys();
                }
                if (debugKeys != 0U) {
                    cycleProcess->paused = 1;
                    hostDebugBreak();
                    cycleProcess->paused = 0;
                }
                return true;
            }
            break;
        /* The cheat: the dead controlled NPC lives again; control toggles
         * immortality (No Regret's does only that, CHEAT_IMMORTALITY_ALONE);
         * the silencer gets credits (CHEAT_CREDITS), batteries, a shield,
         * weapons (No Remorse's a list of three, control and alt picking the
         * other two, WEAPON_LISTS), bombs, mines, medical kits and the
         * others of CHEAT_GIFTS, and clips (CHEAT_CLIPS). */
        case SCAN_F10:
            if (worldActive == 0U) {
                return false;
            }
            if (cheatsOn != 0U) {
                if (CHEAT_IMMORTALITY_ALONE && key->isControlled()) {
                    if (npc.isImmortal() != 0U) {
                        npc.clearImmortal();
                        openModalGump(
                            new MessageBoxGump(NewGumpId(), nullptr, "Immortality disabled.", guiColors->face),
                            static_cast<u32>(-1), [](s32 /*result*/) {});
                    } else {
                        npc.setImmortal();
                        openModalGump(new MessageBoxGump(NewGumpId(), nullptr, "Immortality enabled.", guiColors->face),
                                      static_cast<u32>(-1), [](s32 /*result*/) {});
                    }
                    return true;
                }
                if (controlledNpc.held == 0U) {
                    killItemProcesses(controlledActor, ANY_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
                }
                killItemProcesses(0, DEATH_SILENCE_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
                if ((npc.isDead() != 0U) || (SHAPE_5D6_IS_DEAD && npc.shape() == SHAPE_FROZEN)) {
                    if (npc.isSilencer()) {
                        npc.setShape(1);
                    }
                    npc.clearDead();
                    npc.setHp(npcTypes[npcType(Item(controlledActor).shape())].maxHp);
                    updateStatus(STATUS_HEALTH);
                    return true;
                }
                if (!CHEAT_IMMORTALITY_ALONE && key->isControlled()) {
                    /* The gifts come after the box has closed. */
                    const KeyEvent typed = *key;
                    if (npc.isImmortal() != 0U) {
                        npc.clearImmortal();
                        openModalGump(
                            new MessageBoxGump(NewGumpId(), nullptr, "Immortality disabled.", guiColors->face),
                            static_cast<u32>(-1), [npc, typed](s32 /*result*/) { cheatGifts(npc, typed); });
                    } else {
                        npc.setImmortal();
                        openModalGump(new MessageBoxGump(NewGumpId(), nullptr, "Immortality enabled.", guiColors->face),
                                      static_cast<u32>(-1), [npc, typed](s32 /*result*/) { cheatGifts(npc, typed); });
                    }
                    return true;
                }
                cheatGifts(npc, *key);
                return true;
            }
            break;
        }
    }
    return false;
}

/* The weapon of a number key (weaponKeys): 1-0, then - and =. */
i16 weaponKeyIndex(Action action)
{
    switch (action) {
    case Action::SELECT_WEAPON_1:
        return 0;
    case Action::SELECT_WEAPON_2:
        return 1;
    case Action::SELECT_WEAPON_3:
        return 2;
    case Action::SELECT_WEAPON_4:
        return 3;
    case Action::SELECT_WEAPON_5:
        return 4;
    case Action::SELECT_WEAPON_6:
        return 5;
    case Action::SELECT_WEAPON_7:
        return 6;
    case Action::SELECT_WEAPON_8:
        return 7;
    case Action::SELECT_WEAPON_9:
        return 8;
    case Action::SELECT_WEAPON_10:
        return 9;
    case Action::SELECT_WEAPON_11:
    case Action::MUSIC_QUIETER:
        return 10;
    default:
        return 11;
    }
}

/* The keys that are not moves: 1 when key was one of them (1148:09fd, No
 * Remorse's 1130:0896). They are the weapons (No Regret's), volume, the
 * reticle's look (No Regret's), the camera and taking back the Silencer (the
 * debugger's and the cheats' are cheatKey's); action is the key's
 * (bindings.h). The objects are not used (original). */
bool handleKey(KeyEvent *key, Action action)
{
    Npc npc(movedActor);
    const Npc unusedNpc(0);
    const Item unusedA;
    const Item unusedB;
    const Item unusedC;
    i16 volume;
    const Item unusedD;
    const Item unusedE;
    const Item unusedF;
    const Item unusedG;
    const Item unusedH;
    const Item unusedI;
    if (key->pressed != 0U) {
        switch (action) {
        case Action::RETICLE_STYLE:
            if (RETICLE_STYLE_KEY && key->isControlled()) {
                reticleStyle++;
                if (reticleStyle >= 4) {
                    reticleStyle = 0;
                }
            }
            break;
        case Action::SELECT_WEAPON_1:
        case Action::SELECT_WEAPON_2:
        case Action::SELECT_WEAPON_3:
        case Action::SELECT_WEAPON_4:
        case Action::SELECT_WEAPON_5:
        case Action::SELECT_WEAPON_6:
        case Action::SELECT_WEAPON_7:
        case Action::SELECT_WEAPON_8:
        case Action::SELECT_WEAPON_9:
        case Action::SELECT_WEAPON_10:
        case Action::SELECT_WEAPON_11:
        case Action::SELECT_WEAPON_12:
        case Action::MUSIC_LOUDER:
        case Action::MUSIC_QUIETER:
            /* With Ctrl (held as the roll's mode, or the volume's own) the
             * volume, which the number keys have not; No Remorse's volume
             * keys need no Ctrl and its number keys select no weapon
             * (NUMBER_KEYS_SELECT_WEAPONS). */
            if (!NUMBER_KEYS_SELECT_WEAPONS || key->isControlled()) {
                switch (action) {
                case Action::MUSIC_LOUDER:
                    volume = ASS_GetMusicVolume();
                    if (volume < 0x30) {
                        ASS_SetMusicVolume(volume + 1);
                    }
                    break;
                case Action::MUSIC_QUIETER:
                    volume = ASS_GetMusicVolume();
                    if (volume != 0) {
                        ASS_SetMusicVolume(volume - 1);
                    }
                    break;
                default:
                    break;
                }
            } else if (npc.isSilencer() && (npc.isDead() == 0U)) {
                i16 shape;
                const i16 index = weaponKeyIndex(action);
                shape = weaponKeys[index];
                const Item found(controlledNpc.findItem(shape));
                if (found.isValid()) {
                    i16 slot;
                    for (slot = 1; slot < 13; slot++) {
                        const Item equip(npc.equipment(slot));
                        if (equip.isValid() && equip.shape() == static_cast<ui16>(shape)) {
                            npc.setActiveWeapon(slot);
                            updateStatus(0);
                            updateStatus(1);
                            break;
                        }
                    }
                }
            }
            break;
        case Action::CAMERA:
            if ((worldActive != 0U) && (npc.isDead() == 0U) &&
                (!controlledNpc.isControlled() || (controlledNpc.held == 0U))) {
                if (selectingItems != 0U) {
                    avatarMoved();
                }
                moveCamera(npc.x(), npc.y(), npc.z(), 0);
                return true;
            }
            break;
        case Action::RETURN_CONTROL:
            if (RETURN_KEY_ENDS_STASIS) {
                if ((cycleProcess->paused == 0U) && (cycleProcess->stopped == 0U) && controlledActor == 1) {
                    flushKeys();
                    break;
                }
                if (controlledActor != 1) {
                    clearStasis();
                }
                setControlledNpc(1);
                return true;
            }
            if (movedActor != 1) {
                setControlledNpc(1);
                return true;
            }
            break;
        default:
            break;
        }
    }
    return false;
}
}

namespace {
/* A pressed move key's direction (DIRECTION_TURN_LEFT and _RIGHT for the
 * turns), whether it is a strafe key, and whether it moves (the keypad's
 * keys and the jump); known is false for a key that is no move key. */
struct KeyMove {
    bool known;
    char toward;
    bool strafe;
    bool moving;
};

KeyMove keyMove(Action action, char facing)
{
    switch (action) {
    /* The grey keys: forward, back and the turns; Home, End, Delete,
     * Page Down, Insert and Page Up the combat moves (strafe). */
    case Action::MOVE_FORWARD:
    case Action::MOVE_BACK:
    case Action::KNEEL:
    case Action::ROLL_FORWARD:
        return {true, facing, false, false};
    case Action::TURN_LEFT:
        return {true, DIRECTION_TURN_LEFT, false, false};
    case Action::TURN_RIGHT:
        return {true, DIRECTION_TURN_RIGHT, false, false};
    case Action::ADVANCE:
    case Action::RETREAT:
    case Action::SLIDE_LEFT:
    case Action::SLIDE_RIGHT:
    case Action::ROLL_LEFT:
    case Action::ROLL_RIGHT:
        return {true, facing, true, false};
    /* The keypad's /: on ahead. */
    case Action::JUMP_FORWARD:
        return {true, facing, false, true};
    /* The keypad without Num Lock: its eight directions. */
    case Action::MOVE_NORTHWEST:
        return {true, DIRECTION_NORTHWEST, false, true};
    case Action::MOVE_SOUTHEAST:
        return {true, DIRECTION_SOUTHEAST, false, true};
    case Action::MOVE_SOUTHWEST:
        return {true, DIRECTION_SOUTHWEST, false, true};
    case Action::MOVE_NORTHEAST:
        return {true, DIRECTION_NORTHEAST, false, true};
    case Action::MOVE_WEST:
        return {true, DIRECTION_WEST, false, true};
    case Action::MOVE_NORTH:
        return {true, DIRECTION_NORTH, false, true};
    case Action::MOVE_SOUTH:
        return {true, DIRECTION_SOUTH, false, true};
    case Action::MOVE_EAST:
        return {true, DIRECTION_EAST, false, true};
    /* The keypad with Num Lock: ahead, the turns, the slides and rolls. */
    case Action::NUMPAD_FORWARD:
    case Action::NUMPAD_BACK:
    case Action::NUMPAD_SLIDE_LEFT:
    case Action::NUMPAD_SLIDE_RIGHT:
    case Action::NUMPAD_ROLL_LEFT:
    case Action::NUMPAD_ROLL_RIGHT:
        return {true, facing, false, true};
    case Action::NUMPAD_TURN_LEFT:
        return {true, DIRECTION_TURN_LEFT, false, true};
    case Action::NUMPAD_TURN_RIGHT:
        return {true, DIRECTION_TURN_RIGHT, false, true};
    default:
        return {false, 0, false, false};
    }
}

/* A move key as the choice of its animation reads it: the key, its
 * action, strafe and moving (keyMove), the NPC's facing and last
 * animation, whether it stands at peace (inPeacefulStance), its run
 * (armed or not), walking rather than running (the run key, Shift, turned
 * round by the run lock), and Num Lock. */
struct MoveKey {
    KeyEvent *key;
    Action action;
    bool strafe;
    bool moving;
    char facing;
    u16 lastAnim;
    bool standing;
    u16 runAnim;
    u8 walk;
    bool numLock;
};

/* What a move key starts: an animation (ANIMS for none), a second one
 * after it (No Remorse's jump after unreadying the weapon), and whether
 * the reticle hides while the first runs. */
struct MoveStart {
    u16 anim = ANIMS;
    u16 anim2 = ANIMS;
    bool hide = false;
};

/* A step to the side nearer the key's direction toward: right, toward
 * turned a quarter counterclockwise, when toward is fewer turns clockwise
 * from facing (directions go clockwise from north), else left, a quarter
 * clockwise. */
u16 sideStep(char facing, char &toward, u16 right, u16 left)
{
    const i16 turns = facing > toward ? facing - toward : facing + 16 - toward;
    const i16 back = 16 - turns;
    if (back < turns) {
        toward = (toward - 4) & 0xf;
        return right;
    }
    toward = (toward + 4) & 0xf;
    return left;
}

/* The kneel key, a new press only (the key's repeats do nothing): kneel
 * or stand up. */
u16 kneelOrStand(Npc &npc, const KeyEvent *key)
{
    if (key->repeat != 1) {
        return ANIMS;
    }
    return (npc.isKneeling() != 0U) ? ANIM_KNEEL_END : ANIM_KNEEL_START;
}

/* The jump key ahead: No Regret's jumps forward on from a jump or a run,
 * else a quick jump; No Remorse's advances. */
u16 jumpAhead(Npc &npc, u16 lastAnim)
{
    if (!MOVE_NO_REGRET) {
        return ANIM_ADVANCE_SMALL;
    }
    return (lastAnim == ANIM_JUMP_FORWARD || npc.isRunning()) ? ANIM_JUMP_FORWARD : ANIM_QUICK_JUMP;
}

/* On the base's map the NPC only walks, runs and stands: on if it walks
 * or runs, starting if it faces the key's way, else standing. */
u16 baseMapMove(Npc &npc, const MoveKey &m, char toward)
{
    if (m.lastAnim == ANIM_WALK || npc.isStartingRun() || npc.isRunning()) {
        return (m.walk != 0U) ? ANIM_WALK : m.runAnim;
    }
    if (npc.direction() == toward) {
        return (m.walk != 0U) ? ANIM_WALK : ANIM_START_RUN;
    }
    return ANIM_STAND;
}

/* The combat step (Alt): a keypad key advances (No Regret's, kneeling
 * too) or steps to the side nearer its direction (No Remorse's); the
 * others step ahead, back or aside from the facing, or kneel. */
u16 combatStep(Npc &npc, const MoveKey &m, char &toward)
{
    if (m.moving && !m.numLock) {
        /* Every moving key but the jump. */
        if (m.action == Action::JUMP_FORWARD) {
            return ANIMS;
        }
        if (!MOVE_NO_REGRET) {
            return sideStep(m.facing, toward, ANIM_SLIDE_RIGHT, ANIM_SLIDE_LEFT);
        }
        return (npc.isKneeling() != 0U) ? ANIM_KNEELING_ADVANCE : ANIM_ADVANCE_SMALL;
    }
    toward = m.facing;
    switch (m.action) {
    case Action::MOVE_FORWARD:
    case Action::MOVE_NORTHWEST:
    case Action::NUMPAD_FORWARD:
        return (MOVE_NO_REGRET && (npc.isKneeling() != 0U)) ? ANIM_KNEELING_ADVANCE : ANIM_ADVANCE_SMALL;
    case Action::MOVE_BACK:
    case Action::MOVE_SOUTHEAST:
    case Action::NUMPAD_BACK:
        return (MOVE_NO_REGRET && (npc.isKneeling() != 0U)) ? ANIM_KNEELING_RETREAT : ANIM_RETREAT_SMALL;
    case Action::TURN_LEFT:
    case Action::MOVE_SOUTHWEST:
    case Action::NUMPAD_TURN_LEFT:
        return (MOVE_NO_REGRET && (npc.isKneeling() != 0U)) ? ANIM_KNEELING_SLIDE_LEFT : ANIM_SLIDE_LEFT;
    case Action::TURN_RIGHT:
    case Action::MOVE_NORTHEAST:
    case Action::NUMPAD_TURN_RIGHT:
        return (MOVE_NO_REGRET && (npc.isKneeling() != 0U)) ? ANIM_KNEELING_SLIDE_RIGHT : ANIM_SLIDE_RIGHT;
    case Action::KNEEL:
        return kneelOrStand(npc, m.key);
    default:
        return ANIMS;
    }
}

/* The roll key (Ctrl): a keypad key rolls forward (No Regret's) or to the
 * side nearer its direction (No Remorse's); the others roll ahead or to
 * the side, or kneel. No Remorse's up stands a kneeling NPC up, unreadies
 * the weapon of an armed one, and makes the Silencer jump. No Regret's
 * roll forward's own input (ROLL_FORWARD) is the roll key with up. */
MoveStart roll(Npc &npc, const MoveKey &m, char &toward)
{
    MoveStart start;
    if (m.moving && !m.numLock) {
        /* Every moving key but the jump. */
        if (m.action != Action::JUMP_FORWARD) {
            start.anim = MOVE_NO_REGRET ? ANIM_ROLL_FORWARD
                                        : sideStep(m.facing, toward, ANIM_KNEEL_ROLL_RIGHT, ANIM_KNEEL_ROLL_LEFT);
        }
        return start;
    }
    toward = m.facing;
    switch (m.action) {
    case Action::MOVE_FORWARD:
    case Action::MOVE_NORTHWEST:
    case Action::NUMPAD_FORWARD:
    case Action::ROLL_FORWARD:
        if (MOVE_NO_REGRET) {
            start.anim = ANIM_ROLL_FORWARD;
        } else if (npc.isKneeling() != 0U) {
            start.anim = ANIM_KNEEL_END;
        } else if (m.standing) {
            if (npc.isSilencer()) {
                start.anim = ANIM_QUICK_JUMP;
            }
        } else {
            start.anim = ANIM_UNREADY_SMALL;
            if (npc.isSilencer()) {
                start.anim2 = ANIM_QUICK_JUMP;
            }
        }
        break;
    case Action::MOVE_BACK:
    case Action::MOVE_SOUTHEAST:
    case Action::NUMPAD_BACK:
    case Action::KNEEL:
        start.anim = kneelOrStand(npc, m.key);
        break;
    case Action::TURN_LEFT:
    case Action::MOVE_SOUTHWEST:
    case Action::NUMPAD_TURN_LEFT:
        start.anim = (npc.isKneeling() != 0U) ? ANIM_KNEEL_ROLL_LEFT : ANIM_COMBAT_ROLL_LEFT;
        break;
    case Action::TURN_RIGHT:
    case Action::MOVE_NORTHEAST:
    case Action::NUMPAD_TURN_RIGHT:
        start.anim = (npc.isKneeling() != 0U) ? ANIM_KNEEL_ROLL_RIGHT : ANIM_COMBAT_ROLL_RIGHT;
        break;
    default:
        break;
    }
    return start;
}

/* A move key while walking or running: on (No Regret's jump key jumps
 * forward while running; No Remorse's runs on while walking). */
u16 keepMoving(const MoveKey &m)
{
    if (MOVE_NO_REGRET) {
        if (m.walk != 0U) {
            return ANIM_WALK;
        }
        return (m.action == Action::JUMP_FORWARD) ? ANIM_JUMP_FORWARD : m.runAnim;
    }
    if (m.walk != 0U) {
        return (m.action == Action::JUMP_FORWARD) ? m.runAnim : ANIM_WALK;
    }
    return m.runAnim;
}

/* The strafe keys: slide, advance, retreat, roll. No Remorse's stand a
 * kneeling NPC up (No Regret's move it kneeling), except the rolls, which
 * have the kneeling and standing rolls the other way round (original). */
u16 strafe(Npc &npc, Action action)
{
    switch (action) {
    case Action::SLIDE_LEFT:
        if (npc.isKneeling() != 0U) {
            return MOVE_NO_REGRET ? ANIM_KNEELING_SLIDE_LEFT : ANIM_KNEEL_END;
        }
        return ANIM_SLIDE_LEFT;
    case Action::SLIDE_RIGHT:
        if (npc.isKneeling() != 0U) {
            return MOVE_NO_REGRET ? ANIM_KNEELING_SLIDE_RIGHT : ANIM_KNEEL_END;
        }
        return ANIM_SLIDE_RIGHT;
    case Action::ADVANCE:
        if (npc.isKneeling() != 0U) {
            return MOVE_NO_REGRET ? ANIM_KNEELING_ADVANCE : ANIM_KNEEL_END;
        }
        return ANIM_ADVANCE_SMALL;
    case Action::RETREAT:
        if (npc.isKneeling() != 0U) {
            return MOVE_NO_REGRET ? ANIM_KNEELING_RETREAT : ANIM_KNEEL_END;
        }
        return ANIM_RETREAT_SMALL;
    case Action::ROLL_LEFT:
        if (MOVE_NO_REGRET) {
            return (npc.isKneeling() != 0U) ? ANIM_KNEEL_ROLL_LEFT : ANIM_COMBAT_ROLL_LEFT;
        }
        return (npc.isKneeling() != 0U) ? ANIM_COMBAT_ROLL_LEFT : ANIM_KNEEL_ROLL_LEFT;
    case Action::ROLL_RIGHT:
        if (MOVE_NO_REGRET) {
            return (npc.isKneeling() != 0U) ? ANIM_KNEEL_ROLL_RIGHT : ANIM_COMBAT_ROLL_RIGHT;
        }
        return (npc.isKneeling() != 0U) ? ANIM_COMBAT_ROLL_RIGHT : ANIM_KNEEL_ROLL_RIGHT;
    default:
        return ANIMS;
    }
}

/* The keypad with Num Lock: the jump; 7 and 9 slide (No Regret's
 * kneeling too), 1 and 3 roll; 4 and 6 turn (stand at peace); 8 walks or
 * starts a run, 2 retreats. */
u16 numLockMove(Controller &controller, Npc &npc, const MoveKey &m, char &toward)
{
    u16 anim = ANIMS;
    switch (m.action) {
    case Action::JUMP_FORWARD:
        return jumpAhead(npc, m.lastAnim);
    case Action::MOVE_WEST:
    case Action::NUMPAD_SLIDE_LEFT:
        return (MOVE_NO_REGRET && (npc.isKneeling() != 0U)) ? ANIM_KNEELING_SLIDE_LEFT : ANIM_SLIDE_LEFT;
    case Action::MOVE_NORTH:
    case Action::NUMPAD_SLIDE_RIGHT:
        return (MOVE_NO_REGRET && (npc.isKneeling() != 0U)) ? ANIM_KNEELING_SLIDE_RIGHT : ANIM_SLIDE_RIGHT;
    case Action::MOVE_SOUTH:
    case Action::NUMPAD_ROLL_LEFT:
        return (npc.isKneeling() != 0U) ? ANIM_KNEEL_ROLL_LEFT : ANIM_COMBAT_ROLL_LEFT;
    case Action::MOVE_EAST:
    case Action::NUMPAD_ROLL_RIGHT:
        return (npc.isKneeling() != 0U) ? ANIM_KNEEL_ROLL_RIGHT : ANIM_COMBAT_ROLL_RIGHT;
    case Action::MOVE_SOUTHWEST:
    case Action::NUMPAD_TURN_LEFT:
    case Action::MOVE_NORTHEAST:
    case Action::NUMPAD_TURN_RIGHT:
        if (m.standing) {
            return ANIM_STAND;
        }
        controller.turn(m.action, &toward, &anim, m.walk);
        return anim;
    case Action::MOVE_NORTHWEST:
    case Action::NUMPAD_FORWARD:
        return (m.walk != 0U) ? ANIM_WALK : ANIM_START_RUN_SMALL;
    case Action::MOVE_SOUTHEAST:
    case Action::NUMPAD_BACK:
        return ANIM_RETREAT_SMALL;
    default:
        return ANIMS;
    }
}

/* The combat stand, the reticle hidden while it runs, unless the NPC is
 * readying its weapon. */
MoveStart combatStand(u16 lastAnim)
{
    MoveStart start;
    if (lastAnim != ANIM_READY_SMALL && lastAnim != ANIM_READY_LARGE) {
        start.anim = ANIM_COMBAT_STAND_SMALL;
        start.hide = true;
    }
    return start;
}

/* The other keys, when the NPC does not face the key's way: the keypad's
 * turns (when it can walk) stand at peace, take the combat stance, or
 * walk or run that way; an NPC at peace stands (not while busy); a keypad
 * key without Num Lock takes the combat stance; the rest turn
 * (Controller::turn). */
MoveStart turnMove(Controller &controller, Npc &npc, const MoveKey &m, char &toward)
{
    MoveStart start;
    if ((cannotWalk == 0U) && (m.action == Action::MOVE_SOUTHWEST || m.action == Action::NUMPAD_TURN_LEFT ||
                               m.action == Action::MOVE_NORTHEAST || m.action == Action::NUMPAD_TURN_RIGHT)) {
        if (m.standing) {
            start.anim = ANIM_STAND;
        } else if (inCombatStance()) {
            start = combatStand(m.lastAnim);
        } else {
            start.anim = (m.walk != 0U) ? ANIM_WALK : ANIM_START_RUN_SMALL;
        }
    } else if (m.standing) {
        if (!npc.isBusy() && (m.moving || m.action == Action::TURN_LEFT || m.action == Action::TURN_RIGHT)) {
            start.anim = ANIM_STAND;
        }
    } else if (m.moving && !m.numLock) {
        start = combatStand(m.lastAnim);
    } else {
        controller.turn(m.action, &toward, &start.anim, m.walk);
    }
    return start;
}

/* The animation a pressed move key starts, by the first that applies: the
 * base's map, the roll forward's own input, the combat step (Alt), the roll
 * key (Ctrl), the kneel key, walking or running on, a strafe key, back, the
 * keypad with Num Lock, the key ahead (jump, walk or start a run), the
 * turns. */
MoveStart chooseMove(Controller &controller, Npc &npc, const MoveKey &m, char &toward)
{
    MoveStart start;
    if (npc.onBaseMap()) {
        start.anim = baseMapMove(npc, m, toward);
    } else if (m.action == Action::ROLL_FORWARD) {
        start = roll(npc, m, toward);
    } else if (keyMode(m.key, Action::COMBAT_STEP)) {
        start.anim = combatStep(npc, m, toward);
    } else if (keyMode(m.key, Action::ROLL)) {
        start = roll(npc, m, toward);
    } else if (m.action == Action::KNEEL) {
        start.anim = kneelOrStand(npc, m.key);
    } else if ((!MOVE_NO_REGRET || (npc.isKneeling() == 0U)) &&
               (m.lastAnim == ANIM_WALK || npc.isStartingRun() || npc.isRunning())) {
        start.anim = keepMoving(m);
    } else if (m.strafe) {
        start.anim = strafe(npc, m.action);
    } else if (m.action == Action::MOVE_BACK) {
        start.anim = ANIM_RETREAT_SMALL;
    } else if (m.numLock && m.moving) {
        start.anim = numLockMove(controller, npc, m, toward);
    } else if (m.facing == toward) {
        if (m.action == Action::JUMP_FORWARD) {
            start.anim = jumpAhead(npc, m.lastAnim);
        } else {
            start.anim = (m.walk != 0U) ? ANIM_WALK : ANIM_START_RUN_SMALL;
        }
    } else {
        start = turnMove(controller, npc, m, toward);
    }
    return start;
}

/* The turns' direction for anim: a step from the NPC's facing, of 16
 * directions or of 8. */
char turnToward(Npc &npc, u16 anim, char toward)
{
    if (toward != DIRECTION_TURN_RIGHT && toward != DIRECTION_TURN_LEFT) {
        return toward;
    }
    const AnimInfo info = animInfo(npc.shape(), anim);
    const i16 step = (info.sixteenDirections != 0) ? 1 : 2;
    if (toward == DIRECTION_TURN_RIGHT) {
        return (npc.direction() + step) & 0xf;
    }
    return (npc.direction() - step) & 0xf;
}

/* A busy NPC turns a step toward toward, when the key is new or the NPC
 * neither walks nor runs, and its animation lets a step start
 * (stepAllowed). Returns whether it turned. */
bool turnWhileBusy(Npc &npc, const MoveKey &m, char toward)
{
    if (m.key->repeat != 1 && (m.lastAnim == ANIM_WALK || npc.isRunning() || npc.isStartingRun())) {
        return false;
    }
    AnimPrimitive *process = static_cast<AnimPrimitive *>(findItemProcess(npc.id, ANIM_TYPE));
    if (!stepAllowed(process->anim)) {
        return false;
    }
    process->turnTo(toward);
    return true;
}

/* The animation starts, the reticle hidden while it runs if asked, and its
 * second one after it. */
void startMove(Npc &npc, const MoveStart &start, char toward)
{
    const u16 animPid = npc.doAnim(start.anim, toward, ONLY_IF_IDLE, 0);
    if (start.hide && (animPid != 0U)) {
        AnimPrimitive *started = static_cast<AnimPrimitive *>(findProcess(animPid));
        hideReticle();
        started->holdReticle();
    }
    if (start.anim2 != ANIMS) {
        npc.doAnim(start.anim2, toward, 1, 0);
    }
}

/* No Regret's start: a kneeling NPC stands up for an animation that is not
 * a kneeling one (nor the quick jump), a standing one kneels for a
 * kneeling one (not the rolls). A busy NPC facing elsewhere turns a step
 * instead (turnWhileBusy); one running rolls forward instead of its run.
 * The busy NPC's animation ends (unless it is the same one, but for the
 * rolls) and the new one starts. Returns whether the key was taken. */
bool startMoveNoRegret(Npc &npc, const MoveKey &m, MoveStart start, char toward)
{
    if (npc.isKneeling() != 0U) {
        if (start.anim != ANIM_QUICK_JUMP && !isKneelAnim(start.anim)) {
            start.anim = ANIM_KNEEL_END;
        }
    } else if (start.anim != ANIM_ROLL_FORWARD && start.anim != ANIM_COMBAT_ROLL_LEFT &&
               start.anim != ANIM_COMBAT_ROLL_RIGHT && start.anim != ANIM_KNEEL_ROLL_LEFT &&
               start.anim != ANIM_KNEEL_ROLL_RIGHT && isKneelAnim(start.anim)) {
        start.anim = ANIM_KNEEL_START;
    }
    const bool busy = npc.isBusy();
    if (busy) {
        if (npc.direction() != toward) {
            if (turnWhileBusy(npc, m, toward)) {
                return true;
            }
        } else if (start.anim == ANIM_ROLL_FORWARD && npc.isRunning()) {
            if (countItemProcesses(controlledActor, ANIM_TYPE) != 1) {
                return false;
            }
            AnimPrimitive *running = static_cast<AnimPrimitive *>(findItemProcess(controlledActor, ANIM_TYPE));
            if (running->anim == ANIM_ROLL_FORWARD) {
                return false;
            }
            running->fail(0);
            npc.doAnim(ANIM_ROLL_FORWARD, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
            return true;
        }
        const u16 current = npc.busyAnim();
        if (current != ANIMS &&
            (current != start.anim || current == ANIM_COMBAT_ROLL_LEFT || current == ANIM_COMBAT_ROLL_RIGHT ||
             current == ANIM_KNEEL_ROLL_LEFT || current == ANIM_KNEEL_ROLL_RIGHT || current == ANIM_ROLL_FORWARD)) {
            killItemProcesses(movedActor, ANIM_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
        }
    }
    startMove(npc, start, toward);
    return true;
}

/* No Remorse's start: a busy NPC only turns, a step at a time
 * (turnWhileBusy). Returns whether the key was taken. */
bool startMoveNoRemorse(Npc &npc, const MoveKey &m, const MoveStart &start, char toward)
{
    if (npc.isBusy()) {
        return npc.direction() != toward && turnWhileBusy(npc, m, toward);
    }
    startMove(npc, start, toward);
    return true;
}

/* Whether a move key is still held, of those the let-go key checks
 * (original: the slides and rolls to the left and the keypad's moves
 * without Num Lock to the sides are not among them). */
bool moveKeyHeld()
{
    return (actionHeld(Action::MOVE_FORWARD) != 0U) || (actionHeld(Action::MOVE_BACK) != 0U) ||
           (actionHeld(Action::TURN_LEFT) != 0U) || (actionHeld(Action::TURN_RIGHT) != 0U) ||
           (actionHeld(Action::MOVE_NORTHWEST) != 0U) || (actionHeld(Action::NUMPAD_FORWARD) != 0U) ||
           (actionHeld(Action::MOVE_SOUTHEAST) != 0U) || (actionHeld(Action::NUMPAD_BACK) != 0U) ||
           (actionHeld(Action::MOVE_SOUTHWEST) != 0U) || (actionHeld(Action::NUMPAD_TURN_LEFT) != 0U) ||
           (actionHeld(Action::MOVE_NORTHEAST) != 0U) || (actionHeld(Action::NUMPAD_TURN_RIGHT) != 0U) ||
           (actionHeld(Action::ADVANCE) != 0U) || (actionHeld(Action::ROLL_RIGHT) != 0U) ||
           (actionHeld(Action::RETREAT) != 0U) || (actionHeld(Action::SLIDE_RIGHT) != 0U) ||
           (actionHeld(Action::JUMP_FORWARD) != 0U);
}

/* A move's key (the arrows', Home's, Page Up's, End's, Page Down's and the
 * keypad's /), or a / of its own that is no action's: the original stops
 * on the scan code. */
bool stopsMove(const KeyEvent *key, Action action)
{
    return action == Action::MOVE_FORWARD || action == Action::MOVE_NORTHWEST || action == Action::NUMPAD_FORWARD ||
           action == Action::MOVE_BACK || action == Action::MOVE_SOUTHEAST || action == Action::NUMPAD_BACK ||
           action == Action::TURN_LEFT || action == Action::MOVE_SOUTHWEST || action == Action::NUMPAD_TURN_LEFT ||
           action == Action::TURN_RIGHT || action == Action::MOVE_NORTHEAST || action == Action::NUMPAD_TURN_RIGHT ||
           action == Action::ADVANCE || action == Action::MOVE_WEST || action == Action::NUMPAD_SLIDE_LEFT ||
           action == Action::ROLL_RIGHT || action == Action::MOVE_NORTH || action == Action::NUMPAD_SLIDE_RIGHT ||
           action == Action::RETREAT || action == Action::MOVE_SOUTH || action == Action::NUMPAD_ROLL_LEFT ||
           action == Action::SLIDE_RIGHT || action == Action::MOVE_EAST || action == Action::NUMPAD_ROLL_RIGHT ||
           action == Action::JUMP_FORWARD || (action == Action::NONE && key->scan == SCAN_SLASH);
}

/* The animation that stops a move: standing on the base's map, kneeling,
 * stopping a run (No Regret's NPCs but the Silencer ready instead), the
 * stand or the combat stand (not after a retreat, but the Silencer's);
 * none while sliding. */
u16 stopAnim(Npc &npc, u16 lastAnim, bool standing)
{
    if (npc.onBaseMap()) {
        return ANIM_STAND;
    }
    if (npc.isKneeling() != 0U) {
        return ANIM_KNEELING_SMALL;
    }
    if (npc.isRunning() || npc.isStartingRun()) {
        return (!MOVE_NO_REGRET || npc.isSilencer()) ? ANIM_STOP_RUN_SMALL : ANIM_READY_SMALL;
    }
    if (npc.isSliding()) {
        return ANIMS;
    }
    if (standing) {
        return ANIM_STAND;
    }
    if (npc.isSilencer() || (lastAnim != ANIM_RETREAT_SMALL && lastAnim != ANIM_RETREAT_LARGE)) {
        return ANIM_COMBAT_STAND_SMALL;
    }
    return ANIM_STAND;
}

/* No Regret's: whether the controlled NPC already stops a run. */
bool stoppingRun()
{
    for (auto *running = static_cast<AnimPrimitive *>(findItemProcess(controlledActor, ANIM_TYPE)); running != nullptr;
         running = static_cast<AnimPrimitive *>(nextItemProcess())) {
        if (running->anim == ANIM_STOP_RUN_SMALL || running->anim == ANIM_STOP_RUN_LARGE) {
            return true;
        }
    }
    return false;
}

/* A move key let go when no other is held: the NPC stops (stopAnim), not
 * in No Regret while it stops a run already. Returns whether the key was
 * taken. */
bool stopMove(Npc &npc, const KeyEvent *key, Action action, u16 lastAnim, bool standing)
{
    if (moveKeyHeld() || !stopsMove(key, action)) {
        return false;
    }
    const u16 anim = stopAnim(npc, lastAnim, standing);
    if (anim == ANIMS) {
        return false;
    }
    if (!MOVE_NO_REGRET || !stoppingRun()) {
        npc.doAnim(anim, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
    }
    return true;
}
}

/* The move keys: the numeric keypad walks, runs, strafes and rolls, with
 * Alt and Ctrl for the combat moves (chooseMove); a released key stops the
 * NPC (stopMove). Returns whether the key was taken. */
bool Controller::move(KeyEvent *key, Action action)
{
    Npc npc(movedActor);
    if ((worldActive == 0U) || !npc.isValid() || (npc.isDead() != 0U) ||
        (npc.isSilencer() && (controlledNpc.held != 0U))) {
        return false;
    }
    const u16 lastAnim = npc.lastAnimSet();
    const char facing = npc.direction();
    const bool standing = inPeacefulStance();
    const u16 runAnim = (npc.activeWeapon() != 0U) ? ANIM_COMBAT_RUN_SMALL : ANIM_RUN;
    u8 walk = 0;
    if (keyMode(key, Action::RUN_LOCK)) {
        if (keyMode(key, Action::RUN)) {
            walk = 1;
        }
    } else if (!keyMode(key, Action::RUN)) {
        walk = 1;
    }
    if (key->pressed == 0U) {
        return stopMove(npc, key, action, lastAnim, standing);
    }
    const KeyMove keyed = keyMove(action, facing);
    if (!keyed.known) {
        return false;
    }
    const bool numLock = (key->shift & KEY_NUM_LOCK) != 0;
    const MoveKey m = {key, action, keyed.strafe, keyed.moving, facing, lastAnim, standing, runAnim, walk, numLock};
    char toward = keyed.toward;
    const MoveStart start = chooseMove(*this, npc, m, toward);
    if (start.anim == ANIMS) {
        return false;
    }
    toward = turnToward(npc, start.anim, toward);
    if (MOVE_NO_REGRET) {
        return startMoveNoRegret(npc, m, start, toward);
    }
    return startMoveNoRemorse(npc, m, start, toward);
}

void Controller::keyboard(KeyEvent *pressed, KeyEvent *bound, Action action)
{
    Npc npc(movedActor);
    const u16 lastAnim = npc.lastAnimSet();
    const bool standing = inPeacefulStance();
    /* The other devices' keys (actionKey) type no cheat. */
    if (pressed != nullptr) {
        typeCheat(pressed);
        if (waitOpened()) {
            /* A cheat's box is up: the key goes on after it, its events
             * copied, the caller's being gone by then. */
            const bool hasBound = bound != nullptr;
            giveRest([this, pressedKey = *pressed, hasBound, boundKey = hasBound ? *bound : KeyEvent(), action, npc,
                      lastAnim, standing]() mutable {
                keyboardRest(&pressedKey, hasBound ? &boundKey : nullptr, action, npc, lastAnim, standing);
                return false;
            });
            return;
        }
    }
    keyboardRest(pressed, bound, action, npc, lastAnim, static_cast<u8>(standing));
}

void Controller::keyboardRest(KeyEvent *pressed, KeyEvent *bound, Action action, Npc npc, u16 lastAnim, u8 standing)
{
    bool handled;

    if (keysEnabled == 0U) {
        return;
    }
    if (((pressed != nullptr) && cheatKey(pressed)) || (bound == nullptr)) {
        return;
    }
    KeyEvent key = *bound;
    if (handleKey(&key, action)) {
        return;
    }
    if ((worldActive != 0U) && npc.isValid() && (npc.isDead() == 0U) &&
        (!SHAPE_5D6_IS_DEAD || npc.shape() != SHAPE_FROZEN) && (!npc.isSilencer() || (controlledNpc.held == 0U))) {
        if ((firing != 0U) && action == Action::FIRE && (key.pressed != 0U)) {
            return;
        }
        handled = move(&key, action);
        if (!handled && (key.pressed != 0U)) {
            switch (action) {
            case Action::KEYPAD_USE:
                if (npc.isSilencer() && key.repeat == 1 && (key.extended == 0U)) {
                    NewGump *top = modals[modalTop];
                    top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'u')));
                }
                break;
            case Action::KEYPAD_NEXT_WEAPON:
                if ((key.extended == 0U) && key.repeat == 1) {
                    nextWeapon();
                }
                break;
            case Action::KEYPAD_NEXT_ITEM:
                if (npc.isSilencer() && key.repeat == 1) {
                    nextInventoryItem();
                }
                break;
            case Action::KEYPAD_SELECT:
                if (npc.isSilencer() && key.repeat == 1) {
                    NewGump *top = modals[modalTop];
                    top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 's')));
                }
                break;
            case Action::READY_WEAPON:
                /* The right button's double click's, from any input
                 * (bindings.h). */
                if (key.repeat == 1) {
                    readyWeapon();
                }
                break;
            case Action::JUMP:
                /* The silencer jumps; No Remorse's with a weapon drawn
                 * unreadies it first (JUMP_UNREADIES_WEAPON). */
                if (npc.isSilencer()) {
                    if (!npc.isBusy()) {
                        if (JUMP_UNREADIES_WEAPON && (standing != 0U)) {
                            npc.doAnim(ANIM_QUICK_JUMP, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                        } else if (lastAnim == ANIM_JUMP_FORWARD || npc.isRunning()) {
                            npc.doAnim(ANIM_JUMP_FORWARD, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                        } else if (!JUMP_UNREADIES_WEAPON) {
                            npc.doAnim(ANIM_QUICK_JUMP, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                        } else if (npc.doAnim(ANIM_UNREADY_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0) != 0U) {
                            npc.doAnim(ANIM_QUICK_JUMP, DIRECTION_CURRENT, 1, 0);
                        }
                    } else if (countItemProcesses(controlledActor, ANIM_TYPE) == 1) {
                        AnimPrimitive *running =
                            static_cast<AnimPrimitive *>(findItemProcess(controlledActor, ANIM_TYPE));
                        if ((running != nullptr) && npc.isRunning() && running->anim != ANIM_JUMP_FORWARD) {
                            running->fail(0);
                            npc.doAnim(ANIM_JUMP_FORWARD, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                        }
                    }
                }
                break;
            case Action::FIRE:
                if (!npc.onBaseMap()) {
                    if (key.repeat == 1) {
                        firePressed = systemTimer->time();
                        firedSincePress = 0;
                    }
                    /* No Regret's breaks into the busy NPC's animation to fire
                     * (FIRE_INTERRUPTS_BUSY). */
                    if (npc.isBusy()) {
                        if (FIRE_INTERRUPTS_BUSY && (npc.ammo() != 0U) && lastAnim != ANIM_FIRE_SMALL &&
                            lastAnim != ANIM_FIRE_LARGE && lastAnim != ANIM_KNEEL_FIRE &&
                            lastAnim != ANIM_KNEEL_FIRE_SMALL && lastAnim != ANIM_KNEEL_FIRE_LARGE &&
                            lastAnim != ANIM_BRIGHT_KNEEL_FIRE && lastAnim != ANIM_BRIGHT_FIRE_LARGE) {
                            const u16 busy = npc.busyAnim();
                            if (busy != ANIMS) {
                                killItemProcesses(movedActor, ANIM_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
                                if (npc.isActing()) {
                                    fire(1);
                                } else {
                                    npc.doAnim(ANIM_READY_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                                }
                            }
                        }
                    } else if (npc.isActing()) {
                        fire(1);
                    } else {
                        npc.doAnim(ANIM_READY_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
                    }
                }
                break;
            default:
                /* The PIX cheat's Backspace, as the player pressed it (the
                 * cheats' keys are not bound). */
                if (MORE_CHEAT_CODES && (pressed != nullptr) && pressed->scan == SCAN_BACKSPACE && (pictures != 0U)) {
                    screenshot();
                }
                break;
            }
        } else if (!npc.onBaseMap()) {
            switch (action) {
            case Action::FIRE:
                fire(0);
                break;
            default:
                break;
            }
        }
    }
}

/* The cheat codes (1148:34d2, No Remorse's 1130:2b0a): each key goes on with
 * the scan codes typed so far, or starts them again. JASSICA16 (No Remorse's
 * cheat) turns No Remorse's cheat keys on and off; No Regret's says the
 * cheats were changed, brings the Silencer back and jumps to map E6h with the
 * Christmas songs (JASSICA_JOKE), and LOOSECANNON16 turns its cheat keys on
 * and off (MORE_CHEAT_CODES). */
/* typeCheat's codes in their turn (the port's): a code's box is a wait,
 * after which the codes after it are checked with the same key. */
enum class CheatCode : u8 { JASSICA, LOOSE_CANNON, PIX, MEMORY, VIDTEST };

namespace {
void typeCheatFrom(KeyEvent key, CheatCode code);

/* The video test's movies again until a key stops one (waits of the
 * steps), then the game's screen back. */
void videoTestEnd()
{
    gamePalette->apply(-1, -1);
    gameCamera->repaint();
}

void videoTest()
{
    if (movieAborted != 0U) {
        videoTestEnd();
        return;
    }
    openMovie(MovieRequest{.name = "ORIGIN", .mayDouble = false, .restorePalette = false}, [](bool /*cancelled*/) {
        if (movieAborted != 0U) {
            videoTestEnd();
            return;
        }
        openTimerDelay(30, []() {
            openMovie(MovieRequest{.name = "ANIM01", .restorePalette = false}, [](bool /*cancelled*/) { videoTest(); });
            return false;
        });
    });
}

/* A cheat's message box; the codes from next are checked after it. */
void cheatBox(const char *text, KeyEvent key, CheatCode next)
{
    openModalGump(new MessageBoxGump(NewGumpId(), nullptr, text, guiColors->face), static_cast<u32>(-1),
                  [key, next](s32 /*result*/) { typeCheatFrom(key, next); });
}
}

void typeCheat(KeyEvent *key)
{
    if (key->pressed != 0U) {
        typeCheatFrom(*key, CheatCode::JASSICA);
    }
}

namespace {
void typeCheatFrom(KeyEvent key, CheatCode code)
{
    if (code == CheatCode::JASSICA) {
        if (jassicaCheat[jassicaTyped] == key.scan) {
            jassicaTyped++;
        } else {
            jassicaTyped = 0;
            if (jassicaCheat[jassicaTyped] == key.scan) {
                jassicaTyped++;
            }
        }
        if (jassicaCheat[jassicaTyped] == 0) {
            jassicaTyped = 0;
            if (JASSICA_JOKE) {
                stopMusic();
                playEffect(SOUND_OLD_CHEAT_JOKE);
                christmasMusic = 1;
                christmasForced = 1;
                openModalGump(
                    new MessageBoxGump(NewGumpId(), nullptr, "Of course we changed the cheats...duh.", guiColors->face),
                    static_cast<u32>(-1), [key](s32 /*result*/) {
                        controlledNpc.held = 0;
                        controlledActor = 1;
                        controlledNpc.standUp();
                        controlledNpc.setShape(1);
                        controlledNpc.clearDead();
                        controlledNpc.setHp(npcTypes[npcType(Item(controlledActor).shape())].maxHp);
                        updateStatus(STATUS_HEALTH);
                        startMapJump(0xe6);
                        playSong(0x45);
                        christmasForced = 0;
                        typeCheatFrom(key, CheatCode::LOOSE_CANNON);
                    });
                return;
            }
            cheatTyped = 1;
            cheatsOn = cheatsEnabled = static_cast<u8>(cheatsEnabled == 0U);
            playEffect(SOUND_CHEATS_TOGGLED);
            if (cheatsEnabled != 0U) {
                cheatBox("Cheats are now active.", key, CheatCode::LOOSE_CANNON);
            } else {
                cheatBox("Cheats are now inactive.", key, CheatCode::LOOSE_CANNON);
            }
            return;
        }
        code = CheatCode::LOOSE_CANNON;
    }
    if (!MORE_CHEAT_CODES) {
        return;
    }
    if (code == CheatCode::LOOSE_CANNON) {
        if (looseCannonCheat[looseCannonTyped] == key.scan) {
            looseCannonTyped++;
        } else {
            looseCannonTyped = 0;
            if (looseCannonCheat[looseCannonTyped] == key.scan) {
                looseCannonTyped++;
            }
        }
        if (looseCannonCheat[looseCannonTyped] == 0) {
            looseCannonTyped = 0;
            cheatTyped = 1;
            cheatsOn = cheatsEnabled = static_cast<u8>(cheatsEnabled == 0U);
            controlledNpc.held = 0;
            playEffect(SOUND_CHEAT_CODE);
            if (cheatsEnabled != 0U) {
                if (isChristmas()) {
                    cheatBox("Merry Christmas, CHEATER!", key, CheatCode::PIX);
                } else {
                    cheatBox("Cheats are now active.", key, CheatCode::PIX);
                }
            } else {
                cheatBox("Cheats are now inactive.", key, CheatCode::PIX);
            }
            return;
        }
        code = CheatCode::PIX;
    }
    if (code == CheatCode::PIX) {
        if (pixCheat[pixTyped] == key.scan) {
            pixTyped++;
        } else {
            pixTyped = 0;
            if (pixCheat[pixTyped] == key.scan) {
                pixTyped++;
            }
        }
        if (pixCheat[pixTyped] == 0) {
            pixTyped = 0;
            pictures = static_cast<u8>(pictures == 0U);
            playEffect(SOUND_CHEAT_CODE);
            if (pictures != 0U) {
                cheatBox("Pix active - backspace will take pictures.", key, CheatCode::MEMORY);
            } else {
                cheatBox("Picture taking capability disabled.", key, CheatCode::MEMORY);
            }
            return;
        }
        code = CheatCode::MEMORY;
    }
    if (code == CheatCode::MEMORY) {
        if (memoryCheat[memoryTyped] == key.scan) {
            memoryTyped++;
        } else {
            memoryTyped = 0;
            if (memoryCheat[memoryTyped] == key.scan) {
                memoryTyped++;
            }
        }
        if (memoryCheat[memoryTyped] == 0) {
            memoryTyped = 0;
            memoryProfiler = static_cast<u8>(memoryProfiler == 0U);
            playEffect(SOUND_CHEAT_CODE);
            if (memoryProfiler != 0U) {
                cheatBox("Memory profiler activated.", key, CheatCode::VIDTEST);
            } else {
                cheatBox("Memory profiler deactivated.", key, CheatCode::VIDTEST);
            }
            return;
        }
    }
    if (vidtestCheat[vidtestTyped] == key.scan) {
        vidtestTyped++;
    } else {
        vidtestTyped = 0;
        if (vidtestCheat[vidtestTyped] == key.scan) {
            vidtestTyped++;
        }
    }
    if (vidtestCheat[vidtestTyped] == 0) {
        vidtestTyped = 0;
        playEffect(SOUND_CHEAT_CODE);
        openModalGump(new MessageBoxGump(NewGumpId(), nullptr, "Video test activated.", guiColors->face),
                      static_cast<u32>(-1), [](s32 /*result*/) {
                          stopMusic();
                          movieAborted = 0;
                          videoTest();
                      });
    }
}
}
