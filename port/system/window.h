/* The port's window: SDL3 outside the port's PC (pc.h), which it leaves as
 * it is. A run has a window unless a script drives it (REGRET_SCRIPT,
 * script.h) or the build has none (PORT_WINDOW off: nowindow.cpp for
 * window.cpp); the tests run without one but window_test.
 *
 * A run with a window has two threads: the program's first keeps the window
 * (SDL's events, the drawing, the showing, message boxes), the game's runs
 * the game and its virtual time, the sound with it, and alone touches the
 * game's state. The window thread hands the game thread its input, which
 * the game takes at its frames; the game thread hands it a copy of the
 * screen at each frame. So the window's waits (a slow showing, the host
 * holding the window while it is moved) do not stop the game's time and
 * sound, as the original's sound card and timer did not stop for its
 * drawing. A run without a window (a script) has the one thread. */
#ifndef PORT_WINDOW_H
#define PORT_WINDOW_H
#include "common.h"

#include <bitset>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

/* The run: with a window, game(argc, argv) on the game thread while this
 * thread keeps the window until it returns; else game here. Its value. */
int windowMain(int (*game)(int, char **), int argc, char **argv);
/* Whether the run has a window, and so input that can come at any time. */
bool windowRun();
/* The wide screen's width for 480 rows in the shape of the widest display
 * (640 to WIDE_MOST, video.h; a window's is the shape of its picture); 0
 * without a display or a build without the window. A capture without a
 * window takes it (capture.h): it has no display of its own, and the
 * primary one need not be the one played on. */
u16 displayWideWidth();
/* A frame of the window, which comes every sixtieth of a second of the
 * virtual time (timer.cpp), on the game thread: the first opens the window;
 * each takes the input the window has had since the last (the keys to the
 * keyboard, the mouse to its driver, the window's closing to what the game
 * gave hostCloseAction) and hands the window the screen the mode shows
 * (video.h). False when the run has no window, and then no frame comes
 * again. */
bool windowFrame();
/* A frame of a watched replay not shown yet (replay.h): the first opens the
 * window; each takes the window's input (in a replay its close, its host
 * actions waiting) but hands it no picture. */
bool windowFrameUnshown();
/* The window's picture of the screen now, as a frame draws it: width x
 * height pixels of red, green and blue; false without a window. */
bool windowPicture(std::vector<u8> &rgb, int &width, int &height);
/* The sound's output (audio.cpp): bytes of stereo samples, 16-bit signed or
 * 8-bit unsigned, at rate, to SDL3's audio when the run has a window. The
 * stream is of 16-bit samples, the 8-bit ones made so: the depth may change
 * as it plays (asylum.h's ASS_SetDepth). The driver is the one
 * REGRET_AUDIO_DRIVER names (the tests' "dummy", where the sound goes
 * nowhere), else SDL's choice. The bytes may end inside a frame, whose rest
 * comes next. The stream keeps a lead ahead of the device and no more than
 * twice it, in the stream's bytes: what comes beyond that is dropped. */
void windowAudio(const u8 *bytes, u32 size, u32 rate, bool sixteen);
/* The window's audio as it stands: the bytes its stream holds that the
 * device has not taken, the lead, and the speed it plays them at (1 at the
 * output's rate); false before the first bytes open it. */
struct WindowAudio {
    u32 held, lead;
    float speed;
};
bool windowAudioState(WindowAudio &audioState);
/* A message box over the window, with the mouse freed, until the user
 * closes it (report.h); nothing without a window. */
void windowMessage(const char *text);
/* The same as a warning, which the run goes on after. */
void windowWarning(const char *text);
/* What the window's title says after the game's name (a watched replay's
 * progress before it is shown); "" for nothing. */
void windowTitleNote(const std::string &note);
/* Calls function on the window thread and waits for it: SDL's calls that
 * only the program's first thread may make. The window thread takes the
 * call after the SDL events pushed before it, so it also waits for them
 * (the tests push theirs). Here at once on the window thread, or when the
 * run has no window thread. */
void windowCall(const std::function<void()> &function);
/* The run's end with code: std::exit, which runs the exit's handlers on
 * this thread; on the window thread while the game thread runs, at once
 * (std::_Exit after the output is flushed), as the game's state is in use. */
[[noreturn]] void windowExit(int code);

/* The window's part of a rewind and of the state's save and load
 * (rewind.h), the game thread's. The PC's keys by their scan codes, the
 * E0h-prefixed from 128. */
using KeysHeld = std::bitset<256>;
/* The rewind's key held now; the state asked for since the last look (0
 * none, 1 its save, 2 its load). */
[[nodiscard]] bool windowRewinding();
[[nodiscard]] int windowStateAsked();
/* A watched replay asked to be taken over since the last look
 * (replay.h). */
[[nodiscard]] bool windowTakeOverAsked();
/* The fast forward's key held now (timer.cpp goes faster then). */
[[nodiscard]] bool windowFastForwarding();
/* The pause is on (rewind.cpp's hostFrame stands still then); a frame of
 * it: the screen shown with its state, the input left waiting; false when
 * the window was asked to close. */
[[nodiscard]] bool windowPaused();
bool windowPauseFrame();
/* The keys the game has been given down and not up. */
[[nodiscard]] KeysHeld windowKeysTaken();
/* A frame of a rewind: the screen shown and the sound kept up, the input
 * left waiting; false when the window was asked to close. */
bool windowRewindFrame();
/* The rewind's end, or a state loaded: the input that came during it
 * dropped (but the window's close), and keys given to the game to bring
 * the keys it has down, held, to those down now, and the mouse's buttons
 * down now. */
void windowRewindEnd(const KeysHeld &held);

/* The window's icon: the game's, GOG's goggame-<id>.ico in the game's folder,
 * read where it is (the port carries no picture of the game's). Its pure part:
 * the largest PNG picture of an icon file's bytes, as its place and size in
 * them; none when the file has none or is not one. */
struct IconPicture {
    std::size_t offset, size;
};
std::optional<IconPicture> largestIconPng(const std::vector<u8> &ico);
#endif
