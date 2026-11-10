/* The port's enhanced.cfg. See enhanced.h. */
#include "enhanced.h"
#include "gamestate.h"
#include "asylum.h"
#include "bindings.h"
#include "clock.h"
#include "files.h"
#include "host.h"
#include "language.h"
#include "profile.h"
#include "screen.h"
#include "utility.h"

#include <cctype>
#include <cstdio>
#include <tuple>

namespace {

/* The sound's depth the player chose (8 or 16), 0 none: initAsylum's
 * depth is then crusader.cfg's DMA channel's, which enhanced.cfg does not
 * keep. A setting outside the arena. */
u16 &chosenSoundBits = gameSetting<u16>(0);
/* debug_feature=true read, which the file keeps when it is written again:
 * the debug features themselves are the run's, as it starts (host.h's
 * hostDebugFeatures). */
bool &debugFeatureKept = gameSetting<bool>(false);

/* The text without the spaces, tabs and returns at its ends. */
std::string trimmed(const std::string &text)
{
    const std::size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

std::string lowered(std::string text)
{
    for (char &c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

}

std::string enhancedText()
{
    std::string text = "# The port's own settings beside crusader.cfg, which the originals read and write: the "
                       "language of the game's text, the sound's depth, the full screen, the wide screen, the "
                       "game speed, the keys' preset, whether the mouse rolls, each action's inputs.\r\n";
    text += std::string("game=") + PROFILE_NAME + "\r\n";
    text += std::string("language=") + languageCode(gameLanguage) + "\r\n";
    if (chosenSoundBits != 0U) {
        text += "sound_bits=" + std::to_string(chosenSoundBits) + "\r\n";
    }
    if (hostFullscreen()) {
        text += "fullscreen=1\r\n";
    }
    if (screenWide() != 0U) {
        text += "widescreen=" + std::to_string(screenWide()) + "\r\n";
    }
    /* Always: a recording keeps the speed it ran at, whatever the default
     * becomes. */
    text += std::string("game_speed=") + clockMachineSpeedName(clockMachineSpeed()) + "\r\n";
    if (debugFeatureKept) {
        text += "debug_feature=true\r\n";
    }
    return text + bindingsText();
}

std::vector<std::string> readEnhancedText(const std::string &text)
{
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            end = text.size();
        }
        const std::string line = trimmed(text.substr(start, end - start));
        start = end + 1;
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos || line[0] == '#' || line[0] == '/') {
            continue;
        }
        const std::string key = lowered(trimmed(line.substr(0, equals)));
        if (key == "language") {
            gameLanguage = languageOfCode(lowered(trimmed(line.substr(equals + 1))).c_str());
        }
        if (key == "sound_bits") {
            /* The sound's depth in place of crusader.cfg's DMA channel's:
             * 8 or 16; another leaves the channel's. */
            const std::string value = trimmed(line.substr(equals + 1));
            u16 bits = 0;
            if (value == "8") {
                bits = 8;
            } else if (value == "16") {
                bits = 16;
            }
            chooseSoundBits(bits);
            continue;
        }
        if (key == "widescreen") {
            /* The experimental wide screen: 1 the sides, 2 the sides
             * dimmed; another off. */
            const std::string value = trimmed(line.substr(equals + 1));
            u8 mode = 0;
            if (value == "1") {
                mode = 1;
            } else if (value == "2") {
                mode = 2;
            }
            screenSetWide(mode);
            continue;
        }
        if (key == "game_speed") {
            /* The machine's speed: slow, normal or fast; another leaves it. */
            MachineSpeed named = MachineSpeed::NORMAL;
            if (clockMachineSpeedNamed(lowered(trimmed(line.substr(equals + 1))).c_str(), &named)) {
                clockSetMachineSpeed(named);
            }
            continue;
        }
        if (key == "fullscreen") {
            /* The window over the whole screen: 1; another, a window. */
            hostSetFullscreen(trimmed(line.substr(equals + 1)) == "1");
            continue;
        }
        if (key == "debug_feature") {
            /* The debug features: true (or 1); another off. */
            const std::string value = lowered(trimmed(line.substr(equals + 1)));
            debugFeatureKept = value == "true" || value == "1";
            continue;
        }
        if (key == "game") {
            /* The last game played, which the one program reads as it
             * starts (unified/src/system/values.cpp). */
            continue;
        }
    }
    return readBindingsText(text);
}

void chooseSoundBits(u16 bits)
{
    chosenSoundBits = bits == 8 || bits == 16 ? bits : 0;
    ASS_SetDepth(chosenSoundBits);
}

void readEnhancedOptions()
{
    loadBindings();
    /* The window's own changes of the full screen (its key) kept. */
    hostFullscreenAction(writeEnhancedOptions);
    if (fileExists(ENHANCED_FILE) == 0) {
        return;
    }
    std::FILE *file = fileStream(ENHANCED_FILE, "rb");
    if (file == nullptr) {
        return;
    }
    std::string text;
    char buffer[512];
    std::size_t count = 0;
    while ((count = fileStreamRead(file, buffer, 1, sizeof buffer)) > 0) {
        text.append(buffer, count);
    }
    fileStreamClose(file);
    for (const std::string &line : readEnhancedText(text)) {
        debugPrintf(0xff, "Ignoring %s of %s: a setting it does not know\r\n", line.c_str(), ENHANCED_FILE);
    }
}

void writeEnhancedOptions()
{
    fileRemove(ENHANCED_FILE);
    std::FILE *file = fileStream(ENHANCED_FILE, "wb");
    if (file == nullptr) {
        return;
    }
    const std::string text = enhancedText();
    std::ignore = fileStreamWrite(file, text.data(), 1, text.size());
    fileStreamClose(file);
}
