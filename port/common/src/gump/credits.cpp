/* Code123 (No Regret's 13d0:0000-08ac, No Remorse's 13f0:0000-08f5; file name
 * unknown). See credits.h. The menu's music pauses for the credits and goes on
 * after them, in both games (No Regret's original paused
 * it again at the end, No Remorse's stopped it); the movie after the credits
 * seen to the end with the cheats on is the game's (credits_game.h).
 *
 * The script: '@' and '$' start a line of the title and the text font, '!'
 * is followed by a word, the y of the next line, '^' fades in, '&' ends a page
 * and fades out, '*' ends a page and '#' the credits. Any other character is
 * skipped. A page is drawn one line a tick after a wait of 64 ticks. */
#include "credits.h"
#include "gamestate.h"
#include "credits_game.h"
#include "movie.h"
#include "bmp.h"
#include "cafont.h"
#include "crusys.h"
#include "cycle.h"
#include "dispatch.h"
#include "screen.h"
#include "fade.h"
#include "file.h"
#include "filespec.h"
#include "gamefile.h"
#include "kernel.h"
#include "mouse.h"
#include "music.h"
#include "palette.h"
#include "spmgr.h"
#include "systimer.h"
#include "u9mouse.h"
#include "viewport.h"
#include "input.h"

namespace {
u32 &creditsWait = gameGlobal<u32>(0); /* ticks before the next page */
u8 &creditsDone = gameGlobal<u8>(0);
i16 &creditsPos = gameGlobal<i16>(0);
/* The credits' text while they run. */
std::unique_ptr<char[]> &creditsText = gameGlobal<std::unique_ptr<char[]>>();
/* Set by '&': the page is redrawn until the fade out ends, then it fades in
 * again. */
u8 &creditsFaded = gameGlobal<u8>(0);
u32 &lastTick = gameGlobal<u32>(-1);
u8 &fading = gameGlobal<u8>(0);
i16 &fadeMode = gameGlobal<i16>(0);    /* 1 in, 2 out */
char &fadeLevel = gameGlobal<char>(0); /* subtracted from each component */
RGB *&fadeColors = gameGlobal<RGB *>(nullptr);
i16 &fadeFirst = gameGlobal<i16>(0);
i16 &fadeCount = gameGlobal<i16>(0);
i16 &fadeRate = gameGlobal<i16>(0);
i16 &creditsY = gameGlobal<i16>(80); /* of the next line */

Viewport *&backdrop = gameGlobal<Viewport *>();
GumpFont *&titleFont = gameGlobal<GumpFont *>();
GumpFont *&textFont = gameGlobal<GumpFont *>();
Palette &creditsPalette = gameGlobal<Palette>();

void startFade(BasePalette *palette, i16 first, i16 count, i16 rate, i16 mode);
void scrollCredits();
void stepFade();
void fadeInStep();
void fadeOutStep();

void fadeCreditsOut()
{
    startFade(&creditsPalette, 0, 0x80, 4, 2);
}

void fadeCreditsIn()
{
    startFade(&creditsPalette, 0, 0x80, 4, 1);
}

/* Waits for the start of the next vertical retrace (3DAh's bit 3). */
void waitRetrace()
{
    screenWaitRetrace();
}

void startCredits()
{
    pauseMusic();
    backdrop = gameBackground;
    jumpToBlack();
    titleFont = new CAFont(0x10);
    textFont = new CAFont(0x11);
    titleFont->draw(10, 10, " ");
    textFont->draw(10, 10, " ");
    gameMouse->hide();
    inputMouseStop();
    char *drive = gameDrive;
    creditsPalette.load(makePath(drive, "static", "cred.pal", nullptr), 0);
    BmpFile bmp;
    bmp.open(makePath(drive, "static", "cred.dat", nullptr));
    bmp.draw(backdrop);
    bmp.close();
    drawViewport->blit(backdrop);
    screenViewport->blit(drawViewport);
    waitRetrace();
    creditsPalette.apply();
    File file(makePath(drive, "static", "credits.dat", nullptr), 0, 0);
    creditsText = std::make_unique_for_overwrite<char[]>(file.size());
    file.read(creditsText.get(), file.size());
    file.close();
    pushSong(0x11);
    creditsPos = 0;
    creditsWait = 0;
    creditsDone = 0;
    fading = 0;
    creditsFaded = 0;
    creditsY = 80;
    lastTick = -1;
}

void creditsClosed();

/* The credits' end; No Remorse's movie a wait of the steps. */
void endCredits()
{
    if (titleFont != nullptr) {
        delete titleFont;
        titleFont = nullptr;
    }
    if (textFont != nullptr) {
        delete textFont;
        textFont = nullptr;
    }
    creditsText.reset();
    resumeMusic();
    jumpToBlack();
    /* Seen to the end with the cheats on: the game's movie too (No Remorse's
     * z03). */
    if ((CREDITS_CHEAT_MOVIE != nullptr) && (cheatsEnabled != 0U) && (creditsDone != 0U)) {
        drawViewport->clear(0);
        openMovie(
            MovieRequest{.name = CREDITS_CHEAT_MOVIE, .background = MovieBackground::GREY_FADE, .holdMusic = true},
            [](bool /*cancelled*/) {
                cycleProcess->paused = 1;
                jumpToBlack();
                creditsClosed();
            });
        return;
    }
    creditsClosed();
}

/* endCredits' after the movie: the game's screen, mouse and keys back. */
void creditsClosed()
{
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    redrawSprites();
    inputMouseStart(mouseCallback);
    inputMouseXRange(0, 0x4fe);
    inputMouseYRange(0, 0x1df);
    gameMouse->show();
    popSong();
    flushKeys();
    gumpKeys.clear();
}
}

void openCreditsFrame(std::unique_ptr<GameRest> rest)
{
    startCredits();
    openLoop([]() { return (creditsDone == 0U) && (pendingKeys == 0U); },
             []() {
                 scrollCredits();
                 waitRetrace();
                 stepFade();
             },
             [rest = std::move(rest)]() mutable {
                 endCredits();
                 afterWait(std::move(rest));
                 return false;
             });
}

namespace {
void scrollCredits()
{
    i16 x;
    i16 y;
    i16 height;
    i16 right;
    i16 bottom;

    if (fading != 0U) {
        return;
    }
    if (creditsFaded != 0U) {
        drawViewport->blit(backdrop);
        screenViewport->blit(drawViewport);
    }
    if (systemTimer->ticks == lastTick) {
        return;
    }
    lastTick = systemTimer->ticks;
    if (creditsWait != 0U) {
        creditsWait--;
        return;
    }
    y = creditsY;
    creditsWait = 0x40;
    while (creditsText[creditsPos] != '*' && creditsText[creditsPos] != '&') {
        switch (creditsText[creditsPos]) {
        case '^':
            fadeCreditsIn();
            creditsPos++;
            break;
        case '!':
            creditsPos++;
            y = *reinterpret_cast<i16 *>(creditsText.get() + creditsPos);
            creditsPos += 2;
            break;
        case '@':
            creditsPos++;
            x = static_cast<i16>(SCREEN_WIDTH - titleFont->textWidth(creditsText.get() + creditsPos)) >> 1;
            right = x + titleFont->textWidth(creditsText.get() + creditsPos);
            height = titleFont->textHeight(creditsText.get() + creditsPos);
            bottom = y + height;
            titleFont->draw(x, y, "%s", creditsText.get() + creditsPos);
            screenViewport->blit(drawViewport, x, y, right, bottom, x, y);
            y += height + 4;
            while (creditsText[creditsPos++] != 0) {
                ;
            }
            break;
        case '$':
            creditsPos++;
            x = static_cast<i16>(SCREEN_WIDTH - textFont->textWidth(creditsText.get() + creditsPos)) >> 1;
            right = x + textFont->textWidth(creditsText.get() + creditsPos);
            height = textFont->textHeight(creditsText.get() + creditsPos);
            bottom = y + height;
            textFont->draw(x, y, "%s", creditsText.get() + creditsPos);
            screenViewport->blit(drawViewport, x, y, right, bottom, x, y);
            y += height + 4;
            while (creditsText[creditsPos++] != 0) {
                ;
            }
            break;
        case '#':
            creditsDone = 1;
            return;
        default:
            creditsPos++;
        }
    }
    if (creditsText[creditsPos] == '&') {
        y = 20;
        fadeCreditsOut();
        creditsFaded = 1;
        creditsWait = 0;
    }
    if ((creditsFaded != 0U) && (fading == 0U)) {
        fadeCreditsIn();
        creditsFaded = 0;
    }
    creditsY = y;
    creditsPos++;
}

void stepFade()
{
    if (fading == 0U) {
        return;
    }
    switch (fadeMode) {
    case 1:
        fadeInStep();
        break;
    case 2:
        fadeOutStep();
        break;
    }
}

/* Fades count entries of palette from first, rate levels a retrace: mode 1
 * in from black, 2 out to black. */
void startFade(BasePalette *palette, i16 first, i16 count, i16 rate, i16 mode)
{
    RGB *colors = palette->data.get();

    if (mode == 1) {
        fadeLevel = 0x3f;
    } else {
        fadeLevel = 0;
    }
    fadeColors = colors + first;
    fadeFirst = first;
    fadeCount = count;
    fadeRate = rate;
    fading = 1;
    fadeMode = mode;
    waitRetrace();
    stepFade();
}

/* Writes the colors less fadeLevel to the DAC, then lowers the level. */
void fadeInStep()
{
    RGB *colors = fadeColors;
    u8 level;

    if (fadeLevel < 0) {
        fadeLevel = 0;
    }
    level = fadeLevel;
    {
        u16 count = fadeCount;
        u8 index = fadeFirst;
        u8 *channel = reinterpret_cast<u8 *>(colors);

        do {
            u8 rgb[3];

            for (i16 i = 0; i < 3; i++, channel++) {
                rgb[i] = *channel >= level ? *channel - level : 0;
            }
            screenSetColors(index++, 1, rgb);
        } while (--count != 0U);
    }
    if (fadeLevel == 0) {
        fading = 0;
    } else {
        fadeLevel -= fadeRate;
    }
}

/* The same, raising the level to 3Fh. */
void fadeOutStep()
{
    RGB *colors = fadeColors;
    u8 level;

    if (fadeLevel >= 0x3f) {
        fadeLevel = 0x3f;
    }
    level = fadeLevel;
    {
        u16 count = fadeCount;
        u8 index = fadeFirst;
        u8 *channel = reinterpret_cast<u8 *>(colors);

        do {
            u8 rgb[3];

            for (i16 i = 0; i < 3; i++, channel++) {
                rgb[i] = *channel >= level ? *channel - level : 0;
            }
            screenSetColors(index++, 1, rgb);
        } while (--count != 0U);
    }
    if (fadeLevel == 0x3f) {
        fading = 0;
    } else {
        fadeLevel += fadeRate;
    }
}
}
