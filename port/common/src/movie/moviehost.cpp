/* The movies' hosts and keys (common/src/movie/movie.h): No Regret's
 * XANPLAY.C's (1448) and No Remorse's PLAYER.C's (1468) names of the files,
 * configuration, keys and what they did around a movie, the game's chosen by
 * XAN_MOVIE_HOST (movie_game.h). */
#include <cstring>
#include "gamestate.h"
#include <cctype>
#include "movie.h"
#include "movie_game.h"
#include "cafont.h"
#include "camera.h"
#include "cycle.h"
#include "dsfxman.h"
#include "fade.h"
#include "gamefile.h"
#include "gamepal.h"
#include "host.h"
#include "kernel.h"
#include "mousectl.h"
#include "options.h"
#include "spmgr.h"
#include "system.h"
#include "systimer.h"
#include "u9mouse.h"
#include "utility.h"
#include "viewport.h"
#include "widesides.h"
#include "world.h"
#include "dispatch.h"
#include "flicplay.h"
#include "gametime.h"
#include "gray.h"
#include "gumpshap.h"
#include "music.h"
#include "palette.h"
#include "screen.h"

#include <memory>

namespace {

/* Any key cancels a movie (XANPLAY.C's showFrame); the keys before and
 * after it go (its playXan). */
class RegretMovieInput : public MovieInput {
public:
    void begin() override { flushKeys(); }
    bool skipRequested() override { return keysQueued(); }
    void end() override { flushKeys(); }
};

/* Draws a test text and clears the top 50 lines (XANPLAY.C's). */
void fontTest()
{
    CAFont font(3);

    font.draw(16, 16, "Cru font test.");
    drawViewport->clear(0, 0, 0, SCREEN_RIGHT, 49);
}

/* XANPLAY.C's playMovie around its playXan: the movie centred on a black
 * screen (No Regret's callers ask for no other background), the world
 * frozen, the music on. */
class RegretMovieHost : public MovieHost {
public:
    /* <drive><dir>\name.AVI and its subtitles' name.IFF, a name of two
     * characters with a 0 before it. */
    bool locate(const char *name, MovieFiles &files) override
    {
        char path[102];

        strcpy(path, gameDrive);
        strcat(path, flicPath);
        strcat(path, "\\");
        if (strlen(name) < 3) {
            strcat(path, "0");
        }
        strcat(path, name);
        files.movie = std::string(path) + ".AVI";
        files.subtitles = std::string(path) + ".IFF";
        files.textSubtitles = false;
        return fileExists(files.movie.c_str()) != 0;
    }

    MovieSettings settings() override
    {
        MovieSettings settings;
        settings.big = doubleMovies != 0;
        settings.hiColour = hiColorMovies != 0;
        settings.lowResolution = movieFrameFlags != 0;
        settings.subtitles = movieSubtitles != 0;
        settings.sound = effectsStarted != 0;
        return settings;
    }

    void begin(const MovieRequest & /*request*/, const MoviePlace &place) override
    {
        /* Waits for a fade to end (a wait of the steps, the rest after
         * it). */
        Process *fade = firstProcess(0, FADE_TYPE);
        openWaitWhile([fade]() { return (fade != nullptr) && (fade->hasFlags(PROCESS_ACTIVE) != 0U); },
                      [&place]() {
                          begun(place);
                          return false;
                      });
    }

    void ready(const MovieRequest & /*request*/) override
    {
        openTimerDelay(15, []() { return false; });
    }

    void end(const MovieRequest &request, const MoviePlace &place, bool cancelled) override
    {
        movieAborted = cancelled ? 1 : 0;
        if (place.hiColour) {
            currentSystem->setMode(VESA_640X480);
        } else {
            drawViewport->clear(0);
            screenViewport->blit(drawViewport);
        }
        if (request.restorePalette) {
            gamePalette->apply();
            gameCamera->repaint();
        } else {
            jumpToBlack();
        }
        gameMouse->eventMask |= EVENT_MOUSE;
        thawWorld(1);
        redrawSprites();
        mouseCtl->hide();
        cycleProcess->paused = 0;
    }

private:
    /* begin's after the fade. */
    static void begun(const MoviePlace &place)
    {
        gameMouse->eventMask &= ~EVENT_MOUSE;
        freezeWorld(0);
        /* The port's wide screen: the movie on black, its sides black too
         * (widesides.h), drawn again when the world goes on. */
        coverWideSides();
        cycleProcess->paused = 1;
        fontTest();
        if (place.hiColour) {
            currentSystem->setMode(VESA_640X480_64K);
        } else {
            drawViewport->clear(0);
            screenViewport->blit(drawViewport);
        }
        movieAborted = 0;
    }
};

/* Escape, Space and Enter cancel a movie while one is down (PLAYER.C's
 * event and refill); the keys and the gumps' keys go before and after it
 * (its VideoPlayer and release). */
class RemorseMovieInput : public MovieInput {
public:
    void begin() override { forget(); }
    bool skipRequested() override
    {
        return (keyRepeats(1, 0) != 0U) || (keyRepeats(0x39, 0) != 0U) || (keyRepeats(0x1c, 0) != 0U);
    }
    void end() override { forget(); }

private:
    static void forget()
    {
        flushKeys();
        gumpKeys.clear();
    }
};

/* PLAYER.C's VideoPlayer around a movie: the screen greyed (with a fade
 * first, GreyFade) or kept as it is (Keep, the overlay), the frame of a
 * small movie drawn, the cycling of the palette held and the music too when
 * the request holds it. */
class RemorseMovieHost : public MovieHost {
public:
    /* VideoPlayer::open's names: the subtitles of MVA1 are MVA01.TXT, of 1A
     * 01A.TXT, and the movie the same name's .AVI, in <drive><dir>. The
     * original reads Borland's character types (_ctype, far, 1468:3004); the
     * port calls the C library's isdigit. */
    bool locate(const char *name, MovieFiles &files) override
    {
        char subtitlePath[80];
        char moviePath[80];
        char upper[80];

        memset(subtitlePath, 0, 80);
        memset(moviePath, 0, 80);
        memset(upper, 0, 80);
        memcpy(upper, name, strlen(name));
        upperCase(upper);
        if (upper[0] == 'M' && upper[1] == 'V' && upper[2] == 'A') {
            if (isdigit(static_cast<u8>(upper[4])) != 0) {
                strcat(subtitlePath, upper);
            } else {
                strcat(subtitlePath, "MVA0");
                strcat(subtitlePath, upper + 3);
            }
        } else if ((isdigit(static_cast<u8>(upper[0])) != 0) && (isdigit(static_cast<u8>(upper[1])) == 0)) {
            strcat(subtitlePath, "0");
            memcpy(subtitlePath + strlen(subtitlePath), upper, strlen(upper));
        } else {
            strcat(subtitlePath, upper);
        }
        strcat(subtitlePath, ".TXT");
        strcat(moviePath, gameDrive);
        strcat(moviePath, flicPath);
        strcat(moviePath, "\\");
        strcat(moviePath, subtitlePath);
        files.subtitles = moviePath;
        files.textSubtitles = true;
        moviePath[static_cast<ui16>(strlen(moviePath) - 3)] = 'A';
        moviePath[static_cast<ui16>(strlen(moviePath) - 2)] = 'V';
        moviePath[static_cast<ui16>(strlen(moviePath) - 1)] = 'I';
        files.movie = moviePath;
        return fileExists(moviePath) != 0;
    }

    MovieSettings settings() override
    {
        MovieSettings settings;
        settings.big = flicVideoMode == MODE_13H;
        settings.subtitles = movieSubtitles != 0;
        settings.sound = effectsStarted != 0;
        return settings;
    }

    /* VideoPlayer::prepareDisplay. */
    void begin(const MovieRequest &request, const MoviePlace &place) override
    {
        const bool preserve = request.background == MovieBackground::KEEP;

        musicWasOn = musicEnabled;
        if ((mouseShown = mouseCtl->visible) != 0) {
            mouseCtl->hide();
        }
        cycleProcess->paused = 1;
        if (animationProcess != nullptr) {
            savedAnimations = animationsHeld;
            animationsHeld = 1;
        }
        if (gameCamera != nullptr) {
            gameCamera->flags |= PROCESS_ACTIVE;
        }
        savedPalette = std::make_unique<BasePalette>(0, 256);
        if (savedPalette == nullptr) {
            allocationFailure();
        }
        savedPalette->capture();
        if (!preserve && request.background == MovieBackground::GREY_FADE) {
            /* The rest after the fade (a wait of the steps). */
            openFadeToGrey(40, [this, &place, preserve]() {
                prepared(place, preserve);
                return false;
            });
            return;
        }
        prepared(place, preserve);
    }

    /* The end of VideoPlayer::init. */
    void ready(const MovieRequest &request) override
    {
        stopAllEffects();
        if (request.holdMusic && (musicWasOn != 0U)) {
            pauseMusic();
        }
    }

    /* VideoPlayer::release's and restoreDisplay's. The palette's cycling
     * starts again after the overlay always, after the others unless the
     * usecode suppressed it. */
    void end(const MovieRequest &request, const MoviePlace & /*place*/, bool /*cancelled*/) override
    {
        const bool preserve = request.background == MovieBackground::KEEP;

        if (request.holdMusic && (musicWasOn != 0U)) {
            resumeMusic();
        }
        if (!preserve) {
            drawViewport->clear(0);
            screenViewport->blit(drawViewport);
        }
        /* The port's wide screen: the sides drawn again with the world. */
        ungreyWideSides();
        if (gameCamera != nullptr) {
            gameCamera->flags &= ~PROCESS_ACTIVE;
            gameCamera->repaint();
        }
        if (!preserve && (savedPalette != nullptr)) {
            savedPalette->apply();
        }
        screenViewport->blit(drawViewport);
        if (animationProcess != nullptr) {
            animationsHeld = savedAnimations;
        }
        if (preserve || (movieRestoreSuppressed == 0U)) {
            cycleProcess->paused = 0;
        }
        if (mouseShown != 0U) {
            mouseCtl->show();
        }
        savedPalette.reset();
    }

private:
    /* begin's after the fade: the frame and the screen greyed. */
    void prepared(const MoviePlace &place, bool preserve)
    {
        drawFrame(place);
        if (!preserve) {
            toFixedGray(drawViewport, savedPalette.get());
            /* The port's wide screen: the world beside it greyed the same
             * (widesides.h). */
            greyWideSides(savedPalette.get());
        }
        screenViewport->blit(drawViewport);
    }

    u8 musicWasOn = 0;
    u16 mouseShown = 0;
    u16 savedAnimations = 0;
    /* Its own: a run that ends in the movie leaves it to the host's end. */
    std::unique_ptr<BasePalette> savedPalette;

    /* VideoPlayer::drawFrame: a small movie's frame (the gump shapes 33h-35h
     * by its size), or a doubled movie's place cleared. */
    static void drawFrame(const MoviePlace &place)
    {
        if (!place.doubled) {
            if (place.height <= 0x81) {
                if (place.width <= 0xa1) {
                    const GumpShape frame(0x34);

                    frame.draw(place.x + 2, place.y - 1);
                } else {
                    const GumpShape frame(0x35);

                    frame.draw(place.x + 2, place.y - 1);
                }
            } else if (place.height <= 0xb5) {
                const GumpShape frame(0x33);

                frame.draw(place.x + 2, place.y - 1);
            }
        } else {
            drawViewport->clear(0, place.x, place.y, place.x + (static_cast<i16>(place.width) * 2),
                                place.y + (static_cast<i16>(place.height) * 2));
        }
    }
};

/* The hosts, made as the program starts: made at their first movie (a
 * function's statics), a memory snapshot from before it put back would have
 * them made again, their ends twice at the exit. */
RegretMovieInput &regretInput = gameGlobal<RegretMovieInput>();
RemorseMovieInput &remorseInput = gameGlobal<RemorseMovieInput>();
RegretMovieHost &regretHost = gameGlobal<RegretMovieHost>();
RemorseMovieHost &remorseHost = gameGlobal<RemorseMovieHost>();

}

MovieInput &movieInput()
{
    if (XAN_MOVIE_HOST) {
        return regretInput;
    }
    return remorseInput;
}

MovieHost &movieHost()
{
    if (XAN_MOVIE_HOST) {
        return regretHost;
    }
    return remorseHost;
}

void playMovie(const char *name, u8 restorePalette)
{
    /* The intrinsic's result 0 after the movie. */
    openMovie(MovieRequest{.name = name, .restorePalette = restorePalette != 0},
              [](bool /*cancelled*/) { setRestResult(0); });
}

namespace {
/* The script's movie (host.h's hostScriptMovie) as the game's callers ask for
 * it. No Regret's: 0 the usecode's, 1 the usecode's with the game's palette
 * after it (its restorePalette), 2 the intro's ORIGIN, never doubled. No
 * Remorse's: 0 the usecode's cutscene and the credits (centred on the screen
 * greyed with a fade), 1 the usecode's other cutscene and the intro (on the
 * greyed screen), 2 the usecode's overlay (Item::playMovieOverlay), 3 the
 * weasel's shop at x, y. */
void scriptPlayMovie(const char *name, i16 form, i16 x, i16 y)
{
    if (XAN_MOVIE_HOST) {
        playMovie(MovieRequest{.name = name, .mayDouble = form != 2, .restorePalette = form == 1});
        return;
    }
    switch (form) {
    case 1:
        playMovie(MovieRequest{.name = name, .background = MovieBackground::GREY, .holdMusic = true});
        break;
    case 2:
        flicGreyScreen = 0;
        playMovie(
            MovieRequest{.name = name, .background = MovieBackground::KEEP, .mayDouble = false, .maySubtitle = false});
        break;
    case 3:
        playMovie(MovieRequest{
            .name = name, .centred = false, .x = x, .y = y, .background = MovieBackground::GREY, .holdMusic = true});
        break;
    default:
        playMovie(MovieRequest{.name = name, .background = MovieBackground::GREY_FADE, .holdMusic = true});
        break;
    }
}

const bool SCRIPT_MOVIE = (hostScriptMovie(scriptPlayMovie), true);
}
