/* The host the game runs on: the text screen of the start and the halts,
 * why a run ends, the script's loads, saves and uses, the window's closing
 * and the debugger's break. The system layer gives it (port/system:
 * console.cpp, report.cpp, script.cpp, window.cpp, unimplemented.cpp,
 * interrupts.cpp). */
#ifndef PORT_PLATFORM_HOST_H
#define PORT_PLATFORM_HOST_H
#include <cstdint>

/* The text screen (Borland's conio: CRUSYS.C's title, the halts'
 * messages): on the standard output, a row at a time, without colours.
 * The mode is 80 columns of 25 rows, or 50 with the 8-line font (C4350). */
void hostClearText();
void hostTextAt(int x, int y);
[[gnu::format(printf, 1, 2)]] int hostPrintText(const char *format, ...);
void hostTextColor(int color);
void hostTextBackground(int color);
void hostTextMode(int mode);
/* The game's printf (the halts' and the debug messages, CRUSYS.C's and
 * the item cache's): the text on the standard output as C's printf writes
 * it, beside the text screen's rows. */
[[gnu::format(printf, 1, 2)]] int hostPrint(const char *format, ...);

/* Why the run ends, as the game's halts print it: in a run with a window
 * also in regret.log, and a message box that says the game stopped and
 * where the crash report is (report.h). */
void hostEndMessage(const char *text);
/* What the run passes over and goes on from (the sound's trouble, which
 * the original ended with): on the standard error, and in a run with a
 * window at the end of regret.log too (report.h). */
void hostNotice(const char *text);
/* Whether the run has the port's debug features: the rewind, the fast
 * forward, the states' save and load, the pause, a watched replay's
 * take-over, and the recording's and the replay's options. On with
 * --debug-feature or debug_feature=true in the game folder's enhanced.cfg,
 * as the run starts, for the whole run (commandline.h). */
bool hostDebugFeatures();
/* What the port does not take (a saved game's, an intrinsic without its
 * function): its name printed and the end of the run, the crash report
 * having it. */
[[noreturn]] void hostUnimplemented(const char *name);
[[noreturn]] void hostMissingIntrinsic(std::uint16_t number);
/* The run's end with code: the game's loop and the calls it is in unwound
 * to the system layer, which then has the game release what it holds
 * (gamefile.h's gameRelease) and returns code from main. Without the
 * system layer's run of the game (a test's own main), exit(code). */
[[noreturn]] void hostEnd(int code);
/* The run's end at once with code, as C's exit: the halts', after their
 * messages and the crash report, and the item cache's failed check.
 * Nothing unwinds, unlike hostEnd. */
[[noreturn]] void hostExit(int code);
/* The host's environment variable of the name (C's getenv), nullptr
 * without one: the original's CRUDEBUG, the one program's CRUSADER_GAME
 * (before main). */
const char *hostEnvironment(const char *name);
/* What closing the window does (and SIGINT and SIGTERM, which SDL turns
 * into the same), which the game gives: the run's end at once (system.cpp),
 * from any of its window frames. */
void hostCloseAction(void (*close)());
/* The window over the whole screen: the game's at its start
 * (enhanced.cfg), the window's after its host action's key or its host
 * made it so; the picture 4:3 as large as it fits, in the middle. Without
 * a window, kept for the game to ask. */
void hostSetFullscreen(bool on);
[[nodiscard]] bool hostFullscreen();
/* What a change of it the game did not make does, which the game gives
 * (enhanced.cfg written again): at the game's next frame. */
void hostFullscreenAction(void (*changed)());
/* Whether the game draws: not while a replay watched in a window goes as
 * fast as it can to the part it shows (replay.h's replayFast). The shape
 * drawers (drawframe.h) and the wide screen's sides leave their pixels
 * then; the game's state does not read them, and the replay goes on as
 * recorded. */
[[nodiscard]] bool hostDraws();
/* What the game does when it draws again, which the game gives: the whole
 * screen drawn anew (the camera's repaint), as the game draws again only
 * what changes. Between two of its steps. */
void hostRepaintAction(void (*repaint)());
/* INT 3: a stop in a debugger when one runs the game; nothing without. */
void hostDebugBreak();

/* The game's functions for the script's load, save and use of an item. */
void hostScriptActions(void (*load)(std::int16_t slot), void (*save)(std::int16_t slot),
                       void (*use)(std::int16_t item));
/* The game's test of whether the script's load and save can run at this
 * pass's end: not inside one of its calls (a modal gump, a delay, a usecode
 * thread inside an instruction: its waits), whose state a save cannot hold,
 * as the player's saves never are; else they wait for a pass's end where
 * they can. */
void hostScriptReady(bool (*ready)());
/* The game's map jump for the script's jump (No Remorse's: the teleport pad's
 * startMapJump, by which a mission ends in the base). */
void hostScriptJump(void (*jump)(std::int16_t map));
/* The game's movie for the script's movie (the movies and subtitles): name
 * played as the game's callers play it in form (each game has its forms), at
 * x, y where the form takes a place. */
void hostScriptMovie(void (*play)(const char *name, std::int16_t form, std::int16_t x, std::int16_t y));
/* A movie's frame is finished on the screen, its subtitle with it; frame is
 * its place in the order the movie shows them, from 0. With REGRET_FRAMES
 * set to a file, the frame and a hash of the screen as the mode shows it go
 * there (the movies' baseline of frames, port/test/scenario/movieframes.py). */
void hostMovieFrame(std::uint32_t frame);
#endif
