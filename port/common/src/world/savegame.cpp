/* Code13 (No Regret's 1060:000d-05b2, No Remorse's 1050:000d-05b1, file
 * name unknown): the saved games, both games': the game's values in
 * savegame_game.h. */
#include "gamestate.h"
#include <cstdio>
#include <cstring>
#include "savegame.h"
#include "waits.h"
#include "alert.h"
#include "asylum.h"
#include "assproc.h"
#include "camera.h"
#include "control.h"
#include "dispatch.h"
#include "dlist.h"
#include "dsfxman.h"
#include "dtable.h"
#include "fastflex.h"
#include "filespec.h"
#include "flag.h"
#include "gamefile.h"
#include "gamepal.h"
#include "gumpshap.h"
#include "item.h"
#include "itemcach.h"
#include "joydrv.h"
#include "kernel.h"
#include "mapjump.h"
#include "msgbox.h"
#include "music.h"
#include "npc.h"
#include "savegame_game.h"
#include "spmgr.h"
#include "tracker.h"
#include "uprocess.h"
#include "utility.h"
#include "viewport.h"
#include "world.h"
#include "yamm.h"
#include "files.h"
#include "input.h"
#include "host.h"

char *savePath(i16 slot);

/* The first of the kernel's objects in a saved game. */
constexpr i16 KERNEL_OBJECT = 100;

i16 &saveSlot = gameGlobal<i16>(0);
namespace {
/* The slots' descriptions, as their files have them: the original's on its
 * heap, the port's beside the player's settings, so that they go back with
 * the files and only then (a rewind past a save keeps the save's file, and
 * its description with it). */
char (&descriptions)[SAVE_SLOTS][DESCRIPTION_LENGTH] = gameSetting<char[SAVE_SLOTS][DESCRIPTION_LENGTH]>();
}

void freeDescriptions()
{
    memset(descriptions, 0, sizeof(descriptions));
}

void loadDescriptions()
{
    i16 slot;

    for (slot = 0; slot < SAVE_SLOTS; slot++) {
        memset(descriptions[slot], 0, DESCRIPTION_LENGTH);
        char *path = savePath(slot);
        if (fileExists(path) != 0) {
            FastFlex flex(path, FILE_READ, -1);
            flex.readObject(9, descriptions[slot], DESCRIPTION_LENGTH, 0);
            flex.close();
        }
    }
}

/* The path of slot's file (1060:00dc). */
char *savePath(i16 slot)
{
    const i16 number = slot + 1;
    char name[10];

    snprintf(name, sizeof(name), "%d", number);
    return makePath(nullptr, savegameDirectory, "savegame", name);
}

void saveGame()
{
    if (controlledActor == 1) {
        const i16 currentMusicVolume = ASS_GetMusicVolume();
        const i16 currentSoundVolume = ASS_GetSoundVolume();
        kernelReady = 0;
        gumpKeys.clear();
        flushKeys();
        ASS_SetMusicVolume(0);
        ASS_SetSoundVolume(0);
        char *path = savePath(saveSlot);
        fileRemove(path);
        FastFlex flex(path, FILE_READWRITE, SAVE_FLEX_ENTRIES);
        flex.writeObject(9, descriptions[saveSlot], DESCRIPTION_LENGTH, 0, 1);
        flex.writeObject(90, &alertOn, 1, 0, 1);
        for (const SavedObject &saved : GAME_SAVED_OBJECTS) {
            flex.writeObject(saved.object, saved.place(), saved.size, 0, 1);
        }
        saveItems(&flex, 0);
        saveNpcs(&flex, 10);
        controlledNpc.save(&flex, 20);
        saveSprites(&flex, 30);
        saveYamm(&flex, 40);
        saveTargets(&flex, 50);
        saveFlags(&flex, 60);
        saveMusic(&flex, 70);
        saveKernel(&flex, KERNEL_OBJECT);
        ASS_SetMusicVolume(currentMusicVolume);
        ASS_SetSoundVolume(currentSoundVolume);
        kernelReady = 1;
    }
    inputMouseXRange(0, screenViewport->right() * 2);
    inputMouseYRange(0, screenViewport->bottom());
    /* No Regret's ends (No Remorse's original clears the teleporting in the
     * thawWorld every caller runs after, and lets no save happen in the
     * stasis). */
    clearTeleporting();
    clearStasis();
    flushKeys();
}

/* The objects in saveGame's order; the controlled NPC's with the seed of
 * now (ControlledNpc::writeState) and the kernel's as it is (writeKernel). */
void writeGameState(Flex *flex)
{
    flex->writeObject(90, &alertOn, 1, 0, 1);
    for (const SavedObject &saved : GAME_SAVED_OBJECTS) {
        flex->writeObject(saved.object, saved.place(), saved.size, 0, 1);
    }
    saveItems(flex, 0);
    saveNpcs(flex, 10);
    controlledNpc.writeState(flex, 20);
    saveSprites(flex, 30);
    saveYamm(flex, 40);
    saveTargets(flex, 50);
    saveFlags(flex, 60);
    saveMusic(flex, 70);
    writeKernel(flex, KERNEL_OBJECT);
}

void loadGame()
{
    if (controlledActor == 1) {
        char *path = savePath(saveSlot);
        const i16 savedLimitBlasts = limitBlasts;
        const i16 savedFrameSkip = frameSkip;
        if (fileExists(path) != 0) {
            const i16 currentMusicVolume = ASS_GetMusicVolume();
            const i16 currentSoundVolume = ASS_GetSoundVolume();
            kernelReady = 0;
            gumpKeys.clear();
            flushKeys();
            ASS_SetMusicVolume(0);
            ASS_SetSoundVolume(0);
            FastFlex flex(path, FILE_READ, -1);
            killAll();
            freeDeadProcesses();
            resetUsecode();
            flex.readObject(9, descriptions[saveSlot], DESCRIPTION_LENGTH, 0);
            flex.readObject(90, &alertOn, 1, 0);
            for (const SavedObject &saved : GAME_SAVED_OBJECTS) {
                flex.readObject(saved.object, saved.place(), saved.size, 0);
            }
            loadItems(&flex, 0);
            loadNpcs(&flex, 10);
            controlledNpc.load(&flex, 20);
            loadSprites(&flex, 30);
            loadYamm(&flex, 40);
            loadTargets(&flex, 50);
            loadFlags(&flex, 60);
            loadMusic(&flex, 70);
            loadKernel(&flex, KERNEL_OBJECT);
            resetJoystick();
            startASSProcess();
            releaseNodes();
            ASS_SetMusicVolume(currentMusicVolume);
            ASS_SetSoundVolume(currentSoundVolume);
            kernelReady = 1;
        }
        limitBlasts = savedLimitBlasts;
        frameSkip = savedFrameSkip;
    }
    inputMouseXRange(0, screenViewport->right() * 2);
    inputMouseYRange(0, screenViewport->bottom());
    /* On the easiest level the BA-40 needs no ammunition. */
    if (controlledNpc.difficulty == 1) {
        weaponTypes[1].ammoType = 0;
    } else {
        weaponTypes[1].ammoType = 1;
    }
    /* No Regret's ends: the stasis is not in the save (its original lets a
     * dead Silencer load in it). */
    clearTeleporting();
    clearStasis();
    /* The Silencer's height (its shape's z, which the save does not hold and
     * the next animation sets) as the loaded stance has it. No Regret's
     * original set it for a kneeling Silencer only, No Remorse's not at all:
     * a Silencer loaded in the other stance kept its height until he moved. */
    if (controlledNpc.isKneeling() != 0U) {
        controlledNpc.kneel();
    } else {
        controlledNpc.standUp();
    }
    flushKeys();
}

bool savedGameLoadable()
{
    /* savePath's without makePath's ring of paths, and the bytes in the
     * host's heap: the check changes nothing of the game's (its paths'
     * ring, the arena's blocks), so that a load goes as it did. */
    char path[40];
    snprintf(path, sizeof(path), "%s\\savegame.%d", savegameDirectory, saveSlot + 1);
    if (fileExists(path) == 0) {
        return true;
    }
    return kernelSaveLoadable(fileContents(path), KERNEL_OBJECT);
}

NewGump *newSavedGameRefusal()
{
    return new MessageBoxGump(NewGumpId(), nullptr,
                              "ERROR:\n\nUnable to load game.\n\nThe saved game is damaged\nor of another version.",
                              guiColors->face);
}

char *saveDescription(i16 slot)
{
    return descriptions[slot];
}

void setSaveDescription(i16 slot, char *text)
{
    memset(descriptions[slot], 0, DESCRIPTION_LENGTH);
    strcpy(descriptions[slot], text);
}

void scriptLoadGame(i16 slot)
{
    setSaveSlot(slot);
    stopAllEffects();
    loadGame();
    gameCamera->repaint();
    gamePalette->apply();
}

void scriptSaveGame(i16 slot)
{
    setSaveSlot(slot);
    saveGame();
}

namespace {
/* The script's use of an item, as a double click on it uses it. */
void scriptUseItem(i16 item)
{
    /* In the clock's pass, after which nothing goes on: the use's waits run
     * where they are. */
    blockWaits();
    Item(static_cast<u16>(item)).use();
    unblockWaits();
}

/* The script's (port_machine.h): the port's input calls them. */
/* The script's jump to a map, as a teleport pad ends a mission (TELEPAD's
 * startMapJump). */
void scriptJumpMap(i16 map)
{
    startMapJump(static_cast<u16>(map));
}

const bool SCRIPT_FUNCTIONS = (hostScriptActions(scriptLoadGame, scriptSaveGame, scriptUseItem),
                               hostScriptJump(scriptJumpMap), hostScriptReady([]() { return !waitsOpen(); }), true);
}
