/* The usecode's intrinsics, generated from USECODE\UNKCOFF.DAT, the
 * original's map and the port's definitions; do not edit. See intrinsics.h. */
#include "intrinsics.h"
#include "host.h"
#include "alert.h"
#include "camera.h"
#include "compgump.h"
#include "control.h"
#include "crusader.h"
#include "crusys.h"
#include "distance.h"
#include "dsfxman.h"
#include "egg.h"
#include "explode.h"
#include "fade.h"
#include "gamepal.h"
#include "gametime.h"
#include "gravity.h"
#include "inter.h"
#include "item.h"
#include "itemcach.h"
#include "kernel.h"
#include "keypad.h"
#include "mapjump.h"
#include "movie.h"
#include "music.h"
#include "npc.h"
#include "sprite.h"
#include "status.h"
#include "type.h"
#include "utility.h"
#include "world.h"

s32 callIntrinsic(u16 number, const UsecodeArguments &args)
{
    switch (number) {
    case 0x000: /* @isAlertActive$qv (ALERT) */
        return static_cast<s32>(isAlertActive());
    case 0x001: /* @Item@frame$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->frame());
    case 0x002: /* @Item@setFrame$qus (ITEM) */
        args.pointerAt<Item>(0)->setFrame(args.wordAt(4));
        return 0;
    case 0x003: /* @Item@map$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->map());
    case 0x004: /* @Item@flags$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->flags());
    case 0x005: /* @Item@orFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->orFlags(args.wordAt(4));
        return 0;
    case 0x006: /* @Item@equip$qi (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->equip(static_cast<i16>(args.wordAt(4))));
    case 0x007: /* @Item@isOnScreen$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOnScreen());
    case 0x008: /* @Item@isNpc$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isNpc());
    case 0x009: /* @Item@z$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->z());
    case 0x00A: /* @gameDifficulty$qv (NPC) */
        return static_cast<s32>(gameDifficulty());
    case 0x00B: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x00C: /* @Item@destroy$qv (ITEM) */
        args.pointerAt<Item>(0)->destroy();
        return 0;
    case 0x00D: /* @Npc@waypointGroup$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->waypointGroup());
    case 0x00E: /* @Item@x$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->x());
    case 0x00F: /* @Item@y$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->y());
    case 0x010: /* @Item@playSound$qi (ITEM) */
        args.pointerAt<Item>(0)->playSound(static_cast<i16>(args.wordAt(4)));
        return 0;
    case 0x011: /* @Item@shape$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->shape());
    case 0x012: /* @explodeIntrinsic$qn4Itemiuc (EXPLODE) */
        explodeIntrinsic(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)), static_cast<u8>(args.wordAt(6)));
        return 0;
    case 0x013: /* @randomBetween$qii (UTILITY) */
        return static_cast<s32>(randomBetween(static_cast<i16>(args.wordAt(0)), static_cast<i16>(args.wordAt(2))));
    case 0x014: /* @Item@legalCreateAtCoords$qususususuc (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->legalCreateAtCoords(
            args.wordAt(4), args.wordAt(6), args.wordAt(8), args.wordAt(10), static_cast<u8>(args.wordAt(12))));
    case 0x015: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x016: /* @controlledNpcNumber$qv (CONTROL) */
        return static_cast<s32>(controlledNpcNumber());
    case 0x017: /* @Npc@direction$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->direction());
    case 0x018: /* @Npc@lastAnimSet$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->lastAnimSet());
    case 0x019: /* @fireWeapon$qn4Itemiiizciuc (CRUSADER) */
        return static_cast<s32>(fireWeapon(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                           static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                           static_cast<char>(args.wordAt(10)), static_cast<i16>(args.wordAt(12)),
                                           static_cast<u8>(args.wordAt(14))));
    case 0x01A: /* @Item@create$qusus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->create(args.wordAt(4), args.wordAt(6)));
    case 0x01B: /* @Item@popToCoords$qususuc (ITEM) */
        args.pointerAt<Item>(0)->popToCoords(args.wordAt(4), args.wordAt(6), static_cast<u8>(args.wordAt(8)));
        return 0;
    case 0x01C: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x01D: /* @Item@push$qv (ITEM) */
        args.pointerAt<Item>(0)->push();
        return 0;
    case 0x01E: /* @topEthereal$qv (ITEMCACH) */
        return static_cast<s32>(topEthereal());
    case 0x01F: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x020: /* @Item@setQualityLow$qus (ITEM) */
        args.pointerAt<Item>(0)->setQualityLow(args.wordAt(4));
        return 0;
    case 0x021: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x022: /* @Item@setQualityHigh$qus (ITEM) */
        args.pointerAt<Item>(0)->setQualityHigh(args.wordAt(4));
        return 0;
    case 0x023: /* @closestDirection$qiiiiiii (DISTANCE) */
        return static_cast<s32>(closestDirection(static_cast<i16>(args.wordAt(0)), static_cast<i16>(args.wordAt(2)),
                                                 static_cast<i16>(args.wordAt(4)), static_cast<i16>(args.wordAt(6)),
                                                 static_cast<i16>(args.wordAt(8)), static_cast<i16>(args.wordAt(10)),
                                                 static_cast<i16>(args.wordAt(12))));
    case 0x024: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x025: /* @Item@centreY$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreY());
    case 0x026: /* @Item@centreX$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreX());
    case 0x027: /* @createSpriteProcess$qususususususuc (SPRITE) */
        return static_cast<s32>(createSpriteProcess(args.wordAt(0), args.wordAt(2), args.wordAt(4), args.wordAt(6),
                                                    args.wordAt(8), args.wordAt(10), static_cast<u8>(args.wordAt(12))));
    case 0x028: /* @Item@setNpcNumber$quc (ITEM) */
        args.pointerAt<Item>(0)->setNpcNumber(static_cast<u8>(args.wordAt(4)));
        return 0;
    case 0x029: /* @Item@playLoop$qi (ITEM) */
        args.pointerAt<Item>(0)->playLoop(static_cast<i16>(args.wordAt(4)));
        return 0;
    case 0x02A: /* @Item@setShape$qus (ITEM) */
        args.pointerAt<Item>(0)->setShape(args.wordAt(4));
        return 0;
    case 0x02B: /* @Item@pop$qv (ITEM) */
        args.pointerAt<Item>(0)->pop();
        return 0;
    case 0x02C: /* @Item@stopSounds$qv (ITEM) */
        args.pointerAt<Item>(0)->stopSounds();
        return 0;
    case 0x02D: /* @Item@isCompletelyOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isCompletelyOn(args.wordAt(4)));
    case 0x02E: /* @Item@popToContainer$qus (ITEM) */
        args.pointerAt<Item>(0)->popToContainer(args.wordAt(4));
        return 0;
    case 0x02F: /* @Npc@hp$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->hp());
    case 0x030: /* @Npc@mana$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->mana());
    case 0x031: /* @Item@family$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->family());
    case 0x032: /* @Item@destroyContents$qv (ITEM) */
        args.pointerAt<Item>(0)->destroyContents();
        return 0;
    case 0x033: /* @Item@setSoundVolume$qizc (ITEM) */
        args.pointerAt<Item>(0)->setSoundVolume(static_cast<i16>(args.wordAt(4)), static_cast<char>(args.wordAt(6)));
        return 0;
    case 0x034: /* @Item@directionToItem$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->directionToItem(args.wordAt(4)));
    case 0x035: /* @Item@isSoundPlaying$qi (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isSoundPlaying(static_cast<i16>(args.wordAt(4))));
    case 0x036: /* @rangeIfVisible$qn4Itemus (CRUSADER) */
        return static_cast<s32>(rangeIfVisible(args.pointerAt<Item>(0), args.wordAt(4)));
    case 0x037: /* @Item@restartSound$qi (ITEM) */
        args.pointerAt<Item>(0)->restartSound(static_cast<i16>(args.wordAt(4)));
        return 0;
    case 0x038: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x039: /* @killItem$qusi (KERNEL) */
        killItem(args.wordAt(0), static_cast<i16>(args.wordAt(2)));
        return 0;
    case 0x03A: /* @Item@touch$qv (ITEM) */
        args.pointerAt<Item>(0)->touch();
        return 0;
    case 0x03B: /* @Item@eggId$qv (EGG) */
        return static_cast<s32>(args.pointerAt<Item>(0)->eggId());
    case 0x03C: /* @pickUp$qusuc (WORLD) */
        return static_cast<s32>(pickUp(args.wordAt(0), static_cast<u8>(args.wordAt(2))));
    case 0x03D: /* @npcMap$qnus (NPC) */
        return static_cast<s32>(npcMap(args.pointerAt<u16>(0)));
    case 0x03E: /* @Item@cast$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->cast(args.wordAt(4)));
    case 0x03F: /* @repaintCamera$qv (CAMERA) */
        repaintCamera();
        return 0;
    case 0x040: /* @Item@stopSound$qi (ITEM) */
        args.pointerAt<Item>(0)->stopSound(static_cast<i16>(args.wordAt(4)));
        return 0;
    case 0x041: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x042: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x043: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x044: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x045: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x046: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x047: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x048: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x049: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x04A: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x04B: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x04C: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x04D: /* @Npc@activity$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->activity());
    case 0x04E: /* @Npc@isDead$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isDead());
    case 0x04F: /* @Npc@clrInCombat$qv (NPC) */
        args.pointerAt<Npc>(0)->clrInCombat();
        return 0;
    case 0x050: /* @Npc@setDefaultActivity0$qus (NPC) */
        args.pointerAt<Npc>(0)->setDefaultActivity0(args.wordAt(4));
        return 0;
    case 0x051: /* @Npc@setDefaultActivity1$qus (NPC) */
        args.pointerAt<Npc>(0)->setDefaultActivity1(args.wordAt(4));
        return 0;
    case 0x052: /* @Npc@setDefaultActivity2$qus (NPC) */
        args.pointerAt<Npc>(0)->setDefaultActivity2(args.wordAt(4));
        return 0;
    case 0x053: /* @Npc@setActivity$qus (NPC) */
        args.pointerAt<Npc>(0)->setActivity(args.wordAt(4));
        return 0;
    case 0x054: /* @setControlledNpcNumber$qus (CONTROL) */
        setControlledNpcNumber(args.wordAt(0));
        return 0;
    case 0x055: /* @receiveHitIntrinsic$qn4Itemusucius (CRUSADER) */
        receiveHitIntrinsic(args.pointerAt<Item>(0), args.wordAt(4), static_cast<u8>(args.wordAt(6)),
                            static_cast<i16>(args.wordAt(8)), args.wordAt(10));
        return 0;
    case 0x056: /* @releaseBuild$qv (CRUSYS) */
        /* The original sets AX whole: mov ax,1 or xor ax,ax (1058:00cc). */
        return static_cast<s32>(releaseBuild());
    case 0x057: /* @Npc@setMana$qus (NPC) */
        args.pointerAt<Npc>(0)->setMana(args.wordAt(4));
        return 0;
    case 0x058: /* @Item@use$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->use());
    case 0x059: /* @Item@setQuality$qus (ITEM) */
        args.pointerAt<Item>(0)->setQuality(args.wordAt(4));
        return 0;
    case 0x05A: /* @playSong$qi (MUSIC) */
        playSong(static_cast<i16>(args.wordAt(0)));
        return 0;
    case 0x05B: /* @Item@surfaceWeight$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->surfaceWeight());
    case 0x05C: /* @Item@isCentreOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isCentreOn(args.wordAt(4)));
    case 0x05D: /* @Item@setFrame$qus (ITEM) */
        args.pointerAt<Item>(0)->setFrame(args.wordAt(4));
        return 0;
    case 0x05E: /* @Npc@lastAnimSet$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->lastAnimSet());
    case 0x05F: /* @setHeld$qus (NPC) */
        setHeld(args.wordAt(0));
        return 0;
    case 0x060: /* @Npc@isBusy$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isBusy());
    case 0x061: /* @Npc@isKneeling$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isKneeling());
    case 0x062: /* @Npc@doAnim$qusususuc (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->doAnim(args.wordAt(4), args.wordAt(6), args.wordAt(8),
                                                               static_cast<u8>(args.wordAt(10))));
    case 0x063: /* @Item@legalCreateAtPoint$qususn9ItemPoint (ITEM) */
        return static_cast<s32>(
            args.pointerAt<Item>(0)->legalCreateAtPoint(args.wordAt(4), args.wordAt(6), args.pointerAt<ItemPoint>(8)));
    case 0x064: /* @Item@getPoint$qn9ItemPoint (ITEM) */
        args.pointerAt<Item>(0)->getPoint(args.pointerAt<ItemPoint>(4));
        return 0;
    case 0x065: /* @Item@legalMoveToPoint$qn9ItemPointusus (ITEM) */
        return static_cast<s32>(
            args.pointerAt<Item>(0)->legalMoveToPoint(args.pointerAt<ItemPoint>(4), args.wordAt(8), args.wordAt(10)));
    case 0x066: /* @fall$qn4Item (GRAVITY) */
        fall(args.pointerAt<Item>(0));
        return 0;
    case 0x067: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x068: /* @countItemProcesses$qusi (KERNEL) */
        return static_cast<s32>(countItemProcesses(args.wordAt(0), static_cast<i16>(args.wordAt(2))));
    case 0x069: /* @Item@centreY$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreY());
    case 0x06A: /* @animationsOff$qv (WORLD) */
        return static_cast<s32>(animationsOff());
    case 0x06B: /* @resetKeyboardState$qv (WORLD) */
        resetKeyboardState();
        return 0;
    case 0x06C: /* @pauseMusic$qv (MUSIC) */
        pauseMusic();
        return 0;
    case 0x06D: /* @playMovie$qnzcuc (XANPLAY) */
        playMovie(args.pointerAt<const char>(0), static_cast<u8>(args.wordAt(4)));
        return 0;
    case 0x06E: /* @resumeMusic$qv (MUSIC) */
        resumeMusic();
        return 0;
    case 0x06F: /* @Item@isInNpc$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isInNpc());
    case 0x070: /* @setStasis$qv (WORLD) */
        setStasis();
        return 0;
    case 0x071: /* @clearStasis$qv (WORLD) */
        clearStasis();
        return 0;
    case 0x072: /* @jumpToColor$qiii (FADE) */
        jumpToColor(static_cast<i16>(args.wordAt(0)), static_cast<i16>(args.wordAt(2)),
                    static_cast<i16>(args.wordAt(4)));
        return 0;
    case 0x073: /* @fadeToGamePalette$qus (FADE) */
        return static_cast<s32>(fadeToGamePalette(args.wordAt(0)));
    case 0x074: /* @Npc@isDead$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isDead());
    case 0x075: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x076: /* @Item@nothing$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->nothing());
    case 0x077: /* @violenceEnabled$qv (CRUSYS) */
        /* The original sets AX whole: mov ax,1 or xor ax,ax (1058:00b8). */
        return static_cast<s32>(violenceEnabled());
    case 0x078: /* @Item@unequip$qi (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->unequip(static_cast<i16>(args.wordAt(4))));
    case 0x079: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x07A: /* @Item@move$qususuc (ITEM) */
        args.pointerAt<Item>(0)->move(args.wordAt(4), args.wordAt(6), static_cast<u8>(args.wordAt(8)));
        return 0;
    case 0x07B: /* @isTeleporting$qv (WORLD) */
        return static_cast<s32>(isTeleporting());
    case 0x07C: /* @setTeleporting$qv (WORLD) */
        setTeleporting();
        return 0;
    case 0x07D: /* @clearTeleporting$qv (WORLD) */
        clearTeleporting();
        return 0;
    case 0x07E: /* @Npc@turnToward$qucuc (NPC) */
        return static_cast<s32>(
            args.pointerAt<Npc>(0)->turnToward(static_cast<u8>(args.wordAt(4)), static_cast<u8>(args.wordAt(6))));
    case 0x07F: /* @fadeOut$qv (FADE) */
        return static_cast<s32>(fadeOut());
    case 0x080: /* @clearKeycards$qv (NPC) */
        clearKeycards();
        return 0;
    case 0x081: /* @stopMusic$qv (MUSIC) */
        stopMusic();
        return 0;
    case 0x082: /* @jumpToBlack$qv (FADE) */
        jumpToBlack();
        return 0;
    case 0x083: /* @suppressMovieRestore$qv (WORLD) */
        suppressMovieRestore();
        return 0;
    case 0x084: /* @allowMovieRestore$qv (WORLD) */
        allowMovieRestore();
        return 0;
    case 0x085: /* @startMapJump$qus (MAPJUMP) */
        startMapJump(args.wordAt(0));
        return 0;
    case 0x086: /* @teleportToEgg$qususuc (EGG) */
        return static_cast<s32>(teleportToEgg(args.wordAt(0), args.wordAt(2), static_cast<u8>(args.wordAt(4))));
    case 0x087: /* @fadeIn$qv (FADE) */
        return static_cast<s32>(fadeIn());
    case 0x088: /* @Npc@clearImmortal$qv (NPC) */
        args.pointerAt<Npc>(0)->clearImmortal();
        return 0;
    case 0x089: /* @Npc@setActivity$qus (NPC) */
        args.pointerAt<Npc>(0)->setActivity(args.wordAt(4));
        return 0;
    case 0x08A: /* @Item@familyQuality$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->familyQuality());
    case 0x08B: /* @Item@setFamilyQuality$qus (ITEM) */
        args.pointerAt<Item>(0)->setFamilyQuality(args.wordAt(4));
        return 0;
    case 0x08C: /* @Npc@maxEnergy$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->maxEnergy());
    case 0x08D: /* @moveCamera$qususucus (CAMERA) */
        moveCamera(args.wordAt(0), args.wordAt(2), static_cast<u8>(args.wordAt(4)), args.wordAt(6));
        return 0;
    case 0x08E: /* @Npc@setImmortal$qv (NPC) */
        args.pointerAt<Npc>(0)->setImmortal();
        return 0;
    case 0x08F: /* @cameraX$qv (CAMERA) */
        return static_cast<s32>(cameraX());
    case 0x090: /* @cameraY$qv (CAMERA) */
        return static_cast<s32>(cameraY());
    case 0x091: /* @Item@setMap$qi (ITEM) */
        args.pointerAt<Item>(0)->setMap(static_cast<i16>(args.wordAt(4)));
        return 0;
    case 0x092: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x093: /* @Item@shoot$qn9ItemPointusus (ITEM) */
        args.pointerAt<Item>(0)->shoot(args.pointerAt<ItemPoint>(4), args.wordAt(8), args.wordAt(10));
        return 0;
    case 0x094: /* @setFollowedItem$qus (CAMERA) */
        setFollowedItem(args.wordAt(0));
        return 0;
    case 0x095: /* @Item@enterFastArea$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->enterFastArea());
    case 0x096: /* @Item@setBroken$qv (ITEM) */
        args.pointerAt<Item>(0)->setBroken();
        return 0;
    case 0x097: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x098: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x099: /* @moveKeyDownRecently$qv (ITEM) */
        return static_cast<s32>(moveKeyDownRecently());
    case 0x09A: /* @teleportToEgg$qusuc (EGG) */
        return static_cast<s32>(teleportToEgg(args.wordAt(0), static_cast<u8>(args.wordAt(2))));
    case 0x09B: /* @Npc@createActor$qusus (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->createActor(args.wordAt(4), args.wordAt(6)));
    case 0x09C: /* @Npc@clrInCombat$qv (NPC) */
        args.pointerAt<Npc>(0)->clrInCombat();
        return 0;
    case 0x09D: /* @jumpToGreyScale$qv (GAMEPAL) */
        jumpToGreyScale();
        return 0;
    case 0x09E: /* @jumpToGamePalette$qv (FADE) */
        jumpToGamePalette();
        return 0;
    case 0x09F: /* @showStatusGump$qv (STATUS) */
        showStatusGump();
        return 0;
    case 0x0A0: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x0A1: /* @Item@eggType$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->eggType());
    case 0x0A2: /* @Item@setEggXRange$quc (EGG) */
        args.pointerAt<Item>(0)->setEggXRange(static_cast<u8>(args.wordAt(4)));
        return 0;
    case 0x0A3: /* @Item@setFrame$qus (ITEM) */
        args.pointerAt<Item>(0)->setFrame(args.wordAt(4));
        return 0;
    case 0x0A4: /* @Item@overlaps$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->overlaps(args.wordAt(4)));
    case 0x0A5: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x0A6: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x0A7: /* @Npc@lastAnimSet$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->lastAnimSet());
    case 0x0A8: /* @Item@centreY$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreY());
    case 0x0A9: /* @canExistAt$qusususus (EXPLODE) */
        return static_cast<s32>(canExistAt(args.wordAt(0), args.wordAt(2), args.wordAt(4), args.wordAt(6)));
    case 0x0AA: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x0AB: /* @Npc@isDead$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isDead());
    case 0x0AC: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0AD: /* @Item@inFastArea$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->inFastArea());
    case 0x0AE: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x0AF: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x0B0: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0B1: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x0B2: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0B3: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x0B4: /* @Item@directionTo$qusus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->directionTo(args.wordAt(4), args.wordAt(6)));
    case 0x0B5: /* @removeInventoryItem$qus (WORLD) */
        return static_cast<s32>(removeInventoryItem(args.wordAt(0)));
    case 0x0B6: /* @updateInventoryDisplay$qv (WORLD) */
        updateInventoryDisplay();
        return 0;
    case 0x0B7: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0B8: /* @Item@centreY$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreY());
    case 0x0B9: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x0BA: /* @Item@footpad$qnit1t1 (ITEM) */
        args.pointerAt<Item>(0)->footpad(args.pointerAt<i16>(4), args.pointerAt<i16>(8), args.pointerAt<i16>(12));
        return 0;
    case 0x0BB: /* @Npc@isDead$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isDead());
    case 0x0BC: /* @Item@createNpc$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->createNpc(args.wordAt(4)));
    case 0x0BD: /* @Npc@setActivity$qus (NPC) */
        args.pointerAt<Npc>(0)->setActivity(args.wordAt(4));
        return 0;
    case 0x0BE: /* @showKeypad$qi (KEYPAD) */
        return static_cast<s32>(showKeypad(static_cast<i16>(args.wordAt(0))));
    case 0x0BF: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x0C0: /* @showComputerText$qnzc (COMPGUMP) */
        showComputerText(args.pointerAt<const char>(0));
        return 0;
    case 0x0C1: /* @numberString$qus (INTER) */
        return static_cast<s32>(numberString(args.wordAt(0)));
    case 0x0C2: /* @Item@nothing$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->nothing());
    case 0x0C3: /* @Npc@direction$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->direction());
    case 0x0C4: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x0C5: /* @Item@setFamilyQuality$qus (ITEM) */
        args.pointerAt<Item>(0)->setFamilyQuality(args.wordAt(4));
        return 0;
    case 0x0C6: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0C7: /* @Npc@addHp$qi (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->addHp(static_cast<i16>(args.wordAt(4))));
    case 0x0C8: /* @createHealer$qv (CRUSADER) */
        return static_cast<s32>(createHealer());
    case 0x0C9: /* @Item@equip$qi (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->equip(static_cast<i16>(args.wordAt(4))));
    case 0x0CA: /* @Item@setBroken$qv (ITEM) */
        args.pointerAt<Item>(0)->setBroken();
        return 0;
    case 0x0CB: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x0CC: /* @Npc@teleport$qususucuc (NPC) */
        args.pointerAt<Npc>(0)->teleport(args.wordAt(4), args.wordAt(6), static_cast<u8>(args.wordAt(8)),
                                         static_cast<u8>(args.wordAt(10)));
        return 0;
    case 0x0CD: /* @fineDirection$qiiii (DISTANCE) */
        /* The case returns AX as the original leaves it. */
        {
            /* The original returns with mov al,direction. On an axis (dx or dy 0)
             * it has not touched AX, whose high byte is still the one
             * OP_CALL_INTRINSIC left there, the selector of the thread's stack
             * (13f0:308d); elsewhere a comparison has cleared AH (10e0:02ff). The
             * WALLGUN compares the word with its direction (class 0A19, function
             * 2D). */
            const i16 x1 = static_cast<i16>(args.wordAt(0));
            const i16 y1 = static_cast<i16>(args.wordAt(2));
            const i16 x2 = static_cast<i16>(args.wordAt(4));
            const i16 y2 = static_cast<i16>(args.wordAt(6));
            const u8 direction = static_cast<u8>(fineDirection(x1, y1, x2, y2));
            if (x2 == x1 || y1 == y2) {
                return static_cast<s32>((args.stack() & 0xff00) | direction);
            }
            return direction;
        }
    case 0x0CE: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x0CF: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x0D0: /* @Npc@isInCombat$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isInCombat());
    case 0x0D1: /* @Npc@lastActivity$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->lastActivity());
    case 0x0D2: /* @Npc@setCombatTactic$qus (NPC) */
        args.pointerAt<Npc>(0)->setCombatTactic(args.wordAt(4));
        return 0;
    case 0x0D3: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x0D4: /* @cameraY$qv (CAMERA) */
        return static_cast<s32>(cameraY());
    case 0x0D5: /* @Npc@equipment$qi (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->equipment(static_cast<i16>(args.wordAt(4))));
    case 0x0D6: /* @Npc@setEquip$qusus (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->setEquip(args.wordAt(4), args.wordAt(6)));
    case 0x0D7: /* @Npc@defaultActivity0$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->defaultActivity0());
    case 0x0D8: /* @Npc@defaultActivity1$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->defaultActivity1());
    case 0x0D9: /* @Npc@defaultActivity2$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->defaultActivity2());
    case 0x0DA: /* @Npc@lastAnimSet$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->lastAnimSet());
    case 0x0DB: /* @Npc@isFalling$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isFalling());
    case 0x0DC: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x0DD: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x0DE: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0DF: /* @Npc@setWaypointGroup$qus (NPC) */
        args.pointerAt<Npc>(0)->setWaypointGroup(args.wordAt(4));
        return 0;
    case 0x0E0: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0E1: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x0E2: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x0E3: /* @Item@centreY$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreY());
    case 0x0E4: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0E5: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0E6: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0E7: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0E8: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0E9: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0EA: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0EB: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0EC: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0ED: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0EE: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0EF: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0F0: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0F1: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0F2: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0F3: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0F4: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0F5: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x0F6: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x0F7: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x0F8: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x0F9: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x0FA: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x0FB: /* @Npc@maxHp$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->maxHp());
    case 0x0FC: /* @Npc@setHp$qus (NPC) */
        args.pointerAt<Npc>(0)->setHp(args.wordAt(4));
        return 0;
    case 0x0FD: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x0FE: /* @createBatteryCharger$qv (CRUSADER) */
        return static_cast<s32>(createBatteryCharger());
    case 0x0FF: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x100: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x101: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x102: /* @Npc@isDead$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isDead());
    case 0x103: /* @Npc@setActivity$qus (NPC) */
        args.pointerAt<Npc>(0)->setActivity(args.wordAt(4));
        return 0;
    case 0x104: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x105: /* @Npc@lastAnimSet$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->lastAnimSet());
    case 0x106: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x107: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x108: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x109: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x10A: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x10B: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x10C: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x10D: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x10E: /* @Item@centreY$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreY());
    case 0x10F: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x110: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x111: /* @hasKeycard$quc (NPC) */
        return static_cast<s32>(hasKeycard(static_cast<u8>(args.wordAt(0))));
    case 0x112: /* @Item@nothing$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->nothing());
    case 0x113: /* @Npc@isDead$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isDead());
    case 0x114: /* @Npc@clearImmortal$qv (NPC) */
        args.pointerAt<Npc>(0)->clearImmortal();
        return 0;
    case 0x115: /* @numberString$qus (INTER) */
        return static_cast<s32>(numberString(args.wordAt(0)));
    case 0x116: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x117: /* @Npc@setActivity$qus (NPC) */
        args.pointerAt<Npc>(0)->setActivity(args.wordAt(4));
        return 0;
    case 0x118: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x119: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x11A: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x11B: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x11C: /* @isNpcType$qus (ITEM) */
        return static_cast<s32>(isNpcType(args.wordAt(0)));
    case 0x11D: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x11E: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x11F: /* @Item@avatarStoleSomething$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->avatarStoleSomething(args.wordAt(4)));
    case 0x120: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x121: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x122: /* @Item@q$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->q());
    case 0x123: /* @Item@setQ$qus (ITEM) */
        args.pointerAt<Item>(0)->setQ(args.wordAt(4));
        return 0;
    case 0x124: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x125: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x126: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x127: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x128: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x129: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x12A: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x12B: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x12C: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x12D: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x12E: /* @Npc@direction$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->direction());
    case 0x12F: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x130: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x131: /* @fireDistance$qn4Itemusuciii (CRUSADER) */
        return static_cast<s32>(fireDistance(args.pointerAt<Item>(0), args.wordAt(4), static_cast<u8>(args.wordAt(6)),
                                             static_cast<i16>(args.wordAt(8)), static_cast<i16>(args.wordAt(10)),
                                             static_cast<i16>(args.wordAt(12))));
    case 0x132: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x133: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x134: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x135: /* @cameraY$qv (CAMERA) */
        return static_cast<s32>(cameraY());
    case 0x136: /* @cameraZ$qv (CAMERA) */
        return static_cast<s32>(cameraZ());
    case 0x137: /* @hideStatusGump$qv (STATUS) */
        hideStatusGump();
        return 0;
    case 0x138: /* @Npc@clrInCombat$qv (NPC) */
        args.pointerAt<Npc>(0)->clrInCombat();
        return 0;
    case 0x139: /* @Item@typeFlag$qi (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->typeFlag(static_cast<i16>(args.wordAt(4))));
    case 0x13A: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x13B: /* @hurl$qn4Itemiiii (GRAVITY) */
        return static_cast<s32>(hurl(args.pointerAt<Item>(0), static_cast<i16>(args.wordAt(4)),
                                     static_cast<i16>(args.wordAt(6)), static_cast<i16>(args.wordAt(8)),
                                     static_cast<i16>(args.wordAt(10))));
    case 0x13C: /* @Item@centreY$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreY());
    case 0x13D: /* @Item@centreZ$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->centreZ());
    case 0x13E: /* @Item@setFrame$qus (ITEM) */
        args.pointerAt<Item>(0)->setFrame(args.wordAt(4));
        return 0;
    case 0x13F: /* @playEffect$qi (DSFXMAN) */
        playEffect(static_cast<i16>(args.wordAt(0)));
        return 0;
    case 0x140: /* @isEffectPlaying$qi (DSFXMAN) */
        return static_cast<s32>(isEffectPlaying(static_cast<i16>(args.wordAt(0))));
    case 0x141: /* @clearAlertActive$qv (ALERT) */
        clearAlertActive();
        return 0;
    case 0x142: /* @fadeTowardColor$qiiiusus (FADE) */
        return static_cast<s32>(fadeTowardColor(static_cast<i16>(args.wordAt(0)), static_cast<i16>(args.wordAt(2)),
                                                static_cast<i16>(args.wordAt(4)), args.wordAt(6), args.wordAt(8)));
    case 0x143: /* @Npc@isDead$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->isDead());
    case 0x144: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x145: /* @playCredits$qv (WORLD) */
        return static_cast<s32>(playCredits());
    case 0x146: /* @jumpToWhite$qv (FADE) */
        jumpToWhite();
        return 0;
    case 0x147: /* @familyOfType$qi (TYPE) */
        return static_cast<s32>(familyOfType(static_cast<i16>(args.wordAt(0))));
    case 0x148: /* @Item@npcNumber$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->npcNumber());
    case 0x149: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x14A: /* @Item@andFlags$qus (ITEM) */
        args.pointerAt<Item>(0)->andFlags(args.wordAt(4));
        return 0;
    case 0x14B: /* @currentTimerTick$qv (GAMETIME) */
        return static_cast<s32>(currentTimerTick());
    case 0x14C: /* @setAlertActive$qv (ALERT) */
        setAlertActive();
        return 0;
    case 0x14D: /* @isHeld$qv (NPC) */
        return static_cast<s32>(isHeld());
    case 0x14E: /* @pickUp$qusuc (WORLD) */
        return static_cast<s32>(pickUp(args.wordAt(0), static_cast<u8>(args.wordAt(2))));
    case 0x14F: /* @Item@eggXRange$qv (EGG) */
        return static_cast<s32>(args.pointerAt<Item>(0)->eggXRange());
    case 0x150: /* @Npc@clrInCombat$qv (NPC) */
        args.pointerAt<Npc>(0)->clrInCombat();
        return 0;
    case 0x151: /* @jumpToColor$qiii (FADE) */
        jumpToColor(static_cast<i16>(args.wordAt(0)), static_cast<i16>(args.wordAt(2)),
                    static_cast<i16>(args.wordAt(4)));
        return 0;
    case 0x152: /* @Item@setFrame$qus (ITEM) */
        args.pointerAt<Item>(0)->setFrame(args.wordAt(4));
        return 0;
    case 0x153: /* @numberString$qus (INTER) */
        return static_cast<s32>(numberString(args.wordAt(0)));
    case 0x154: /* @Npc@direction$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->direction());
    case 0x155: /* @numberString$qus (INTER) */
        return static_cast<s32>(numberString(args.wordAt(0)));
    case 0x156: /* @Item@isOn$qus (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->isOn(args.wordAt(4)));
    case 0x157: /* @Npc@direction$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->direction());
    case 0x158: /* @Npc@setDead$qv (NPC) */
        args.pointerAt<Npc>(0)->setDead();
        return 0;
    case 0x159: /* @Item@qualityHigh$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityHigh());
    case 0x15A: /* @Item@qualityLow$qv (ITEM) */
        return static_cast<s32>(args.pointerAt<Item>(0)->qualityLow());
    case 0x15B: /* @numberString$qus (INTER) */
        return static_cast<s32>(numberString(args.wordAt(0)));
    case 0x15C: /* @Npc@direction$qv (NPC) */
        return static_cast<s32>(args.pointerAt<Npc>(0)->direction());
    }
    hostMissingIntrinsic(number);
}
