/* No Regret's profile: what the system layer names and sizes by the game
 * it runs. Each game's tree has its own profile.h with these names; the
 * system layer (port/system) includes the one of the game it is built
 * with. */
#ifndef PORT_PROFILE_H
#define PORT_PROFILE_H
#include <cstdint>

/* The game's name: the window's title, the message box's, the crash
 * report's head. */
inline constexpr char PROFILE_TITLE[] = "Crusader: No Regret";
/* The program's name: its messages' prefix ("regret: "), its log
 * (regret.log) and its crash reports (regret-crash-*.log). */
inline constexpr char PROFILE_NAME[] = "regret";
/* The bytes the machine gives the game (platform/memory.h): what the
 * original's CPROTMEM.C measures under DOSBox with 16 MB (memsize=16). */
inline constexpr std::uint32_t PROFILE_MEMORY = 15014912;
#endif
