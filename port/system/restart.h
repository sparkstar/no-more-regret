/* A run of the other game: the program chooses its game's values before
 * anything else starts (unified/src/system/values.cpp: CRUSADER_GAME, else
 * the last game in enhanced.cfg, else No Remorse), so another game is the
 * same program run again with CRUSADER_GAME set to it; the run ends with
 * that run's code. A program of one game (regret, remorse) ends instead. */
#ifndef PORT_RESTART_H
#define PORT_RESTART_H
#include <string>

[[noreturn]] void restartAs(const std::string &game, char *const *argv);
#endif
