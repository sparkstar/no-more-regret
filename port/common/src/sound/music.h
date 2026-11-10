/* No Regret's 1210:0000-0473, No Remorse's 11f8, file name unknown: the
 * game's music; both games', the addresses No Regret's. A periodic process at
 * 30 Hz plays one song at a time through ASYLUM.DLL and keeps a stack of the
 * songs it interrupted. The game's songs are music_game.h's. */
#ifndef PORT_MUSIC_H
#define PORT_MUSIC_H
#include <array>
#include "common.h"
#include "kprocess.h"

class Flex;

/* musicState */
constexpr int MUSIC_PLAYING = 0x1;
constexpr int MUSIC_PAUSED = 0x8;
/* The song of the region the avatar is in (playSong; the game's
 * REGION_SONGS). */
constexpr int SONG_REGION = 0x45;

/* A song that takes its files' names in turn, the next each time it plays
 * (music_game.h's SONG_TURNS). */
struct SongTurns {
    i16 song;
    std::array<const char *, 3> names;
};
/* The game's songs: SONG_NAMES, SONG_FILES, REGION_SONGS, CHRISTMAS_SONGS,
 * SONG_TURNS. */
#include "music_game.h"

class MusicProcess : public PeriodicProcess {
public:
    /* UNCALLED: nothing calls it. */
    explicit MusicProcess(Loading loading) : PeriodicProcess(loading) {}
    /* UNCALLED: nothing calls it. */
    MusicProcess();
    void run() override;
    void event(u32 event, EventData data) override;
};
ASSERT_SIZE(MusicProcess, 0x40);

extern u8 &musicEnabled;
/* Files that replace the songs, crusader.cfg's tuneNN; 0 for none
 * (1480:4671). */
extern char *(&songFiles)[SONG_FILES_CAPACITY];
extern i16 &currentSong;
/* The Christmas songs may play (1480:46ca), and play whatever the date
 * (46c9). */
extern u8 &christmasMusic;
extern u8 &christmasForced;

/* Whether it is Christmas (24-31 December), or the season is forced. */
bool isChristmas();
/* Plays song (SONG_REGION: the region's); the song playing goes on, but
 * one of SONG_TURNS plays its next file. */
void playSong(i16 song);
u16 musicStatus();
void pauseMusic();
void resumeMusic();
void stopMusic();
/* Frees the song loaded, which stopMusic keeps while the music is off (the port's, for gameRelease). */
void closeMusic();
void enableMusic();
void disableMusic();
/* Plays song and remembers the one it interrupts; popSong goes back to it. */
void pushSong(i16 song);
void popSong();
void clearSongs();
void loadMusic(Flex *flex, i16 index);
void saveMusic(Flex *flex, i16 index);
#endif
