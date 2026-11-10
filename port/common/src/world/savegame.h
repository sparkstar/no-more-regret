/* Code13 (file name unknown): the saved games: the descriptions of the
 * slots, saving and loading. A saved game is a FLEX archive,
 * savegame\savegame.<slot+1>, whose objects hold the description (9), the
 * alert (90), the game's own (No Remorse's weasel's visit, 91) and what each
 * module saves. */
#ifndef PORT_SAVEGAME_H
#define PORT_SAVEGAME_H
#include "common.h"
#include "dispatch.h"

#include <utility>

class Flex;
class NewGump;

constexpr int SAVE_SLOTS = 13;
constexpr int DESCRIPTION_LENGTH = 32;

/* An object a game saves besides the modules' (savegame_game.h's
 * GAME_SAVED_OBJECTS): its number in the FLEX and its bytes, at the place
 * place gives when it is saved or loaded (a global's, as savefields.h's
 * SaveGlobal). */
struct SavedObject {
    u16 object;
    u8 *(*place)();
    u16 size;
    /* The original's global (the pass oracle's). */
    const char *name;
};

/* The slot to save to or load from. */
extern i16 &saveSlot;
inline void setSaveSlot(i16 slot)
{
    saveSlot = slot;
}

/* Reads each slot's description. */
void loadDescriptions();
/* Deletes loadDescriptions' texts (the port's, for gameRelease). */
void freeDescriptions();
/* Save and load the game in saveSlot, when the Silencer is the controlled
 * NPC. */
void saveGame();
/* saveGame's objects as the game is, without its work before and after them
 * and without the slot's description: the port's, for the pass dump. */
void writeGameState(Flex *flex);
void loadGame();
/* The port's: whether loadGame can take saveSlot's file, or there is none
 * (whose load does nothing, as the original's): its bytes as kernel.h's
 * kernelSaveLoadable has them. A file loadGame cannot take, damaged or of
 * another version, ended the run, after the load had cleared the game
 * being played; the original read its bytes as they were. */
[[nodiscard]] bool savedGameLoadable();
/* The port's: the box that says saveSlot's game cannot be loaded, the
 * original's MessageBoxGump in English as the game's own boxes are; then,
 * once it is closed, the caller's way back (the game going on, the start
 * menu again). */
[[nodiscard]] NewGump *newSavedGameRefusal();
template <typename Then> void refuseSavedGame(Then then)
{
    openModalGump(newSavedGameRefusal(), static_cast<u32>(-1),
                  [then = std::move(then)](s32 /*result*/) mutable { then(); });
}
char *saveDescription(i16 slot);
void setSaveDescription(i16 slot, char *text);
/* The script's load and save (port/system/script.h, which calls them
 * through what savegame.cpp gives host.h's hostScriptActions): the game in
 * slot loaded as the main menu's LOAD loads it (the effects stopped, the
 * screen and the palette again after), or saved. They come at a pass's end,
 * between the processes' runs; a load and a save at one time go one after
 * the other, so that the save holds the state just loaded. */
void scriptLoadGame(i16 slot);
void scriptSaveGame(i16 slot);
#endif
