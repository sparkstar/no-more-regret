/* Code9 (1040, file name unknown): the game's clock and the process that times
 * the item animations. */
#include "gametime.h"
#include "gamestate.h"
#include "camera.h"
#include "kernel.h"
#include "systimer.h"
#include "world.h"

GameTime *&gameTime = gameGlobal<GameTime *>(nullptr);
AnimationProcess *&animationProcess = gameGlobal<AnimationProcess *>(nullptr);
u8 &animationsHeld = gameGlobal<u8>(0);
u8 &animationsStopped = gameSetting<u8>(0);

GameTime::GameTime()
{
    frames = 0;
    hold = 0;
    newQuarter = 0;
    setType(GAME_TIME_TYPE);
    nameProcess(pid, "GameTime");
    makeSystem();
}

void GameTime::run()
{
    if (newQuarter != 0U) {
        newQuarter = 0;
    }
    if (hold != 0U) {
        if (hold != 0xffff) {
            hold--;
        }
    } else {
        frames++;
    }
}

AnimationProcess::AnimationProcess()
{
    time = ticks = gameTicks = gameQuarters = 0;
    newTicks = -1;
    frozen = 0;
    nameProcess(pid, "Animation");
    setType(ANIMATION_TYPE);
    pause();
    setRate(30);
    addEvents(EVENT_TIMER);
    makeSystem();
}

void AnimationProcess::event(u32 event, EventData /*data*/)
{
    switch (event) {
    case EVENT_TIMER:
        if (newTicks >= 0) {
            if (gameTicks / TICKS_PER_QUARTER != newTicks / TICKS_PER_QUARTER) {
                gameQuarters = newTicks / TICKS_PER_QUARTER;
                gameTime->newQuarter = 1;
            }
            gameTicks = newTicks;
            newTicks = -1;
        } else {
            gameTicks++;
            if (gameTicks % TICKS_PER_QUARTER == 0) {
                gameQuarters++;
                gameTime->newQuarter = 1;
            }
        }
        if (!gameTime->held() && (animationsHeld == 0U) && (animationsStopped == 0U) && (worldFrozen == 0U)) {
            ticks++;
            if (frozen == 0U) {
                time = ticks;
            }
            rebuildRequested++;
        }
        break;
    }
}

void AnimationProcess::freeze()
{
    frozen = 1;
}

void AnimationProcess::thaw()
{
    time = ticks;
    frozen = 0;
}

void startGameTime()
{
    if (gameTime == nullptr) {
        gameTime = new GameTime;
    }
    if (animationProcess == nullptr) {
        animationProcess = new AnimationProcess;
        animationProcess->startTimer(1);
    }
}

/* The game's time: frames (1040:0308), seconds (0317), minutes (0331),
 * quarter hours (034b), and the setters (035a, 0377, 0397). Nothing calls
 * them. */
u32 gameFrames()
{
    return gameTime->frames;
}

s32 gameSeconds()
{
    return animationProcess->gameTicks / TICKS_PER_SECOND;
}

s32 gameMinutes()
{
    return animationProcess->gameTicks / TICKS_PER_MINUTE;
}

s32 gameQuarterHours()
{
    return animationProcess->gameQuarters;
}

void setGameSeconds(s32 seconds)
{
    animationProcess->setGameTicks(seconds * TICKS_PER_SECOND);
}

void setGameMinutes(s32 minutes)
{
    animationProcess->setGameTicks(minutes * TICKS_PER_MINUTE);
}

void setGameQuarterHours(i16 quarters)
{
    animationProcess->setGameTicks(quarters * TICKS_PER_QUARTER);
}

/* SystemTimer's ticks (1040:03b8, intrinsic 14B). */
u32 currentTimerTick()
{
    return systemTimer->ticks;
}
