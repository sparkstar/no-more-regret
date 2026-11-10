/* No Regret's 1210:0000-0473, No Remorse's 11f8, file name unknown: the
 * game's music, both games'. See music.h. */
#include "gamestate.h"
#include "music.h"
#include "kernel.h"
#include "amfload.h"
#include "asylum.h"
#include "utility.h"
#include "filespec.h"
#include "flex.h"
#include "gamefile.h"
#include "npc.h"
#include "clock.h"

u8 &musicEnabled = gameGlobal<u8>(0);
u16 &musicState = gameGlobal<u16>(0);
i16 &songDepth = gameGlobal<i16>(0);
const char *&musicExtension = gameGlobal<const char *>("amf");
/* Files that replace the songs. */
char *(&songFiles)[SONG_FILES_CAPACITY] = gameGlobal<char *[SONG_FILES_CAPACITY]>({nullptr});
namespace {
/* Each of SONG_TURNS's songs' turn among its names. */
std::array<u8, SONG_TURNS_CAPACITY> &songTurns = gameGlobal<std::array<u8, SONG_TURNS_CAPACITY>>();
}
u8 &christmasForced = gameGlobal<u8>(0);
u8 &christmasMusic = gameGlobal<u8>(1);
AmfModule &music = gameGlobal<AmfModule>();
i16 (&songStack)[4] = gameGlobal<i16[4]>();
i16 &currentSong = gameGlobal<i16>();

bool isChristmas()
{
    ClockDate date;

    if (christmasForced != 0U) {
        return true;
    }
    date = clockDate();
    return date.month == 12 && date.day >= 24 && date.day <= 31;
}

/* UNCALLED: nothing calls it. */
MusicProcess::MusicProcess()
{
    setRate(30);
    eventMask = EVENT_TIMER;
    pause();
    makeSystem();
    nameProcess(pid, "MusicProcess");
    startTimer(1);
}

void MusicProcess::run() {}

void MusicProcess::event(u32 event, EventData /*data*/)
{
    /* The test is left of a report that the release build leaves out. */
    if (((event & EVENT_TIMER) != 0U) && ((musicState & MUSIC_PAUSED) == 0) && ((musicState & MUSIC_PLAYING) != 0)) {
        /* empty in the original */
    }
}

namespace {
/* The place of song in SONG_TURNS, -1 when it is not there. */
i16 turnsOf(i16 song)
{
    for (std::size_t i = 0; i < SONG_TURNS.size(); i++) {
        if (SONG_TURNS[i].song == song) {
            return static_cast<i16>(i);
        }
    }
    return -1;
}

/* The name of a song's file now: its turn's for one of SONG_TURNS. */
const char *songName(i16 song)
{
    const i16 turns = turnsOf(song);
    if (turns >= 0) {
        return SONG_TURNS[static_cast<std::size_t>(turns)].names[songTurns[static_cast<std::size_t>(turns)]];
    }
    return SONG_NAMES[static_cast<std::size_t>(song)];
}

/* The file of a song, 0 when there is none. Out of the game's songs,
 * none. */
char *songPath(i16 song)
{
    char *drive;

    drive = gameDrive;
    if (song < 0 || song >= static_cast<i16>(SONG_NAMES.size())) {
        return nullptr;
    }
    if (song < SONG_FILES && (songFiles[song] != nullptr)) {
        return songFiles[song];
    }
    if (songName(song) != nullptr) {
        return makePath(drive, soundDirectory, songName(song), musicExtension);
    }
    return nullptr;
}

/* The song of a region; 1 past REGION_SONGS. */
u16 regionSong(i16 region)
{
    if (region < 0 || region >= static_cast<i16>(REGION_SONGS.size())) {
        return 1;
    }
    return REGION_SONGS[static_cast<std::size_t>(region)];
}
}

void playSong(i16 song)
{
    i16 number;
    char *path;
    char *drive;

    number = song;
    if (number == SONG_REGION) {
        number = regionSong(npcMap(&controlledNpc.id));
    }
    /* The song playing goes on (No Remorse's original began it again), but
     * one that takes its names in turn plays its next (No Remorse's m16). */
    const i16 turns = turnsOf(number);
    if (number == currentSong && turns < 0) {
        return;
    }
    if (musicEnabled != 0U) {
        stopMusic();
        if (turns >= 0) {
            const std::size_t place = static_cast<std::size_t>(turns);
            songTurns[place] = static_cast<u8>((songTurns[place] + 1) % SONG_TURNS[place].names.size());
        }
        path = nullptr;
        path = songPath(number);
        if (song == SONG_REGION && (christmasMusic != 0U) && isChristmas()) {
            drive = gameDrive;
            for (const char *name : CHRISTMAS_SONGS) {
                path = makePath(drive, soundDirectory, name, musicExtension);
            }
        }
        if (path != nullptr) {
            if (fileExists(path) == 0) {
                return;
            }
            pauseMusic();
            loadAmf(&music, path);
            ASS_PlayMusic(&music);
            ASS_StartMusic();
            musicState |= MUSIC_PLAYING;
            resumeMusic();
        } else {
            musicState = 0;
        }
        currentSong = number;
    }
}

u16 musicStatus()
{
    return musicState;
}

void pauseMusic()
{
    if ((musicEnabled != 0U) && ((musicState & MUSIC_PAUSED) == 0)) {
        musicState |= MUSIC_PAUSED;
        ASS_PauseMusic();
    }
}

void resumeMusic()
{
    if ((musicEnabled != 0U) && ((musicState & MUSIC_PAUSED) != 0)) {
        ASS_ResumeMusic();
        musicState &= ~MUSIC_PAUSED;
    }
}

void stopMusic()
{
    currentSong = -1;
    if ((musicEnabled != 0U) && ((musicState & MUSIC_PLAYING) != 0)) {
        musicState = 0;
        ASS_StopMusic();
        freeAmf(&music);
    }
}

void closeMusic()
{
    freeAmf(&music);
    musicState = 0;
    currentSong = -1;
}

void enableMusic()
{
    musicEnabled = 1;
}

void disableMusic()
{
    musicEnabled = 0;
}

void pushSong(i16 song)
{
    if ((currentSong != 0) && songDepth < 4) {
        songStack[songDepth] = currentSong;
        songDepth++;
    }
    playSong(song);
}

void popSong()
{
    if (songDepth != 0) {
        songDepth--;
        playSong(songStack[songDepth]);
    }
}

void clearSongs()
{
    songDepth = 0;
    playSong(0);
}

void loadMusic(Flex *flex, i16 index)
{
    i16 song;

    flex->readObject(index, &song, sizeof(song), 0);
    flex->readObject(index + 1, &musicState, sizeof(musicState), 0);
    playSong(song);
}

void saveMusic(Flex *flex, i16 index)
{
    flex->writeObject(index, &currentSong, sizeof(currentSong), 0, 1);
    flex->writeObject(index + 1, &musicState, sizeof(musicState), 0, 1);
}
