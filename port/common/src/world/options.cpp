/* Code5 (No Regret's 1020, No Remorse's 1018, file name unknown): the game's
 * options, read from and written to crusader.cfg. The keys both games know,
 * and each game's own kept by its values (options_game.h). */
#include <algorithm>
#include <cstdio>
#include "gamestate.h"
#include <cstring>
#include "options.h"
#include "options_game.h"
#include "enhanced.h"
#include "assproc.h"
#include "asylum.h"
#include "control.h"
#include "crusys.h"
#include "filepars.h"
#include "gamefile.h"
#include "gametime.h"
#include "joydrv.h"
#include "mainmenu.h"
#include "music.h"
#include "shaphand.h"
#include "sndmgr.h"
#include "utility.h"
#include "world.h"
#include "movie.h"
#include "flicplay.h"
#include "supervga.h"
#include "files.h"

namespace {
/* No Regret's; nothing reads it (1480:0308; No Remorse's 1478:0097 pads the
 * startup module's data instead). */
[[maybe_unused]] u8 unknown0308 = 0;
}
u8 &hiColorMovies = gameSetting<u8>(0);
/* No Remorse's noweasel. */
u8 &noWeasel = gameGlobal<u8>(0);
u16 &movieSubtitles = gameSetting<u16>(0);

namespace {
/* A setting's word: "true" (isTrue); "true", "1" or "yes" (isYes). */
bool isTrue(const char *value)
{
    return strcmp(value, "true") == 0;
}

bool isYes(const char *value)
{
    return isTrue(value) || (strcmp(value, "1") == 0) || (strcmp(value, "yes") == 0);
}

/* The keys of the joystick's buttons and their pairs, joystickActions' in
 * turn. */
constexpr const char *BUTTON_KEYS[10] = {"button1",   "button2",   "button3",   "button4",   "button1_2",
                                         "button1_3", "button1_4", "button2_3", "button2_4", "button3_4"};

/* crusader.cfg's line key=value (both lower case): the sound card and its
 * volumes, the CD and the movies, the frame skip and the effects, the
 * mouse and the joystick, the songs' files; a key it does not know is
 * ignored (the port's own settings are enhanced.cfg's). */
void readOption(const char *key, const char *value)
{
    for (i16 button = 0; button < 10; button++) {
        if (strcmp(key, BUTTON_KEYS[button]) == 0) {
            sscanf(value, "%hd", &joystickActions[button]);
            return;
        }
    }
    if (strcmp(key, "sound") == 0) {
        soundEnabled = (strcmp(value, "on") == 0) ? 1 : 0;
    } else if (strcmp(key, "music") == 0) {
        musicOn = (strcmp(value, "on") == 0) ? 1 : 0;
    } else if (strcmp(key, "port") == 0) {
        sscanf(value, "%hx", &soundPort);
    } else if (strcmp(key, "dma") == 0) {
        sscanf(value, "%hu", &soundDma);
    } else if (strcmp(key, "irq") == 0) {
        sscanf(value, "%hu", &soundIrq);
    } else if (strcmp(key, "cdletter") == 0) {
        gameDrive[0] = (strcmp(value, "none") == 0) ? 0 : value[0];
    } else if (strcmp(key, "flicpath") == 0) {
        flicPath = new char[strlen(value) + 1];
        strcpy(flicPath, value);
    } else if (!XAN_MOVIE_OPTIONS && strcmp(key, "videosize") == 0) {
        flicVideoMode = ((strcmp(value, "big") == 0) || (strcmp(value, "large") == 0)) ? MODE_13H : VESA_640X480;
    } else if (strcmp(key, "soundcard") == 0) {
        sscanf(value, "%hu", &soundCard);
    } else if (INSTALL_OPTIONS && strcmp(key, "noweasel") == 0) {
        noWeasel = isTrue(value) ? 1 : 0;
    } else if (strcmp(key, "mixrate") == 0) {
        sscanf(value, "%hu", &sampleRate);
    } else if (strcmp(key, "frameskip") == 0) {
        /* No Remorse's original keeps frameSkip in a byte: its %d stores a
         * word, whose high byte goes to the byte after it, limitBlasts
         * (FRAME_SKIP_SPILLS). */
        i16 word;
        if (sscanf(value, "%hd", &word) == 1) {
            if (FRAME_SKIP_SPILLS) {
                frameSkip = static_cast<u8>(word);
                limitBlasts = static_cast<u8>(static_cast<u16>(word) >> 8);
            } else {
                frameSkip = word;
            }
        }
    } else if (strcmp(key, "subtitles") == 0) {
        movieSubtitles = isTrue(value) ? 1 : 0;
    } else if (strcmp(key, "cdicon") == 0) {
        ; /* read and ignored (No Regret's original) */
    } else if (INSTALL_OPTIONS && strcmp(key, "fullinstall") == 0) {
        allShapes = static_cast<u8>(isTrue(value));
    } else if (strcmp(key, "bigvideo") == 0) {
        /* No Regret's doubles the movies, No Remorse's picks the flic's
         * mode. */
        if (!XAN_MOVIE_OPTIONS) {
            flicVideoMode = isTrue(value) ? MODE_13H : VESA_640X480;
        } else {
            doubleMovies = isYes(value) ? 2 : 0;
        }
    } else if (XAN_MOVIE_OPTIONS && strcmp(key, "moviehicolor") == 0) {
        if (isYes(value)) {
            movieHiColor = 1;
            hiColorMovies = 1;
        } else {
            hiColorMovies = 0;
            movieHiColor = 0;
        }
    } else if (strcmp(key, "filter") == 0) {
        oversampling = static_cast<u8>(isTrue(value));
    } else if (strcmp(key, "limitblasts") == 0) {
        limitBlasts = static_cast<u8>(isTrue(value));
    } else if (XAN_MOVIE_OPTIONS && strcmp(key, "lowresmovies") == 0) {
        movieFrameFlags = static_cast<i16>(isTrue(value));
    } else if (strcmp(key, "mousespeed") == 0) {
        sscanf(value, "%hd", &mouseSpeed);
    } else if (strcmp(key, "musicvolume") == 0) {
        sscanf(value, "%hu", &musicVolume);
    } else if (strcmp(key, "soundvolume") == 0) {
        sscanf(value, "%hu", &soundVolume);
    } else if (strcmp(key, "animation") == 0) {
        /* The originals' animationsHeld (gametime.h) is a byte: the word %d
         * stores also writes the byte after it, the first letter of the name
         * "GameTime". */
        sscanf(value, "%hhu", &animationsStopped);
    } else if (strcmp(key, "control") == 0) {
        sscanf(value, "%hd", &joystickType);
    } else if (CALIBRATE_ONCE && strcmp(key, "joystick") == 0) {
        sscanf(value, "(%hd,%hd),(%hd,%hd),(%hd,%hd)", &joystickLow.x, &joystickLow.y, &joystickHigh.x, &joystickHigh.y,
               &joystickThird.x, &joystickThird.y);
        joystickUncalibrated = 0;
    } else if (strncmp(key, "tune", 4) == 0) {
        i16 tune;
        sscanf(key + 4, "%hd", &tune);
        if (tune > 0 && tune < TUNES_READ) {
            songFiles[tune] = new char[strlen(value) + 1];
            strcpy(songFiles[tune], value);
            if (TUNES_BY_MISSION) {
                debugPrintf(0xff, "Redirecting mission %d tune to '%s'%s", tune, value, CFG_LINE_END);
            } else {
                debugPrintf(0xff, "Redirecting tune #%d to '%s'%s", tune, value, CFG_LINE_END);
            }
        }
    } else if (INSTALL_OPTIONS && strcmp(key, "crupath") == 0) {
        cruPath = new char[80];
        strcpy(cruPath, value);
    } else if (VIOLENCE_OPTION && strcmp(key, "violence") == 0) {
        violenceSetting[9] = isYes(value) ? '1' : '0';
    }
}
}

void readOptionText(const char *text, const char *only)
{
    std::size_t start = 0;
    const std::size_t size = strlen(text);
    while (start < size) {
        std::size_t end = start;
        while (end < size && text[end] != '\n') {
            end++;
        }
        char line[160];
        const std::size_t length = std::min(end - start, sizeof(line) - 1);
        memcpy(line, text + start, length);
        line[length] = 0;
        start = end + 1;
        char *equals = strchr(line, '=');
        const char first = line[strspn(line, " \t")];
        if (first == '#' || first == '/' || equals == nullptr) {
            continue;
        }
        *equals = 0;
        char *key = line + strspn(line, " \t");
        char *value = equals + 1 + strspn(equals + 1, " \t");
        key[strcspn(key, " \t\r")] = 0;
        value[strcspn(value, " \t\r")] = 0;
        strlwr(key);
        strlwr(value);
        if (only == nullptr || strcmp(key, only) == 0) {
            readOption(key, value);
        }
    }
}

/* crusader.cfg's settings, a key=value a line (# and / lines are notes),
 * then the port's own (enhanced.cfg, readEnhancedOptions). No Remorse's
 * says which installation it runs with; No Regret's keeps the swap.
 *
 * The port's: without the file (a player who took it away), the settings
 * of the GOG install's (GOG_OPTIONS), which the port's program sits in;
 * the original went on with the game's first values and no movies' path,
 * whose strcat copied from a null pointer (its data segment's start, so
 * that no movie was found), where the port faulted. A file without
 * flicpath gets GOG's too. */
void readOptions()
{
    if (fileExists("crusader.cfg") == 0) {
        readOptionText(GOG_OPTIONS, nullptr);
    } else {
        FileParser *parser = new FileParser("crusader.cfg");
        char key[80];
        char value[80];
        while (parser->atEnd() == 0U) {
            if (*parser->cursor == '#' || *parser->cursor == '/') {
                parser->skipLine();
                continue;
            }
            parser->skipSpace();
            parser->readUntil('=', key);
            parser->readByte();
            strlwr(key);
            i16 i = 0;
            while (key[i] != 0 && key[i] != ' ' && key[i] != '\n' && key[i] != '\t' && key[i] != '\r') {
                i++;
            }
            key[i] = 0;
            parser->readWord(value);
            strlwr(value);
            parser->skipLine();
            readOption(key, value);
        }
        delete parser;
    }
    if (flicPath == nullptr) {
        readOptionText(GOG_OPTIONS, "flicpath");
    }
    if (!INSTALL_OPTIONS) {
        keepSwap = 1;
    } else if (keepNpcShapes != 0U) {
        debugPrintf(0xff, ".\n\r");
    } else if (allShapes == 0U) {
        debugPrintf(0xff, "Running with partial installation.\n\r");
    } else {
        debugPrintf(0xff, "Running with full installation.\n\r");
    }
    /* The port's own settings (enhanced.h). */
    readEnhancedOptions();
}

void writeOptions()
{
    FILE *file;
    i16 i;

    fileRemove("crusader.cfg");
    file = fileStream("crusader.cfg", "wb");
    if (soundEnabled != 0U) {
        fileStreamPrint(file, "sound=on%s", CFG_LINE_END);
    } else {
        fileStreamPrint(file, "sound=off%s", CFG_LINE_END);
    }
    if (musicOn != 0U) {
        fileStreamPrint(file, "music=on%s", CFG_LINE_END);
    } else {
        fileStreamPrint(file, "music=off%s", CFG_LINE_END);
    }
    if ((soundEnabled != 0U) || (musicOn != 0U)) {
        fileStreamPrint(file, "port=0x%X%s", soundPort, CFG_LINE_END);
        fileStreamPrint(file, "irq=%d%s", soundIrq, CFG_LINE_END);
        fileStreamPrint(file, "dma=%d%s", soundDma, CFG_LINE_END);
        fileStreamPrint(file, "soundcard=%d%s", soundCard, CFG_LINE_END);
        fileStreamPrint(file, "musicvolume=%d%s", ASS_GetMusicVolume(), CFG_LINE_END);
        fileStreamPrint(file, "soundvolume=%d%s", ASS_GetSoundVolume(), CFG_LINE_END);
    }
    fileStreamPrint(file, "cdletter=%c%s", gameDrive[0], CFG_LINE_END);
    fileStreamPrint(file, "flicpath=%s%s", flicPath, CFG_LINE_END);
    if (!XAN_MOVIE_OPTIONS) {
        fileStreamPrint(file, "newvideo=true%s", CFG_LINE_END);
    }
    fileStreamPrint(file, "mousespeed=%d%s", mouseSpeed, CFG_LINE_END);
    fileStreamPrint(file, "mixrate=%u%s", sampleRate, CFG_LINE_END);
    fileStreamPrint(file, "animation=%d%s", animationsStopped, CFG_LINE_END);
    fileStreamPrint(file, "control=%d%s", joystickType, CFG_LINE_END);
    if (joystickActions[0] != -1) {
        fileStreamPrint(file, "button1=%d%s", joystickActions[0], CFG_LINE_END);
    }
    if (joystickActions[1] != -1) {
        fileStreamPrint(file, "button2=%d%s", joystickActions[1], CFG_LINE_END);
    }
    if (joystickActions[2] != -1) {
        fileStreamPrint(file, "button3=%d%s", joystickActions[2], CFG_LINE_END);
    }
    if (joystickActions[3] != -1) {
        fileStreamPrint(file, "button4=%d%s", joystickActions[3], CFG_LINE_END);
    }
    if (joystickActions[4] != -1) {
        fileStreamPrint(file, "button1_2=%d%s", joystickActions[4], CFG_LINE_END);
    }
    if (joystickActions[5] != -1) {
        fileStreamPrint(file, "button1_3=%d%s", joystickActions[5], CFG_LINE_END);
    }
    if (joystickActions[6] != -1) {
        fileStreamPrint(file, "button1_4=%d%s", joystickActions[6], CFG_LINE_END);
    }
    if (joystickActions[7] != -1) {
        fileStreamPrint(file, "button2_3=%d%s", joystickActions[7], CFG_LINE_END);
    }
    if (joystickActions[8] != -1) {
        fileStreamPrint(file, "button2_4=%d%s", joystickActions[8], CFG_LINE_END);
    }
    if (joystickActions[9] != -1) {
        fileStreamPrint(file, "button3_4=%d%s", joystickActions[9], CFG_LINE_END);
    }
    if (limitBlasts != 0U) {
        fileStreamPrint(file, "limitblasts=true%s", CFG_LINE_END);
    } else {
        fileStreamPrint(file, "limitblasts=false%s", CFG_LINE_END);
    }
    fileStreamPrint(file, "frameskip=%d%s", frameSkip, CFG_LINE_END);
    if (ASS_GetOversampling() != 0U) {
        fileStreamPrint(file, "filter=true%s", CFG_LINE_END);
    } else {
        fileStreamPrint(file, "filter=false%s", CFG_LINE_END);
    }
    if (allShapes != 0U) {
        fileStreamPrint(file, "fullinstall=true%s", CFG_LINE_END);
    }
    if (movieSubtitles != 0U) {
        fileStreamPrint(file, "subtitles=true%s", CFG_LINE_END);
    }
    if (XAN_MOVIE_OPTIONS) {
        if (movieFrameFlags != 0) {
            fileStreamPrint(file, "lowresmovies=true%s", CFG_LINE_END);
        } else {
            fileStreamPrint(file, "lowresmovies=false%s", CFG_LINE_END);
        }
        if (doubleMovies != 0) {
            fileStreamPrint(file, "bigvideo=true%s", CFG_LINE_END);
        }
        if (hiColorMovies != 0U) {
            fileStreamPrint(file, "moviehicolor=true%s", CFG_LINE_END);
        } else {
            fileStreamPrint(file, "moviehicolor=false%s", CFG_LINE_END);
        }
    } else if (flicVideoMode == MODE_13H) {
        fileStreamPrint(file, "bigvideo=true%s", CFG_LINE_END);
    }
    if (INSTALL_OPTIONS) {
        if (noWeasel != 0U) {
            fileStreamPrint(file, "noweasel=true%s", CFG_LINE_END);
        }
        if (cruPath != nullptr) {
            fileStreamPrint(file, "crupath=%s%s", cruPath, CFG_LINE_END);
        }
    }
    /* No Remorse's reads tunes 1-16 and writes one more (original). */
    for (i = 1; i < TUNES_WRITTEN; i++) {
        if (songFiles[i] != nullptr) {
            fileStreamPrint(file, "tune%02d=%s%s", i, songFiles[i], CFG_LINE_END);
        }
    }
    if (CALIBRATE_ONCE) {
        fileStreamPrint(file, "joystick=(%d,%d),(%d,%d),(%d,%d)%s", joystickLow.x, joystickLow.y, joystickHigh.x,
                        joystickHigh.y, joystickThird.x, joystickThird.y, CFG_LINE_END);
    }
    if (VIOLENCE_OPTION) {
        if (violenceSetting[9] == '1') {
            fileStreamPrint(file, "violence=true%s", CFG_LINE_END);
        } else {
            fileStreamPrint(file, "violence=false%s", CFG_LINE_END);
        }
    }
    fileStreamClose(file);
    /* The port's own settings in a file of their own (enhanced.h): the
     * originals' options gumps write crusader.cfg again without them. */
    writeEnhancedOptions();
}
