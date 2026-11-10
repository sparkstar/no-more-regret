/* The movies (the movies and subtitles): what the game's callers ask for
 * (MovieRequest) and the one entry they ask through (playMovie), and what
 * each game gives the player (MovieInput and MovieHost, both games' in
 * moviehost.cpp, the game's chosen by XAN_MOVIE_HOST). */
#ifndef PORT_MOVIE_H
#define PORT_MOVIE_H
#include "common.h"
#include "waits.h"

#include <string>

/* What is under a movie and around it while it plays. */
enum class MovieBackground {
    /* The screen cleared (No Regret's movies). */
    BLACK,
    /* The game's screen in fixed greys. */
    GREY,
    /* The same, the palette faded to grey first. */
    GREY_FADE,
    /* The game's screen and palette as they are (No Remorse's overlay on
     * the monitors). */
    KEEP,
};

/* A movie as a caller asks for it. */
struct MovieRequest {
    /* The caller's name of it; the game finds its files from it. */
    const char *name = nullptr;
    /* Centred on the screen, or its top left at x, y. */
    bool centred = true;
    i16 x = 0;
    i16 y = 0;
    MovieBackground background = MovieBackground::BLACK;
    /* Drawn 2 by 2 when the configuration asks for big movies. */
    bool mayDouble = true;
    /* With the subtitles when the configuration asks for them. */
    bool maySubtitle = true;
    /* The music paused while it plays. */
    bool holdMusic = false;
    /* The game's palette and screen back after it; else the screen stays
     * black (No Regret's intro and its usecode's choice). */
    bool restorePalette = true;
};

/* Plays the movie as asked and returns once it has ended or been
 * cancelled: whether a key cancelled it. */
bool playMovie(const MovieRequest &request);
/* playMovie as a wait: starts the movie and returns; then, a lambda on
 * whether a key stopped it, runs after its end. playMovie runs it where it
 * is (waits.h's blockWaits), for the callers that do not go on after it
 * as a wait. */
void openMovieFrame(const MovieRequest &request, std::unique_ptr<GameRest> rest);
template <typename Then> void openMovie(const MovieRequest &request, Then then)
{
    auto rest = [then = std::move(then)]() mutable {
        then(restResult() != 0);
        return false;
    };
    openMovieFrame(request, std::make_unique<CodeRest<decltype(rest)>>(std::move(rest)));
}

/* The keys around a movie and the one that cancels it: each game's. */
class MovieInput {
public:
    virtual ~MovieInput() = default;
    /* Before the movie: the keys pressed before it go. */
    virtual void begin() = 0;
    /* Whether the player asks for the movie to stop. */
    virtual bool skipRequested() = 0;
    /* After it: the keys pressed while it played go. */
    virtual void end() = 0;
};

/* The game's (moviehost.cpp). */
MovieInput &movieInput();

/* What the game's configuration says of the movies (its MovieHost's). */
struct MovieSettings {
    /* Each pixel 2 across (bigvideo). */
    bool big = false;
    /* On the 16-bit screen (No Regret's moviehicolor), which only Xan's
     * decoder draws. */
    bool hiColour = false;
    /* Every frame decoded as a late one (No Regret's lowresmovies). */
    bool lowResolution = false;
    /* The subtitles shown (subtitles). */
    bool subtitles = false;
    /* The sound effects started: the movie's sound plays. */
    bool sound = false;
};

/* Where a movie is on the screen and how it is drawn. */
struct MoviePlace {
    /* Its top left on the 640-pixel screen and its size in the file. */
    i16 x = 0;
    i16 y = 0;
    u16 width = 0;
    u16 height = 0;
    /* Each pixel 2 across (Xan's 2 down too; RRV's leaves every other
     * line). */
    bool doubled = false;
    /* On the 16-bit screen. */
    bool hiColour = false;
    MovieBackground background = MovieBackground::BLACK;

    /* The line under it. */
    [[nodiscard]] i16 bottom() const { return static_cast<i16>(y + (height * (doubled ? 2 : 1))); }
};

/* A movie's files. */
struct MovieFiles {
    /* The AVI file, as the game opens it (gamefile.h's paths). */
    std::string movie;
    /* Its subtitles' file, of TXT's form (No Remorse's) or IFF's. */
    std::string subtitles;
    bool textSubtitles = false;
};

/* What the player asks of the game: each game's (moviehost.cpp). */
class MovieHost {
public:
    virtual ~MovieHost() = default;
    /* The files of the caller's name; false when the movie is not there,
     * and nothing plays. */
    virtual bool locate(const char *name, MovieFiles &files) = 0;
    virtual MovieSettings settings() = 0;
    /* Before the movie: the game held and the screen made ready for it at
     * place, its background as the request asks. */
    virtual void begin(const MovieRequest &request, const MoviePlace &place) = 0;
    /* The movie's palette and sound set up, before its first frame. */
    virtual void ready(const MovieRequest &request) = 0;
    /* After it: the screen and the game back; cancelled when a key stopped
     * it. */
    virtual void end(const MovieRequest &request, const MoviePlace &place, bool cancelled) = 0;
};

MovieHost &movieHost();
/* No Regret's usecode's movie (intrinsic 6Dh, XANPLAY.C's playMovie,
 * 1448:0b5a): name as playMovie plays it; after it the game palette comes
 * back when restorePalette, else the screen stays black. */
void playMovie(const char *name, u8 restorePalette);
/* A key stopped the last movie: No Regret's host sets it, its vidtest
 * cheat's loop reads it (typeCheat). */
extern u8 &movieAborted;
/* No Regret's configuration (options.cpp): the movies drawn at twice their
 * size (2, bigvideo), every frame decoded as a late one (lowresmovies). */
extern i16 &doubleMovies;
extern i16 &movieFrameFlags;
#endif
