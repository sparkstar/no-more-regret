/* No Regret's songs (common/src/sound/music.cpp), which music.h includes. */
#ifndef PORT_MUSIC_GAME_H
#define PORT_MUSIC_GAME_H

/* The songs' files by number in the sound directory, 0 for none. */
inline constexpr std::array<const char *, 23> SONG_NAMES = {
    nullptr,  "ninth", "phil",   "straight", "party",  "demo", "stint",  "mk",  "space2", "d3",    "space", "rhythm",
    "intent", "m03",   "silver", "m01",      "techno", "cred", "regret", "m13", "retro",  "metal", nullptr};
/* The entries of songFiles: crusader.cfg's tune lines and 0. */
inline constexpr i16 SONG_FILES = 22;
/* The song files table's room: SONG_FILES here; the one program's build takes the larger of
 * the two games'. */
inline constexpr i16 SONG_FILES_CAPACITY = SONG_FILES;
/* REGION_SONGS: the song of each region (SONG_REGION). */
inline constexpr std::array<u16, 26> REGION_SONGS = {0, 1,  10, 2,  0, 3,  11, 4,  16, 5,  20, 6, 0,
                                                     7, 13, 8,  15, 9, 12, 10, 19, 14, 21, 0,  0, 2};
/* CHRISTMAS_SONGS: the Christmas song SONG_REGION plays from 24 to 31 December. */
inline constexpr std::array<const char *, 1> CHRISTMAS_SONGS = {"xmas"};
/* SONG_TURNS: none. */
inline constexpr std::array<SongTurns, 0> SONG_TURNS{};
/* SONG_TURNS's room for their turns (music.cpp's songTurns): 0 here; the one program's
 * build takes the larger of the two games'. */
inline constexpr int SONG_TURNS_CAPACITY = 0;
#endif
