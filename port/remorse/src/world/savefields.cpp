/* The saved game's schema, generated from BCC's layout of the classes,
 * the original's vtables and the pointers' kinds; do not edit. For each class under Process, the function that moves
 * its fields between the port's object and the original's bytes at
 * BCC's offsets, its base's first. See savefields.h. */
#include "savefields.h"
#include "activity.h"
#include "anchor.h"
#include "animprim.h"
#include "assproc.h"
#include "attack.h"
#include "basecam.h"
#include "biostimr.h"
#include "camera.h"
#include "control.h"
#include "crusader.h"
#include "crusys.h"
#include "cycle.h"
#include "dispatch.h"
#include "display.h"
#include "dumbtimr.h"
#include "egg.h"
#include "fade.h"
#include "flicplay.h"
#include "gamepal.h"
#include "gametime.h"
#include "gravity.h"
#include "irgump.h"
#include "item.h"
#include "joydrv.h"
#include "kdaemon.h"
#include "keyio.h"
#include "kprocess.h"
#include "mapjump.h"
#include "mousectl.h"
#include "music.h"
#include "pathfind.h"
#include "refstore.h"
#include "snap.h"
#include "sprite.h"
#include "stdinter.h"
#include "systimer.h"
#include "u9mouse.h"
#include "uprocess.h"
#include "world.h"

#include <typeinfo>

namespace {
void fieldsProcess(SaveImage &image, Process &p)
{
    {
        auto value = p.Process::pid;
        image.integer(0x2, value);
        p.Process::pid = value;
    }
    {
        auto value = p.Process::item;
        image.integer(0x4, value);
        p.Process::item = value;
    }
    {
        auto value = p.Process::type;
        image.integer(0x6, value);
        p.Process::type = value;
    }
    {
        auto value = p.Process::priority;
        image.integer(0x8, value);
        p.Process::priority = value;
    }
    {
        auto value = p.Process::reserved;
        image.integer(0x9, value);
        p.Process::reserved = value;
    }
    {
        auto value = p.Process::timeoutStart;
        image.integer(0xa, value);
        p.Process::timeoutStart = value;
    }
    {
        auto value = p.Process::timeout;
        image.integer(0xe, value);
        p.Process::timeout = value;
    }
    {
        auto value = p.Process::result;
        image.integer(0x12, value);
        p.Process::result = value;
    }
    {
        auto value = p.Process::flags;
        image.integer(0x16, value);
        p.Process::flags = value;
    }
    {
        auto value = p.Process::status;
        image.integer(0x18, value);
        p.Process::status = value;
    }
    {
        auto value = p.Process::allocations;
        image.integer(0x1a, value);
        p.Process::allocations = value;
    }
    {
        auto value = p.Process::messageMask;
        image.integer(0x1c, value);
        p.Process::messageMask = value;
    }
    image.vtable(0x1e, 0x2d10);
    {
        auto value = p.Process::waitList.MList::count;
        image.integer(0x20, value);
        p.Process::waitList.MList::count = value;
    }
    {
        auto value = p.Process::waitList.MList::tag;
        image.integer(0x22, value);
        p.Process::waitList.MList::tag = value;
    }
    image.pointer(0x24, SAVE_LIST, p.Process::waitList.MList::data);
    image.vtable(0x28, 0x3afe);
    {
        auto value = p.Process::relations.MList::count;
        image.integer(0x2a, value);
        p.Process::relations.MList::count = value;
    }
    {
        auto value = p.Process::relations.MList::tag;
        image.integer(0x2c, value);
        p.Process::relations.MList::tag = value;
    }
    image.pointer(0x2e, SAVE_LIST, p.Process::relations.MList::data);
}

void fieldsEventProcess(SaveImage &image, EventProcess &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.EventProcess::eventMask;
        image.integer(0x32, value);
        p.EventProcess::eventMask = value;
    }
}

void fieldsPeriodicProcess(SaveImage &image, PeriodicProcess &p)
{
    fieldsEventProcess(image, p);
    {
        auto value = p.PeriodicProcess::rate;
        image.integer(0x36, value);
        p.PeriodicProcess::rate = value;
    }
    {
        auto value = p.PeriodicProcess::period;
        image.integer(0x38, value);
        p.PeriodicProcess::period = value;
    }
    {
        auto value = p.PeriodicProcess::elapsed;
        image.integer(0x3c, value);
        p.PeriodicProcess::elapsed = value;
    }
}

void fieldsASSProcess(SaveImage &image, ASSProcess &p)
{
    fieldsPeriodicProcess(image, p);
}

void fieldsAccWait(SaveImage &image, AccWait &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.AccWait::start;
        image.integer(0x40, value);
        p.AccWait::start = value;
    }
    {
        auto value = p.AccWait::remaining;
        image.integer(0x44, value);
        p.AccWait::remaining = value;
    }
    {
        auto value = p.AccWait::count;
        image.integer(0x46, value);
        p.AccWait::count = value;
    }
}

void fieldsActivity(SaveImage &image, Activity &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.Activity::npc;
        image.integer(0x40, value);
        p.Activity::npc = value;
    }
}

void fieldsAnimPrimitive(SaveImage &image, AnimPrimitive &p)
{
    fieldsEventProcess(image, p);
    {
        auto value = p.AnimPrimitive::anim;
        image.integer(0x36, value);
        p.AnimPrimitive::anim = value;
    }
    {
        auto value = p.AnimPrimitive::direction;
        image.integer(0x38, value);
        p.AnimPrimitive::direction = value;
    }
    {
        auto value = p.AnimPrimitive::frames;
        image.integer(0x39, value);
        p.AnimPrimitive::frames = value;
    }
    {
        auto value = p.AnimPrimitive::started;
        image.integer(0x3a, value);
        p.AnimPrimitive::started = value;
    }
    {
        auto value = p.AnimPrimitive::stayPut;
        image.integer(0x3b, value);
        p.AnimPrimitive::stayPut = value;
    }
    {
        auto value = p.AnimPrimitive::blockedByItem;
        image.integer(0x3c, value);
        p.AnimPrimitive::blockedByItem = value;
    }
    {
        auto value = p.AnimPrimitive::reachedTarget;
        image.integer(0x3d, value);
        p.AnimPrimitive::reachedTarget = value;
    }
    {
        auto value = p.AnimPrimitive::loopAtEnd;
        image.integer(0x3e, value);
        p.AnimPrimitive::loopAtEnd = value;
    }
    {
        auto value = p.AnimPrimitive::failPending;
        image.integer(0x3f, value);
        p.AnimPrimitive::failPending = value;
    }
    {
        auto value = p.AnimPrimitive::reticleHidden;
        image.integer(0x40, value);
        p.AnimPrimitive::reticleHidden = value;
    }
    {
        auto value = p.AnimPrimitive::fast;
        image.integer(0x41, value);
        p.AnimPrimitive::fast = value;
    }
    {
        auto value = p.AnimPrimitive::status.AnimStatus::value;
        image.integer(0x42, value);
        p.AnimPrimitive::status.AnimStatus::value = value;
    }
    {
        auto value = p.AnimPrimitive::touchItem;
        image.integer(0x46, value);
        p.AnimPrimitive::touchItem = value;
    }
    {
        auto value = p.AnimPrimitive::unknown49;
        image.integer(0x48, value);
        p.AnimPrimitive::unknown49 = value;
    }
    for (u16 i = 0; i < 2; i++) {
        auto value = p.AnimPrimitive::unknown4d[i];
        image.integer(0x4e + (i * 1), value);
        p.AnimPrimitive::unknown4d[i] = value;
    }
    image.pointer(0x50, SAVE_TRANSIENT, p.AnimPrimitive::targetPoint);
    {
        auto value = p.AnimPrimitive::savedFirstStep;
        image.integer(0x54, value);
        p.AnimPrimitive::savedFirstStep = value;
    }
    {
        auto value = p.AnimPrimitive::savedLastAnim;
        image.integer(0x55, value);
        p.AnimPrimitive::savedLastAnim = value;
    }
    {
        auto value = p.AnimPrimitive::savedDirection;
        image.integer(0x57, value);
        p.AnimPrimitive::savedDirection = value;
    }
    {
        auto value = p.AnimPrimitive::savedFlipped;
        image.integer(0x58, value);
        p.AnimPrimitive::savedFlipped = value;
    }
    {
        auto value = p.AnimPrimitive::savedUnknown77_4;
        image.integer(0x59, value);
        p.AnimPrimitive::savedUnknown77_4 = value;
    }
    {
        auto value = p.AnimPrimitive::savedAnimFrame;
        image.integer(0x5a, value);
        p.AnimPrimitive::savedAnimFrame = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::frames;
        image.bits(0x4a, 0, 8, value);
        p.AnimPrimitive::info.AnimInfo::frames = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::twoStep;
        image.bits(0x4b, 0, 1, value);
        p.AnimPrimitive::info.AnimInfo::twoStep = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::attack;
        image.bits(0x4b, 1, 1, value);
        p.AnimPrimitive::info.AnimInfo::attack = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::looping;
        image.bits(0x4b, 2, 1, value);
        p.AnimPrimitive::info.AnimInfo::looping = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::unstoppable;
        image.bits(0x4b, 3, 1, value);
        p.AnimPrimitive::info.AnimInfo::unstoppable = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::unknown0c;
        image.bits(0x4b, 4, 1, value);
        p.AnimPrimitive::info.AnimInfo::unknown0c = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::unknown0d;
        image.bits(0x4b, 5, 1, value);
        p.AnimPrimitive::info.AnimInfo::unknown0d = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::endLoop;
        image.bits(0x4b, 6, 1, value);
        p.AnimPrimitive::info.AnimInfo::endLoop = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::flying;
        image.bits(0x4b, 7, 1, value);
        p.AnimPrimitive::info.AnimInfo::flying = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::repeat;
        image.bits(0x4c, 0, 4, value);
        p.AnimPrimitive::info.AnimInfo::repeat = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::rotated;
        image.bits(0x4c, 4, 1, value);
        p.AnimPrimitive::info.AnimInfo::rotated = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::unknown15;
        image.bits(0x4c, 5, 5, value);
        p.AnimPrimitive::info.AnimInfo::unknown15 = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::unknown1a;
        image.bits(0x4d, 2, 4, value);
        p.AnimPrimitive::info.AnimInfo::unknown1a = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::sixteenDirections;
        image.bits(0x4d, 6, 1, value);
        p.AnimPrimitive::info.AnimInfo::sixteenDirections = value;
    }
    {
        u32 value = p.AnimPrimitive::info.AnimInfo::destroyActor;
        image.bits(0x4d, 7, 1, value);
        p.AnimPrimitive::info.AnimInfo::destroyActor = value;
    }
}

void fieldsAdvanceLargeAnim(SaveImage &image, AdvanceLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsAdvanceSmallAnim(SaveImage &image, AdvanceSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsAnim06(SaveImage &image, Anim06 &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsAnim11(SaveImage &image, Anim11 &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsAnim13(SaveImage &image, Anim13 &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsAnimationProcess(SaveImage &image, AnimationProcess &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.AnimationProcess::ticks;
        image.integer(0x40, value);
        p.AnimationProcess::ticks = value;
    }
    {
        auto value = p.AnimationProcess::time;
        image.integer(0x44, value);
        p.AnimationProcess::time = value;
    }
    {
        auto value = p.AnimationProcess::gameTicks;
        image.integer(0x48, value);
        p.AnimationProcess::gameTicks = value;
    }
    {
        auto value = p.AnimationProcess::gameQuarters;
        image.integer(0x4c, value);
        p.AnimationProcess::gameQuarters = value;
    }
    {
        auto value = p.AnimationProcess::newTicks;
        image.integer(0x50, value);
        p.AnimationProcess::newTicks = value;
    }
    {
        auto value = p.AnimationProcess::frozen;
        image.integer(0x54, value);
        p.AnimationProcess::frozen = value;
    }
}

void fieldsAttack(SaveImage &image, Attack &p)
{
    fieldsActivity(image, p);
    {
        auto value = p.Attack::unknown48;
        image.integer(0x42, value);
        p.Attack::unknown48 = value;
    }
    {
        auto value = p.Attack::homeDirection;
        image.integer(0x43, value);
        p.Attack::homeDirection = value;
    }
    image.pointer(0x45, SAVE_TACTIC, p.Attack::tactic);
    image.pointer(0x49, SAVE_TACTIC, p.Attack::script);
    {
        auto value = p.Attack::position;
        image.integer(0x4d, value);
        p.Attack::position = value;
    }
    {
        auto value = p.Attack::block;
        image.integer(0x4f, value);
        p.Attack::block = value;
    }
    {
        auto value = p.Attack::tacticNumber;
        image.integer(0x51, value);
        p.Attack::tacticNumber = value;
    }
    {
        auto value = p.Attack::unknown59;
        image.integer(0x53, value);
        p.Attack::unknown59 = value;
    }
    {
        auto value = p.Attack::blockStart;
        image.integer(0x55, value);
        p.Attack::blockStart = value;
    }
    {
        auto value = p.Attack::loopCount;
        image.integer(0x57, value);
        p.Attack::loopCount = value;
    }
    {
        auto value = p.Attack::loopStart;
        image.integer(0x59, value);
        p.Attack::loopStart = value;
    }
    for (u16 i = 0; i < 10; i++) {
        auto value = p.Attack::data[i];
        image.integer(0x5b + (i * 2), value);
        p.Attack::data[i] = value;
    }
    {
        auto value = p.Attack::readyTime;
        image.integer(0x6f, value);
        p.Attack::readyTime = value;
    }
    {
        auto value = p.Attack::sightedTime;
        image.integer(0x73, value);
        p.Attack::sightedTime = value;
    }
    {
        auto value = p.Attack::feintEnd;
        image.integer(0x77, value);
        p.Attack::feintEnd = value;
    }
    {
        auto value = p.Attack::kneeling;
        image.integer(0x7b, value);
        p.Attack::kneeling = value;
    }
    {
        auto value = p.Attack::aggressive;
        image.integer(0x7c, value);
        p.Attack::aggressive = value;
    }
    {
        auto value = p.Attack::doubleDelay;
        image.integer(0x7d, value);
        p.Attack::doubleDelay = value;
    }
    {
        auto value = p.Attack::targetSighted;
        image.integer(0x7e, value);
        p.Attack::targetSighted = value;
    }
    {
        auto value = p.Attack::stoppedToFire;
        image.integer(0x7f, value);
        p.Attack::stoppedToFire = value;
    }
    {
        auto value = p.Attack::feint;
        image.integer(0x80, value);
        p.Attack::feint = value;
    }
    {
        auto value = p.Attack::shots;
        image.integer(0x81, value);
        p.Attack::shots = value;
    }
    {
        auto value = p.Attack::greeted;
        image.integer(0x83, value);
        p.Attack::greeted = value;
    }
    {
        auto value = p.Attack::lastSpoke;
        image.integer(0x84, value);
        p.Attack::lastSpoke = value;
    }
    {
        auto value = p.Attack::speech;
        image.integer(0x88, value);
        p.Attack::speech = value;
    }
    {
        auto value = p.Attack::lastReady;
        image.integer(0x8a, value);
        p.Attack::lastReady = value;
    }
    {
        auto value = p.Attack::readyDelay;
        image.integer(0x8e, value);
        p.Attack::readyDelay = value;
    }
    {
        auto value = p.Attack::lastFire;
        image.integer(0x90, value);
        p.Attack::lastFire = value;
    }
    {
        auto value = p.Attack::fireDelay;
        image.integer(0x94, value);
        p.Attack::fireDelay = value;
    }
    {
        auto value = p.Attack::justMoved;
        image.integer(0x96, value);
        p.Attack::justMoved = value;
    }
    {
        auto value = p.Attack::fromGuard;
        image.integer(0x97, value);
        p.Attack::fromGuard = value;
    }
}

void fieldsAutoFirer(SaveImage &image, AutoFirer &p)
{
    fieldsPeriodicProcess(image, p);
}

void fieldsBaseCamera(SaveImage &image, BaseCamera &p)
{
    fieldsProcess(image, p);
}

void fieldsBatteryCharger(SaveImage &image, BatteryCharger &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.BatteryCharger::maxEnergy;
        image.integer(0x32, value);
        p.BatteryCharger::maxEnergy = value;
    }
}

void fieldsBiosTimer(SaveImage &image, BiosTimer &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.BiosTimer::realVector;
        image.integer(0x40, value);
        p.BiosTimer::realVector = value;
    }
    image.pointer(0x44, SAVE_BIOS_VECTOR, p.BiosTimer::protectedVector);
}

void fieldsBrightFireLargeAnim(SaveImage &image, BrightFireLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsBrightKneelFireAnim(SaveImage &image, BrightKneelFireAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsCamera(SaveImage &image, Camera &p)
{
    fieldsBaseCamera(image, p);
}

void fieldsCombatRollLeftAnim(SaveImage &image, CombatRollLeftAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsCombatRollRightAnim(SaveImage &image, CombatRollRightAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsCombatRunLargeAnim(SaveImage &image, CombatRunLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsCombatRunSmallAnim(SaveImage &image, CombatRunSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsCombatStandLargeAnim(SaveImage &image, CombatStandLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsCombatStandSmallAnim(SaveImage &image, CombatStandSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsController(SaveImage &image, Controller &p)
{
    fieldsPeriodicProcess(image, p);
}

void fieldsCreditsProcess(SaveImage &image, CreditsProcess &p)
{
    fieldsProcess(image, p);
}

void fieldsCruHealer(SaveImage &image, CruHealer &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.CruHealer::maxHp;
        image.integer(0x32, value);
        p.CruHealer::maxHp = value;
    }
}

void fieldsCycleProcess(SaveImage &image, CycleProcess &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.CycleProcess::paused;
        image.integer(0x40, value);
        p.CycleProcess::paused = value;
    }
    {
        auto value = p.CycleProcess::stopped;
        image.integer(0x41, value);
        p.CycleProcess::stopped = value;
    }
}

void fieldsCycleResumer(SaveImage &image, CycleResumer &p)
{
    fieldsProcess(image, p);
}

void fieldsDeathSilence(SaveImage &image, DeathSilence &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.DeathSilence::ticks;
        image.integer(0x40, value);
        p.DeathSilence::ticks = value;
    }
}

void fieldsDispatcher(SaveImage &image, Dispatcher &p)
{
    fieldsEventProcess(image, p);
}

void fieldsDumbTimer(SaveImage &image, DumbTimer &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.DumbTimer::lastTick;
        image.integer(0x40, value);
        p.DumbTimer::lastTick = value;
    }
}

void fieldsEggHatcher(SaveImage &image, EggHatcher &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.EggHatcher::inRange;
        image.integer(0x32, value);
        p.EggHatcher::inRange = value;
    }
}

void fieldsElectrocutedAnim(SaveImage &image, ElectrocutedAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsFadeProcess(SaveImage &image, FadeProcess &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.FadeProcess::wasPaused;
        image.integer(0x40, value);
        p.FadeProcess::wasPaused = value;
    }
    {
        auto value = p.FadeProcess::resumeCycling;
        image.integer(0x41, value);
        p.FadeProcess::resumeCycling = value;
    }
    {
        auto value = p.FadeProcess::stepsLeft;
        image.integer(0x42, value);
        p.FadeProcess::stepsLeft = value;
    }
    {
        auto value = p.FadeProcess::step;
        image.integer(0x44, value);
        p.FadeProcess::step = value;
    }
    image.pointer(0x46, SAVE_FADE, p.FadeProcess::current);
    image.pointer(0x4a, SAVE_FADE, p.FadeProcess::target);
}

void fieldsFallBackwardsAnim(SaveImage &image, FallBackwardsAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsFallForwardsAnim(SaveImage &image, FallForwardsAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsFinishFiringAnim(SaveImage &image, FinishFiringAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsFireLargeAnim(SaveImage &image, FireLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsFireSmallAnim(SaveImage &image, FireSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsFlicPlayer(SaveImage &image, FlicPlayer &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.FlicPlayer::x;
        image.integer(0x40, value);
        p.FlicPlayer::x = value;
    }
    {
        auto value = p.FlicPlayer::y;
        image.integer(0x42, value);
        p.FlicPlayer::y = value;
    }
    {
        auto value = p.FlicPlayer::unknown44;
        image.integer(0x44, value);
        p.FlicPlayer::unknown44 = value;
    }
    {
        auto value = p.FlicPlayer::hasAudio;
        image.integer(0x45, value);
        p.FlicPlayer::hasAudio = value;
    }
    image.pointer(0x46, SAVE_FLIC, p.FlicPlayer::soundBuffer);
    {
        auto value = p.FlicPlayer::unknown4a;
        image.integer(0x4a, value);
        p.FlicPlayer::unknown4a = value;
    }
    {
        auto value = p.FlicPlayer::unknown4c;
        image.integer(0x4c, value);
        p.FlicPlayer::unknown4c = value;
    }
    image.pointer(0x4e, SAVE_FLIC, p.FlicPlayer::soundWrite);
    image.pointer(0x52, SAVE_FLIC, p.FlicPlayer::backdrop);
    {
        auto value = p.FlicPlayer::bandsPerTick;
        image.integer(0x56, value);
        p.FlicPlayer::bandsPerTick = value;
    }
    {
        auto value = p.FlicPlayer::frameBands;
        image.integer(0x58, value);
        p.FlicPlayer::frameBands = value;
    }
    for (u16 i = 0; i < 5; i++) {
        {
            auto value = p.FlicPlayer::blocks[i].FlicBlock::kind;
            image.integer(0x5a + (i * 0x6) + 0x0, value);
            p.FlicPlayer::blocks[i].FlicBlock::kind = value;
        }
        image.pointer(0x5a + (i * 0x6) + 0x2, SAVE_FLIC, p.FlicPlayer::blocks[i].FlicBlock::data);
    }
    {
        auto value = p.FlicPlayer::readAhead;
        image.integer(0x78, value);
        p.FlicPlayer::readAhead = value;
    }
    {
        auto value = p.FlicPlayer::applyPalette;
        image.integer(0x79, value);
        p.FlicPlayer::applyPalette = value;
    }
    {
        auto value = p.FlicPlayer::restoreScreen;
        image.integer(0x7a, value);
        p.FlicPlayer::restoreScreen = value;
    }
    {
        auto value = p.FlicPlayer::screenSaved;
        image.integer(0x7b, value);
        p.FlicPlayer::screenSaved = value;
    }
    {
        auto value = p.FlicPlayer::band;
        image.integer(0x7c, value);
        p.FlicPlayer::band = value;
    }
    {
        auto value = p.FlicPlayer::nextPreload;
        image.integer(0x7e, value);
        p.FlicPlayer::nextPreload = value;
    }
    {
        auto value = p.FlicPlayer::eventIndex;
        image.integer(0x80, value);
        p.FlicPlayer::eventIndex = value;
    }
    {
        auto value = p.FlicPlayer::showBorder;
        image.integer(0x82, value);
        p.FlicPlayer::showBorder = value;
    }
    {
        auto value = p.FlicPlayer::drawIndex;
        image.integer(0x83, value);
        p.FlicPlayer::drawIndex = value;
    }
    image.pointer(0x84, SAVE_FLIC, p.FlicPlayer::background);
    {
        auto value = p.FlicPlayer::fading;
        image.integer(0x88, value);
        p.FlicPlayer::fading = value;
    }
    {
        auto value = p.FlicPlayer::fadeRate;
        image.integer(0x89, value);
        p.FlicPlayer::fadeRate = value;
    }
    image.pointer(0x8b, SAVE_FLIC, p.FlicPlayer::flex);
    {
        auto value = p.FlicPlayer::startTime;
        image.integer(0x8f, value);
        p.FlicPlayer::startTime = value;
    }
    {
        auto value = p.FlicPlayer::unknown93;
        image.integer(0x93, value);
        p.FlicPlayer::unknown93 = value;
    }
    {
        auto value = p.FlicPlayer::eventFrame;
        image.integer(0x97, value);
        p.FlicPlayer::eventFrame = value;
    }
    {
        auto value = p.FlicPlayer::frameTicks;
        image.integer(0x99, value);
        p.FlicPlayer::frameTicks = value;
    }
    image.pointer(0x9b, SAVE_FLIC, p.FlicPlayer::greyPalette);
    {
        auto value = p.FlicPlayer::greyScreen;
        image.integer(0x9f, value);
        p.FlicPlayer::greyScreen = value;
    }
    {
        auto value = p.FlicPlayer::entry.FlexEntry::offset;
        image.integer(0xa0, value);
        p.FlicPlayer::entry.FlexEntry::offset = value;
    }
    {
        auto value = p.FlicPlayer::entry.FlexEntry::size;
        image.integer(0xa4, value);
        p.FlicPlayer::entry.FlexEntry::size = value;
    }
    for (u16 i = 0; i < 20; i++) {
        {
            auto value = p.FlicPlayer::preloads[i].FlicPreload::frame;
            image.integer(0xa8 + (i * 0x6) + 0x0, value);
            p.FlicPlayer::preloads[i].FlicPreload::frame = value;
        }
        image.pointer(0xa8 + (i * 0x6) + 0x2, SAVE_FLIC, p.FlicPlayer::preloads[i].FlicPreload::data);
    }
    {
        auto value = p.FlicPlayer::readIndex;
        image.integer(0x120, value);
        p.FlicPlayer::readIndex = value;
    }
    {
        auto value = p.FlicPlayer::loops;
        image.integer(0x121, value);
        p.FlicPlayer::loops = value;
    }
    {
        auto value = p.FlicPlayer::frameCount;
        image.integer(0x123, value);
        p.FlicPlayer::frameCount = value;
    }
    {
        auto value = p.FlicPlayer::savedMusic;
        image.integer(0x127, value);
        p.FlicPlayer::savedMusic = value;
    }
    image.pointer(0x129, SAVE_FLIC, p.FlicPlayer::savedPalette);
    {
        auto value = p.FlicPlayer::savedMode;
        image.integer(0x12d, value);
        p.FlicPlayer::savedMode = value;
    }
    {
        auto value = p.FlicPlayer::eventValue;
        image.integer(0x131, value);
        p.FlicPlayer::eventValue = value;
    }
    for (u16 i = 0; i < 256; i++) {
        {
            auto value = p.FlicPlayer::events[i].FlicEvent::frame;
            image.integer(0x133 + (i * 0x6) + 0x0, value);
            p.FlicPlayer::events[i].FlicEvent::frame = value;
        }
        {
            auto value = p.FlicPlayer::events[i].FlicEvent::type;
            image.integer(0x133 + (i * 0x6) + 0x2, value);
            p.FlicPlayer::events[i].FlicEvent::type = value;
        }
        {
            auto value = p.FlicPlayer::events[i].FlicEvent::value;
            image.integer(0x133 + (i * 0x6) + 0x4, value);
            p.FlicPlayer::events[i].FlicEvent::value = value;
        }
    }
    image.pointer(0x733, SAVE_FLIC, p.FlicPlayer::palette);
    {
        auto value = p.FlicPlayer::pauseCycling;
        image.integer(0x737, value);
        p.FlicPlayer::pauseCycling = value;
    }
    image.pointer(0x738, SAVE_FLIC, p.FlicPlayer::frameData);
    {
        auto value = p.FlicPlayer::raisePriority;
        image.integer(0x73c, value);
        p.FlicPlayer::raisePriority = value;
    }
    {
        auto value = p.FlicPlayer::soundStarted;
        image.integer(0x73d, value);
        p.FlicPlayer::soundStarted = value;
    }
    {
        auto value = p.FlicPlayer::frame;
        image.integer(0x73e, value);
        p.FlicPlayer::frame = value;
    }
    {
        auto value = p.FlicPlayer::frameLoaded;
        image.integer(0x740, value);
        p.FlicPlayer::frameLoaded = value;
    }
    {
        auto value = p.FlicPlayer::restore;
        image.integer(0x741, value);
        p.FlicPlayer::restore = value;
    }
    {
        auto value = p.FlicPlayer::seekNeeded;
        image.integer(0x742, value);
        p.FlicPlayer::seekNeeded = value;
    }
    {
        auto value = p.FlicPlayer::playStart;
        image.integer(0x743, value);
        p.FlicPlayer::playStart = value;
    }
    {
        auto value = p.FlicPlayer::state;
        image.integer(0x747, value);
        p.FlicPlayer::state = value;
    }
    {
        auto value = p.FlicPlayer::restoreMusic;
        image.integer(0x749, value);
        p.FlicPlayer::restoreMusic = value;
    }
    {
        auto value = p.FlicPlayer::stopEffects;
        image.integer(0x74a, value);
        p.FlicPlayer::stopEffects = value;
    }
    {
        auto value = p.FlicPlayer::framesDrawn;
        image.integer(0x74b, value);
        p.FlicPlayer::framesDrawn = value;
    }
    {
        auto value = p.FlicPlayer::videoMode;
        image.integer(0x74f, value);
        p.FlicPlayer::videoMode = value;
    }
    {
        auto value = p.FlicPlayer::skipTicks;
        image.integer(0x753, value);
        p.FlicPlayer::skipTicks = value;
    }
    {
        auto value = p.FlicPlayer::unknown755;
        image.integer(0x755, value);
        p.FlicPlayer::unknown755 = value;
    }
    {
        auto value = p.FlicPlayer::keepMouse;
        image.integer(0x756, value);
        p.FlicPlayer::keepMouse = value;
    }
}

void fieldsFlicRestoreProcess(SaveImage &image, FlicRestoreProcess &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.FlicRestoreProcess::animationsHeld;
        image.integer(0x32, value);
        p.FlicRestoreProcess::animationsHeld = value;
    }
    {
        auto value = p.FlicRestoreProcess::greyScreen;
        image.integer(0x33, value);
        p.FlicRestoreProcess::greyScreen = value;
    }
}

void fieldsFlicWait(SaveImage &image, FlicWait &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.FlicWait::count;
        image.integer(0x40, value);
        p.FlicWait::count = value;
    }
}

void fieldsGameTime(SaveImage &image, GameTime &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.GameTime::frames;
        image.integer(0x32, value);
        p.GameTime::frames = value;
    }
    {
        auto value = p.GameTime::hold;
        image.integer(0x36, value);
        p.GameTime::hold = value;
    }
    {
        auto value = p.GameTime::newQuarter;
        image.integer(0x38, value);
        p.GameTime::newQuarter = value;
    }
}

void fieldsGravityProcess(SaveImage &image, GravityProcess &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.GravityProcess::speedX;
        image.integer(0x32, value);
        p.GravityProcess::speedX = value;
    }
    {
        auto value = p.GravityProcess::speedY;
        image.integer(0x34, value);
        p.GravityProcess::speedY = value;
    }
    {
        auto value = p.GravityProcess::speedZ;
        image.integer(0x36, value);
        p.GravityProcess::speedZ = value;
    }
    {
        auto value = p.GravityProcess::gravity;
        image.integer(0x38, value);
        p.GravityProcess::gravity = value;
    }
    {
        auto value = p.GravityProcess::sliding;
        image.integer(0x3a, value);
        p.GravityProcess::sliding = value;
    }
    {
        auto value = p.GravityProcess::lifetime;
        image.integer(0x3b, value);
        p.GravityProcess::lifetime = value;
    }
    {
        auto value = p.GravityProcess::unknown3d;
        image.integer(0x3d, value);
        p.GravityProcess::unknown3d = value;
    }
    image.pointer(0x3e, SAVE_TRANSIENT, p.GravityProcess::target);
}

void fieldsGuard(SaveImage &image, Guard &p)
{
    fieldsActivity(image, p);
}

void fieldsWaitProcess(SaveImage &image, WaitProcess &p)
{
    fieldsEventProcess(image, p);
    {
        auto value = p.WaitProcess::start;
        image.integer(0x36, value);
        p.WaitProcess::start = value;
    }
    {
        auto value = p.WaitProcess::duration;
        image.integer(0x3a, value);
        p.WaitProcess::duration = value;
    }
}

void fieldsGumpNotifyProcess(SaveImage &image, GumpNotifyProcess &p)
{
    fieldsWaitProcess(image, p);
    for (u16 i = 0; i < 10; i++) {
        {
            auto value = p.GumpNotifyProcess::path[i].NewGumpId::value;
            image.integer(0x3e + (i * 0x5) + 0x0, value);
            p.GumpNotifyProcess::path[i].NewGumpId::value = value;
        }
        {
            auto value = p.GumpNotifyProcess::path[i].NewGumpId::inverse;
            image.integer(0x3e + (i * 0x5) + 0x2, value);
            p.GumpNotifyProcess::path[i].NewGumpId::inverse = value;
        }
        {
            auto value = p.GumpNotifyProcess::path[i].NewGumpId::zero;
            image.integer(0x3e + (i * 0x5) + 0x4, value);
            p.GumpNotifyProcess::path[i].NewGumpId::zero = value;
        }
    }
    {
        auto value = p.GumpNotifyProcess::depth;
        image.integer(0x70, value);
        p.GumpNotifyProcess::depth = value;
    }
    {
        auto value = p.GumpNotifyProcess::unknown72;
        image.integer(0x72, value);
        p.GumpNotifyProcess::unknown72 = value;
    }
    {
        auto value = p.GumpNotifyProcess::waiting;
        image.integer(0x73, value);
        p.GumpNotifyProcess::waiting = value;
    }
}

void fieldsJoystickDriver(SaveImage &image, JoystickDriver &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.JoystickDriver::direction;
        image.integer(0x32, value);
        p.JoystickDriver::direction = value;
    }
    {
        auto value = p.JoystickDriver::lastDirection;
        image.integer(0x33, value);
        p.JoystickDriver::lastDirection = value;
    }
    {
        auto value = p.JoystickDriver::spaceHeld;
        image.integer(0x34, value);
        p.JoystickDriver::spaceHeld = value;
    }
    {
        auto value = p.JoystickDriver::unknown35;
        image.integer(0x35, value);
        p.JoystickDriver::unknown35 = value;
    }
    {
        auto value = p.JoystickDriver::unknown36;
        image.integer(0x36, value);
        p.JoystickDriver::unknown36 = value;
    }
    {
        auto value = p.JoystickDriver::up;
        image.integer(0x37, value);
        p.JoystickDriver::up = value;
    }
    {
        auto value = p.JoystickDriver::down;
        image.integer(0x38, value);
        p.JoystickDriver::down = value;
    }
    {
        auto value = p.JoystickDriver::left;
        image.integer(0x39, value);
        p.JoystickDriver::left = value;
    }
    {
        auto value = p.JoystickDriver::right;
        image.integer(0x3a, value);
        p.JoystickDriver::right = value;
    }
    {
        auto value = p.JoystickDriver::wasUp;
        image.integer(0x3b, value);
        p.JoystickDriver::wasUp = value;
    }
    {
        auto value = p.JoystickDriver::wasDown;
        image.integer(0x3c, value);
        p.JoystickDriver::wasDown = value;
    }
    {
        auto value = p.JoystickDriver::wasLeft;
        image.integer(0x3d, value);
        p.JoystickDriver::wasLeft = value;
    }
    {
        auto value = p.JoystickDriver::wasRight;
        image.integer(0x3e, value);
        p.JoystickDriver::wasRight = value;
    }
    {
        auto value = p.JoystickDriver::button1;
        image.integer(0x3f, value);
        p.JoystickDriver::button1 = value;
    }
    {
        auto value = p.JoystickDriver::button2;
        image.integer(0x40, value);
        p.JoystickDriver::button2 = value;
    }
    {
        auto value = p.JoystickDriver::button3;
        image.integer(0x41, value);
        p.JoystickDriver::button3 = value;
    }
    {
        auto value = p.JoystickDriver::button4;
        image.integer(0x42, value);
        p.JoystickDriver::button4 = value;
    }
    {
        auto value = p.JoystickDriver::lastButtons;
        image.integer(0x43, value);
        p.JoystickDriver::lastButtons = value;
    }
    {
        auto value = p.JoystickDriver::lastArrow;
        image.integer(0x45, value);
        p.JoystickDriver::lastArrow = value;
    }
    {
        auto value = p.JoystickDriver::lastAction;
        image.integer(0x47, value);
        p.JoystickDriver::lastAction = value;
    }
}

void fieldsJumpForwardAnim(SaveImage &image, JumpForwardAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsJumpLandingAnim(SaveImage &image, JumpLandingAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKeyDaemon(SaveImage &image, KeyDaemon &p)
{
    fieldsPeriodicProcess(image, p);
}

void fieldsKeyboardProcess(SaveImage &image, KeyboardProcess &p)
{
    fieldsEventProcess(image, p);
}

void fieldsKneelEndAnim(SaveImage &image, KneelEndAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelFireLargeAnim(SaveImage &image, KneelFireLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelFireSmallAnim(SaveImage &image, KneelFireSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelRollLeftAnim(SaveImage &image, KneelRollLeftAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelRollRightAnim(SaveImage &image, KneelRollRightAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelStartAnim(SaveImage &image, KneelStartAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelingAdvanceAnim(SaveImage &image, KneelingAdvanceAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelingLargeAnim(SaveImage &image, KneelingLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelingRetreatAnim(SaveImage &image, KneelingRetreatAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelingSlideLeftAnim(SaveImage &image, KneelingSlideLeftAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsKneelingSmallAnim(SaveImage &image, KneelingSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsLoiter(SaveImage &image, Loiter &p)
{
    fieldsActivity(image, p);
    {
        auto value = p.Loiter::count;
        image.integer(0x42, value);
        p.Loiter::count = value;
    }
    {
        auto value = p.Loiter::homeX;
        image.integer(0x44, value);
        p.Loiter::homeX = value;
    }
    {
        auto value = p.Loiter::homeY;
        image.integer(0x46, value);
        p.Loiter::homeY = value;
    }
    {
        auto value = p.Loiter::homeZ;
        image.integer(0x48, value);
        p.Loiter::homeZ = value;
    }
}

void fieldsLookLeftAnim(SaveImage &image, LookLeftAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsLookRightAnim(SaveImage &image, LookRightAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsMapJumpProcess(SaveImage &image, MapJumpProcess &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.MapJumpProcess::map;
        image.integer(0x32, value);
        p.MapJumpProcess::map = value;
    }
}

void fieldsMouseCtl(SaveImage &image, MouseCtl &p)
{
    fieldsPeriodicProcess(image, p);
    for (u16 i = 0; i < 10; i++) {
        {
            auto value = p.MouseCtl::events.items[i].MouseRecord::state;
            image.integer(0x40 + (i * 0x10) + 0x0, value);
            p.MouseCtl::events.items[i].MouseRecord::state = value;
        }
        {
            auto value = p.MouseCtl::events.items[i].MouseRecord::ticks;
            image.integer(0x40 + (i * 0x10) + 0x2, value);
            p.MouseCtl::events.items[i].MouseRecord::ticks = value;
        }
        {
            auto value = p.MouseCtl::events.items[i].MouseRecord::action;
            image.integer(0x40 + (i * 0x10) + 0x6, value);
            p.MouseCtl::events.items[i].MouseRecord::action = value;
        }
        {
            auto value = p.MouseCtl::events.items[i].MouseRecord::buttons;
            image.integer(0x40 + (i * 0x10) + 0x8, value);
            p.MouseCtl::events.items[i].MouseRecord::buttons = value;
        }
        {
            auto value = p.MouseCtl::events.items[i].MouseRecord::shift;
            image.integer(0x40 + (i * 0x10) + 0xa, value);
            p.MouseCtl::events.items[i].MouseRecord::shift = value;
        }
        {
            auto value = p.MouseCtl::events.items[i].MouseRecord::x;
            image.integer(0x40 + (i * 0x10) + 0xc, value);
            p.MouseCtl::events.items[i].MouseRecord::x = value;
        }
        {
            auto value = p.MouseCtl::events.items[i].MouseRecord::y;
            image.integer(0x40 + (i * 0x10) + 0xe, value);
            p.MouseCtl::events.items[i].MouseRecord::y = value;
        }
    }
    {
        auto value = p.MouseCtl::events.head;
        image.integer(0xe0, value);
        p.MouseCtl::events.head = value;
    }
    {
        auto value = p.MouseCtl::events.tail;
        image.integer(0xe2, value);
        p.MouseCtl::events.tail = value;
    }
    {
        auto value = p.MouseCtl::events.count;
        image.integer(0xe4, value);
        p.MouseCtl::events.count = value;
    }
    {
        auto value = p.MouseCtl::current.MouseRecord::state;
        image.integer(0xe6, value);
        p.MouseCtl::current.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::current.MouseRecord::ticks;
        image.integer(0xe8, value);
        p.MouseCtl::current.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::current.MouseRecord::action;
        image.integer(0xec, value);
        p.MouseCtl::current.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::current.MouseRecord::buttons;
        image.integer(0xee, value);
        p.MouseCtl::current.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::current.MouseRecord::shift;
        image.integer(0xf0, value);
        p.MouseCtl::current.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::current.MouseRecord::x;
        image.integer(0xf2, value);
        p.MouseCtl::current.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::current.MouseRecord::y;
        image.integer(0xf4, value);
        p.MouseCtl::current.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::chord.MouseRecord::state;
        image.integer(0xf6, value);
        p.MouseCtl::chord.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::chord.MouseRecord::ticks;
        image.integer(0xf8, value);
        p.MouseCtl::chord.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::chord.MouseRecord::action;
        image.integer(0xfc, value);
        p.MouseCtl::chord.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::chord.MouseRecord::buttons;
        image.integer(0xfe, value);
        p.MouseCtl::chord.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::chord.MouseRecord::shift;
        image.integer(0x100, value);
        p.MouseCtl::chord.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::chord.MouseRecord::x;
        image.integer(0x102, value);
        p.MouseCtl::chord.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::chord.MouseRecord::y;
        image.integer(0x104, value);
        p.MouseCtl::chord.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::unused106.MouseRecord::state;
        image.integer(0x106, value);
        p.MouseCtl::unused106.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::unused106.MouseRecord::ticks;
        image.integer(0x108, value);
        p.MouseCtl::unused106.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::unused106.MouseRecord::action;
        image.integer(0x10c, value);
        p.MouseCtl::unused106.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::unused106.MouseRecord::buttons;
        image.integer(0x10e, value);
        p.MouseCtl::unused106.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::unused106.MouseRecord::shift;
        image.integer(0x110, value);
        p.MouseCtl::unused106.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::unused106.MouseRecord::x;
        image.integer(0x112, value);
        p.MouseCtl::unused106.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::unused106.MouseRecord::y;
        image.integer(0x114, value);
        p.MouseCtl::unused106.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::leftDown.MouseRecord::state;
        image.integer(0x116, value);
        p.MouseCtl::leftDown.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::leftDown.MouseRecord::ticks;
        image.integer(0x118, value);
        p.MouseCtl::leftDown.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::leftDown.MouseRecord::action;
        image.integer(0x11c, value);
        p.MouseCtl::leftDown.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::leftDown.MouseRecord::buttons;
        image.integer(0x11e, value);
        p.MouseCtl::leftDown.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::leftDown.MouseRecord::shift;
        image.integer(0x120, value);
        p.MouseCtl::leftDown.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::leftDown.MouseRecord::x;
        image.integer(0x122, value);
        p.MouseCtl::leftDown.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::leftDown.MouseRecord::y;
        image.integer(0x124, value);
        p.MouseCtl::leftDown.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::leftUp.MouseRecord::state;
        image.integer(0x126, value);
        p.MouseCtl::leftUp.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::leftUp.MouseRecord::ticks;
        image.integer(0x128, value);
        p.MouseCtl::leftUp.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::leftUp.MouseRecord::action;
        image.integer(0x12c, value);
        p.MouseCtl::leftUp.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::leftUp.MouseRecord::buttons;
        image.integer(0x12e, value);
        p.MouseCtl::leftUp.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::leftUp.MouseRecord::shift;
        image.integer(0x130, value);
        p.MouseCtl::leftUp.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::leftUp.MouseRecord::x;
        image.integer(0x132, value);
        p.MouseCtl::leftUp.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::leftUp.MouseRecord::y;
        image.integer(0x134, value);
        p.MouseCtl::leftUp.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::rightDown.MouseRecord::state;
        image.integer(0x136, value);
        p.MouseCtl::rightDown.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::rightDown.MouseRecord::ticks;
        image.integer(0x138, value);
        p.MouseCtl::rightDown.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::rightDown.MouseRecord::action;
        image.integer(0x13c, value);
        p.MouseCtl::rightDown.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::rightDown.MouseRecord::buttons;
        image.integer(0x13e, value);
        p.MouseCtl::rightDown.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::rightDown.MouseRecord::shift;
        image.integer(0x140, value);
        p.MouseCtl::rightDown.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::rightDown.MouseRecord::x;
        image.integer(0x142, value);
        p.MouseCtl::rightDown.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::rightDown.MouseRecord::y;
        image.integer(0x144, value);
        p.MouseCtl::rightDown.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::rightUp.MouseRecord::state;
        image.integer(0x146, value);
        p.MouseCtl::rightUp.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::rightUp.MouseRecord::ticks;
        image.integer(0x148, value);
        p.MouseCtl::rightUp.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::rightUp.MouseRecord::action;
        image.integer(0x14c, value);
        p.MouseCtl::rightUp.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::rightUp.MouseRecord::buttons;
        image.integer(0x14e, value);
        p.MouseCtl::rightUp.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::rightUp.MouseRecord::shift;
        image.integer(0x150, value);
        p.MouseCtl::rightUp.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::rightUp.MouseRecord::x;
        image.integer(0x152, value);
        p.MouseCtl::rightUp.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::rightUp.MouseRecord::y;
        image.integer(0x154, value);
        p.MouseCtl::rightUp.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::moved.MouseRecord::state;
        image.integer(0x156, value);
        p.MouseCtl::moved.MouseRecord::state = value;
    }
    {
        auto value = p.MouseCtl::moved.MouseRecord::ticks;
        image.integer(0x158, value);
        p.MouseCtl::moved.MouseRecord::ticks = value;
    }
    {
        auto value = p.MouseCtl::moved.MouseRecord::action;
        image.integer(0x15c, value);
        p.MouseCtl::moved.MouseRecord::action = value;
    }
    {
        auto value = p.MouseCtl::moved.MouseRecord::buttons;
        image.integer(0x15e, value);
        p.MouseCtl::moved.MouseRecord::buttons = value;
    }
    {
        auto value = p.MouseCtl::moved.MouseRecord::shift;
        image.integer(0x160, value);
        p.MouseCtl::moved.MouseRecord::shift = value;
    }
    {
        auto value = p.MouseCtl::moved.MouseRecord::x;
        image.integer(0x162, value);
        p.MouseCtl::moved.MouseRecord::x = value;
    }
    {
        auto value = p.MouseCtl::moved.MouseRecord::y;
        image.integer(0x164, value);
        p.MouseCtl::moved.MouseRecord::y = value;
    }
    {
        auto value = p.MouseCtl::bounds.Rect::left;
        image.integer(0x166, value);
        p.MouseCtl::bounds.Rect::left = value;
    }
    {
        auto value = p.MouseCtl::bounds.Rect::top;
        image.integer(0x168, value);
        p.MouseCtl::bounds.Rect::top = value;
    }
    {
        auto value = p.MouseCtl::bounds.Rect::right;
        image.integer(0x16a, value);
        p.MouseCtl::bounds.Rect::right = value;
    }
    {
        auto value = p.MouseCtl::bounds.Rect::bottom;
        image.integer(0x16c, value);
        p.MouseCtl::bounds.Rect::bottom = value;
    }
    {
        auto value = p.MouseCtl::cursorX;
        image.integer(0x16e, value);
        p.MouseCtl::cursorX = value;
    }
    {
        auto value = p.MouseCtl::cursorY;
        image.integer(0x170, value);
        p.MouseCtl::cursorY = value;
    }
    {
        auto value = p.MouseCtl::pointerX;
        image.integer(0x172, value);
        p.MouseCtl::pointerX = value;
    }
    {
        auto value = p.MouseCtl::pointerY;
        image.integer(0x174, value);
        p.MouseCtl::pointerY = value;
    }
    {
        auto value = p.MouseCtl::width;
        image.integer(0x176, value);
        p.MouseCtl::width = value;
    }
    {
        auto value = p.MouseCtl::height;
        image.integer(0x178, value);
        p.MouseCtl::height = value;
    }
    {
        auto value = p.MouseCtl::hotX;
        image.integer(0x17a, value);
        p.MouseCtl::hotX = value;
    }
    {
        auto value = p.MouseCtl::hotY;
        image.integer(0x17c, value);
        p.MouseCtl::hotY = value;
    }
    {
        auto value = p.MouseCtl::visible;
        image.integer(0x17e, value);
        p.MouseCtl::visible = value;
    }
    {
        auto value = p.MouseCtl::drawn;
        image.integer(0x17f, value);
        p.MouseCtl::drawn = value;
    }
    for (u16 i = 0; i < 4; i++) {
        auto value = p.MouseCtl::unknown180[i];
        image.integer(0x180 + (i * 1), value);
        p.MouseCtl::unknown180[i] = value;
    }
    {
        auto value = p.MouseCtl::buttons;
        image.integer(0x184, value);
        p.MouseCtl::buttons = value;
    }
    for (u16 i = 0; i < 8; i++) {
        auto value = p.MouseCtl::shapes[i];
        image.integer(0x186 + (i * 2), value);
        p.MouseCtl::shapes[i] = value;
    }
    {
        auto value = p.MouseCtl::depth;
        image.integer(0x196, value);
        p.MouseCtl::depth = value;
    }
}

void fieldsMusicProcess(SaveImage &image, MusicProcess &p)
{
    fieldsPeriodicProcess(image, p);
}

void fieldsNpcHold(SaveImage &image, NpcHold &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.NpcHold::wasHeld;
        image.integer(0x32, value);
        p.NpcHold::wasHeld = value;
    }
}

void fieldsPace(SaveImage &image, Pace &p)
{
    fieldsActivity(image, p);
    {
        auto value = p.Pace::unknown48;
        image.integer(0x42, value);
        p.Pace::unknown48 = value;
    }
    {
        auto value = p.Pace::failures;
        image.integer(0x44, value);
        p.Pace::failures = value;
    }
}

void fieldsPathFinder(SaveImage &image, PathFinder &p)
{
    fieldsEventProcess(image, p);
    {
        auto value = p.PathFinder::target.ItemPoint::x;
        image.integer(0x36, value);
        p.PathFinder::target.ItemPoint::x = value;
    }
    {
        auto value = p.PathFinder::target.ItemPoint::y;
        image.integer(0x38, value);
        p.PathFinder::target.ItemPoint::y = value;
    }
    {
        auto value = p.PathFinder::target.ItemPoint::z;
        image.integer(0x3a, value);
        p.PathFinder::target.ItemPoint::z = value;
    }
    {
        auto value = p.PathFinder::heading;
        image.integer(0x3b, value);
        p.PathFinder::heading = value;
    }
    {
        auto value = p.PathFinder::lastHeading;
        image.integer(0x3c, value);
        p.PathFinder::lastHeading = value;
    }
    {
        auto value = p.PathFinder::tryHeading;
        image.integer(0x3d, value);
        p.PathFinder::tryHeading = value;
    }
    {
        auto value = p.PathFinder::detour;
        image.integer(0x3e, value);
        p.PathFinder::detour = value;
    }
    {
        auto value = p.PathFinder::detourSide;
        image.integer(0x3f, value);
        p.PathFinder::detourSide = value;
    }
    {
        auto value = p.PathFinder::solid;
        image.integer(0x40, value);
        p.PathFinder::solid = value;
    }
    {
        auto value = p.PathFinder::distance;
        image.integer(0x41, value);
        p.PathFinder::distance = value;
    }
    {
        auto value = p.PathFinder::stopDistance;
        image.integer(0x43, value);
        p.PathFinder::stopDistance = value;
    }
    {
        auto value = p.PathFinder::maxSteps;
        image.integer(0x45, value);
        p.PathFinder::maxSteps = value;
    }
    {
        auto value = p.PathFinder::step;
        image.integer(0x47, value);
        p.PathFinder::step = value;
    }
    {
        auto value = p.PathFinder::probe;
        image.integer(0x49, value);
        p.PathFinder::probe = value;
    }
    {
        auto value = p.PathFinder::arrived;
        image.integer(0x4a, value);
        p.PathFinder::arrived = value;
    }
    {
        auto value = p.PathFinder::turnAtEnd;
        image.integer(0x4b, value);
        p.PathFinder::turnAtEnd = value;
    }
    {
        auto value = p.PathFinder::standAtEnd;
        image.integer(0x4c, value);
        p.PathFinder::standAtEnd = value;
    }
    {
        auto value = p.PathFinder::targetItem;
        image.integer(0x4d, value);
        p.PathFinder::targetItem = value;
    }
    {
        auto value = p.PathFinder::randomSide;
        image.integer(0x4f, value);
        p.PathFinder::randomSide = value;
    }
}

void fieldsPickupProcess(SaveImage &image, PickupProcess &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.PickupProcess::lastTime;
        image.integer(0x32, value);
        p.PickupProcess::lastTime = value;
    }
}

void fieldsQuickJumpAnim(SaveImage &image, QuickJumpAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsReadyLargeAnim(SaveImage &image, ReadyLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsReadySmallAnim(SaveImage &image, ReadySmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsReleaseProcess(SaveImage &image, ReleaseProcess &p)
{
    fieldsProcess(image, p);
}

void fieldsReloadLargeAnim(SaveImage &image, ReloadLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsResumeCycling(SaveImage &image, ResumeCycling &p)
{
    fieldsProcess(image, p);
}

void fieldsRetreatLargeAnim(SaveImage &image, RetreatLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsRetreatSmallAnim(SaveImage &image, RetreatSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsRollForwardAnim(SaveImage &image, RollForwardAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsRunAnim(SaveImage &image, RunAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsSlideLeftAnim(SaveImage &image, SlideLeftAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsSlideRightAnim(SaveImage &image, SlideRightAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsSnapProcess(SaveImage &image, SnapProcess &p)
{
    fieldsProcess(image, p);
    for (u16 i = 0; i < 64; i++) {
        auto value = p.SnapProcess::eggs[i];
        image.integer(0x32 + (i * 2), value);
        p.SnapProcess::eggs[i] = value;
    }
    {
        auto value = p.SnapProcess::eggCount;
        image.integer(0xb2, value);
        p.SnapProcess::eggCount = value;
    }
    {
        auto value = p.SnapProcess::area.Rect::left;
        image.integer(0xb4, value);
        p.SnapProcess::area.Rect::left = value;
    }
    {
        auto value = p.SnapProcess::area.Rect::top;
        image.integer(0xb6, value);
        p.SnapProcess::area.Rect::top = value;
    }
    {
        auto value = p.SnapProcess::area.Rect::right;
        image.integer(0xb8, value);
        p.SnapProcess::area.Rect::right = value;
    }
    {
        auto value = p.SnapProcess::area.Rect::bottom;
        image.integer(0xba, value);
        p.SnapProcess::area.Rect::bottom = value;
    }
    {
        auto value = p.SnapProcess::nextArea.Rect::left;
        image.integer(0xbc, value);
        p.SnapProcess::nextArea.Rect::left = value;
    }
    {
        auto value = p.SnapProcess::nextArea.Rect::top;
        image.integer(0xbe, value);
        p.SnapProcess::nextArea.Rect::top = value;
    }
    {
        auto value = p.SnapProcess::nextArea.Rect::right;
        image.integer(0xc0, value);
        p.SnapProcess::nextArea.Rect::right = value;
    }
    {
        auto value = p.SnapProcess::nextArea.Rect::bottom;
        image.integer(0xc2, value);
        p.SnapProcess::nextArea.Rect::bottom = value;
    }
    {
        auto value = p.SnapProcess::nextEgg;
        image.integer(0xc4, value);
        p.SnapProcess::nextEgg = value;
    }
    {
        auto value = p.SnapProcess::currentEgg;
        image.integer(0xc6, value);
        p.SnapProcess::currentEgg = value;
    }
    {
        auto value = p.SnapProcess::holdsCamera;
        image.integer(0xc8, value);
        p.SnapProcess::holdsCamera = value;
    }
}

void fieldsSprite(SaveImage &image, Sprite &p)
{
    fieldsEventProcess(image, p);
    {
        auto value = p.Sprite::firstFrame;
        image.integer(0x36, value);
        p.Sprite::firstFrame = value;
    }
    {
        auto value = p.Sprite::lastFrame;
        image.integer(0x38, value);
        p.Sprite::lastFrame = value;
    }
    {
        auto value = p.Sprite::frame;
        image.integer(0x3a, value);
        p.Sprite::frame = value;
    }
    {
        auto value = p.Sprite::repeats;
        image.integer(0x3c, value);
        p.Sprite::repeats = value;
    }
    {
        auto value = p.Sprite::fire;
        image.integer(0x3e, value);
        p.Sprite::fire = value;
    }
    {
        auto value = p.Sprite::handle;
        image.integer(0x3f, value);
        p.Sprite::handle = value;
    }
}

void fieldsStand(SaveImage &image, Stand &p)
{
    fieldsActivity(image, p);
}

void fieldsStandAnim(SaveImage &image, StandAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsStartRunAnim(SaveImage &image, StartRunAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsStartRunLargeAnim(SaveImage &image, StartRunLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsStartRunSmallAnim(SaveImage &image, StartRunSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsStdIntHandler(SaveImage &image, StdIntHandler &p)
{
    fieldsProcess(image, p);
    image.vtable(0x32, 0x3a89);
    {
        auto value = p.StdIntHandler::subscribers.MList::count;
        image.integer(0x34, value);
        p.StdIntHandler::subscribers.MList::count = value;
    }
    {
        auto value = p.StdIntHandler::subscribers.MList::tag;
        image.integer(0x36, value);
        p.StdIntHandler::subscribers.MList::tag = value;
    }
    image.pointer(0x38, SAVE_LIST, p.StdIntHandler::subscribers.MList::data);
    {
        auto value = p.StdIntHandler::subscribers.SubscriberList::stop;
        image.integer(0x3c, value);
        p.StdIntHandler::subscribers.SubscriberList::stop = value;
    }
    {
        auto value = p.StdIntHandler::subscribers.SubscriberList::index;
        image.integer(0x3d, value);
        p.StdIntHandler::subscribers.SubscriberList::index = value;
    }
    {
        auto value = p.StdIntHandler::subscribers.SubscriberList::total;
        image.integer(0x3f, value);
        p.StdIntHandler::subscribers.SubscriberList::total = value;
    }
    image.vtable(0x41, 0x3a89);
    {
        auto value = p.StdIntHandler::spare.MList::count;
        image.integer(0x43, value);
        p.StdIntHandler::spare.MList::count = value;
    }
    {
        auto value = p.StdIntHandler::spare.MList::tag;
        image.integer(0x45, value);
        p.StdIntHandler::spare.MList::tag = value;
    }
    image.pointer(0x47, SAVE_LIST, p.StdIntHandler::spare.MList::data);
    {
        auto value = p.StdIntHandler::spare.SubscriberList::stop;
        image.integer(0x4b, value);
        p.StdIntHandler::spare.SubscriberList::stop = value;
    }
    {
        auto value = p.StdIntHandler::spare.SubscriberList::index;
        image.integer(0x4c, value);
        p.StdIntHandler::spare.SubscriberList::index = value;
    }
    {
        auto value = p.StdIntHandler::spare.SubscriberList::total;
        image.integer(0x4e, value);
        p.StdIntHandler::spare.SubscriberList::total = value;
    }
}

void fieldsStopRunLargeAnim(SaveImage &image, StopRunLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsStopRunSmallAnim(SaveImage &image, StopRunSmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsStorageData(SaveImage &image, StorageData &p)
{
    fieldsProcess(image, p);
    image.pointer(0x32, SAVE_TRANSIENT, p.StorageData::store);
    {
        auto value = p.StorageData::hitter;
        image.integer(0x36, value);
        p.StorageData::hitter = value;
    }
    {
        auto value = p.StorageData::force;
        image.integer(0x38, value);
        p.StorageData::force = value;
    }
    {
        auto value = p.StorageData::index;
        image.integer(0x3a, value);
        p.StorageData::index = value;
    }
    {
        auto value = p.StorageData::kind;
        image.integer(0x3c, value);
        p.StorageData::kind = value;
    }
    {
        auto value = p.StorageData::removed;
        image.integer(0x3e, value);
        p.StorageData::removed = value;
    }
}

void fieldsSuperSprite(SaveImage &image, SuperSprite &p)
{
    fieldsEventProcess(image, p);
    {
        auto value = p.SuperSprite::vx;
        image.integer(0x36, value);
        p.SuperSprite::vx = value;
    }
    {
        auto value = p.SuperSprite::vy;
        image.integer(0x38, value);
        p.SuperSprite::vy = value;
    }
    {
        auto value = p.SuperSprite::vz;
        image.integer(0x3a, value);
        p.SuperSprite::vz = value;
    }
    {
        auto value = p.SuperSprite::sprite;
        image.integer(0x3c, value);
        p.SuperSprite::sprite = value;
    }
    {
        auto value = p.SuperSprite::shape;
        image.integer(0x3e, value);
        p.SuperSprite::shape = value;
    }
    {
        auto value = p.SuperSprite::last.ItemPoint::x;
        image.integer(0x40, value);
        p.SuperSprite::last.ItemPoint::x = value;
    }
    {
        auto value = p.SuperSprite::last.ItemPoint::y;
        image.integer(0x42, value);
        p.SuperSprite::last.ItemPoint::y = value;
    }
    {
        auto value = p.SuperSprite::last.ItemPoint::z;
        image.integer(0x44, value);
        p.SuperSprite::last.ItemPoint::z = value;
    }
    {
        auto value = p.SuperSprite::now.ItemPoint::x;
        image.integer(0x45, value);
        p.SuperSprite::now.ItemPoint::x = value;
    }
    {
        auto value = p.SuperSprite::now.ItemPoint::y;
        image.integer(0x47, value);
        p.SuperSprite::now.ItemPoint::y = value;
    }
    {
        auto value = p.SuperSprite::now.ItemPoint::z;
        image.integer(0x49, value);
        p.SuperSprite::now.ItemPoint::z = value;
    }
    {
        auto value = p.SuperSprite::next.ItemPoint::x;
        image.integer(0x4a, value);
        p.SuperSprite::next.ItemPoint::x = value;
    }
    {
        auto value = p.SuperSprite::next.ItemPoint::y;
        image.integer(0x4c, value);
        p.SuperSprite::next.ItemPoint::y = value;
    }
    {
        auto value = p.SuperSprite::next.ItemPoint::z;
        image.integer(0x4e, value);
        p.SuperSprite::next.ItemPoint::z = value;
    }
    {
        auto value = p.SuperSprite::start.ItemPoint::x;
        image.integer(0x4f, value);
        p.SuperSprite::start.ItemPoint::x = value;
    }
    {
        auto value = p.SuperSprite::start.ItemPoint::y;
        image.integer(0x51, value);
        p.SuperSprite::start.ItemPoint::y = value;
    }
    {
        auto value = p.SuperSprite::start.ItemPoint::z;
        image.integer(0x53, value);
        p.SuperSprite::start.ItemPoint::z = value;
    }
    {
        auto value = p.SuperSprite::splash.ItemPoint::x;
        image.integer(0x54, value);
        p.SuperSprite::splash.ItemPoint::x = value;
    }
    {
        auto value = p.SuperSprite::splash.ItemPoint::y;
        image.integer(0x56, value);
        p.SuperSprite::splash.ItemPoint::y = value;
    }
    {
        auto value = p.SuperSprite::splash.ItemPoint::z;
        image.integer(0x58, value);
        p.SuperSprite::splash.ItemPoint::z = value;
    }
    {
        auto value = p.SuperSprite::launched;
        image.integer(0x59, value);
        p.SuperSprite::launched = value;
    }
    {
        auto value = p.SuperSprite::steps;
        image.integer(0x5a, value);
        p.SuperSprite::steps = value;
    }
    {
        auto value = p.SuperSprite::dy;
        image.integer(0x5e, value);
        p.SuperSprite::dy = value;
    }
    {
        auto value = p.SuperSprite::dx;
        image.integer(0x62, value);
        p.SuperSprite::dx = value;
    }
    {
        auto value = p.SuperSprite::counter;
        image.integer(0x66, value);
        p.SuperSprite::counter = value;
    }
    {
        auto value = p.SuperSprite::damage;
        image.integer(0x6a, value);
        p.SuperSprite::damage = value;
    }
    {
        auto value = p.SuperSprite::distance;
        image.integer(0x6c, value);
        p.SuperSprite::distance = value;
    }
    {
        auto value = p.SuperSprite::dz;
        image.integer(0x6e, value);
        p.SuperSprite::dz = value;
    }
    {
        auto value = p.SuperSprite::fire;
        image.integer(0x70, value);
        p.SuperSprite::fire = value;
    }
    {
        auto value = p.SuperSprite::target.ItemPoint::x;
        image.integer(0x72, value);
        p.SuperSprite::target.ItemPoint::x = value;
    }
    {
        auto value = p.SuperSprite::target.ItemPoint::y;
        image.integer(0x74, value);
        p.SuperSprite::target.ItemPoint::y = value;
    }
    {
        auto value = p.SuperSprite::target.ItemPoint::z;
        image.integer(0x76, value);
        p.SuperSprite::target.ItemPoint::z = value;
    }
    {
        auto value = p.SuperSprite::hitItem;
        image.integer(0x77, value);
        p.SuperSprite::hitItem = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::mask;
        image.integer(0x79, value);
        p.SuperSprite::sweep.Sweep::mask = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::options;
        image.integer(0x7b, value);
        p.SuperSprite::sweep.Sweep::options = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::width;
        image.integer(0x7d, value);
        p.SuperSprite::sweep.Sweep::width = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::depth;
        image.integer(0x7f, value);
        p.SuperSprite::sweep.Sweep::depth = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::height;
        image.integer(0x81, value);
        p.SuperSprite::sweep.Sweep::height = value;
    }
    for (u16 i = 0; i < 6; i++) {
        auto value = p.SuperSprite::sweep.Sweep::unknown0a[i];
        image.integer(0x83 + (i * 1), value);
        p.SuperSprite::sweep.Sweep::unknown0a[i] = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::moveX;
        image.integer(0x89, value);
        p.SuperSprite::sweep.Sweep::moveX = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::moveY;
        image.integer(0x8b, value);
        p.SuperSprite::sweep.Sweep::moveY = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::moveZ;
        image.integer(0x8d, value);
        p.SuperSprite::sweep.Sweep::moveZ = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::fullX;
        image.integer(0x8f, value);
        p.SuperSprite::sweep.Sweep::fullX = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::fullY;
        image.integer(0x91, value);
        p.SuperSprite::sweep.Sweep::fullY = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::fullZ;
        image.integer(0x93, value);
        p.SuperSprite::sweep.Sweep::fullZ = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::drop;
        image.integer(0x95, value);
        p.SuperSprite::sweep.Sweep::drop = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::end.ItemPoint::x;
        image.integer(0x97, value);
        p.SuperSprite::sweep.Sweep::end.ItemPoint::x = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::end.ItemPoint::y;
        image.integer(0x99, value);
        p.SuperSprite::sweep.Sweep::end.ItemPoint::y = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::end.ItemPoint::z;
        image.integer(0x9b, value);
        p.SuperSprite::sweep.Sweep::end.ItemPoint::z = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::blockedAxes;
        image.integer(0x9c, value);
        p.SuperSprite::sweep.Sweep::blockedAxes = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::item;
        image.integer(0x9e, value);
        p.SuperSprite::sweep.Sweep::item = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::hitItem;
        image.integer(0xa0, value);
        p.SuperSprite::sweep.Sweep::hitItem = value;
    }
    image.pointer(0xa2, SAVE_TRANSIENT, p.SuperSprite::sweep.Sweep::hits);
    {
        auto value = p.SuperSprite::sweep.Sweep::target;
        image.integer(0xa6, value);
        p.SuperSprite::sweep.Sweep::target = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::blocker;
        image.integer(0xa8, value);
        p.SuperSprite::sweep.Sweep::blocker = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::targetFirst;
        image.integer(0xaa, value);
        p.SuperSprite::sweep.Sweep::targetFirst = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::targetHit;
        image.integer(0xab, value);
        p.SuperSprite::sweep.Sweep::targetHit = value;
    }
    {
        auto value = p.SuperSprite::sweep.Sweep::outside;
        image.integer(0xac, value);
        p.SuperSprite::sweep.Sweep::outside = value;
    }
    {
        auto value = p.SuperSprite::source;
        image.integer(0xad, value);
        p.SuperSprite::source = value;
    }
    {
        auto value = p.SuperSprite::targetItem;
        image.integer(0xaf, value);
        p.SuperSprite::targetItem = value;
    }
    {
        auto value = p.SuperSprite::bounces;
        image.integer(0xb1, value);
        p.SuperSprite::bounces = value;
    }
    {
        auto value = p.SuperSprite::homing;
        image.integer(0xb3, value);
        p.SuperSprite::homing = value;
    }
    {
        auto value = p.SuperSprite::expired;
        image.integer(0xb4, value);
        p.SuperSprite::expired = value;
    }
    {
        auto value = p.SuperSprite::startTime;
        image.integer(0xb5, value);
        p.SuperSprite::startTime = value;
    }
    {
        auto value = p.SuperSprite::unknownb9;
        image.integer(0xb9, value);
        p.SuperSprite::unknownb9 = value;
    }
    {
        auto value = p.SuperSprite::direction;
        image.integer(0xbd, value);
        p.SuperSprite::direction = value;
    }
    {
        auto value = p.SuperSprite::frame;
        image.integer(0xbe, value);
        p.SuperSprite::frame = value;
    }
}

void fieldsSurrender(SaveImage &image, Surrender &p)
{
    fieldsActivity(image, p);
    {
        auto value = p.Surrender::soundPlayed;
        image.integer(0x42, value);
        p.Surrender::soundPlayed = value;
    }
    {
        auto value = p.Surrender::unknown48;
        image.integer(0x43, value);
        p.Surrender::unknown48 = value;
    }
}

void fieldsSurrenderAnim(SaveImage &image, SurrenderAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsSurrenderStandAnim(SaveImage &image, SurrenderStandAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsSystemTimer(SaveImage &image, SystemTimer &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.SystemTimer::ticks;
        image.integer(0x40, value);
        p.SystemTimer::ticks = value;
    }
    for (u16 i = 0; i < 6; i++) {
        auto value = p.SystemTimer::reserved[i];
        image.integer(0x44 + (i * 1), value);
        p.SystemTimer::reserved[i] = value;
    }
}

void fieldsTeleportInAnim(SaveImage &image, TeleportInAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsTeleportOutAnim(SaveImage &image, TeleportOutAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsTeleporter(SaveImage &image, Teleporter &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.Teleporter::map;
        image.integer(0x32, value);
        p.Teleporter::map = value;
    }
    {
        auto value = p.Teleporter::egg;
        image.integer(0x34, value);
        p.Teleporter::egg = value;
    }
}

void fieldsTestAnchor(SaveImage &image, TestAnchor &p)
{
    fieldsProcess(image, p);
}

void fieldsTickCounter(SaveImage &image, TickCounter &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.TickCounter::ticks;
        image.integer(0x40, value);
        p.TickCounter::ticks = value;
    }
}

void fieldsTimerProcess(SaveImage &image, TimerProcess &p)
{
    fieldsPeriodicProcess(image, p);
    {
        auto value = p.TimerProcess::running;
        image.integer(0x40, value);
        p.TimerProcess::running = value;
    }
    {
        auto value = p.TimerProcess::seconds;
        image.integer(0x41, value);
        p.TimerProcess::seconds = value;
    }
    {
        auto value = p.TimerProcess::tenths;
        image.integer(0x43, value);
        p.TimerProcess::tenths = value;
    }
}

void fieldsU9MouseControlProcess(SaveImage &image, U9MouseControlProcess &p)
{
    fieldsMouseCtl(image, p);
    {
        auto value = p.U9MouseControlProcess::range;
        image.integer(0x198, value);
        p.U9MouseControlProcess::range = value;
    }
    {
        auto value = p.U9MouseControlProcess::direction;
        image.integer(0x19a, value);
        p.U9MouseControlProcess::direction = value;
    }
    {
        auto value = p.U9MouseControlProcess::centreX;
        image.integer(0x19c, value);
        p.U9MouseControlProcess::centreX = value;
    }
    {
        auto value = p.U9MouseControlProcess::centreY;
        image.integer(0x19e, value);
        p.U9MouseControlProcess::centreY = value;
    }
}

void fieldsUnreadyLargeAnim(SaveImage &image, UnreadyLargeAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsUnreadySmallAnim(SaveImage &image, UnreadySmallAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

void fieldsUsecodeProcess(SaveImage &image, UsecodeProcess &p)
{
    fieldsProcess(image, p);
    {
        auto value = p.UsecodeProcess::classId;
        image.integer(0x32, value);
        p.UsecodeProcess::classId = value;
    }
    {
        auto value = p.UsecodeProcess::function;
        image.integer(0x34, value);
        p.UsecodeProcess::function = value;
    }
    image.pointer(0x36, SAVE_USECODE_CALLS, p.UsecodeProcess::thread.DataStack::calls);
    for (u16 i = 0; i < 200; i++) {
        auto value = p.UsecodeProcess::thread.DataStack::data[i];
        image.integer(0x3a + (i * 1), value);
        p.UsecodeProcess::thread.DataStack::data[i] = value;
    }
    image.pointer(0x102, SAVE_USECODE_TOP, p.UsecodeProcess::thread.DataStack::top);
    image.pointer(0x106, SAVE_USECODE_END, p.UsecodeProcess::thread.DataStack::end);
    {
        auto value = p.UsecodeProcess::thread.DataStack::base;
        image.integer(0x10a, value);
        p.UsecodeProcess::thread.DataStack::base = value;
    }
    image.pointer(0x10c, SAVE_USECODE_IP, p.UsecodeProcess::thread.UsecodeThread::ip);
    image.pointer(0x110, SAVE_USECODE_PROCESS, p.UsecodeProcess::thread.UsecodeThread::process);
    {
        auto value = p.UsecodeProcess::thread.UsecodeThread::unknownDE;
        image.integer(0x114, value);
        p.UsecodeProcess::thread.UsecodeThread::unknownDE = value;
    }
    {
        auto value = p.UsecodeProcess::thread.UsecodeThread::started;
        image.integer(0x116, value);
        p.UsecodeProcess::thread.UsecodeThread::started = value;
    }
    image.pointer(0x117, SAVE_USECODE_CLASS, p.UsecodeProcess::thread.UsecodeThread::className);
    image.pointer(0x11b, SAVE_USECODE_RESULT, p.UsecodeProcess::resultPointer);
    {
        auto value = p.UsecodeProcess::localsOffset;
        image.integer(0x11f, value);
        p.UsecodeProcess::localsOffset = value;
    }
    {
        auto value = p.UsecodeProcess::argumentSize;
        image.integer(0x121, value);
        p.UsecodeProcess::argumentSize = value;
    }
    {
        auto value = p.UsecodeProcess::finished;
        image.integer(0x122, value);
        p.UsecodeProcess::finished = value;
    }
    {
        auto value = p.UsecodeProcess::running;
        image.integer(0x123, value);
        p.UsecodeProcess::running = value;
    }
}

void fieldsWalkAnim(SaveImage &image, WalkAnim &p)
{
    fieldsAnimPrimitive(image, p);
}

}

const SaveClass SAVE_CLASSES[] = {
    {"EventProcess", &typeid(EventProcess), 0x3ad2, 0x36,
     [](SaveImage &image, Process &p) { fieldsEventProcess(image, static_cast<EventProcess &>(p)); },
     []() -> Process * { return new (Loading{}) EventProcess(Loading{}); }},
    {"PeriodicProcess", &typeid(PeriodicProcess), 0x3aa6, 0x40,
     [](SaveImage &image, Process &p) { fieldsPeriodicProcess(image, static_cast<PeriodicProcess &>(p)); },
     []() -> Process * { return new (Loading{}) PeriodicProcess(Loading{}); }},
    {"ASSProcess", &typeid(ASSProcess), 0x461f, 0x40,
     [](SaveImage &image, Process &p) { fieldsASSProcess(image, static_cast<ASSProcess &>(p)); },
     []() -> Process * { return new (Loading{}) ASSProcess(Loading{}); }},
    {"AccWait", &typeid(AccWait), 0x2e63, 0x48,
     [](SaveImage &image, Process &p) { fieldsAccWait(image, static_cast<AccWait &>(p)); },
     []() -> Process * { return new (Loading{}) AccWait(Loading{}); }},
    {"Activity", &typeid(Activity), 0x1f3b, 0x42,
     [](SaveImage &image, Process &p) { fieldsActivity(image, static_cast<Activity &>(p)); },
     []() -> Process * { return new (Loading{}) Activity(Loading{}); }},
    {"AnimPrimitive", &typeid(AnimPrimitive), 0x2a57, 0x5b,
     [](SaveImage &image, Process &p) { fieldsAnimPrimitive(image, static_cast<AnimPrimitive &>(p)); },
     []() -> Process * { return new (Loading{}) AnimPrimitive(Loading{}); }},
    {"AdvanceLargeAnim", &typeid(AdvanceLargeAnim), 0x155f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsAdvanceLargeAnim(image, static_cast<AdvanceLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) AdvanceLargeAnim(Loading{}); }},
    {"AdvanceSmallAnim", &typeid(AdvanceSmallAnim), 0x158f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsAdvanceSmallAnim(image, static_cast<AdvanceSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) AdvanceSmallAnim(Loading{}); }},
    {"Anim06", &typeid(Anim06), 0x182f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsAnim06(image, static_cast<Anim06 &>(p)); },
     []() -> Process * { return new (Loading{}) Anim06(Loading{}); }},
    {"Anim11", &typeid(Anim11), 0x164f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsAnim11(image, static_cast<Anim11 &>(p)); },
     []() -> Process * { return new (Loading{}) Anim11(Loading{}); }},
    {"Anim13", &typeid(Anim13), 0x15ef, 0x5b,
     [](SaveImage &image, Process &p) { fieldsAnim13(image, static_cast<Anim13 &>(p)); },
     []() -> Process * { return new (Loading{}) Anim13(Loading{}); }},
    {"AnimationProcess", &typeid(AnimationProcess), 0x5a2, 0x55,
     [](SaveImage &image, Process &p) { fieldsAnimationProcess(image, static_cast<AnimationProcess &>(p)); },
     []() -> Process * { return new (Loading{}) AnimationProcess(Loading{}); }},
    {"Attack", &typeid(Attack), 0x1fc9, 0x98,
     [](SaveImage &image, Process &p) { fieldsAttack(image, static_cast<Attack &>(p)); },
     []() -> Process * { return new (Loading{}) Attack(Loading{}); }},
    {"AutoFirer", &typeid(AutoFirer), 0x28b5, 0x40,
     [](SaveImage &image, Process &p) { fieldsAutoFirer(image, static_cast<AutoFirer &>(p)); },
     []() -> Process * { return new (Loading{}) AutoFirer(Loading{}); }},
    {"BaseCamera", &typeid(BaseCamera), 0x4724, 0x32,
     [](SaveImage &image, Process &p) { fieldsBaseCamera(image, static_cast<BaseCamera &>(p)); },
     []() -> Process * { return new (Loading{}) BaseCamera(Loading{}); }},
    {"BatteryCharger", &typeid(BatteryCharger), 0x2750, 0x34,
     [](SaveImage &image, Process &p) { fieldsBatteryCharger(image, static_cast<BatteryCharger &>(p)); },
     []() -> Process * { return new (Loading{}) BatteryCharger(Loading{}); }},
    {"BiosTimer", &typeid(BiosTimer), 0x3172, 0x48,
     [](SaveImage &image, Process &p) { fieldsBiosTimer(image, static_cast<BiosTimer &>(p)); },
     []() -> Process * { return new (Loading{}) BiosTimer(Loading{}); }},
    {"BrightFireLargeAnim", &typeid(BrightFireLargeAnim), 0x1acf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsBrightFireLargeAnim(image, static_cast<BrightFireLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) BrightFireLargeAnim(Loading{}); }},
    {"BrightKneelFireAnim", &typeid(BrightKneelFireAnim), 0x1aff, 0x5b,
     [](SaveImage &image, Process &p) { fieldsBrightKneelFireAnim(image, static_cast<BrightKneelFireAnim &>(p)); },
     []() -> Process * { return new (Loading{}) BrightKneelFireAnim(Loading{}); }},
    {"Camera", &typeid(Camera), 0x2c2b, 0x32,
     [](SaveImage &image, Process &p) { fieldsCamera(image, static_cast<Camera &>(p)); },
     []() -> Process * { return new (Loading{}) Camera(Loading{}); }},
    {"CombatRollLeftAnim", &typeid(CombatRollLeftAnim), 0x191f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsCombatRollLeftAnim(image, static_cast<CombatRollLeftAnim &>(p)); },
     []() -> Process * { return new (Loading{}) CombatRollLeftAnim(Loading{}); }},
    {"CombatRollRightAnim", &typeid(CombatRollRightAnim), 0x18ef, 0x5b,
     [](SaveImage &image, Process &p) { fieldsCombatRollRightAnim(image, static_cast<CombatRollRightAnim &>(p)); },
     []() -> Process * { return new (Loading{}) CombatRollRightAnim(Loading{}); }},
    {"CombatRunLargeAnim", &typeid(CombatRunLargeAnim), 0x12ef, 0x5b,
     [](SaveImage &image, Process &p) { fieldsCombatRunLargeAnim(image, static_cast<CombatRunLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) CombatRunLargeAnim(Loading{}); }},
    {"CombatRunSmallAnim", &typeid(CombatRunSmallAnim), 0x131f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsCombatRunSmallAnim(image, static_cast<CombatRunSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) CombatRunSmallAnim(Loading{}); }},
    {"CombatStandLargeAnim", &typeid(CombatStandLargeAnim), 0x176f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsCombatStandLargeAnim(image, static_cast<CombatStandLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) CombatStandLargeAnim(Loading{}); }},
    {"CombatStandSmallAnim", &typeid(CombatStandSmallAnim), 0x179f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsCombatStandSmallAnim(image, static_cast<CombatStandSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) CombatStandSmallAnim(Loading{}); }},
    {"Controller", &typeid(Controller), 0x28e1, 0x40,
     [](SaveImage &image, Process &p) { fieldsController(image, static_cast<Controller &>(p)); },
     []() -> Process * { return new (Loading{}) Controller(Loading{}); }},
    {"CreditsProcess", &typeid(CreditsProcess), 0x62d5, 0x32,
     [](SaveImage &image, Process &p) { fieldsCreditsProcess(image, static_cast<CreditsProcess &>(p)); },
     []() -> Process * { return new (Loading{}) CreditsProcess(Loading{}); }},
    {"CruHealer", &typeid(CruHealer), 0x2774, 0x34,
     [](SaveImage &image, Process &p) { fieldsCruHealer(image, static_cast<CruHealer &>(p)); },
     []() -> Process * { return new (Loading{}) CruHealer(Loading{}); }},
    {"CycleProcess", &typeid(CycleProcess), 0x687f, 0x42,
     [](SaveImage &image, Process &p) { fieldsCycleProcess(image, static_cast<CycleProcess &>(p)); },
     []() -> Process * { return new (Loading{}) CycleProcess(Loading{}); }},
    {"CycleResumer", &typeid(CycleResumer), 0x4fa, 0x32,
     [](SaveImage &image, Process &p) { fieldsCycleResumer(image, static_cast<CycleResumer &>(p)); },
     []() -> Process * { return new (Loading{}) CycleResumer(Loading{}); }},
    {"DeathSilence", &typeid(DeathSilence), 0x2798, 0x42,
     [](SaveImage &image, Process &p) { fieldsDeathSilence(image, static_cast<DeathSilence &>(p)); },
     []() -> Process * { return new (Loading{}) DeathSilence(Loading{}); }},
    {"Dispatcher", &typeid(Dispatcher), 0x4ad4, 0x36,
     [](SaveImage &image, Process &p) { fieldsDispatcher(image, static_cast<Dispatcher &>(p)); },
     []() -> Process * { return new (Loading{}) Dispatcher(Loading{}); }},
    {"DumbTimer", &typeid(DumbTimer), 0x67fc, 0x44,
     [](SaveImage &image, Process &p) { fieldsDumbTimer(image, static_cast<DumbTimer &>(p)); },
     []() -> Process * { return new (Loading{}) DumbTimer(Loading{}); }},
    {"EggHatcher", &typeid(EggHatcher), 0xf16, 0x33,
     [](SaveImage &image, Process &p) { fieldsEggHatcher(image, static_cast<EggHatcher &>(p)); },
     []() -> Process * { return new (Loading{}) EggHatcher(Loading{}); }},
    {"ElectrocutedAnim", &typeid(ElectrocutedAnim), 0x1b2f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsElectrocutedAnim(image, static_cast<ElectrocutedAnim &>(p)); },
     []() -> Process * { return new (Loading{}) ElectrocutedAnim(Loading{}); }},
    {"FadeProcess", &typeid(FadeProcess), 0x51e, 0x4e,
     [](SaveImage &image, Process &p) { fieldsFadeProcess(image, static_cast<FadeProcess &>(p)); },
     []() -> Process * { return new (Loading{}) FadeProcess(Loading{}); }},
    {"FallBackwardsAnim", &typeid(FallBackwardsAnim), 0x161f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsFallBackwardsAnim(image, static_cast<FallBackwardsAnim &>(p)); },
     []() -> Process * { return new (Loading{}) FallBackwardsAnim(Loading{}); }},
    {"FallForwardsAnim", &typeid(FallForwardsAnim), 0x15bf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsFallForwardsAnim(image, static_cast<FallForwardsAnim &>(p)); },
     []() -> Process * { return new (Loading{}) FallForwardsAnim(Loading{}); }},
    {"FinishFiringAnim", &typeid(FinishFiringAnim), 0x1caf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsFinishFiringAnim(image, static_cast<FinishFiringAnim &>(p)); },
     []() -> Process * { return new (Loading{}) FinishFiringAnim(Loading{}); }},
    {"FireLargeAnim", &typeid(FireLargeAnim), 0x1b5f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsFireLargeAnim(image, static_cast<FireLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) FireLargeAnim(Loading{}); }},
    {"FireSmallAnim", &typeid(FireSmallAnim), 0x1b8f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsFireSmallAnim(image, static_cast<FireSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) FireSmallAnim(Loading{}); }},
    {"FlicPlayer", &typeid(FlicPlayer), 0x6930, 0x757,
     [](SaveImage &image, Process &p) { fieldsFlicPlayer(image, static_cast<FlicPlayer &>(p)); },
     []() -> Process * { return new (Loading{}) FlicPlayer(Loading{}); }},
    {"FlicRestoreProcess", &typeid(FlicRestoreProcess), 0x695c, 0x34,
     [](SaveImage &image, Process &p) { fieldsFlicRestoreProcess(image, static_cast<FlicRestoreProcess &>(p)); },
     []() -> Process * { return new (Loading{}) FlicRestoreProcess(Loading{}); }},
    {"FlicWait", &typeid(FlicWait), 0x6980, 0x42,
     [](SaveImage &image, Process &p) { fieldsFlicWait(image, static_cast<FlicWait &>(p)); },
     []() -> Process * { return new (Loading{}) FlicWait(Loading{}); }},
    {"GameTime", &typeid(GameTime), 0x5ce, 0x39,
     [](SaveImage &image, Process &p) { fieldsGameTime(image, static_cast<GameTime &>(p)); },
     []() -> Process * { return new (Loading{}) GameTime(Loading{}); }},
    {"GravityProcess", &typeid(GravityProcess), 0x610, 0x42,
     [](SaveImage &image, Process &p) { fieldsGravityProcess(image, static_cast<GravityProcess &>(p)); },
     []() -> Process * { return new (Loading{}) GravityProcess(Loading{}); }},
    {"Guard", &typeid(Guard), 0x1e8b, 0x42,
     [](SaveImage &image, Process &p) { fieldsGuard(image, static_cast<Guard &>(p)); },
     []() -> Process * { return new (Loading{}) Guard(Loading{}); }},
    {"WaitProcess", &typeid(WaitProcess), 0x2e33, 0x3e,
     [](SaveImage &image, Process &p) { fieldsWaitProcess(image, static_cast<WaitProcess &>(p)); },
     []() -> Process * { return new (Loading{}) WaitProcess(Loading{}); }},
    {"GumpNotifyProcess", &typeid(GumpNotifyProcess), 0x5a69, 0x74,
     [](SaveImage &image, Process &p) { fieldsGumpNotifyProcess(image, static_cast<GumpNotifyProcess &>(p)); },
     []() -> Process * { return new (Loading{}) GumpNotifyProcess(Loading{}); }},
    {"JoystickDriver", &typeid(JoystickDriver), 0x6577, 0x49,
     [](SaveImage &image, Process &p) { fieldsJoystickDriver(image, static_cast<JoystickDriver &>(p)); },
     []() -> Process * { return new (Loading{}) JoystickDriver(Loading{}); }},
    {"JumpForwardAnim", &typeid(JumpForwardAnim), 0x1c4f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsJumpForwardAnim(image, static_cast<JumpForwardAnim &>(p)); },
     []() -> Process * { return new (Loading{}) JumpForwardAnim(Loading{}); }},
    {"JumpLandingAnim", &typeid(JumpLandingAnim), 0x1d0f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsJumpLandingAnim(image, static_cast<JumpLandingAnim &>(p)); },
     []() -> Process * { return new (Loading{}) JumpLandingAnim(Loading{}); }},
    {"KeyDaemon", &typeid(KeyDaemon), 0x2d52, 0x40,
     [](SaveImage &image, Process &p) { fieldsKeyDaemon(image, static_cast<KeyDaemon &>(p)); },
     []() -> Process * { return new (Loading{}) KeyDaemon(Loading{}); }},
    {"KeyboardProcess", &typeid(KeyboardProcess), 0x2da6, 0x36,
     [](SaveImage &image, Process &p) { fieldsKeyboardProcess(image, static_cast<KeyboardProcess &>(p)); },
     []() -> Process * { return new (Loading{}) KeyboardProcess(Loading{}); }},
    {"KneelEndAnim", &typeid(KneelEndAnim), 0x140f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelEndAnim(image, static_cast<KneelEndAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelEndAnim(Loading{}); }},
    {"KneelFireLargeAnim", &typeid(KneelFireLargeAnim), 0x13af, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelFireLargeAnim(image, static_cast<KneelFireLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelFireLargeAnim(Loading{}); }},
    {"KneelFireSmallAnim", &typeid(KneelFireSmallAnim), 0x13df, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelFireSmallAnim(image, static_cast<KneelFireSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelFireSmallAnim(Loading{}); }},
    {"KneelRollLeftAnim", &typeid(KneelRollLeftAnim), 0x18bf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelRollLeftAnim(image, static_cast<KneelRollLeftAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelRollLeftAnim(Loading{}); }},
    {"KneelRollRightAnim", &typeid(KneelRollRightAnim), 0x188f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelRollRightAnim(image, static_cast<KneelRollRightAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelRollRightAnim(Loading{}); }},
    {"KneelStartAnim", &typeid(KneelStartAnim), 0x143f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelStartAnim(image, static_cast<KneelStartAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelStartAnim(Loading{}); }},
    {"KneelingAdvanceAnim", &typeid(KneelingAdvanceAnim), 0x16af, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelingAdvanceAnim(image, static_cast<KneelingAdvanceAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelingAdvanceAnim(Loading{}); }},
    {"KneelingLargeAnim", &typeid(KneelingLargeAnim), 0x134f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelingLargeAnim(image, static_cast<KneelingLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelingLargeAnim(Loading{}); }},
    {"KneelingRetreatAnim", &typeid(KneelingRetreatAnim), 0x16df, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelingRetreatAnim(image, static_cast<KneelingRetreatAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelingRetreatAnim(Loading{}); }},
    {"KneelingSlideLeftAnim", &typeid(KneelingSlideLeftAnim), 0x167f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelingSlideLeftAnim(image, static_cast<KneelingSlideLeftAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelingSlideLeftAnim(Loading{}); }},
    {"KneelingSmallAnim", &typeid(KneelingSmallAnim), 0x137f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsKneelingSmallAnim(image, static_cast<KneelingSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) KneelingSmallAnim(Loading{}); }},
    {"Loiter", &typeid(Loiter), 0x1e5f, 0x49,
     [](SaveImage &image, Process &p) { fieldsLoiter(image, static_cast<Loiter &>(p)); },
     []() -> Process * { return new (Loading{}) Loiter(Loading{}); }},
    {"LookLeftAnim", &typeid(LookLeftAnim), 0x173f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsLookLeftAnim(image, static_cast<LookLeftAnim &>(p)); },
     []() -> Process * { return new (Loading{}) LookLeftAnim(Loading{}); }},
    {"LookRightAnim", &typeid(LookRightAnim), 0x170f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsLookRightAnim(image, static_cast<LookRightAnim &>(p)); },
     []() -> Process * { return new (Loading{}) LookRightAnim(Loading{}); }},
    {"MapJumpProcess", &typeid(MapJumpProcess), 0x4b6, 0x34,
     [](SaveImage &image, Process &p) { fieldsMapJumpProcess(image, static_cast<MapJumpProcess &>(p)); },
     []() -> Process * { return new (Loading{}) MapJumpProcess(Loading{}); }},
    {"MouseCtl", &typeid(MouseCtl), 0x4f6c, 0x198,
     [](SaveImage &image, Process &p) { fieldsMouseCtl(image, static_cast<MouseCtl &>(p)); },
     []() -> Process * { return new (Loading{}) MouseCtl(Loading{}); }},
    {"MusicProcess", &typeid(MusicProcess), 0x3c81, 0x40,
     [](SaveImage &image, Process &p) { fieldsMusicProcess(image, static_cast<MusicProcess &>(p)); },
     []() -> Process * { return new (Loading{}) MusicProcess(Loading{}); }},
    {"NpcHold", &typeid(NpcHold), 0xf5e, 0x33,
     [](SaveImage &image, Process &p) { fieldsNpcHold(image, static_cast<NpcHold &>(p)); },
     []() -> Process * { return new (Loading{}) NpcHold(Loading{}); }},
    {"Pace", &typeid(Pace), 0x1eb7, 0x46,
     [](SaveImage &image, Process &p) { fieldsPace(image, static_cast<Pace &>(p)); },
     []() -> Process * { return new (Loading{}) Pace(Loading{}); }},
    {"PathFinder", &typeid(PathFinder), 0x2012, 0x50,
     [](SaveImage &image, Process &p) { fieldsPathFinder(image, static_cast<PathFinder &>(p)); },
     []() -> Process * { return new (Loading{}) PathFinder(Loading{}); }},
    {"PickupProcess", &typeid(PickupProcess), 0x2c82, 0x36,
     [](SaveImage &image, Process &p) { fieldsPickupProcess(image, static_cast<PickupProcess &>(p)); },
     []() -> Process * { return new (Loading{}) PickupProcess(Loading{}); }},
    {"QuickJumpAnim", &typeid(QuickJumpAnim), 0x1c7f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsQuickJumpAnim(image, static_cast<QuickJumpAnim &>(p)); },
     []() -> Process * { return new (Loading{}) QuickJumpAnim(Loading{}); }},
    {"ReadyLargeAnim", &typeid(ReadyLargeAnim), 0x1a6f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsReadyLargeAnim(image, static_cast<ReadyLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) ReadyLargeAnim(Loading{}); }},
    {"ReadySmallAnim", &typeid(ReadySmallAnim), 0x1a9f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsReadySmallAnim(image, static_cast<ReadySmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) ReadySmallAnim(Loading{}); }},
    {"ReleaseProcess", &typeid(ReleaseProcess), 0x102f, 0x32,
     [](SaveImage &image, Process &p) { fieldsReleaseProcess(image, static_cast<ReleaseProcess &>(p)); },
     []() -> Process * { return new (Loading{}) ReleaseProcess(Loading{}); }},
    {"ReloadLargeAnim", &typeid(ReloadLargeAnim), 0x1d6f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsReloadLargeAnim(image, static_cast<ReloadLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) ReloadLargeAnim(Loading{}); }},
    {"ResumeCycling", &typeid(ResumeCycling), 0x68bf, 0x32,
     [](SaveImage &image, Process &p) { fieldsResumeCycling(image, static_cast<ResumeCycling &>(p)); },
     []() -> Process * { return new (Loading{}) ResumeCycling(Loading{}); }},
    {"RetreatLargeAnim", &typeid(RetreatLargeAnim), 0x152f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsRetreatLargeAnim(image, static_cast<RetreatLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) RetreatLargeAnim(Loading{}); }},
    {"RetreatSmallAnim", &typeid(RetreatSmallAnim), 0x1a0f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsRetreatSmallAnim(image, static_cast<RetreatSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) RetreatSmallAnim(Loading{}); }},
    {"RollForwardAnim", &typeid(RollForwardAnim), 0x185f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsRollForwardAnim(image, static_cast<RollForwardAnim &>(p)); },
     []() -> Process * { return new (Loading{}) RollForwardAnim(Loading{}); }},
    {"RunAnim", &typeid(RunAnim), 0x19df, 0x5b,
     [](SaveImage &image, Process &p) { fieldsRunAnim(image, static_cast<RunAnim &>(p)); },
     []() -> Process * { return new (Loading{}) RunAnim(Loading{}); }},
    {"SlideLeftAnim", &typeid(SlideLeftAnim), 0x197f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsSlideLeftAnim(image, static_cast<SlideLeftAnim &>(p)); },
     []() -> Process * { return new (Loading{}) SlideLeftAnim(Loading{}); }},
    {"SlideRightAnim", &typeid(SlideRightAnim), 0x194f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsSlideRightAnim(image, static_cast<SlideRightAnim &>(p)); },
     []() -> Process * { return new (Loading{}) SlideRightAnim(Loading{}); }},
    {"SnapProcess", &typeid(SnapProcess), 0xc9a, 0xc9,
     [](SaveImage &image, Process &p) { fieldsSnapProcess(image, static_cast<SnapProcess &>(p)); },
     []() -> Process * { return new (Loading{}) SnapProcess(Loading{}); }},
    {"Sprite", &typeid(Sprite), 0x29aa, 0x41,
     [](SaveImage &image, Process &p) { fieldsSprite(image, static_cast<Sprite &>(p)); },
     []() -> Process * { return new (Loading{}) Sprite(Loading{}); }},
    {"Stand", &typeid(Stand), 0x1f0f, 0x42,
     [](SaveImage &image, Process &p) { fieldsStand(image, static_cast<Stand &>(p)); },
     []() -> Process * { return new (Loading{}) Stand(Loading{}); }},
    {"StandAnim", &typeid(StandAnim), 0x19af, 0x5b,
     [](SaveImage &image, Process &p) { fieldsStandAnim(image, static_cast<StandAnim &>(p)); },
     []() -> Process * { return new (Loading{}) StandAnim(Loading{}); }},
    {"StartRunAnim", &typeid(StartRunAnim), 0x14ff, 0x5b,
     [](SaveImage &image, Process &p) { fieldsStartRunAnim(image, static_cast<StartRunAnim &>(p)); },
     []() -> Process * { return new (Loading{}) StartRunAnim(Loading{}); }},
    {"StartRunLargeAnim", &typeid(StartRunLargeAnim), 0x146f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsStartRunLargeAnim(image, static_cast<StartRunLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) StartRunLargeAnim(Loading{}); }},
    {"StartRunSmallAnim", &typeid(StartRunSmallAnim), 0x149f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsStartRunSmallAnim(image, static_cast<StartRunSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) StartRunSmallAnim(Loading{}); }},
    {"StdIntHandler", &typeid(StdIntHandler), 0x3b4d, 0x50,
     [](SaveImage &image, Process &p) { fieldsStdIntHandler(image, static_cast<StdIntHandler &>(p)); },
     []() -> Process * { return new (Loading{}) StdIntHandler(Loading{}); }},
    {"StopRunLargeAnim", &typeid(StopRunLargeAnim), 0x1cdf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsStopRunLargeAnim(image, static_cast<StopRunLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) StopRunLargeAnim(Loading{}); }},
    {"StopRunSmallAnim", &typeid(StopRunSmallAnim), 0x14cf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsStopRunSmallAnim(image, static_cast<StopRunSmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) StopRunSmallAnim(Loading{}); }},
    {"StorageData", &typeid(StorageData), 0x1dbf, 0x3f,
     [](SaveImage &image, Process &p) { fieldsStorageData(image, static_cast<StorageData &>(p)); },
     []() -> Process * { return new (Loading{}) StorageData(Loading{}); }},
    {"SuperSprite", &typeid(SuperSprite), 0x297e, 0xc0,
     [](SaveImage &image, Process &p) { fieldsSuperSprite(image, static_cast<SuperSprite &>(p)); },
     []() -> Process * { return new (Loading{}) SuperSprite(Loading{}); }},
    {"Surrender", &typeid(Surrender), 0x1ee3, 0x45,
     [](SaveImage &image, Process &p) { fieldsSurrender(image, static_cast<Surrender &>(p)); },
     []() -> Process * { return new (Loading{}) Surrender(Loading{}); }},
    {"SurrenderAnim", &typeid(SurrenderAnim), 0x1bef, 0x5b,
     [](SaveImage &image, Process &p) { fieldsSurrenderAnim(image, static_cast<SurrenderAnim &>(p)); },
     []() -> Process * { return new (Loading{}) SurrenderAnim(Loading{}); }},
    {"SurrenderStandAnim", &typeid(SurrenderStandAnim), 0x1d3f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsSurrenderStandAnim(image, static_cast<SurrenderStandAnim &>(p)); },
     []() -> Process * { return new (Loading{}) SurrenderStandAnim(Loading{}); }},
    {"SystemTimer", &typeid(SystemTimer), 0x2e97, 0x4a,
     [](SaveImage &image, Process &p) { fieldsSystemTimer(image, static_cast<SystemTimer &>(p)); },
     []() -> Process * { return new (Loading{}) SystemTimer(Loading{}); }},
    {"TeleportInAnim", &typeid(TeleportInAnim), 0x17ff, 0x5b,
     [](SaveImage &image, Process &p) { fieldsTeleportInAnim(image, static_cast<TeleportInAnim &>(p)); },
     []() -> Process * { return new (Loading{}) TeleportInAnim(Loading{}); }},
    {"TeleportOutAnim", &typeid(TeleportOutAnim), 0x17cf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsTeleportOutAnim(image, static_cast<TeleportOutAnim &>(p)); },
     []() -> Process * { return new (Loading{}) TeleportOutAnim(Loading{}); }},
    {"Teleporter", &typeid(Teleporter), 0xf3a, 0x36,
     [](SaveImage &image, Process &p) { fieldsTeleporter(image, static_cast<Teleporter &>(p)); },
     []() -> Process * { return new (Loading{}) Teleporter(Loading{}); }},
    {"TestAnchor", &typeid(TestAnchor), 0x2a33, 0x32,
     [](SaveImage &image, Process &p) { fieldsTestAnchor(image, static_cast<TestAnchor &>(p)); },
     []() -> Process * { return new (Loading{}) TestAnchor(Loading{}); }},
    {"TickCounter", &typeid(TickCounter), 0xbc4, 0x42,
     [](SaveImage &image, Process &p) { fieldsTickCounter(image, static_cast<TickCounter &>(p)); },
     []() -> Process * { return new (Loading{}) TickCounter(Loading{}); }},
    {"TimerProcess", &typeid(TimerProcess), 0x2e07, 0x45,
     [](SaveImage &image, Process &p) { fieldsTimerProcess(image, static_cast<TimerProcess &>(p)); },
     []() -> Process * { return new (Loading{}) TimerProcess(Loading{}); }},
    {"U9MouseControlProcess", &typeid(U9MouseControlProcess), 0x5eb2, 0x1a0,
     [](SaveImage &image, Process &p) { fieldsU9MouseControlProcess(image, static_cast<U9MouseControlProcess &>(p)); },
     []() -> Process * { return new (Loading{}) U9MouseControlProcess(Loading{}); }},
    {"UnreadyLargeAnim", &typeid(UnreadyLargeAnim), 0x1bbf, 0x5b,
     [](SaveImage &image, Process &p) { fieldsUnreadyLargeAnim(image, static_cast<UnreadyLargeAnim &>(p)); },
     []() -> Process * { return new (Loading{}) UnreadyLargeAnim(Loading{}); }},
    {"UnreadySmallAnim", &typeid(UnreadySmallAnim), 0x1c1f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsUnreadySmallAnim(image, static_cast<UnreadySmallAnim &>(p)); },
     []() -> Process * { return new (Loading{}) UnreadySmallAnim(Loading{}); }},
    {"UsecodeProcess", &typeid(UsecodeProcess), 0x6714, 0x124,
     [](SaveImage &image, Process &p) { fieldsUsecodeProcess(image, static_cast<UsecodeProcess &>(p)); },
     []() -> Process * { return new (Loading{}) UsecodeProcess(Loading{}); }},
    {"WalkAnim", &typeid(WalkAnim), 0x1a3f, 0x5b,
     [](SaveImage &image, Process &p) { fieldsWalkAnim(image, static_cast<WalkAnim &>(p)); },
     []() -> Process * { return new (Loading{}) WalkAnim(Loading{}); }},
};
const u16 SAVE_CLASS_COUNT = 115;

/* The names the processes give themselves (nameProcess, the kernel's
 * "Default"), at the original's DS offsets of their literals, which the
 * kernel's table of names holds. */
const SaveName SAVE_NAMES[] = {
    {0x58f, "GameTime"},
    {0x598, "Animation"},
    {0x5f2, "Gravity"},
    {0xc7a, "SnapProcess"},
    {0xee3, "Teleporter"},
    {0xf0b, "EggHatcher"},
    {0x1da0, "StorageData"},
    {0x1e3a, "Stand"},
    {0x1e40, "Surrendering"},
    {0x1e4d, "Pace"},
    {0x1e52, "Guard"},
    {0x1e58, "Loiter"},
    {0x1fa7, "Attack"},
    {0x2007, "PathFinder"},
    {0x271f, "DeathSilence"},
    {0x2737, "CruHealer"},
    {0x2741, "BatteryCharger"},
    {0x2845, "Controller"},
    {0x28ab, "AutoFirer"},
    {0x2962, "Sprite"},
    {0x2969, "SuperSprite"},
    {0x2a1a, "AnimPrimitive"},
    {0x2be4, "Camera"},
    {0x2d48, "KeyDaemon"},
    {0x2d8e, "KeyboardProcess"},
    {0x2de8, "SystemTimer"},
    {0x2dee, "Timer"},
    {0x2dff, "AccWait"},
    {0x2e02, "Wait"},
    {0x3168, "BiosTimer"},
    {0x3a70, "Default"},
    {0x3b3f, "StdIntHandler"},
    {0x3c74, "MusicProcess"},
    {0x4612, "ASSProcess"},
    {0x4719, "BaseCamera"},
    {0x4a13, "Dispatcher"},
    {0x4f49, "Mouse_Ctl"},
    {0x6417, "JoystickDriver"},
    {0x67f2, "DumbTimer"},
    {0x6872, "CycleProcess"},
    {0x68ac, "FadeProcess"},
    {0x68ec, "FlicWait"},
    {0x68f5, "FlicPlayer"},
};
const u16 SAVE_NAME_COUNT = 43;

/* What the original's saves hold where its load sets the values again
 * (savefields.h). */
const u16 ORIGINAL_DS = 0x05f7;
const u16 ORIGINAL_UNIT_FILE = 0x111f;
const u16 ORIGINAL_STACK = 0x05ff;
const u16 ORIGINAL_FIRST_TACTIC = 0x1fd7;
const u32 ORIGINAL_BIOS_VECTOR = 0x011b0008;
const u16 SUBSCRIBER_LIST_VTABLE = 0x3a89;
