/* Written by port/tools/make_unified_values.py from regret/src/sound/music_game.h and remorse/src/sound/music_game.h:
 * the one program's run-time values; do not edit. */

#ifndef PORT_MUSIC_GAME_H
#define PORT_MUSIC_GAME_H
#include <span>

extern std::span<const char *const> SONG_NAMES;
extern i16 SONG_FILES;
inline constexpr i16 SONG_FILES_CAPACITY = 22;
extern std::span<const u16> REGION_SONGS;
extern std::span<const char *const> CHRISTMAS_SONGS;
extern std::span<const SongTurns> SONG_TURNS;
inline constexpr int SONG_TURNS_CAPACITY = 1;
#endif
