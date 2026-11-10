/* FADE.C: palette fades. A FadeProcess moves each colour of a copy of
 * the hardware palette toward a target palette by step on each tick of its
 * rate, and stops when all of them arrived or after steps ticks. Palette
 * cycling (CycleProcess) is paused while a fade runs. */
#ifndef PORT_FADE_H
#define PORT_FADE_H
#include "common.h"
#include "palette.h"
#include "kprocess.h"
#include "waits.h"

/* FadeProcess's process type. */
constexpr int FADE_TYPE = 0x206;

class FadeProcess : public PeriodicProcess {
public:
    explicit FadeProcess(Loading loading) : PeriodicProcess(loading) {}
    u8 wasPaused;     /* the palette cycling was paused before */
    u8 resumeCycling; /* resume the cycling at the end */
    u16 stepsLeft;    /* 0: until the target is reached */
    i16 step;
    Palette *current;
    Palette *target;

    /* rate 0 applies the palette at once. */
    FadeProcess(BasePalette *palette, u16 fadeRate, u8 resume, u16 steps, i16 stepSize)
    {
        init(palette, fadeRate, resume, steps, stepSize);
    }
    void init(BasePalette *palette, u16 fadeRate, u8 resume, u16 steps, i16 stepSize);
    void onKill() override; /* the original's destructor, GAMEPAL.C keeps the copy. */
    /* The port's: the palettes of a fade the kernel frees without a kill (at
     * its end, at a load), which the original's free left. */
    ~FadeProcess() override;
    void run() override { terminate(0); }
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(FadeProcess, 0x4e);

/* The fades return the pid of a process to wait on. The rate is in ticks of
 * the kernel's timer. */
u16 fadeToBlack(u16 rate);
u16 fadeOut(); /* fadeToBlack(40) */
/* Fades to the game palette and then resumes the palette cycling. */
u16 fadeToGamePalette(u16 rate);
u16 fadeIn(); /* fadeToGamePalette(40) */
/* Set the palette at once, unless a fade runs. */
void jumpToBlack();
void jumpToWhite();
void jumpToColor(i16 red, i16 green, i16 blue);
void jumpToGamePalette();
/* Fade toward a palette for steps ticks, then resume the palette cycling. */
u16 fadeTowardColor(i16 red, i16 green, i16 blue, u16 rate, u16 steps);
u16 fadeTowardGamePalette(u16 rate, u16 steps);
u16 fadeTowardBlack(u16 rate, u16 steps);
/* Fade and wait until the fade is over (the system's shutdown). */
void fadeToPaletteAndWait(BasePalette *palette, u16 rate);
/* Declared in fade.cpp in the original; here so that the saved games'
 * schema reaches it. */
/* Waits on a fade (Process::block) and then resumes the palette cycling. */
class ResumeCycling : public Process {
public:
    /* UNCALLED: nothing calls it. */
    explicit ResumeCycling(Loading loading) : Process(loading) {}
    ResumeCycling() = default;
    void run() override;
};

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
u16 fadeUp();
/* UNCALLED: nothing calls it. */
u16 fadeToColor(char red, char green, char blue, u16 rate);
/* The original's fadeToBlackAndWait and fadeToGreyAndWait (No Remorse's
 * PLAYER.C before a movie) as waits: start the fade and return; code (a
 * lambda as waits.h's giveRest's) runs when it is over. */
void openFadeToBlackFrame(u16 rate, std::unique_ptr<GameRest> rest);
void openFadeToGreyFrame(u16 rate, std::unique_ptr<GameRest> rest);
template <typename Code> void openFadeToBlack(u16 rate, Code code)
{
    openFadeToBlackFrame(rate, std::make_unique<CodeRest<Code>>(std::move(code)));
}
template <typename Code> void openFadeToGrey(u16 rate, Code code)
{
    openFadeToGreyFrame(rate, std::make_unique<CodeRest<Code>>(std::move(code)));
}
#endif
