/* No Remorse's values of the world (common/src/world/world.cpp). */
#ifndef PORT_WORLD_GAME_H
#define PORT_WORLD_GAME_H
#include <array>
#include "common.h"
#include "dtable.h"
#include "rect.h"

/* The items the controlled NPC keeps one of: another it picks up is
 * destroyed (pickUp): the shields. */
inline constexpr std::array<u16, 3> UNSTACKED_SHAPES = {SHAPE_IONIC_SHIELD, SHAPE_PLASMA_SHIELD, SHAPE_GRAVITON_SHIELD};
/* The sound of an item chosen with the keys (WorldGump::selectItems): it starts again over the last one (No Regret's
 * waits). */
inline constexpr u16 SELECT_SOUND = 0xb0;
inline constexpr bool SELECT_SOUND_WAITS = false;
/* The credits' process ends tidily (No Regret's: the status drawn again, the
 * screen blacked, the keys dropped and the process ended before the start
 * menu); No Remorse's goes straight to the menu (CreditsProcess::run). */
inline constexpr bool CREDITS_END_TIDILY = false;
/* thawWorld draws the sprites again first: No Regret's. */
inline constexpr bool THAW_REDRAWS_SPRITES = false;
/* thawWorld ends the teleporting. */
inline constexpr bool THAW_ENDS_TELEPORT = true;
/* callIntrinsics, which nothing calls, opens the weasel's shop too, and the
 * text it shows. */
inline constexpr bool INTRINSICS_SHOP = true;
inline constexpr const char *INTRINSICS_TEXT = "FART  ...TRY... -laurie (Have fun, Jely)";
/* The help's fifth page plays one of five sounds now and then, at 1 in
 * 8000 a run (HelpGump::execute). */
inline constexpr bool HELP_PAGE_SOUNDS = true;
/* The weapon keys work only while the Silencer is controlled (nextWeapon, previousWeapon). */
inline constexpr bool WEAPON_KEYS_SILENCER_ONLY = true;
/* previousWeapon puts the controlled NPC in the first frame of its stance, as
 * nextWeapon does: No Regret's (No Remorse's does not; kept). */
inline constexpr bool PREVIOUS_WEAPON_SETS_STANCE = false;
/* The help: its pages' pictures in static (none for a page that has none),
 * the page past the last one, which closes it (No Regret's goes back to the first), and No Remorse's its
 * pointer and two shape buttons, or No Regret's sound with the pointer hidden (HelpGump). */
inline constexpr std::array<const char *, 5> HELP_FILES = {"help1.dat", "help2.dat", nullptr, nullptr, "help3.dat"};
inline constexpr i16 HELP_PAGES = 2;
inline constexpr bool HELP_PAGES_WRAP = false;
inline constexpr bool HELP_BUTTONS = true;
inline constexpr u16 HELP_SOUND = 0;
/* The help's keys:  the arrows turn the pages, Escape and x close it, and e
 * then three - show the hidden page
 * (HelpGump::handleKeyboard). */
inline constexpr bool HELP_ARROW_KEYS = true;
/* The sounds of the quit's, the quick load's and the quick save's questions,
 * and of the version (WorldGump::handleKeyboard). */
inline constexpr u16 QUIT_SOUND = 0x109;
inline constexpr u16 QUICK_LOAD_SOUND = 0x107;
inline constexpr u16 QUICK_SAVE_SOUND = 0x108;
inline constexpr u16 VERSION_SOUND = 0x103;
/* The places of the questions' yes and no and of the quick load's and save's
 * notice. */
inline constexpr Point QUESTION_YES = Point(0x37, 0x2f);
inline constexpr Point QUESTION_NO = Point(0x87, 0x2f);
inline constexpr Point NOTICE_PLACE = Point(0x5b, 0x2f);
/* The shapes the detonator sets off around the controlled NPC. */
inline constexpr std::array<u16, 1> DETONATED_SHAPES = {{SHAPE_DET_PAC}};
/* The keys of the items (ITEM_KEYS, world.h's ItemKey). */
inline constexpr std::array<ItemKey, 2> ITEM_KEYS = {
    {{Action::USE_ENERGY_CUBE, SHAPE_ENERGY_CUBE, true}, {Action::USE_MEDKIT, SHAPE_MEDICAL_KIT, true}}};
/* The items the use key only uses, which have no charges. */
inline constexpr std::array<u16, 5> UNCHARGED_ITEMS = {
    {SHAPE_DATA_LINK, SHAPE_INHIBITOR, SHAPE_GRAVITON_SHIELD, SHAPE_PLASMA_SHIELD, SHAPE_IONIC_SHIELD}};
/* An item key and the use key use a charged item before its charge goes: No
 * Remorse's; No Regret's uses it after, and an item key at the last charge
 * moves to the next inventory item before the item goes (No Remorse's
 * after). */
inline constexpr bool ITEM_USED_FIRST = true;
/* The fragments the version's memory status lists. */
inline constexpr i16 VERSION_FRAGMENTS = 4;
/* The weapon the drop key keeps on difficulty 1 and 2. */
inline constexpr u16 KEPT_WEAPON = SHAPE_BA_40;
/* The targeting reticle's key turns it on and off with a message: No
 * Remorse's. */
inline constexpr bool TARGETING_KEY = true;
/* The position key shows where the controlled NPC is: No Remorse's. */
inline constexpr bool POSITION_KEY = true;
/* With the cheats, T uses the controlled NPC, ~ turns the cheat keys on and
 * off and Ctrl+Q the CD transfer display: No Remorse's. */
inline constexpr bool MORE_DEBUG_KEYS = true;
/* WorldGump::handleKeyboard's rules: No Regret's (true) Escape and F1 both
 * clear the selection and flush the keys, F1 opens the menu while the
 * Silencer is controlled, Escape flushes the keys in a selection, quick load
 * asks only when slot 13 is there, also with the Silencer dead, and applies
 * the palette after, load works with the Silencer dead and needs canAct, as
 * the options and help do, the next song ends the Christmas songs and goes
 * round 22 of them from 1, and the version shows the map's place and the
 * memory's sharing; No Remorse's (false) Escape ends a target's waiting
 * (WORLD_TARGETING) and the menu opens while the Silencer was controlled when
 * Escape was pressed (unknown00a2), the options and the help work in the
 * stasis, and the next song skips 11h and goes round at 14h. */
inline constexpr bool WORLD_KEYS_NO_REGRET = false;
#endif
