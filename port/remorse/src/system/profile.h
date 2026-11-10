/* No Remorse's profile: what the system layer names and sizes by the game
 * it runs. No Regret's is regret/src/system/profile.h. */
#ifndef PORT_PROFILE_H
#define PORT_PROFILE_H
#include <cstdint>

/* The game's name: the window's title, the message box's, the crash
 * report's head. */
inline constexpr char PROFILE_TITLE[] = "Crusader: No Remorse";
/* The program's name: its messages' prefix ("remorse: "), its log
 * (remorse.log) and its crash reports (remorse-crash-*.log). */
inline constexpr char PROFILE_NAME[] = "remorse";
/* The bytes the machine gives the game (platform/memory.h): what the
 * original's CPROTMEM.C measures under DOSBox with 16 MB (memsize=16,
 * CRUSADER -x43: "System memory: 15000576 bytes"), less than No Regret's
 * 15014912 on the same machine. */
inline constexpr std::uint32_t PROFILE_MEMORY = 15000576;
#endif
