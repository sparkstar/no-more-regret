/* The script's input of the port's PC: the keyboard's scan codes and the
 * mouse's events at their times, which come as the virtual time reaches them
 * (pc.h's takeInput), and the script's other events: the screen written as a
 * PNG and the end of the run. (The game's input is the platform's input.h.) */
#ifndef PORT_SCRIPT_H
#define PORT_SCRIPT_H
#include "common.h"
#include "input.h"

#include <cstdint>
#include <string>
#include <vector>

struct InputEvent {
    /* In ticks of the interval timer's clock (pc.h's PIT_HZ). */
    std::uint64_t time;
    enum Kind { SCAN, MOUSE, SCREEN, QUIT, LOAD, SAVE, USE, JUMP, MOVIE, DEVICE } kind;
    u8 scan;
    /* The mouse's place in the driver's coordinates and its buttons; MOVIE's
     * place. */
    i16 x, y;
    u16 buttons;
    /* SCREEN's PNG, MOVIE's name. */
    std::string path;
    /* LOAD's and SAVE's slot, USE's item, JUMP's map, MOVIE's form. */
    i16 slot;
    /* DEVICE's event of the other devices (input.h). */
    DeviceEvent device;
};

/* Adds an event; events come in the order of their times, and in the
 * order they were added at one time. */
void addInput(const InputEvent &event);
/* Reads a script of events into the input, a line each, the time first in
 * sixtieths of a second (the port's passes, clockPass):
 *     <ticks> key <scan code>...     the bytes the keyboard sends, in hex
 *     <ticks> release <scan code>    the key's break code (code | 80h)
 *     <ticks> mouse <x> <y> [<buttons>]
 *                                    the driver's place; buttons bits 0-2
 *                                    left, right, middle
 *     <ticks> pad <button> <1|0>     the gamepad's button down or up: a b
 *                                    x y back guide start ls rs lb rb up
 *                                    down left right misc paddle1-4
 *                                    touchpad
 *     <ticks> axis <axis> <value>    a gamepad axis's place: leftx lefty
 *                                    rightx righty lt rt
 *     <ticks> padgone                the gamepad taken away
 *     <ticks> mousebutton <button> <1|0>
 *                                    the mouse's middle, x1 or x2 down or
 *                                    up (the devices' events, input.h,
 *                                    on the keys' line)
 *     <ticks> wheel <notches>        the mouse's wheel, up above 0
 *     <ticks> screen <file.png>      the screen as the mode shows it
 *     <ticks> quit                   the end of the run
 *     <ticks> load <slot>            the game in savegame\savegame.<slot+1>
 *     <ticks> save <slot>            loaded or saved at the end of that
 *                                    pass (runScriptActions), one after
 *                                    the other
 *     <ticks> use <item>             the item used as a double click uses
 *                                    it (Item::use), at the end of that
 *                                    pass
 *     <ticks> jump <map>             the controlled NPC to the map as a
 *                                    teleport pad takes it at a mission's
 *                                    end (startMapJump), at the end of that
 *                                    pass; a game that gives it
 *                                    (hostScriptJump)
 *     <ticks> movie <name> [<form> [<x> <y>]]
 *                                    the movie played as the game's
 *                                    callers play it in form (0 by
 *                                    default; the game's forms) at x, y,
 *                                    at the end of that pass, its line in
 *                                    REGRET_FRAMES' file first
 *                                    (hostScriptMovie, hostMovieFrame)
 * and # comments. Stops the run with the line of a mistake. The input reads
 * REGRET_SCRIPT's script before its first event. */
void readScript(const std::string &path);
/* The script's loads, saves, uses and jumps that have come, at the end of a pass
 * of the game's loop (clockPass), between the processes' runs. */
void runScriptActions();
/* The script's movie begins: its line in REGRET_FRAMES' file
 * (movieframes.cpp). */
void beginMovieFrames(const std::string &name, i16 form, i16 x, i16 y);
/* A device's event as a script's words, the verb first (pad a 1, axis
 * leftx -1200, padgone, mousebutton middle 1, wheel 1), which a replay's
 * lines have too (replay.h); and back from the verb and the words after
 * it, false for words no event has. */
[[nodiscard]] std::vector<std::string> deviceWords(const DeviceEvent &device);
bool readDeviceWords(const std::string &verb, const std::string &rest, DeviceEvent *device);
#endif
