/* No Regret's values of the jump to another map (common/src/world/mapjump.cpp). */
#ifndef PORT_MAPJUMP_GAME_H
#define PORT_MAPJUMP_GAME_H
#include <array>
#include "movie.h"

/* The jump shows the loading picture first (leaveMap). */
inline constexpr bool JUMP_SHOWS_LOADING = true;
/* The jump compacts the sound blocks and the memory; No Remorse's empties the sound cache (leaveMap). */
inline constexpr bool JUMP_COMPACTS_MEMORY = true;
inline constexpr bool JUMP_RESETS_SOUND_CACHE = false;
/* The jump puts the controlled NPC at the new map's egg 30 with moveToEgg (back
 * where it was without one, the camera alone while the world is not active),
 * then blacks the screen up to map 200 and shows the status; No Remorse's with teleportToEgg alone
 * (MapJumpProcess::run). */
inline constexpr bool JUMP_MOVES_TO_EGG = true;
/* The movies before the start menu (startGame), and the ticks between them. */
inline constexpr std::array<MovieRequest, 2> INTRO_MOVIES = {
    MovieRequest{.name = "ORIGIN", .mayDouble = false, .restorePalette = false},
    MovieRequest{.name = "ANIM01", .restorePalette = false}};
inline constexpr u16 INTRO_PAUSE = 30;
/* After the intro the screen is cleared and the world frozen again, and the
 * Silencer is held and teleporting until the game starts (startGame). */
inline constexpr bool START_HOLDS_SILENCER = true;
#endif
