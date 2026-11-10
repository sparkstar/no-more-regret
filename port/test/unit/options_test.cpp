/* crusader.cfg's settings (common/src/world/options.cpp) over a game's
 * values, one program a game (options, remorse_options): without the file
 * the GOG install's (options_game.h's GOG_OPTIONS), the sound card a Sound
 * Blaster 16 and the movies' path the install's; a text's lines with their
 * notes, spaces and returns; one key alone; and readOptions without the
 * file, where the original went on with no movies' path. */
#include "assproc.h"
#include "gamefile.h"
#include "options.h"
#include "options_game.h"
#include "profile.h"
#include "sndmgr.h"
#include "system.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH options: %s\n", what.c_str());
    }
}

/* The movies' path GOG's install has: No Regret's flics, No Remorse's
 * \flics. */
const char *gogFlicPath()
{
    return std::strcmp(PROFILE_NAME, "remorse") == 0 ? "\\flics" : "flics";
}

void clearSettings()
{
    soundCard = 0;
    soundPort = 0;
    soundIrq = 0;
    soundDma = 0;
    soundEnabled = 0;
    delete[] flicPath;
    flicPath = nullptr;
}

void gogText()
{
    clearSettings();
    readOptionText(GOG_OPTIONS, nullptr);
    check(soundCard == 3 && soundPort == 0x220 && soundIrq == 5 && soundEnabled == 1,
          "GOG's: a Sound Blaster 16 at 220h, IRQ 5, the sound on");
    check(soundDma == (std::strcmp(PROFILE_NAME, "remorse") == 0 ? 1 : 5),
          "GOG's DMA channel: " + std::to_string(soundDma));
    check(flicPath != nullptr && std::strcmp(flicPath, gogFlicPath()) == 0,
          std::string("GOG's movies' path: ") + (flicPath != nullptr ? flicPath : "none"));
}

void lines()
{
    clearSettings();
    readOptionText("# a note\r\n// another\r\nSoundCard = 1 \r\nsound=ON\r\n\r\nirq\r\n", nullptr);
    check(soundCard == 1 && soundEnabled == 1, "a key's case, the spaces, the returns; notes and a line without =");
    readOptionText("sound=off\nsoundcard=4\nflicpath=movies\n", "flicpath");
    check(soundCard == 1 && soundEnabled == 1 && flicPath != nullptr && std::strcmp(flicPath, "movies") == 0,
          "one key alone");
}

/* readOptions where there is no crusader.cfg (the test's directory, made
 * empty): GOG's settings. */
void noFile()
{
    namespace fs = std::filesystem;
    const fs::path before = fs::current_path();
    const fs::path empty = fs::temp_directory_path() / (std::string(PROFILE_NAME) + "-options-test");
    std::error_code error;
    fs::remove_all(empty, error);
    fs::create_directories(empty);
    fs::current_path(empty);
    clearSettings();
    readOptions();
    fs::current_path(before);
    fs::remove_all(empty, error);
    check(soundCard == 3 && soundEnabled == 1 && flicPath != nullptr && std::strcmp(flicPath, gogFlicPath()) == 0,
          "no crusader.cfg: GOG's");
}

}

int main()
{
    /* What the game's System makes before the options (debugPrintf's). */
    std::vector<char> text(FORMAT_BUFFER_SIZE);
    formatBuffer = text.data();
    gogText();
    lines();
    noFile();
    formatBuffer = nullptr;
    std::printf("options: %d checks, %d mismatches\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
