/* Code9 (file name unknown): the game's clock and the process that times
 * the item animations. */
#ifndef PORT_GAMETIME_H
#define PORT_GAMETIME_H
#include "common.h"
#include "kprocess.h"

/* AnimationProcess::gameTicks runs at 30 a second; gameQuarters counts its
 * quarter hours. */
constexpr int TICKS_PER_SECOND = 30;
constexpr int TICKS_PER_MINUTE = 1800;
constexpr s32 TICKS_PER_QUARTER = (static_cast<s32>(27000L));

/* The process types. */
constexpr int ANIMATION_TYPE = 0x230;
constexpr int GAME_TIME_TYPE = 0x245;

class GameTime;
class AnimationProcess;
extern GameTime *&gameTime;
/* No Remorse's PLAYER.C reads it far. */
extern AnimationProcess *&animationProcess;
/* The animations are held while a full-screen gump or movie runs. */
extern u8 &animationsHeld;
/* The player's ANIMATIONS OFF (crusader.cfg's animation=1, the options): a
 * setting outside the arena, which a snapshot put back keeps. The
 * originals keep it in animationsHeld, which the holds above save and give
 * back; the port keeps them apart, so that a rewind into a hold holds the
 * animations and gives back the hold, not the choice. The animations run
 * when neither is set. */
extern u8 &animationsStopped;

/* The process that counts the game's frames. */
class GameTime : public Process {
public:
    explicit GameTime(Loading loading) : Process(loading) {}
    u32 frames;
    /* Frames to hold the count; -1 holds it until cleared. */
    u16 hold;
    /* Set when AnimationProcess starts a new quarter hour. */
    u8 newQuarter;

    GameTime();
    [[nodiscard]] bool held() const { return hold != 0; }
    void run() override;
    void afterLoad() override { gameTime = this; }
};
ASSERT_SIZE(GameTime, 0x39);

/* The process that times the item animations and the game's time. While it
 * is frozen, time stays and ticks go on. */
class AnimationProcess : public PeriodicProcess {
public:
    explicit AnimationProcess(Loading loading) : PeriodicProcess(loading) {}
    u32 ticks;
    u32 time;
    s32 gameTicks;
    s32 gameQuarters;
    /* The next gameTicks the setters asked for; -1 for none. */
    s32 newTicks;
    u8 frozen;

    AnimationProcess();
    void freeze();
    void thaw();
    void setGameTicks(s32 wantedTicks) { newTicks = wantedTicks; }
    void run() override {}
    void event(u32 event, EventData data) override;
    void afterLoad() override { animationProcess = this; }
};
ASSERT_SIZE(AnimationProcess, 0x55);

/* Makes both processes when they are not there. */
void startGameTime();

u32 currentTimerTick();

/* Nothing calls these, in the original either. */
u32 gameFrames();
s32 gameSeconds();
s32 gameMinutes();
s32 gameQuarterHours();
void setGameSeconds(s32 seconds);
void setGameMinutes(s32 minutes);
void setGameQuarterHours(i16 quarters);
#endif
