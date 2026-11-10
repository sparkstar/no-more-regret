/* FADE.C (No Remorse's 1440:0000-0fba, whose addresses these are; No
 * Regret's 1418:0000-1043): palette fades. See fade.h. Both games' are No
 * Remorse's: a fade runs and a jump applies while
 * another fade runs (No Regret's original left a second fade inactive and
 * dropped the jumps while one ran, which the usecode and the scenarios never
 * do but for a jump after a fade whose steps are done and whose process is
 * not yet gone); a fade's end resumes the cycling only when there is a
 * cycling process (No Remorse's original did not check it). */
#include "fade.h"
#include "clock.h"
#include "cycle.h"
#include "gamepal.h"
#include "kernel.h"

void ResumeCycling::run()
{
    if ((cycleProcess != nullptr) && (cycleProcess->paused != 0U)) {
        cycleProcess->paused = 0;
    }
    terminate(0);
}

void FadeProcess::init(BasePalette *palette, u16 fadeRate, u8 resume, u16 steps, i16 stepSize)
{
    current = nullptr;
    target = nullptr;
    stepsLeft = steps;
    resumeCycling = resume;
    step = stepSize;
    /* No Regret's place, so that a fade at once has it too; nothing between
     * here and No Remorse's (after the palettes) touches the cycling. */
    if (cycleProcess != nullptr) {
        wasPaused = cycleProcess->paused;
    } else {
        wasPaused = 1;
    }
    if (fadeRate == 0U) {
        palette->apply();
        return;
    }
    /* The fade runs even while another does: no check for another
     * FadeProcess. */
    nameProcess(pid, "FadeProcess");
    setType(FADE_TYPE);
    setRate(fadeRate);
    pause();
    addEvents(EVENT_TIMER);
    flags |= PROCESS_ACTIVE;
    status &= ~STATUS_STARTED;
    if (palette == nullptr) {
        halt();
    }
    current = target = nullptr;
    current = new Palette;
    current->capture();
    target = new Palette(*palette);
    if ((wasPaused == 0U) && (cycleProcess != nullptr)) {
        cycleProcess->paused = 1;
    }
    startTimer(1);
}

FadeProcess::~FadeProcess()
{
    delete current;
    delete target;
}

void FadeProcess::onKill()
{
    if (current != nullptr) {
        delete current;
        current = nullptr;
    }
    if (target != nullptr) {
        delete target;
        target = nullptr;
    }
    if (((resumeCycling != 0U) || (wasPaused == 0U)) && (cycleProcess != nullptr)) {
        cycleProcess->paused = 0;
    }
}

void FadeProcess::event(u32 /*event*/, EventData /*data*/)
{
    u8 *from;
    u8 *to;
    u16 matched;
    u16 i;

    matched = 0;
    from = reinterpret_cast<u8 *>(current->data.get());
    to = reinterpret_cast<u8 *>(target->data.get());
    for (i = 0; i < static_cast<ui16>(current->count * 3); i++, from++, to++) {
        if (*from == *to) {
            matched++;
            continue;
        }
        if (static_cast<i16>(*from + step) < *to) {
            *from += step;
        } else if (static_cast<i16>(*from - step) > *to) {
            *from -= step;
        } else if (*from < *to || *from > *to) {
            *from = *to;
        }
    }
    stepsLeft--;
    if (static_cast<ui16>(current->count * 3) == matched || stepsLeft == 0) {
        eventMask &= ~EVENT_TIMER;
        flags &= ~PROCESS_ACTIVE;
    } else {
        current->apply();
    }
}

u16 fadeToBlack(u16 rate)
{
    Palette black(RGB(0, 0, 0));

    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    return (new FadeProcess(&black, rate, 0, 0, 1))->getPid();
}

namespace {
u16 fadeToGrey(u16 rate)
{
    Palette grey(RGB(0, 0, 0));
    i16 i;

    grey.capture();
    grey.toYiq();
    /* The luminance of each entry. */
    for (i = 0; i < 256; i++) {
        grey.data[i].blue = grey.data[i].green = grey.data[i].red;
    }
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    return (new FadeProcess(&grey, rate, 0, 0, 1))->getPid();
}
}

u16 fadeOut()
{
    return fadeToBlack(40);
}

u16 fadeToGamePalette(u16 rate)
{
    FadeProcess *fade = new FadeProcess(gamePalette, rate, 0, 0, 1);
    Process *resume = new ResumeCycling;

    fade->block(resume);
    return resume->pid;
}

/* fadeToGamePalette(40) */
/* UNCALLED: nothing calls it. */
u16 fadeUp()
{
    return fadeToGamePalette(40);
}

u16 fadeIn()
{
    return fadeToGamePalette(40);
}

/* The jumps do not wait for a running FadeProcess. */
void jumpToBlack()
{
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    Palette black(RGB(0, 0, 0));

    black.apply();
}

void jumpToWhite()
{
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    Palette white(RGB(63, 63, 63));

    white.apply();
}

void jumpToColor(i16 red, i16 green, i16 blue)
{
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    Palette palette(RGB(red, green, blue));

    palette.apply();
}

/* UNCALLED: nothing calls it. */
u16 fadeToColor(char red, char green, char blue, u16 rate)
{
    if (red < 0 || red > 63 || green < 0 || green > 63 || blue < 0 || blue > 63) {
        halt();
    }
    Palette palette(RGB(red, green, blue));

    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    return (new FadeProcess(&palette, rate, 0, 0, 1))->getPid();
}

void jumpToGamePalette()
{
    gamePalette->apply();
    if ((cycleProcess != nullptr) && (cycleProcess->paused != 0U)) {
        cycleProcess->paused = 0;
    }
}

u16 fadeTowardColor(i16 red, i16 green, i16 blue, u16 rate, u16 steps)
{
    if (red < 0 || red > 63 || green < 0 || green > 63 || blue < 0 || blue > 63) {
        halt();
    }
    Palette palette(RGB(red, green, blue));
    FadeProcess *fade = new FadeProcess(&palette, rate, 1, steps, 1);

    return fade->pid;
}

u16 fadeTowardGamePalette(u16 rate, u16 steps)
{
    FadeProcess *fade = new FadeProcess(gamePalette, rate, 1, steps, 1);

    return fade->pid;
}

u16 fadeTowardBlack(u16 rate, u16 steps)
{
    Palette black(RGB(0, 0, 0));
    FadeProcess *fade = new FadeProcess(&black, rate, 1, steps, 1);

    return fade->pid;
}

/* The wait spins until the timer's interrupt has run the fade to its end.
 * The port's interrupts come as time passes, so each turn of the spin lets
 * it pass (clockWait): a spin without it never ends, or an optimizer drops
 * it as a loop without effects and the game goes on at once. */
void fadeToPaletteAndWait(BasePalette *palette, u16 rate)
{
    FadeProcess *fade = new FadeProcess(palette, rate, 0, 0, 1);

    u32 events;

    do {
        events = fade->eventMask;
        if ((events & EVENT_TIMER) != 0U) {
            clockWait();
        }
    } while ((events & EVENT_TIMER) != 0U);
}

void openFadeToBlackFrame(u16 rate, std::unique_ptr<GameRest> rest)
{
    Palette black(RGB(0, 0, 0));
    FadeProcess *fade = new FadeProcess(&black, rate, 0, 0, 1);

    openWaitWhileFrame([fade]() { return (fade->eventMask & EVENT_TIMER) != 0U; }, std::move(rest));
}

void openFadeToGreyFrame(u16 rate, std::unique_ptr<GameRest> rest)
{
    EventProcess *fade = static_cast<EventProcess *>(findProcess(fadeToGrey(rate)));

    openWaitWhileFrame([fade]() { return (fade->eventMask & EVENT_TIMER) != 0U; }, std::move(rest));
}
