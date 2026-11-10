/* No Remorse's values of the jump to another map (common/src/world/mapjump.cpp). */
#ifndef PORT_MAPJUMP_GAME_H
#define PORT_MAPJUMP_GAME_H
#include <array>
#include "movie.h"

/* The jump shows the loading picture first (leaveMap): not No Remorse's. */
inline constexpr bool JUMP_SHOWS_LOADING = false;
/* The jump empties the sound cache; No Regret's compacts the sound blocks and the memory (leaveMap). */
inline constexpr bool JUMP_COMPACTS_MEMORY = false;
inline constexpr bool JUMP_RESETS_SOUND_CACHE = true;
/* The jump puts the controlled NPC at the new map's egg 30 with moveToEgg (back
 * where it was without one, the camera alone while the world is not active),
 * then blacks the screen up to map 200 and shows the status; not No Remorse's, with teleportToEgg alone
 * (MapJumpProcess::run). */
inline constexpr bool JUMP_MOVES_TO_EGG = false;
/* The movies before the start menu (startGame), and the ticks between them. */
inline constexpr std::array<MovieRequest, 2> INTRO_MOVIES = {
    MovieRequest{.name = "T01", .background = MovieBackground::GREY, .holdMusic = true},
    MovieRequest{.name = "T02", .background = MovieBackground::GREY, .holdMusic = true}};
inline constexpr u16 INTRO_PAUSE = 0;
/* After the intro the screen is cleared and the world frozen again, and the
 * Silencer is held and teleporting until the game starts (startGame): No Regret's. */
inline constexpr bool START_HOLDS_SILENCER = false;
#endif
