/* CONTROL.C (Code42, 1148), No Regret's own part of it (the rest is
 * common/src/game/control.cpp's): the globals of the saved game whose layout
 * is No Regret's (CONTROLLER_GLOBALS). */
#include "control.h"
#include "gamestate.h"
#include "savefields.h"

namespace {
u32 &unknown2f8c = gameGlobal<u32>(0);
u32 &unknown2fba = gameGlobal<u32>(5000);
u16 &unknown2fbe = gameGlobal<u16>(0);
}

/* The globals Controller::save writes whole: the original's data segment
 * from fireHeld (1480:2F7B) to the word after targetItem, frameSkip, 79
 * bytes, which the port's globals are not laid out as. Each with its size
 * in the original; the save's tables are checked against the original's
 * offsets. */
const SaveGlobal CONTROLLER_GLOBALS[] = {
    SAVE_GLOBAL(fireHeld, 1, GLOBAL_AS_IS),        SAVE_GLOBAL(keysEnabled, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(firing, 1, GLOBAL_AS_IS),          SAVE_GLOBAL(controllerInTimer, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(shotsLeft, 2, GLOBAL_AS_IS),       SAVE_GLOBAL(movedActor, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(unknown2f83, 2, GLOBAL_AS_IS),     SAVE_GLOBAL(reticleAngle, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(reticleSprite, 2, GLOBAL_AS_IS),   SAVE_GLOBAL(reticleMoved, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(reticleShown, 1, GLOBAL_AS_IS),    SAVE_GLOBAL(cannotWalk, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(unknown2f8c, 4, GLOBAL_AS_IS),     SAVE_GLOBAL(dodgeStart, 4, GLOBAL_AS_IS),
    SAVE_GLOBAL(dodgeLength, 4, GLOBAL_AS_IS),     SAVE_GLOBAL(lastAlert, 4, GLOBAL_AS_IS),
    SAVE_GLOBAL(silencerSprite, 2, GLOBAL_AS_IS),  SAVE_GLOBAL(dodgeFactor, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(shotgunPumped, 1, GLOBAL_AS_IS),   SAVE_GLOBAL(firedSincePress, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(firePressed, 4, GLOBAL_AS_IS),     SAVE_GLOBAL(fireHeldFor, 4, GLOBAL_AS_IS),
    SAVE_GLOBAL(lastStep, 4, GLOBAL_AS_IS),        SAVE_GLOBAL(targeting, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(reticleHolds, 2, GLOBAL_AS_IS),    SAVE_GLOBAL(targetSprite, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(targetSpritePid, 2, GLOBAL_AS_IS), SAVE_GLOBAL(lastFacing, 1, GLOBAL_AS_IS),
    SAVE_GLOBAL(targetTime, 4, GLOBAL_AS_IS),      SAVE_GLOBAL(unknown2fba, 4, GLOBAL_AS_IS),
    SAVE_GLOBAL(unknown2fbe, 2, GLOBAL_AS_IS),     SAVE_GLOBAL(bossShots, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(reticleStyle, 2, GLOBAL_AS_IS),    SAVE_GLOBAL(bossShield, 2, GLOBAL_AS_IS),
    SAVE_GLOBAL(targetItem, 2, GLOBAL_AS_IS),      SAVE_GLOBAL(frameSkip, 2, GLOBAL_AS_IS)};
const u16 CONTROLLER_GLOBAL_COUNT = sizeof(CONTROLLER_GLOBALS) / sizeof(CONTROLLER_GLOBALS[0]);
