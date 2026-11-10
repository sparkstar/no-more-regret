/* The pass dump: with REGRET_PASS_DUMP set to a file (a FIFO, when the pass
 * oracle reads it as the port runs), the run as records: where the time
 * passes (clockPass, clockWait, clockDelay, clockPassCounts) and to when,
 * each delivery of an interrupt line to the game's handler with what the
 * handler gets, and at each pass's end the state line (statelog.cpp) and
 * the game's state as saveGame's objects without its work before and after
 * them (writeGameState), which the pass oracle compares with the
 * original's.
 *
 * A record is a tag byte, its payload's length (32 bits) and the payload,
 * little-endian:
 *   T  the kind (P, W, D or C: clockPass, clockWait, clockDelay,
 *      clockPassCounts) and the time to pass to (64 bits, in the interval
 *      timer's counts)
 *   I  the line and the time (64 bits); line 1's scan code, or FFh and a
 *      device event's kind (the game's devices, which the original has
 *      not); line 12's events, buttons, x and y (16 bits each) and whether
 *      the game's handler was called (a byte)
 *   S  the state line
 *   O  an object: its number (16 bits) and its bytes
 *   C  a loaded function's code: its number in the usecode's memory
 *      (usemem.h), its class and the function (16 bits each), which the
 *      usecode's pointers into it hold
 *   A  a call of the sound module's (ASYLUM.DLL's, asylum.h) as it
 *      returned: its number (REGRET.EXE's import), the count of its plain
 *      arguments and they (16 bits each, the pointers left out), the result
 *      (16 bits) and what it wrote for the game (GetCardName's name, Init's
 *      config read)
 *   B  the module calls the game's callback with a channel (16 bits), in
 *      the call whose A record follows; the callback's own calls come
 *      between
 *   L, V, X  a script's or a recording's action at the pass's end (load,
 *      save, another: use, jump), before the game does it: its slot or
 *      argument (16 bits)
 *   Q  the run ends (hostEnd: a replay's end, the game's quit), before the
 *      game's work at its end
 *   E  the pass (64 bits) and whether the objects were written (a byte:
 *      not before the game's tables are made)
 * When the reader has gone (the pass oracle has compared its passes) the
 * run ends, its other files written out. */
#ifndef PORT_PASSDUMP_H
#define PORT_PASSDUMP_H
#include <cstdint>
#include <initializer_list>
#include <vector>

bool passDumping();
void passDumpTime(char kind, std::uint64_t until);
void passDumpTick();
void passDumpScan(std::uint8_t scan);
void passDumpDevice(std::uint8_t kind);
void passDumpMouse(std::uint16_t events, std::uint16_t buttons, std::int16_t x, std::int16_t y, bool handled);
void passDumpPass(std::uint64_t pass, const char *line);
/* The sound module's (common/src/sound/asylum.cpp). */
void passDumpSound(std::uint8_t number, std::initializer_list<std::uint16_t> arguments, std::uint16_t result,
                   const std::uint8_t *bytes = nullptr, std::uint16_t size = 0);
void passDumpSoundCallback(std::int16_t channel);
void passDumpAction(char tag, std::int16_t argument);
void passDumpEnd();
/* A record (its tag and payload), and the game's state at a pass's end
 * (its O and C records, system/passstate.cpp's): whether it wrote them,
 * not before the game's tables are made. */
void passDumpRecord(char tag, const std::vector<std::uint8_t> &payload);
extern bool (*passDumpStateHook)();
#endif
