/* ..\U9UI\WORLD.C (Code2, No Regret's 1008:0000-3f04, No Remorse's
 * 13e8:0000-4029; the addresses are No Regret's): the world's gump, its keys
 * and menu, the help, the weapons' keys, freezing the world and the
 * intrinsics' calls, the games' differences as each game's values
 * (world_game.h, npc_game.h). See world.h and help.h. */
#include "gamestate.h"
#include <cstdio>
#include <memory>
#include "world.h"
#include "widesides.h"
#include "waits.h"
#include "world_game.h"
#include "mapjump_game.h"
#include "shapebtn.h"
#include "wezzy.h"
#include "compgump.h"
#include "bindings.h"
#include "devices.h"
#include "alert.h"
#include "animprim.h"
#include "bmp.h"
#include "camera.h"
#include "compgump.h"
#include "control.h"
#include "cprotmem.h"
#include "credits.h"
#include "crusys.h"
#include "cycle.h"
#include "desc.h"
#include "dialog.h"
#include "distance.h"
#include "dispatch.h"
#include "display.h"
#include "dlist.h"
#include "dlitem.h"
#include "dragitem.h"
#include "dsfxman.h"
#include "dtable.h"
#include "egg.h"
#include "explode.h"
#include "fade.h"
#include "filespec.h"
#include "flag.h"
#include "gamefile.h"
#include "help.h"
#include "gamepal.h"
#include "gametime.h"
#include "itemcach.h"
#include "itemfind.h"
#include "joydrv.h"
#include "kernel.h"
#include "keyboard.h"
#include "keypad.h"
#include "loadsave.h"
#include "mainmenu.h"
#include "mapjump.h"
#include "msgbox.h"
#include "mousectl.h"
#include "music.h"
#include "npc.h"
#include "priority.h"
#include "kprocess.h"
#include "savegame.h"
#include "scrit.h"
#include "settings.h"
#include "shapedlg.h"
#include "snap.h"
#include "sndcache.h"
#include "sndmgr.h"
#include "sndsrc.h"
#include "spmgr.h"
#include "status.h"
#include "system.h"
#include "tracker.h"
#include "u9mouse.h"
#include "random.h"
#include "utility.h"
#include "viewport.h"
#include "yamm.h"
#include "shapes.h"
#include "screen.h"

/* This file's: calls a list of intrinsics while the world is not active;
 * nothing calls it. */
void callIntrinsics();

namespace {
/* Nothing reads or writes them (1480:0098). */
[[maybe_unused]] u8 unknown0098[3] = {0, 0, 0};
}

u8 &mouseTurn = gameGlobal<u8>(0);
u8 &selectingItems = gameGlobal<u8>(0);
i16 &mouseTurnAmount = gameGlobal<i16>(1);
/* H turns the hack mover on and off: the mouse moves items. */
u8 &hackMover = gameGlobal<u8>(0);
/* Intrinsics 070 and 071: while it is set the keys do little. */
u8 &stasis = gameGlobal<u8>(0);
/* No Remorse's Ctrl-Q with the cheats turns it on and off ("CD TRANSFER
 * DISPLAY ACTIVE."); while it is on, ShapeHandler::getFrame shows each shape
 * it copies from the CD and waits for a key. */
u8 &cdTransferDisplay = gameGlobal<u8>(0);

namespace {
u8 &crusaderTeleporting = gameGlobal<u8>(0);
}
i16 &mouseSpeed = gameSetting<i16>(2);

/* Shows the credits, then starts again from the start menu. No Remorse's
 * leaves the status, the screen, the keys and the process as they are
 * (startGame's menu clears them after). */
void CreditsProcess::run()
{
    if (CREDITS_END_TIDILY) {
        updateInventoryDisplay();
    }
    stopMusic();
    clearSongs();
    atStartMenu = 1;
    gameMouse->hide();
    jumpToBlack();
    /* The port's wide screen: the credits in their palette, the world's
     * sides covered (widesides.h). */
    coverWideSides();
    /* The start again after the credits. */
    openCredits([this]() {
        if (CREDITS_END_TIDILY) {
            jumpToBlack();
            flushKeys();
            gumpKeys.clear();
            terminate(0);
        }
        startGame();
        return false;
    });
}

/* Draws the status display's parts again (1008:005d, 0B6 I_updateInventoryUI;
 * No Regret only). */
void updateInventoryDisplay()
{
    updateStatus(STATUS_ITEM);
    updateStatus(STATUS_WEAPON);
    updateStatus(STATUS_AMMO);
    updateStatus(STATUS_HEALTH);
    updateStatus(STATUS_ENERGY);
}

/* Intrinsic 083 (1008:0090). */
void suppressMovieRestore()
{
    movieRestoreSuppressed = 1;
}

void allowMovieRestore()
{
    movieRestoreSuppressed = 0;
}

/* Starts the end credits and gives the process's pid (1008:009c, 145
 * I_playCredits); No Remorse's usecode does not read it. */
u16 playCredits()
{
    CreditsProcess *credits = new CreditsProcess;

    return credits->getPid();
}

/* The stasis: the keys are held while it is set (1008:00e7; 070
 * I_setCruStasis). */
void setStasis()
{
    stasis = 1;
}

/* The stasis ends, and the keys pressed during it go (1008:00ed; 071
 * I_clrCruStasis). No Remorse's original kept them, to come in after
 * it. */
void clearStasis()
{
    stasis = 0;
    flushKeys();
}

/* The controlled NPC gives up one of its items of item's shape: a weapon
 * goes, anything else loses one of its quantity (1008:0b7f, 0B5
 * I_removeItem). No Remorse's usecode does not call it (its own lowered the
 * family's quantity, Item::familyQuality, which is 0 but for quantities:
 * such an item went whole). */
bool removeInventoryItem(u16 item)
{
    bool result = false;

    if (item == 0U) {
        return result;
    }
    Item it(item);
    if (it.isValid()) {
        if (controlledNpc.findItem(shapeOf(it.id)) == 0U) {
            result = false;
        } else if (it.family() == FAMILY_WEAPON) {
            const i16 slot = controlledNpc.slotOf(it.id);
            if (slot != 0) {
                nextWeapon();
                it.destroy();
                controlledNpc.clearSlot(slot);
                result = true;
            }
        } else {
            const i16 slot = controlledNpc.slotOf(it.id);
            if (slot != 0) {
                it.setQuality(it.quality() - 1);
                if (it.quality() == 0) {
                    it.destroy();
                    controlledNpc.clearSlot(slot);
                }
                result = true;
            }
        }
    }
    return result;
}

/* The controlled NPC takes its previous weapon (1008:0c8e, Q). The weapon
 * slots are 1-12, the inventory slots 21-44. After a new weapon the NPC
 * takes the first frame of its kneeling (2Eh, 2Fh) or standing (4, 25h)
 * animation. */
void previousWeapon()
{
    i16 start = controlledNpc.activeWeaponSlot();

    if (WEAPON_KEYS_SILENCER_ONLY && controlledActor != 1) {
        return;
    }
    if (start == 0) {
        return;
    }
    start--;
    if (start < 1) {
        start = WEAPON_SLOTS.last;
    }
    i16 slot = start;
    while (controlledNpc.equipment(slot) == 0U) {
        slot--;
        if (slot < 1) {
            slot = WEAPON_SLOTS.last;
        }
        if (start == slot) {
            slot = 0;
            break;
        }
    }
    if (slot == 0) {
        return;
    }
    controlledNpc.setActiveWeapon(slot);
    updateStatus(STATUS_WEAPON);
    updateStatus(STATUS_AMMO);
    if (!PREVIOUS_WEAPON_SETS_STANCE) {
        return;
    }
    i16 anim = ANIMS;
    if (controlledNpc.isKneeling() != 0U) {
        if (controlledNpc.usesSmallAnims()) {
            anim = ANIM_KNEELING_SMALL;
        } else {
            anim = ANIM_KNEELING_LARGE;
        }
    } else if (!inPeacefulStance()) {
        if (controlledNpc.usesSmallAnims()) {
            anim = ANIM_COMBAT_STAND_SMALL;
        } else {
            anim = ANIM_COMBAT_STAND_LARGE;
        }
    }
    if (anim != ANIMS && !controlledNpc.isBusy()) {
        setFirstFrame(1, anim);
    }
}

void nextWeapon()
{
    i16 start = controlledNpc.activeWeaponSlot();

    if (WEAPON_KEYS_SILENCER_ONLY && controlledActor != 1) {
        return;
    }
    if (start == 0) {
        return;
    }
    start++;
    if (start > WEAPON_SLOTS.last) {
        start = 1;
    }
    i16 slot = start;
    while (controlledNpc.equipment(slot) == 0U) {
        slot++;
        if (slot > WEAPON_SLOTS.last) {
            slot = 1;
        }
        if (start == slot) {
            slot = 0;
            break;
        }
    }
    if (slot == 0) {
        return;
    }
    controlledNpc.setActiveWeapon(slot);
    updateStatus(STATUS_WEAPON);
    updateStatus(STATUS_AMMO);
    i16 anim = ANIMS;
    if (controlledNpc.isKneeling() != 0U) {
        if (controlledNpc.usesSmallAnims()) {
            anim = ANIM_KNEELING_SMALL;
        } else {
            anim = ANIM_KNEELING_LARGE;
        }
    } else if (!inPeacefulStance()) {
        if (controlledNpc.usesSmallAnims()) {
            anim = ANIM_COMBAT_STAND_SMALL;
        } else {
            anim = ANIM_COMBAT_STAND_LARGE;
        }
    }
    if (anim != ANIMS && !controlledNpc.isBusy()) {
        setFirstFrame(1, anim);
    }
}

namespace {
/* Whether the status display showed; nothing reads it (1480:84d6). */
u8 statusWasShown;
}

/* Stops the game; pointer shows the game's mouse pointer (1008:0f68). No
 * Remorse's takes no argument and always shows it (13e8:0e7d). */
void freezeWorld(u8 pointer)
{
    pauseEffects();
    statusWasShown = statusShown;
    setStatusShown(0);
    freezeSprites();
    pickupsFrozen = 1;
    keysEnabled = 0;
    if (pointer != 0U) {
        gameMouse->show();
        gameMouse->resetShape(GAME_POINTER);
    } else {
        gameMouse->hide();
    }
    worldFrozen = 1;
    gameCamera->repaint();
    if (worldActive != 0U) {
        gameCamera->run();
    }
}

/* The status display comes back whatever freezeWorld saved (the original's). */
void thawWorld(u8 pointer)
{
    if (THAW_REDRAWS_SPRITES) {
        redrawSprites();
    }
    thawSprites();
    pickupsFrozen = 0;
    setStatusShown(1);
    worldFrozen = 0;
    keysEnabled = 1;
    if (pointer != 0U) {
        gameMouse->popShape();
        gameMouse->hide();
    }
    resetKeyboardState();
    hackMover = 0;
    resumeEffects();
    if (THAW_ENDS_TELEPORT) {
        clearTeleporting();
    }
}

/* Calls every intrinsic above when worldActive is clear (1008:0f26). The body
 * never runs, the original's: the only caller, unknown1047, calls it only while
 * worldActive is set. */
void callIntrinsics()
{
    if (worldActive == 0) {
        clearTeleporting();
        setTeleporting();
        isTeleporting();
        suppressMovieRestore();
        allowMovieRestore();
        playCredits();
        setStasis();
        clearStasis();
        animationsOff();
        showKeypad(0);
        if (INTRINSICS_SHOP) {
            showWeaselShop(0);
        }
        showComputerText(INTRINSICS_TEXT);
    }
}

void avatarMoved()
{
    WorldGump *world = static_cast<WorldGump *>(modals[modalTop]);

    world->clearSelection();
}

/* Flushes the keys (1008:0123, 06B ResetKeyboardState). */
void resetKeyboardState()
{
    gumpKeys.clear();
    flushKeys();
}

/* The animations are off in crusader.cfg (1008:0134, 06A): the port's
 * choice and hold apart (gametime.h's animationsStopped). */
i16 animationsOff()
{
    return static_cast<i16>(animationsHeld == 0 && animationsStopped == 0);
}

/* The tests of pickUp without the picking up; nothing calls it (1008:0143). The
 * shapes pickUp treats apart: SHAPE_CREDITS adds its quality to the one the NPC
 * has (canPickUp keeps the sum under 60000), SHAPE_KEYCARD is a keycard (the
 * low byte of its quality), the batteries (a better one replaces the
 * Silencer's, else it gives mana), the ionic, plasma and graviton shields;
 * SHAPE_SPIDER_BOMB stacks to 20 and the other inventory items to 10. */
bool canPickUp(u16 item)
{
    bool result = false;

    if (item == 0U) {
        return result;
    }
    const Item it(item);
    if (it.isValid()) {
        if (it.shape() == SHAPE_CREDITS) {
            const u16 other = controlledNpc.findItem(it.shape());
            if (other != 0U) {
                const Item stack(other);
                if (static_cast<u16>(stack.familyQuality() + it.familyQuality()) < 60000) {
                    result = true;
                }
            } else {
                const i16 slot = controlledNpc.packSlot();
                if (slot != 0) {
                    result = true;
                }
            }
        } else if (it.family() == FAMILY_WEAPON) {
            const i16 slot = controlledNpc.weaponSlot();
            if (slot != 0) {
                result = true;
            }
        } else if (it.family() == FAMILY_AMMO) {
            const u16 other = controlledNpc.findItem(it.shape());
            if (other != 0U) {
                const Item stack(other);
                if (stack.quality() < 20) {
                    result = true;
                }
            } else {
                const i16 slot = controlledNpc.utilitySlot();
                if (slot != 0) {
                    result = true;
                }
            }
        } else if (it.family() == FAMILY_BOMB || it.family() == FAMILY_INVENTORY) {
            const u16 shape = it.shape();
            if (shape == SHAPE_KEYCARD) {
                result = true;
            } else if (shape == SHAPE_CHEMICAL_BATTERY || shape == SHAPE_FUSION_BATTERY ||
                       shape == SHAPE_FISSION_BATTERY) {
                result = true;
            } else {
                const u16 other = controlledNpc.findItem(it.shape());
                if (other != 0U) {
                    const Item stack(other);
                    const u16 kind = stack.shape();
                    if (kind == SHAPE_IONIC_SHIELD || kind == SHAPE_PLASMA_SHIELD) {
                        result = true;
                    } else if (kind == SHAPE_GRAVITON_SHIELD) {
                        result = true;
                    } else if (kind == SHAPE_SPIDER_BOMB) {
                        if (stack.quality() < 20) {
                            result = true;
                        }
                    } else if (stack.quality() < 10) {
                        result = true;
                    }
                } else {
                    const i16 slot = controlledNpc.packSlot();
                    if (slot != 0) {
                        result = true;
                    }
                }
            }
        }
    }
    return result;
}

namespace {
/* pickUp's steps: each gives it (item) to the controlled NPC, shown among
 * the pickups on the screen with flag, and returns whether it took it. */

/* The credits: onto the credits it has, else into a pack slot (chosen). */
bool pickUpCredits(Item &it, u16 item, u8 flag)
{
    const u16 other = controlledNpc.findItem(it.shape());
    if (other != 0U) {
        const i16 quality = it.familyQuality();
        Item stack(other);
        stack.setQuality(quality + stack.quality());
        if (flag != 0U) {
            addPickup(item);
        }
        it.destroy();
        updateStatus(STATUS_ITEM);
        return true;
    }
    const u16 slot = controlledNpc.packSlot();
    if (slot == 0U) {
        return false;
    }
    it.push();
    it.popToEnd(controlledNpc.id);
    it.setFrame(0);
    controlledNpc.setEquip(slot, item);
    controlledNpc.setActiveSlot3(slot);
    updateStatus(STATUS_ITEM);
    if (flag != 0U) {
        addPickup(item);
    }
    return true;
}

/* A weapon it has none of, into a weapon slot: loaded with a clip (empty
 * without ammunition), the active weapon when there is none. */
bool pickUpWeapon(Item &it, u16 item, u8 flag)
{
    if (controlledNpc.findItem(shapeOf(it.id)) != 0U) {
        return false;
    }
    const u16 slot = controlledNpc.weaponSlot();
    if (slot == 0U) {
        return false;
    }
    const u16 shape = it.shape();
    if (ammoShape(shape) != 0U) {
        const u16 rounds = ammoTypes[ammoIndex(ammoShape(shape))].clipSize;
        it.setQuality(rounds);
    } else {
        it.setQuality(0);
    }
    it.push();
    it.popToEnd(controlledNpc.id);
    controlledNpc.setEquip(slot, item);
    if (controlledNpc.activeWeapon() == 0U) {
        controlledNpc.setActiveWeapon(slot);
    }
    if (flag != 0U) {
        addPickup(item);
    }
    updateStatus(STATUS_WEAPON);
    updateStatus(STATUS_AMMO);
    return true;
}

/* Ammunition: one more clip on the stack it has (at most 20), else into a
 * utility slot. */
bool pickUpAmmo(Item &it, u16 item, u8 flag)
{
    const u16 other = controlledNpc.findItem(it.shape());
    if (other != 0U) {
        Item stack(other);
        if (stack.quality() >= 20) {
            return false;
        }
        stack.setQuality(stack.quality() + 1);
        if (flag != 0U) {
            addPickup(item);
        }
        it.destroy();
        updateStatus(STATUS_AMMO);
        return true;
    }
    updateStatus(STATUS_AMMO);
    const u16 slot = controlledNpc.utilitySlot();
    if (slot == 0U) {
        return false;
    }
    it.setQuality(1);
    it.push();
    it.popToEnd(controlledNpc.id);
    controlledNpc.setEquip(slot, item);
    if (flag != 0U) {
        addPickup(item);
    }
    return true;
}

/* A keycard opens its doors (its number, below 20h) and goes. */
bool pickUpKeycard(Item &it, u16 item, u8 flag)
{
    const i16 card = it.qualityLow();
    if (card < 0x20) {
        setKeycard(card);
    }
    if (flag != 0U) {
        addPickup(item);
    }
    it.destroy();
    return true;
}

/* A battery goes: a better one than the Silencer's is fitted, another gives
 * energy (chemical 2500, fission 5000, fusion 10000). */
bool pickUpBattery(Item &it, u16 item, u8 flag, u16 shape)
{
    if (flag != 0U) {
        addPickup(item);
    }
    it.destroy();
    i16 mana = 0;
    switch (shape) {
    case SHAPE_CHEMICAL_BATTERY:
        if (controlledNpc.batteryType() == 0U) {
            controlledNpc.setBatteryType(1);
        } else {
            mana = 2500;
        }
        break;
    case SHAPE_FISSION_BATTERY:
        if (controlledNpc.batteryType() < 2) {
            controlledNpc.setBatteryType(2);
        } else {
            mana = 5000;
        }
        break;
    case SHAPE_FUSION_BATTERY:
        if (controlledNpc.batteryType() < 3) {
            controlledNpc.setBatteryType(3);
        } else {
            mana = 10000;
        }
        break;
    default:
        halt();
    }
    controlledNpc.addMana(mana);
    updateStatus(STATUS_ENERGY);
    return true;
}

/* A shield's type (Npc::shieldType): ionic 1, plasma 2, graviton 3; 0 for
 * another shape. */
i16 shieldTypeOf(u16 shape)
{
    switch (shape) {
    case SHAPE_IONIC_SHIELD:
        return 1;
    case SHAPE_PLASMA_SHIELD:
        return 2;
    case SHAPE_GRAVITON_SHIELD:
        return 3;
    default:
        return 0;
    }
}

/* Another bomb or inventory item: one more on the stack it has (one more
 * of UNSTACKED_SHAPES goes; spider bombs at most 20, the others 10); a
 * shield is fitted when better than the Silencer's and goes; else into a
 * pack slot (chosen). The data link is not shown among the pickups. */
bool pickUpInventory(Item &it, u16 item, u8 flag)
{
    const u16 other = controlledNpc.findItem(it.shape());
    if (other != 0U) {
        Item stack(other);
        const u16 kind = stack.shape();
        if (isOneOf(kind, UNSTACKED_SHAPES)) {
            it.destroy();
            return true;
        }
        if (stack.quality() >= ((kind == SHAPE_SPIDER_BOMB) ? 20 : 10)) {
            return false;
        }
        stack.setQuality(1 + stack.quality());
        if (it.shape() != SHAPE_DATA_LINK && (flag != 0U)) {
            addPickup(item);
        }
        it.destroy();
        updateStatus(STATUS_ITEM);
        return true;
    }
    const i16 shield = shieldTypeOf(shapeOf(it.id));
    if (shield != 0) {
        if (controlledNpc.shieldType() < shield) {
            controlledNpc.setShieldType(shield);
        }
        it.destroy();
        if (flag != 0U) {
            addPickup(item);
        }
        return true;
    }
    const u16 slot = controlledNpc.packSlot();
    if (slot == 0U) {
        return false;
    }
    it.push();
    it.popToEnd(controlledNpc.id);
    it.setFrame(0);
    controlledNpc.setEquip(slot, item);
    it.setQuality(1);
    controlledNpc.setActiveSlot3(slot);
    updateStatus(STATUS_ITEM);
    if (it.shape() != SHAPE_DATA_LINK && (flag != 0U)) {
        addPickup(item);
    }
    return true;
}
}

/* The controlled NPC picks up item, shown among the pickups on the screen
 * with flag: the credits, a weapon, ammunition, a keycard, a battery or
 * another bomb or inventory item. */
bool pickUp(u16 item, u8 flag)
{
    if (item == 0U) {
        return false;
    }
    Item it(item);
    if (!it.isValid()) {
        return false;
    }
    if (it.shape() == SHAPE_CREDITS) {
        return pickUpCredits(it, item, flag);
    }
    if (it.family() == FAMILY_WEAPON) {
        return pickUpWeapon(it, item, flag);
    }
    if (it.family() == FAMILY_AMMO) {
        return pickUpAmmo(it, item, flag);
    }
    if (it.family() != FAMILY_BOMB && it.family() != FAMILY_INVENTORY) {
        return false;
    }
    const u16 shape = it.shape();
    if (shape == SHAPE_KEYCARD) {
        return pickUpKeycard(it, item, flag);
    }
    if (shape == SHAPE_CHEMICAL_BATTERY || shape == SHAPE_FUSION_BATTERY || shape == SHAPE_FISSION_BATTERY) {
        return pickUpBattery(it, item, flag, shape);
    }
    return pickUpInventory(it, item, flag);
}

/* The controlled NPC takes its previous inventory item (1008:0e54, I), the
 * pack's slots PACK_SLOTS (No Regret's 21-44, No Remorse's 11-25). */
void previousInventoryItem()
{
    i16 start = controlledNpc.activeSlot3();

    if (start == 0) {
        return;
    }
    start--;
    if (start < PACK_SLOTS.first) {
        start = PACK_SLOTS.last;
    }
    i16 slot = start;
    while (controlledNpc.equipment(slot) == 0U) {
        slot--;
        if (slot < PACK_SLOTS.first) {
            slot = PACK_SLOTS.last;
        }
        if (start == slot) {
            slot = 0;
            break;
        }
    }
    if (slot == 0) {
        return;
    }
    controlledNpc.setActiveSlot3(slot);
    updateStatus(STATUS_ITEM);
}

/* The controlled NPC takes its next inventory item (1008:0eb5, O). No
 * Remorse's never reached slot 25, the pack's last: it went back to 11 from
 * 24 (the original's bug). */
void nextInventoryItem()
{
    i16 start = controlledNpc.activeSlot3();

    if (start == 0) {
        return;
    }
    start++;
    if (start > PACK_SLOTS.last) {
        start = PACK_SLOTS.first;
    }
    i16 slot = start;
    while (controlledNpc.equipment(slot) == 0U) {
        slot++;
        if (slot > PACK_SLOTS.last) {
            slot = PACK_SLOTS.first;
        }
        if (start == slot) {
            slot = 0;
            break;
        }
    }
    if (slot == 0) {
        return;
    }
    controlledNpc.setActiveSlot3(slot);
    updateStatus(STATUS_ITEM);
}

void clearTeleporting()
{
    crusaderTeleporting = 0;
}

u8 isTeleporting()
{
    return crusaderTeleporting;
}

void setTeleporting()
{
    crusaderTeleporting = 1;
}

void unknown1047()
{
    if (worldActive != 0U) {
        callIntrinsics();
    }
}

namespace {
/* Clears the sprites and the sounds (1008:1053). */
/* UNCALLED: nothing calls it. */
void clearEffects()
{
    clearSprites();
    if (effectsStarted != 0U) {
        stopAllSounds();
        resetChannels();
        resetSoundSources();
        compactBlocks();
    }
}
}

WorldGump::WorldGump()
    : NewGump(NewGumpId(GUMP_WORLD, 0), nullptr, EVENT_LEFT_BUTTON | EVENT_RIGHT_BUTTON | EVENT_MOTION | EVENT_COMMAND)
{
    enabled2e = 0;
    hackMover = 0;
    modes = 0;
    bounds = screenViewport->bounds();
    addAcceptedArguments(-1, 0);
    target = nullptr;
    waiting = nullptr;
}

/* Nothing calls these (1008:1172, 118e). */
/* UNCALLED: nothing calls it. */
void unknown1172()
{
    unknown1047();
    mouseCtl->moveTo(320, 200);
}

/* UNCALLED: nothing calls it. */
void unknown118e()
{
    clearEffects();
    killAll();
    guiDispatcher->swapModal();
}

namespace {
/* The main menu's entries: 0 a new game, set up as MAPJUMP.C's startGame
 * does, 1 load, 2 save, 5 quit; anything else goes back to the game. */
/* gameMenu's new game's end, after its move to the first map. */
void menuNewGameEnd()
{
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    releaseControlledNpc();
    unknown1047();
    thawWorld(1);
}
}

void WorldGump::gameMenu()
{
    cycleProcess->paused = 1;
    jumpToBlack();
    freezeWorld(1);
    coverWideSides();
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    const u8 shown = statusShown;
    if (shown != 0U) {
        hideStatus();
    }
    /* The menu's choice is done after it has closed. */
    openModalGump(new MainMenuGump, static_cast<u32>(-1), [shown](s32 result) {
        const i16 choice = static_cast<i16>(result);
        if (shown != 0U) {
            showStatus();
        }
        gameMouse->resetShape(GAME_POINTER);
        gameMouse->hide();
        drawViewport->clear(0);
        screenViewport->blit(drawViewport);
        if (choice == MAINMENU_QUIT) {
            systemShutdown(1);
        } else if (choice == MAINMENU_LOAD && !savedGameLoadable()) {
            /* The port's: a saved game loadGame cannot take, said over the
             * world (its palette back from the menu's black), and the game
             * played going on (savegame.h). */
            gameCamera->repaint();
            gamePalette->apply();
            refuseSavedGame([]() { thawWorld(1); });
        } else if (choice == MAINMENU_LOAD) {
            allowMovieRestore();
            stopAllEffects();
            loadGame();
            gameCamera->repaint();
            gamePalette->apply();
            thawWorld(1);
        } else if (choice == MAINMENU_SAVE) {
            saveGame();
            gameCamera->repaint();
            gamePalette->apply();
            thawWorld(1);
        } else if (choice == MAINMENU_NEW_GAME) {
            if (JUMP_SHOWS_LOADING) {
                clearStasis();
                showLoading();
            }
            allowMovieRestore();
            stopMusic();
            killAll();
            freeDeadProcesses();
            alertOn = 0;
            initTracker();
            freeYamm();
            initYamm();
            resetTargeting(controller);
            snapProcess->clearEggs();
            cycleProcess->paused = 1;
            clearSprites();
            freeFlags();
            loadFlags(nullptr, 0);
            closeItemCache();
            initItemCache();
            freeNpcs();
            allocateNpcs();
            controlledNpc.reset();
            Item dataLink;
            if (dataLink.create(SHAPE_DATA_LINK, 0)) {
                dataLink.popToCoords(controlledNpc.x(), controlledNpc.y(), controlledNpc.z());
                pickUp(dataLink.id, 0);
            }
            Item extra;
            if (extra.create(0x598, 0)) {
                extra.popToContainer(controlledNpc.id);
            }
            setFollowedItem(0);
            moveCamera(0, 0, 0, 0);
            unknown1453 = 0;
            setFollowedItem(1);
            if (JUMP_MOVES_TO_EGG) {
                moveToEgg(1, 30, 1);
                /* After the map's loading's wait. */
                whenDone([]() {
                    jumpToBlack();
                    menuNewGameEnd();
                });
                return;
            }
            teleportToEgg(1, 30, 1);
            menuNewGameEnd();
        } else {
            gameCamera->repaint();
            gamePalette->apply();
            thawWorld(1);
        }
    });
}

/* WORLD_TARGETING: the item clicked goes to the waiting process. */
void WorldGump::handleCommand(NewGumpEvent *event)
{
    if ((modes & WORLD_TARGETING) != 0) {
        Item item;
        itemAt(&item.id, event->x, event->y);
        gameMouse->popShape();
        if (target != nullptr) {
            *target = item;
            waiting->terminate(waiting->result);
            waiting = nullptr;
            target = nullptr;
        }
        modes &= ~WORLD_TARGETING;
    }
}

/* The mouse steers the controlled NPC: it stays in the middle of the screen
 * and a move sideways turns the NPC like the arrow keys. */
void WorldGump::handleMouseMotion(NewGumpEvent *event)
{
    i16 dx = 0;

    if ((hackMover == 0U) && ((modes & WORLD_SELECTING) == 0)) {
        mouseCtl->moveTo(320, 200);
        dx = 320 - event->x;
    }
    if (controlledNpc.held != 0U) {
        return;
    }
    if ((modes & 0x40) != 0) {
        return;
    }
    if (hackMover == 0U) {
        const Npc actor(controlledActor); /* not used, the original's */
        if (inCombatStance()) {
            mouseTurnAmount = (dx < 0 ? -dx : dx) >> static_cast<i16>(3 - mouseSpeed);
        } else {
            mouseTurnAmount = (dx < 0 ? -dx : dx) >> static_cast<i16>(7 - (mouseSpeed << 1));
        }
    } else {
        mouseTurnAmount = 0;
    }
    if (mouseTurnAmount != 0) {
        /* The motion's input gives its action (bindings.h): a turn in the
         * mouse's way, with the modes held unless the mouse does not roll
         * (deviceTurnShift), another action as its key or code. */
        const Device device = dx > 0 ? Device::MOUSE_MOVE_LEFT : Device::MOUSE_MOVE_RIGHT;
        const Action action = deviceAction(device, false);
        if (action == Action::TURN_LEFT || action == Action::TURN_RIGHT) {
            KeyEvent key;
            key.ascii = 0;
            key.scan = action == Action::TURN_LEFT ? SCAN_LEFT : SCAN_RIGHT;
            key.shift = deviceTurnShift(keyShift);
            key.time = 0;
            key.pressed = 1;
            key.extended = 1;
            key.repeat = 1;
            mouseTurn = 1;
            controller->syntheticKey(&key);
            mouseTurn = 0;
        } else {
            devicePulse(device, action);
        }
    }
    if ((modes & (WORLD_WALKING | WORLD_FIRING)) != 0) {
        execute(event);
        return;
    }
    if (((modes & WORLD_DRAGGING) != 0) && (hackMover != 0U)) {
        dragTo(event);
    }
}

namespace {
/* The item the left button clicked (the look's and the hack mover's), and
 * the input each mouse button's press gave (its double click's, or none)
 * with its action, which the button's release ends (mouseButton). */
Item &clicked = gameGlobal<Item>();
Device (&buttonDevices)[2] = gameGlobal<Device[2]>({Device::NONE, Device::NONE});
Action (&buttonActions)[2] = gameGlobal<Action[2]>({Action::NONE, Action::NONE});
}

/* The left button: in the selection the item clicked, with the hack mover
 * the item dragged, else what it is bound to (mouseButton: the original's
 * fire while it is held). */
void WorldGump::handleLeftButton(NewGumpEvent *event)
{
    const Npc actor(controlledActor); /* not used, the original's */

    if ((modes & WORLD_SELECTING) != 0) {
        if (event->isMousePress()) {
            if (current != nullptr) {
                Item item(current->item);
                const ShapeInfo info = typeFlags.info(item.shape());
                if (info.selectable) {
                    item.use();
                    /* After the use's wait, if it opened one (a
                     * terminal). */
                    whenDone([this]() {
                        selected = 0;
                        useSelection();
                    });
                    return;
                }
                selected = current->item;
            } else {
                selected = 0;
            }
            useSelection();
        }
    } else {
        if ((modes & 0x40) != 0) {
            modes &= ~0x40;
            gameMouse->popShape();
        }
        if (event->isMouseRelease() && ((modes & (WORLD_LOOKING | WORLD_DRAGGING)) != 0)) {
            if (controlledNpc.isControlled() && (controlledNpc.held != 0U)) {
                return;
            }
            if ((modes & WORLD_LOOKING) != 0) {
                clicked.look();
                eventMask &= ~EVENT_EXECUTE;
                clicked.id = 0;
                modes &= ~WORLD_LOOKING;
            } else {
                endDrag(event);
            }
        } else if (!event->isMouseRelease() && (hackMover != 0U)) {
            if (controlledNpc.isControlled() && (controlledNpc.held != 0U)) {
                return;
            }
            if (((static_cast<u8>(event->input >> 16) & 3) != 0) && modes == 0) {
                itemAt(&clicked.id, event->x, event->y);
                if (clicked.isValid()) {
                    startDrag(event);
                }
            }
        } else {
            mouseButton(0, event);
        }
    }
}

/* The mouse's fire (the original's left button): pressed, the item under
 * the pointer noted and the controlled NPC firing while it is held
 * (execute); let go, the last shot. */
void WorldGump::fireByMouse(NewGumpEvent *event)
{
    if (controlledNpc.isControlled() && (controlledNpc.held != 0U)) {
        return;
    }
    if (event->isMouseRelease()) {
        if ((modes & (WORLD_FIRING | 0x10)) != 0) {
            mouseFire(0, event->x, event->y);
            modes &= ~(WORLD_FIRING | 0x10);
        }
    } else {
        itemAt(&clicked.id, event->x, event->y);
        modes |= WORLD_FIRING;
        eventMask |= EVENT_EXECUTE;
        execute(event);
    }
}

void WorldGump::mouseButton(u8 right, NewGumpEvent *event)
{
    if (event->isMouseRelease()) {
        const Device device = buttonDevices[right];
        const Action action = buttonActions[right];
        buttonDevices[right] = Device::NONE;
        buttonActions[right] = Action::NONE;
        if (action == Action::FIRE) {
            fireByMouse(event);
        } else if (action == Action::MOVE_FORWARD) {
            walkByMouse(event);
        }
        if (device != Device::NONE) {
            deviceRelease(device);
        }
        return;
    }
    Device device = (right != 0U) ? Device::MOUSE_RIGHT : Device::MOUSE_LEFT;
    if (event->isMouseRepeat()) {
        device = (right != 0U) ? Device::MOUSE_RIGHT_DOUBLE : Device::MOUSE_LEFT_DOUBLE;
    }
    const Action action = deviceAction(device, false);
    buttonDevices[right] = device;
    buttonActions[right] = action;
    if (action == Action::FIRE) {
        devicePress(device, Action::NONE);
        fireByMouse(event);
    } else if (action == Action::MOVE_FORWARD) {
        devicePress(device, Action::NONE);
        walkByMouse(event);
    } else if (action == Action::READY_WEAPON) {
        devicePress(device, Action::NONE);
        if (!controlledNpc.isControlled() || (controlledNpc.held == 0U)) {
            readyWeapon();
        }
    } else {
        devicePress(device, action);
    }
}

/* The right button: what it is bound to (mouseButton: the original's walk
 * while it is held, and its double click's weapon readied or put away). */
void WorldGump::handleRightButton(NewGumpEvent *event)
{
    if ((modes & (WORLD_TARGETING | WORLD_SELECTING)) != 0) {
        return;
    }
    if ((modes & 0x40) != 0) {
        modes &= ~0x40;
        gameMouse->popShape();
    }
    mouseButton(1, event);
}

/* The mouse's walk (the original's right button): the controlled NPC walks
 * while it is held (execute). The turn is dead code, the original's: the
 * direction read is compared with itself. */
void WorldGump::walkByMouse(NewGumpEvent *event)
{
    Npc actor(controlledActor);

    if (controlledNpc.isControlled() && (controlledNpc.held != 0U) &&
        (!event->isMouseRelease() || ((modes & WORLD_WALKING) == 0))) {
        return;
    }
    if (event->isMouseRelease()) {
        if ((modes & WORLD_WALKING) != 0) {
            modes &= ~WORLD_WALKING;
            eventMask &= ~EVENT_EXECUTE;
            KeyEvent key;
            key.scan = SCAN_UP;
            key.shift = modeShift(keyShift);
            key.time = 0;
            key.pressed = 0;
            key.extended = 1;
            key.repeat = 1;
            controller->syntheticKey(&key);
        }
    } else {
        modes |= WORLD_WALKING;
        eventMask |= EVENT_EXECUTE;
        if (inCombatStance()) {
            if ((static_cast<u8>(event->input >> 16 & 3)) == 0U) {
                execute(event);
                return;
            }
            if (actor.isKneeling() != 0U) {
                actor.doAnim(ANIM_KNEEL_END, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
            }
            actor.doAnim(ANIM_UNREADY_SMALL, DIRECTION_CURRENT, 1, 0);
            const char direction = actor.direction();
            if (actor.direction() == direction) {
                execute(event);
                return;
            }
            if (!actor.isBusy()) {
                turnNpc(actor.id, direction, 1, 0);
            }
        } else {
            const char direction = actor.direction();
            if (actor.direction() == direction) {
                execute(event);
                return;
            }
            if (!actor.isBusy()) {
                turnNpc(actor.id, direction, 1, 0);
            }
        }
    }
}

void readyWeapon()
{
    Npc actor(controlledActor);

    if (!actor.isBusy()) {
        if (inCombatStance()) {
            if (actor.isKneeling() != 0U) {
                actor.doAnim(ANIM_KNEEL_END, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
            }
            actor.doAnim(ANIM_UNREADY_SMALL, DIRECTION_CURRENT, 1, 0);
        } else {
            actor.doAnim(ANIM_READY_SMALL, DIRECTION_CURRENT, ONLY_IF_IDLE, 0);
        }
    }
}

void WorldGump::execute(NewGumpEvent *event)
{
    Npc actor(controlledActor);

    if (controlledNpc.isControlled() && (controlledNpc.held != 0U)) {
        return;
    }
    if ((modes & 0x40) != 0) {
        return;
    }
    if ((modes & WORLD_SELECTING) != 0) {
        if (current != nullptr) {
            const Item item(current->item);
            showSelector(item.centreX(), item.centreY(), item.centreZ());
            if (grab == 0U) {
                eventMask &= ~EVENT_EXECUTE;
            } else {
                gameCamera->run();
                selected = current->item;
                useSelection();
            }
        }
        if (selectionCount == 0) {
            selected = 0;
            current = nullptr;
            useSelection();
        }
    } else {
        if ((modes & WORLD_WALKING) != 0) {
            const i16 direction = actor.direction();
            if (!actor.isBusy()) {
                const u8 turned = turnNpc(actor.id, direction, 0, 0);
                if (turned == 0U) {
                    KeyEvent key;
                    key.scan = SCAN_UP;
                    key.shift = modeShift(keyShift);
                    key.time = 0;
                    key.pressed = 1;
                    key.extended = 1;
                    key.repeat = 1;
                    if (inCombatStance()) {
                        key.shift = KEY_LEFT_ALT;
                    }
                    controller->syntheticKey(&key);
                }
            }
        }
        if ((modes & WORLD_FIRING) != 0) {
            mouseFire(1, event->x, event->y);
        }
    }
}

/* The hack mover picks up the item under the pointer. */
void WorldGump::startDrag(NewGumpEvent *event)
{
    const u16 node = nodeAt(event->x, event->y);
    DL_ItemNode *entry = static_cast<DL_ItemNode *>(nodeOf(node));

    if ((node != 0U) && entry->kind == NODE_ITEM) {
        Point at(entry->screenX, entry->screenY);
        const Item item(entry->item);
        draggedItem.pickUp(item.id);
        at.x -= scrollX;
        at.y -= scrollY;
        draggedItem.grab.x = event->x - at.x;
        draggedItem.grab.y = event->y - at.y;
        draggedItem.dragsItem = 1;
        beginDrag(event);
    }
}

void WorldGump::beginDrag(NewGumpEvent *event)
{
    if (draggedItem.item.isValid()) {
        draggedItem.item.grab();
        capture(this);
        eventMask &= ~EVENT_KEY;
        modes |= WORLD_DRAGGING;
        handleMouseMotion(event);
    }
}

void WorldGump::endDrag(NewGumpEvent * /*unused*/)
{
    if ((modes & WORLD_DRAGGING) != 0) {
        release();
        modes &= ~WORLD_DRAGGING;
        eventMask |= EVENT_KEY;
        draggedItem.drop();
        draggedItem.dragsItem = 0;
    }
}

/* The item's place is read and not used, the original's. */
void WorldGump::dragTo(NewGumpEvent *event)
{
    ItemPoint from;

    from.x = draggedItem.item.x();
    from.y = draggedItem.item.y();
    from.z = draggedItem.item.z();
    draggedItem.dropOnMap(event->x, event->y);
}

/* The items within 80h of the controlled NPC and a little above or below it
 * that can be picked up or used, nearest first. */
void WorldGump::selectItems(u8 take)
{
    grab = take;
    selection.clear();
    selectionCount = 0;
    selected = 0;
    for (i16 itemNumber = 0x100; itemNumber < ITEMS; itemNumber++) {
        const Item item(itemNumber);
        if (item.z() == ETHEREAL_Z) {
            continue;
        }
        if ((item.flags() & ITEM_CONTAINED) != 0) {
            continue;
        }
        const ShapeInfo info = typeFlags.info(item.shape());
        const i16 family = item.family();
        const u16 shape = item.shape();
        if (shape != SHAPE_CREDITS && family != FAMILY_WEAPON && family != FAMILY_INVENTORY && family != FAMILY_BOMB &&
            family != FAMILY_AMMO && !info.selectable) {
            continue;
        }
        const i16 distance =
            mapDistance(item.centreX(), item.centreY(), controlledNpc.centreX(), controlledNpc.centreY());
        if (itemZ[item.id] >= static_cast<i16>(itemZ[controlledNpc.id] + 0x50)) {
            continue;
        }
        if (itemZ[item.id] <= static_cast<i16>(itemZ[controlledNpc.id] - 0x18)) {
            continue;
        }
        if (distance < 0x80 && ((item.flags() & ITEM_HANGING) == 0)) {
            SelectionLink *link = new SelectionLink;
            link->item = itemNumber;
            link->distance = distance;
            selection.append(link);
            selectionCount++;
        }
    }
    if (selectionCount != 0) {
        bool swapped;
        do {
            swapped = false;
            SelectionLink *link = nullptr;
            while (selection.forward(reinterpret_cast<GumpLink **>(&link))) {
                if (link->next != nullptr) {
                    SelectionLink *nextLink = static_cast<SelectionLink *>(link->next);
                    if (link->distance > nextLink->distance) {
                        selection.moveAfter(nextLink, link);
                        swapped = true;
                    }
                }
            }
        } while (swapped);
    }
    current = static_cast<SelectionLink *>(selection.first);
    if (selectionCount != 0) {
        const Item item(current->item);
        createSelector(item.centreX(), item.centreY(), item.centreZ());
        eventMask |= EVENT_EXECUTE;
        modes |= WORLD_SELECTING;
        selectingItems = 1;
    } else {
        current = nullptr;
        selected = 0;
        grab = 0;
        useSelection();
        if (!SELECT_SOUND_WAITS || !isEffectPlaying(SELECT_SOUND)) {
            playEffect(SELECT_SOUND);
        }
    }
    if (grab != 0U) {
        while (current != nullptr) {
            execute(&lvalue(NewGumpEvent()));
        }
    }
}

void WorldGump::useSelection()
{
    const u16 item = selected;

    if (grab == 0U) {
        modes &= ~WORLD_SELECTING;
        selectingItems = 0;
        if (current != nullptr) {
            freeSelector();
        }
    }
    pickUp(item, 1);
    if (grab == 0U) {
        return;
    }
    nextSelection();
}

void WorldGump::clearSelection()
{
    selected = 0;
    grab = 0;
    useSelection();
}

void WorldGump::nextSelection()
{
    if (selectionCount == 0) {
        return;
    }
    selected = 0;
    if (current != nullptr) {
        current = static_cast<SelectionLink *>(current->next);
    }
    if (current == nullptr) {
        if (grab != 0U) {
            grab = 0;
            freeSelector();
            useSelection();
        } else {
            current = static_cast<SelectionLink *>(selection.first);
        }
    }
}

namespace {
/* The sprite over the item the selection shows; -1 while there is none. */
i16 &selectorSprite = gameGlobal<i16>(-1);
}

void WorldGump::createSelector(i16 x, i16 y, i16 z)
{
    selectorSprite = createSprite(0x5a3, 0, 0, x, y, z);
}

void WorldGump::freeSelector()
{
    if (selectorSprite != -1) {
        freeSprite(selectorSprite);
    }
}

void WorldGump::showSelector(i16 x, i16 y, i16 z)
{
    if (selectorSprite != -1) {
        showSprite(selectorSprite, x, y, z);
    }
}

void WorldGump::previousSelection()
{
    selected = 0;
    if (selectionCount == 0) {
        return;
    }
    if (current != nullptr) {
        current = static_cast<SelectionLink *>(current->previous);
    }
    if (current == nullptr) {
        current = static_cast<SelectionLink *>(selection.last);
    }
}

HelpGump::HelpGump() : DialogGump(NewGumpId(), nullptr, Rect(0, 0, SCREEN_RIGHT, SCREEN_BOTTOM), guiColors->face)
{
    jumpToBlack();
    freezeWorld(1);
    coverWideSides();
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    eventMask |= EVENT_EXECUTE | EVENT_KEY;
    addAcceptedArguments(-1, 0);
    cycleProcess->paused = 1;
    page = 0;
    unknown47 = 0;
    showPage();
    if (HELP_BUTTONS) {
        mouseCtl->resetShape(HELP_POINTER);
        mouseCtl->show();
        ShapeButtonGump *button = new ShapeButtonGump(0, this, 0x51);

        button->bounds.moveTo(493, 449);
        button = new ShapeButtonGump(1, this, 0x52);
        button->bounds.moveTo(549, 449);
    } else {
        mouseCtl->hide();
        playEffect(HELP_SOUND);
    }
}

HelpGump::~HelpGump()
{
    flushKeys();
    jumpToBlack();
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    if (HELP_BUTTONS) {
        mouseCtl->hide();
    }
    gamePalette->apply();
    cycleProcess->paused = 0;
    thawWorld(1);
    gameCamera->repaint();
}

bool HelpGump::showPage()
{
    if (page == -1) {
        page = 0;
        return true;
    }
    if (page == HELP_PAGES) {
        if (!HELP_PAGES_WRAP) {
            close();
            return false;
        }
        page = 0;
    }
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    char *drive = gameDrive;
    if (page >= 0 && page < static_cast<i16>(HELP_FILES.size()) && HELP_FILES[page] != nullptr) {
        bmp.open(makePath(drive, "static", HELP_FILES[page], nullptr));
    }
    bmp.draw(drawViewport);
    bmp.getPalette(palette.data);
    killItemProcesses(0, FADE_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    palette.apply();
    bmp.close();
    paint(0);
    return true;
}

void HelpGump::handleKeyboard(NewGumpEvent *event)
{
    if (HELP_ARROW_KEYS) {
        if (page == 4) {
            page = 0;
        }
        if (event->argument == ASCII_ESCAPE || event->argument == 'x' || event->argument == 'X') {
            close();
            return;
        }
        if (event->argument == (KEYCODE_SCAN | SCAN_LEFT) || event->argument == (KEYCODE_SCAN | SCAN_UP)) {
            page--;
            showPage();
            unknown47 = 0;
            flushKeys();
            gumpKeys.clear();
            return;
        }
        if (event->argument == (KEYCODE_SCAN | SCAN_RIGHT) || event->argument == (KEYCODE_SCAN | SCAN_DOWN) ||
            event->argument == 'n' || event->argument == 'N') {
            page++;
            if (showPage()) {
                unknown47 = 0;
            }
            flushKeys();
            gumpKeys.clear();
            return;
        }
        if (event->argument == 'e' || event->argument == 'E') {
            unknown47 = 1;
            return;
        }
        if (event->argument == '-' && (unknown47 != 0U)) {
            unknown47++;
            if (unknown47 == 4) {
                page = 4;
                showPage();
            }
        } else {
            unknown47 = 0;
        }
        return;
    }
    if (page == 5) {
        page = 0;
    }
    if (event->argument == ASCII_ESCAPE) {
        close();
        return;
    }
    page++;
    if (showPage()) {
        unknown47 = 0;
    }
    flushKeys();
    gumpKeys.clear();
}

void HelpGump::handleCommand(NewGumpEvent *event)
{
    const i16 buttonId = event->sender->id.instance();

    if (buttonId == 0) {
        page++;
        showPage();
        return;
    }
    close();
}

namespace {
/* The hidden help page's screams, No Remorse's (SCREAM2B, SCRJP2A, FIREGUY3,
 * FEMBURN5, FEMBURN3 in its sound archive). */
constexpr std::array<i16, 5> HELP_PAGE_SCREAMS = {0xb8, 0x62, 0x67, 10, 11};
}

/* On the hidden page a scream now and then. */
void HelpGump::execute(NewGumpEvent * /*unused*/)
{
    if (HELP_PAGE_SOUNDS && page == 4 && randomBelow(8000) == 1) {
        playEffect(HELP_PAGE_SCREAMS[randomBelow(static_cast<u16>(HELP_PAGE_SCREAMS.size()))]);
    }
}

void HelpGump::paintSelf(i16 /*x*/, i16 /*y*/) {}

u8 &cheatsOn = gameGlobal<u8>(0);

u16 &unknown00a2 = gameGlobal<u16>(1);
/* The usecode sets it before some movies and clears it after (intrinsics
 * 083 and 084). In No Remorse the movie player reads it and then does not
 * restore the screen after the movie; No Regret's XAN player reads nothing. */
u8 &movieRestoreSuppressed = gameGlobal<u8>(0);
namespace {
/* No Regret's; nothing reads or writes it. */
[[maybe_unused]] u8 unknown00a5 = 0;
}

u8 &cheatTyped = gameGlobal<u8>();
u8 &worldFrozen = gameGlobal<u8>();

/* The game's keys. The argument is the character, or the scan code with
 * 100h (200h with Alt, 400h with Ctrl). canAct: the controlled NPC is the
 * Silencer, alive, free to move and (No Regret's) not in shape 5D6h. The
 * values and rules are each game's (world_game.h). */
void WorldGump::handleKeyboard(NewGumpEvent *event)
{
    const u8 canAct =
        static_cast<u8>(controlledNpc.held == 0 && (controlledNpc.isDead() == 0U) && controlledNpcNumber() == 1 &&
                        (!SHAPE_5D6_IS_DEAD || controlledNpc.shape() != SHAPE_FROZEN));
    /* The action of the key code (bindings.h). */
    const Action action = worldAction(event->argument);

    if (!WORLD_KEYS_NO_REGRET) {
        /* Escape gives the waiting process no item. */
        if (((modes & WORLD_TARGETING) != 0) && action == Action::MENU) {
            gameMouse->popShape();
            if (target != nullptr) {
                target->id = 0;
                waiting->terminate(waiting->result);
                waiting = nullptr;
                target = nullptr;
            }
            modes &= ~WORLD_TARGETING;
            return;
        }
    }
    if (action == Action::QUIT) { /* quit */
        freezeWorld(1);
        playEffect(QUIT_SOUND);
        /* The answer is done after the question has closed, as the other keys' gumps. */
        openModalGump(new QuestionGump(NewGumpId(), nullptr, 0x15, 0x13, 0x14, QUESTION_YES, QUESTION_NO),
                      static_cast<u32>(-1), [](s32 answer) {
                          if (answer != 0) {
                              cycleProcess->paused = 1;
                              openFadeToBlack(40, []() {
                                  freezeWorld(1);
                                  systemShutdown(1);
                                  return false;
                              });
                              return;
                          }
                          thawWorld(1);
                      });
        return;
    }
    if (WORLD_KEYS_NO_REGRET) {
        /* Escape and F1, also with Alt or Ctrl; F1 opens the main menu. */
        if ((action == Action::CANCEL || action == Action::MENU) && (stasis == 0U)) {
            flushKeys();
            if ((modes & WORLD_SELECTING) != 0) {
                clearSelection();
            }
            if (action == Action::MENU && controlledNpcNumber() == 1) {
                gameMenu();
            }
        }
    } else {
        /* Escape ends the selection, or opens the main menu while the Silencer
         * was controlled when it was pressed (DISPATCH.C). */
        if (action == Action::MENU && (stasis == 0U)) {
            if ((modes & WORLD_SELECTING) != 0) {
                clearSelection();
            } else if (unknown00a2 == 1) {
                gameMenu();
            }
        }
    }
    if (waitOpened()) {
        giveRest([this, event, canAct, action]() {
            keyboardRest(event, canAct, action);
            return false;
        });
        return;
    }
    keyboardRest(event, canAct, action);
}

namespace {
/* keyboardRest's active item's charge after No Remorse's use first, else
 * with No Regret's use at the last. */
void chargeActiveItem(Item item, u16 shape)
{
    if (!isOneOf(shape, UNCHARGED_ITEMS)) {
        item.setQuality(item.quality() - 1);
        if (item.quality() == 0) {
            if (!ITEM_USED_FIRST) {
                item.use();
                whenDone([item]() mutable {
                    item.destroy();
                    controlledNpc.removeEquip(item.id);
                    nextInventoryItem();
                    updateStatus(STATUS_ITEM);
                });
                return;
            }
            item.destroy();
            controlledNpc.removeEquip(item.id);
            nextInventoryItem();
        } else if (!ITEM_USED_FIRST) {
            item.use();
            whenDone([]() { updateStatus(STATUS_ITEM); });
            return;
        }
        updateStatus(STATUS_ITEM);
    } else if (!ITEM_USED_FIRST) {
        item.use();
    }
}
}

/* The selection's keys (bindings.h selectionAction): choose the item (a
 * selectable one is used, the selection ending after the use's wait, if
 * it opened one: a terminal), use it (one that is no inventory item),
 * cancel (No Regret's Escape flushes the keys, the arrows do not), step
 * to the previous or the next. */
void WorldGump::selectionKey(NewGumpEvent *event)
{
    switch (selectionAction(event->argument)) {
    case Action::SELECTION_CHOOSE:
        if (current != nullptr) {
            Item item(current->item);
            const ShapeInfo info = typeFlags.info(item.shape());
            if (info.selectable) {
                item.use();
                whenDone([this]() {
                    selected = 0;
                    useSelection();
                });
                return;
            }
            selected = current->item;
        } else {
            selected = 0;
        }
        useSelection();
        break;
    case Action::SELECTION_USE:
        if (current != nullptr) {
            Item item;
            item = current->item;
            if (inventoryIndex(item.shape()) == 0) {
                item.use();
                whenDone([this]() {
                    selected = 0;
                    useSelection();
                });
                break;
            }
            selected = 0;
            useSelection();
        }
        break;
    case Action::SELECTION_CANCEL:
        if (WORLD_KEYS_NO_REGRET && event->argument == ASCII_ESCAPE) {
            flushKeys();
        }
        selected = 0;
        useSelection();
        break;
    case Action::SELECTION_PREVIOUS:
        eventMask |= EVENT_EXECUTE;
        previousSelection();
        break;
    case Action::SELECTION_NEXT:
        eventMask |= EVENT_EXECUTE;
        nextSelection();
        break;
    default:
        break;
    }
}

namespace {
/* The remote bombs near the controlled NPC (DETONATED_SHAPES) go off. */
void detonateBombs()
{
    const i16 x = controlledNpc.x();
    const i16 y = controlledNpc.y();
    AreaSearch search;
    for (const u16 detonated : DETONATED_SHAPES) {
        for (search.init(x - 0x800, y - 0x800, x + 0x800, y + 0x800,
                         LoopScript(SCRIPT_SHAPE, SCRIPT_INT, detonated, SCRIPT_EQUAL, SCRIPT_END), -1);
             search.isValid(); search.next()) {
            search.use();
            /* The bombs' use events wait for nothing before they suspend. */
            expectNoWait();
        }
    }
}

/* The quick load (F4) asked: yes loads slot 12 (No Regret's with its
 * palette) and says so; the world thaws after. */
void askQuickLoad()
{
    freezeWorld(1);
    playEffect(QUICK_LOAD_SOUND);
    openModalGump(new QuestionGump(NewGumpId(), nullptr, 0x11, 0x13, 0x14, QUESTION_YES, QUESTION_NO),
                  static_cast<u32>(-1), [](s32 answer) {
                      if (answer != 0) {
                          setSaveSlot(12);
                          /* The port's: a saved game loadGame cannot take,
                           * said, and the game going on (savegame.h). */
                          if (!savedGameLoadable()) {
                              refuseSavedGame([]() { thawWorld(1); });
                              return;
                          }
                          stopAllEffects();
                          loadGame();
                          if (WORLD_KEYS_NO_REGRET) {
                              gamePalette->apply();
                          }
                          openModalGump(new NoticeGump(NewGumpId(), nullptr, 0x4e, 0x50, NOTICE_PLACE),
                                        static_cast<u32>(-1), [](s32 /*result*/) { thawWorld(1); });
                          return;
                      }
                      thawWorld(1);
                  });
}

/* The quick load's key: No Regret's when the controlled NPC is dead or it
 * may act out of the stasis, and there is a quick save; No Remorse's out
 * of the stasis with the Silencer controlled. */
void quickLoad(u8 canAct)
{
    if (WORLD_KEYS_NO_REGRET) {
        if (((controlledNpc.isDead() != 0U) || ((stasis == 0U) && (canAct != 0U))) &&
            fileExists(makePath(nullptr, savegameDirectory, "savegame", "13")) != 0) {
            askQuickLoad();
        }
    } else if ((stasis == 0U) && controlledNpcNumber() == 1) {
        askQuickLoad();
    }
}

/* The quick save (F5) asked: yes saves slot 12 and says so; the world
 * thaws after. */
void askQuickSave()
{
    freezeWorld(1);
    playEffect(QUICK_SAVE_SOUND);
    openModalGump(new QuestionGump(NewGumpId(), nullptr, 0x12, 0x13, 0x14, QUESTION_YES, QUESTION_NO),
                  static_cast<u32>(-1), [](s32 answer) {
                      if (answer != 0) {
                          setSaveSlot(12);
                          saveGame();
                          openModalGump(new NoticeGump(NewGumpId(), nullptr, 0x4f, 0x50, NOTICE_PLACE),
                                        static_cast<u32>(-1), [](s32 /*result*/) { thawWorld(1); });
                          return;
                      }
                      thawWorld(1);
                  });
}

/* The screen cleared to black. */
void clearScreen()
{
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
}

/* The game's pointer back, hidden. */
void hidePointer()
{
    gameMouse->resetShape(GAME_POINTER);
    gameMouse->hide();
}

/* A screen over the world (the options, the load and save screens): the
 * palette black, the world frozen, the wide sides covered, the screen
 * cleared. */
void coverWorld()
{
    jumpToBlack();
    freezeWorld(1);
    coverWideSides();
    clearScreen();
}

/* The world back after the load and save screens: its palette, thawed,
 * the palette's cycling and the camera's drawing on again. */
void returnToWorld()
{
    gamePalette->apply();
    thawWorld(1);
    cycleProcess->paused = 0;
    gameCamera->repaint();
}

/* The options (F2): the port's settings screen. */
void openOptions()
{
    coverWorld();
    openModalGump(new SettingsGump, static_cast<u32>(-1), [](s32 /*result*/) {
        hidePointer();
        clearScreen();
        thawWorld(1);
        gamePalette->apply();
        cycleProcess->paused = 0;
        gameCamera->repaint();
    });
}

/* The load screen (F8): a slot chosen is loaded behind a black screen. */
void openLoad()
{
    coverWorld();
    openModalGump(new LoadSaveGump(1), static_cast<u32>(-1), [](s32 chosen) {
        hidePointer();
        if (chosen != 0 && !savedGameLoadable()) {
            /* The port's: a saved game loadGame cannot take, said over the
             * world (its palette back from coverWorld's black), and the
             * game going on (savegame.h). */
            clearScreen();
            gamePalette->apply();
            gameCamera->repaint();
            refuseSavedGame([]() { returnToWorld(); });
            return;
        }
        if (chosen != 0) {
            jumpToBlack();
            clearScreen();
            loadGame();
        } else {
            clearScreen();
        }
        returnToWorld();
    });
}

/* The save screen (F9): a slot chosen is saved behind a black screen. */
void openSave()
{
    coverWorld();
    openModalGump(new LoadSaveGump(0), static_cast<u32>(-1), [](s32 chosen) {
        if (chosen != 0) {
            hidePointer();
            jumpToBlack();
            clearScreen();
            saveGame();
        } else {
            clearScreen();
        }
        returnToWorld();
    });
}

/* The item of an item key (ITEM_KEYS): a charged one loses a charge, and
 * goes at the last, used before (No Remorse's, ITEM_USED_FIRST) or after
 * it; the next inventory item is chosen when it was the chosen one. */
void useItemKey(Action action, u8 canAct)
{
    for (const ItemKey &itemKey : ITEM_KEYS) {
        if (itemKey.action != action || (canAct == 0U) || (stasis != 0U)) {
            continue;
        }
        Item item(controlledNpc.findItem(itemKey.shape));
        if (!item.isValid()) {
            continue;
        }
        /* None of the keys' items' use events waits before it suspends (No
         * Regret's datalink's after it). */
        if (!itemKey.charged) {
            item.use();
            expectNoWait();
        } else if (ITEM_USED_FIRST) {
            item.use();
            expectNoWait();
            item.setQuality(item.quality() - 1);
            if (item.quality() == 0) {
                item.destroy();
                controlledNpc.removeEquip(item.id);
                if (controlledNpc.activeItem3() == item.id) {
                    nextInventoryItem();
                }
            }
        } else {
            item.setQuality(item.quality() - 1);
            if (item.quality() == 0) {
                if (controlledNpc.activeItem3() == item.id) {
                    nextInventoryItem();
                }
                item.use();
                expectNoWait();
                item.destroy();
                controlledNpc.removeEquip(item.id);
            } else {
                item.use();
                expectNoWait();
            }
        }
        updateStatus(STATUS_ITEM);
    }
}

/* The chosen inventory item used (U), not the credits: the uncharged items
 * (UNCHARGED_ITEMS) are only used; No Remorse's uses the item before the
 * charge (ITEM_USED_FIRST), the charge after the use's wait if it opened
 * one (No Remorse's datalink). */
void useActiveItem()
{
    Item item(controlledNpc.activeItem3());
    if (!item.isValid()) {
        return;
    }
    const u16 shape = item.shape();
    if (shape == SHAPE_CREDITS) {
        return;
    }
    if (ITEM_USED_FIRST) {
        item.use();
        whenDone([item, shape]() { chargeActiveItem(item, shape); });
        return;
    }
    chargeActiveItem(item, shape);
}

/* A box with a line of text, closed by a key. */
void showMessage(const char *text)
{
    openModalGump(new MessageBoxGump(NewGumpId(), nullptr, text, guiColors->face), static_cast<u32>(-1),
                  [](s32 /*result*/) {});
}

/* A box with text the box owns: freed when the box closes, or with the
 * wait when the run ends with the box up: the original's was on its
 * stack. */
void showOwnedText(std::unique_ptr<char[]> text)
{
    char *shown = text.get();
    openModalGump(new MessageBoxGump(NewGumpId(), nullptr, shown, guiColors->face), static_cast<u32>(-1),
                  [text = std::move(text)](s32 /*result*/) mutable { text.reset(); });
}

/* The next song (Ctrl+M): No Regret's round its 21 songs from the first,
 * the Christmas music ended; No Remorse's skipping 11h and round after 13h
 * (playSong takes the song for the current one: the music both games'). */
void playNextSong()
{
    if (WORLD_KEYS_NO_REGRET) {
        christmasMusic = 0;
    }
    i16 song = currentSong + 1;
    if (WORLD_KEYS_NO_REGRET) {
        if (song < 0 || song >= 22) {
            song = 1;
        }
    } else {
        if (song == 0x11) {
            song = 0x12;
        }
        if (song == 0x14) {
            song = 0;
        }
    }
    playSong(song);
}

/* No Remorse's place of the controlled NPC (POSITION_KEY). */
void showPosition()
{
    const u16 textSize = 80;
    std::unique_ptr<char[]> text(new char[textSize]);
    snprintf(text.get(), textSize, "X: %u\nY: %u\nZ: %u", controlledNpc.x(), controlledNpc.y(), controlledNpc.z());
    showOwnedText(std::move(text));
}

/* The version (Ctrl+V): the game's, the skill, the map, the free item
 * slots, No Regret's place of the controlled NPC, and the memory's
 * fragments. */
void showVersion()
{
    playEffect(VERSION_SOUND);
    const u16 textSize = 1000;
    std::unique_ptr<char[]> text(new char[textSize]);
    u32 sizes[5];
    u32 frees[5];
    u32 largests[5];
    /* The count is compared signed, and the addresses are SS-based near
     * pointers made far. */
    for (i16 i = 0; i < VERSION_FRAGMENTS; i++) {
        if (i < static_cast<i16>(fragmentCount)) {
            fragmentFreeSpace(i, (&frees[i]), (&largests[i]));
            sizes[i] = fragments[i].size;
        } else {
            sizes[i] = 0;
            frees[i] = 0;
            largests[i] = 0;
        }
    }
    const u32 total = systemMemory;
    u32 freeTotal = 0;
    for (i16 i = 2; i < static_cast<i16>(fragmentCount); i++) {
        u32 fragmentFree;
        u32 largest;
        fragmentFreeSpace(i, &fragmentFree, &largest);
        freeTotal += fragmentFree;
    }
    if (WORLD_KEYS_NO_REGRET) {
        Npc npc(controlledActor);
        if (!npc.isValid()) {
            npc = Npc(controlledNpc.id);
        }
        /* The original's last % is alone: Borland's sprintf writes it as it
         * is, glibc's drops it and fails, so the port writes %%. */
        snprintf(text.get(), textSize,
                 "CRUSADER:  No Regret %s\n\n(c) 1996 Origin Systems\n\nSkill: %d  Map: %d  Sels: %d\n"
                 "Loc=(%d,%d,%d)\n\nMemory Status (Size/Free/Largest)\nFrag 0:  %d %d %d\n"
                 "Frag 1:  %d %d %d\nFrag 2:  %d %d %d\nFrag 3:  %d %d %d\nFrag 4:  %d %d %d\n"
                 "Total/Free/Dynamic:  %d / %d / %d%%",
                 gameVersion, controlledNpc.difficulty, npcMap(&controlledNpc.id), poolFree, itemX[npc.id],
                 itemY[npc.id], itemZ[npc.id], sizes[0], frees[0], largests[0], sizes[1], frees[1], largests[1],
                 sizes[2], frees[2], largests[2], sizes[3], frees[3], largests[3], sizes[4], frees[4], largests[4],
                 total, freeTotal, 100 - firstFragmentPercent);
    } else {
        snprintf(text.get(), textSize,
                 "CRUSADER:  No Remorse V1.21\n\n(c) 1995 Origin Systems\n\nSkill: %d  Map: %d  Sels: %d\n\n"
                 "Memory Status (Size/Free/Largest)\nFrag 0:  %d %d %d\nFrag 1:  %d %d %d\n"
                 "Frag 2:  %d %d %d\nFrag 3:  %d %d %d\nTotal/Free:  %d / %d",
                 controlledNpc.difficulty, npcMap(&controlledNpc.id), poolFree, sizes[0], frees[0], largests[0],
                 sizes[1], frees[1], largests[1], sizes[2], frees[2], largests[2], sizes[3], frees[3], largests[3],
                 total, freeTotal);
    }
    showOwnedText(std::move(text));
}

/* The active weapon dropped at the controlled NPC's feet (not the kept
 * weapon at the easiest skill), the next one taken. */
void dropWeapon()
{
    Item weapon(controlledNpc.activeWeapon());
    if (weapon.isValid() && (weapon.shape() != KEPT_WEAPON || controlledNpc.difficulty > 1)) {
        nextWeapon();
        weapon.push();
        weapon.popToCoords(controlledNpc.x(), controlledNpc.y(), controlledNpc.z());
    }
}

/* The player controls its own NPC (not another, setControlledNpcNumber),
 * which may act. */
bool controllingOwn(u8 canAct)
{
    return controlledNpcNumber() == controlledNpc.id && (canAct != 0U);
}

/* The cheats' keys, which are not bound (bindings.h): No Remorse's T (the
 * controlled NPC used), ~ (the cheats) and Ctrl+Q (the CD's transfer
 * shown; MORE_DEBUG_KEYS); the eggs (Ctrl+F7), the snap eggs (Alt+F7) and
 * the chunks (F7) shown; the items' boxes (F); the hack mover (H, once a
 * code was typed). */
void cheatKey(s16 key)
{
    switch (key) {
    case 'T':
    case 't':
        if (MORE_DEBUG_KEYS && (cheatsEnabled != 0U)) {
            controlledNpc.use();
        }
        break;
    case KEYCODE_CONTROL | SCAN_F7:
        if (cheatsEnabled != 0U) {
            showEggs = static_cast<u8>(showEggs == 0U);
            gameCamera->repaint();
        }
        break;
    case KEYCODE_ALT | SCAN_F7:
        if (cheatsEnabled != 0U) {
            showSnaps = 1 - showSnaps;
            gameCamera->repaint();
        }
        break;
    case KEYCODE_SCAN | SCAN_F7:
        if (cheatsEnabled != 0U) {
            showChunks = 1 - showChunks;
            gameCamera->repaint();
        }
        break;
    case '~':
        if (MORE_DEBUG_KEYS && (cheatsEnabled != 0U)) {
            cheatsOn = 1 - cheatsOn;
            showMessage((cheatsOn != 0U) ? "CHEATS ON" : "CHEATS OFF");
        }
        break;
    case KEYCODE_CONTROL | SCAN_Q:
        if (MORE_DEBUG_KEYS && (cheatsEnabled != 0U)) {
            cdTransferDisplay = static_cast<u8>(cdTransferDisplay == 0U);
            showMessage((cdTransferDisplay != 0U) ? "CD TRANSFER DISPLAY ACTIVE." : "CD TRANSFER DISPLAY INACTIVE.");
        }
        break;
    case 'f':
        if (cheatsEnabled != 0U) {
            setHeld(0);
            showItemBoxes = static_cast<u8>(showItemBoxes == 0U);
        }
        break;
    case 'H':
    case 'h':
        if ((cheatTyped != 0U) && (cheatsEnabled != 0U)) {
            hackMover = 1 - hackMover;
            if (hackMover != 0U) {
                mouseCtl->show();
                mouseCtl->resetShape(GAME_POINTER);
                showMessage("HACK MOVER ON");
            } else {
                openModalGump(new MessageBoxGump(NewGumpId(), nullptr, "HACK MOVER OFF", guiColors->face),
                              static_cast<u32>(-1), [](s32 /*result*/) { mouseCtl->hide(); });
            }
        }
        break;
    default:
        break;
    }
}
}

/* The world screen's keys after the main menu's: the selection's while it
 * is up (not while it grabs and the controlled NPC may act), the others'
 * when no mode is on, then the cheats'. */
void WorldGump::keyboardRest(NewGumpEvent *event, u8 canAct, Action action)
{
    if ((modes & WORLD_SELECTING) != 0) {
        if ((grab == 0U) || (canAct == 0U)) {
            selectionKey(event);
        }
        return;
    }
    if (modes != 0U) {
        return;
    }
    const bool mayAct = (stasis == 0U) && (canAct != 0U);
    switch (action) {
    case Action::DETONATE:
        if (canAct != 0U) {
            detonateBombs();
        }
        break;
    case Action::QUICK_LOAD:
        quickLoad(canAct);
        break;
    case Action::QUICK_SAVE:
        if (mayAct) {
            askQuickSave();
        }
        break;
    case Action::OPTIONS: /* No Remorse's even in the stasis */
        if (!WORLD_KEYS_NO_REGRET || mayAct) {
            openOptions();
        }
        break;
    case Action::LOAD: /* No Regret's when the controlled NPC is dead too */
        if (WORLD_KEYS_NO_REGRET ? ((controlledNpc.isDead() != 0U) || mayAct) : (stasis == 0U)) {
            openLoad();
        }
        break;
    case Action::SAVE:
        if (mayAct) {
            openSave();
        }
        break;
    case Action::HELP:
        if (!WORLD_KEYS_NO_REGRET || mayAct) {
            openModalGump(new HelpGump, static_cast<u32>(-1), [](s32 /*result*/) {});
        }
        break;
    case Action::USE_ENERGY_CUBE:
    case Action::USE_VIR_IMAGER:
    case Action::USE_MINE_DETECTOR:
    case Action::USE_RADIATION_SHIELD:
    case Action::DATALINK:
    case Action::USE_MEDKIT:
        useItemKey(action, canAct);
        break;
    case Action::CALIBRATE_JOYSTICK: /* No Regret's dialog */
        if (CALIBRATE_ONCE) {
            calibrateJoystick(1);
        }
        break;
    case Action::TARGETING_RETICLE: /* No Remorse's */
        if (TARGETING_KEY) {
            targeting = static_cast<u8>(targeting == 0U);
            showMessage((targeting != 0U) ? "TARGETING RETICLE ACTIVE." : "TARGETING RETICLE INACTIVE.");
        }
        break;
    case Action::NEXT_SONG:
        playNextSong();
        break;
    case Action::PREVIOUS_WEAPON:
        if (controllingOwn(canAct)) {
            previousWeapon();
        }
        break;
    case Action::NEXT_WEAPON:
        if (controllingOwn(canAct)) {
            nextWeapon();
        }
        break;
    case Action::USE_ITEM:
        if (canAct != 0U) {
            useActiveItem();
        }
        break;
    case Action::PREVIOUS_ITEM:
        if (controllingOwn(canAct)) {
            previousInventoryItem();
        }
        break;
    case Action::NEXT_ITEM:
        if (controllingOwn(canAct)) {
            nextInventoryItem();
        }
        break;
    case Action::SHOW_POSITION: /* No Remorse's */
        if (POSITION_KEY) {
            showPosition();
        }
        break;
    case Action::VERSION:
        showVersion();
        break;
    case Action::GRAB_ITEMS:
    case Action::SELECT_ITEMS:
        if (mayAct) {
            clearTeleporting();
            selectItems((action == Action::GRAB_ITEMS) ? 1 : 0);
        }
        break;
    case Action::DROP_WEAPON:
        if (controlledNpc.isControlled() && (canAct != 0U)) {
            dropWeapon();
        }
        break;
    default:
        break;
    }
    if (action == Action::NONE) {
        cheatKey(event->argument);
    }
}
