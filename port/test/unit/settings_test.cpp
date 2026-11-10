/* The settings the player chose, outside the arena, over a game's library,
 * one program a game (settings, remorse_settings): a copy of the arena
 * taken, as a snapshot takes it (snapshot.h); each setting of the settings
 * gump (gameSettingColumns) changed to its next choice or step, the keys'
 * preset and an action's inputs (bindings.h) and the joystick's calibration
 * (joydrv.h); then the copy put back, as a rewind or a state's load puts it
 * back: each change kept, the volumes as the sound has them too (the
 * sliders' and the music's keys' alike, control.cpp). A global of the
 * game's changed between is put back. */
#include "asylum.h"
#include "bindings.h"
#include "control.h"
#include "gamememory.h"
#include "joydrv.h"
#include "settings.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH settings: %s\n", what.c_str());
    }
}

/* A setting's choices or steps. */
i16 countOf(const Setting &setting)
{
    return setting.choices.empty() ? setting.steps : static_cast<i16>(setting.choices.size());
}

}

int main()
{
    loadBindings();
    const SettingColumns columns = gameSettingColumns();
    std::vector<const Setting *> settings;
    for (const std::vector<Setting> *column : {&columns.left, &columns.right}) {
        for (const Setting &setting : *column) {
            if (countOf(setting) >= 2) {
                settings.push_back(&setting);
            }
        }
    }
    check(settings.size() >= 12, "the settings gump's settings: " + std::to_string(settings.size()));

    const std::vector<std::byte> copy = gameMemoryCopy();
    const i16 dodge = dodgeFactor;
    std::vector<i16> chosen;
    for (const Setting *setting : settings) {
        const i16 next = static_cast<i16>((setting->read() + 1) % countOf(*setting));
        setting->write(next);
        check(setting->read() == next, std::string(setting->heading.english) + ": changed");
        chosen.push_back(next);
    }
    setKeyPreset(keyPreset() == KeyPreset::ORIGINAL ? KeyPreset::MODERN : KeyPreset::ORIGINAL);
    const KeyPreset preset = keyPreset();
    check(addInputs(Action::MOVE_FORWARD, "k"), "an input added");
    const std::string forward = boundText(Action::MOVE_FORWARD);
    const i16 music = ASS_GetMusicVolume();
    const i16 sound = ASS_GetSoundVolume();
    joystickLow = Point{11, 12};
    joystickHigh = Point{21, 22};
    joystickThird = Point{31, 32};
    joystickUncalibrated = 0;
    dodgeFactor = static_cast<i16>(dodge + 1);

    gameMemoryPut(copy);
    for (std::size_t i = 0; i < settings.size(); i++) {
        check(settings[i]->read() == chosen[i], std::string(settings[i]->heading.english) + ": kept");
    }
    check(keyPreset() == preset, "the keys' preset kept");
    check(boundText(Action::MOVE_FORWARD) == forward, "an action's inputs kept: " + boundText(Action::MOVE_FORWARD));
    check(ASS_GetMusicVolume() == music && ASS_GetSoundVolume() == sound, "the volumes kept");
    check(joystickLow.x == 11 && joystickLow.y == 12 && joystickHigh.x == 21 && joystickThird.y == 32 &&
              joystickUncalibrated == 0,
          "the joystick's calibration kept");
    check(dodgeFactor == dodge, "a global of the game's put back");

    std::printf("settings: %d checks, %d mismatches\n", checks, failures);
    return failures != 0 ? 1 : 0;
}
