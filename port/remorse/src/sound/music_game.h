/* No Remorse's songs (common/src/sound/music.cpp), which music.h includes. */
#ifndef PORT_MUSIC_GAME_H
#define PORT_MUSIC_GAME_H

/* The songs' files by number in the sound directory, 0 for none. */
inline constexpr std::array<const char *, 21> SONG_NAMES = {nullptr, "m01", "m02",  "m03",  "m04",  "m05",   "m06",
                                                            "m07",   "m08", "m09",  "m10",  "m11",  "m12",   "m13",
                                                            "m14",   "m15", "m16a", "cred", "menu", "buyme", nullptr};
/* The entries of songFiles: crusader.cfg's tune lines and 0. */
inline constexpr i16 SONG_FILES = 20;
/* The song files table's room: SONG_FILES here; the one program's build takes the larger of
 * the two games'. */
inline constexpr i16 SONG_FILES_CAPACITY = SONG_FILES;
/* REGION_SONGS: none. */
inline constexpr std::array<u16, 0> REGION_SONGS{};
/* CHRISTMAS_SONGS: no Christmas song. */
inline constexpr std::array<const char *, 0> CHRISTMAS_SONGS{};
/* SONG_TURNS: the 16th, m16b, m16c, m16a and so on (its original turned the
 * name's fourth letter before each play). */
inline constexpr std::array<SongTurns, 1> SONG_TURNS{{{16, {"m16a", "m16b", "m16c"}}}};
/* SONG_TURNS's room for their turns (music.cpp's songTurns): 1 here; the one program's
 * build takes the larger of the two games'. */
inline constexpr int SONG_TURNS_CAPACITY = 1;
#endif
