/* No Regret's values of the system object (common/src/world/crusys.cpp). */
#ifndef PORT_CRUSYS_GAME_H
#define PORT_CRUSYS_GAME_H
#include <conio.h>

/* The title showTitle prints before the version (CruSystem::CruSystem). */
inline constexpr const char *GAME_TITLE = "CRUSADER:  No Regret  ";
/* checkInstall's message without the CD (FIXED.DAT on gameDrive). */
inline constexpr const char *MISSING_CD_MESSAGE = "\n\rCANNOT FIND THE CRUSADER: No Regret CD.\n\r";
/* What -? prints: nothing in No Regret. */
inline constexpr const char *HELP_MESSAGE = nullptr;
/* An option that starts with "-laurie" turns the cheats on (checkCheatOption):
 * not in No Regret. */
inline constexpr bool CHEAT_OPTION = false;
/* The start's title bar and the copyright line's colours, and the title's
 * space between its name and its version (showTitle). */
inline constexpr u8 TITLE_BAR_COLOUR = RED;
inline constexpr u8 COPYRIGHT_COLOUR = RED;
inline constexpr bool TITLE_SPACED = false;
/* The line above the copyright (showTitle). */
inline constexpr bool PRODUCTION_LINE = true;
inline constexpr const char *PRODUCTION = "A Loose Cannon Production";
/* The last line, without and with the cheats (shutdown). */
inline constexpr const char *GOODBYE = "No pity.  No mercy.  No regret.\n\r";
inline constexpr const char *GOODBYE_WITH_CHEATS = "No pity.  No mercy.  No regret.\n\r";
/* shutdown does nothing before startup got through (cruStarted), ends the
 * fades and drops the keys. */
inline constexpr bool SHUTDOWN_AFTER_START = true;
inline constexpr bool SHUTDOWN_ENDS_FADES = true;
/* The start checks the program's files and greets at Christmas before the
 * kernel starts (startup). */
inline constexpr bool START_CHECKS_FILES = true;
/* A step of the loading bar. */
inline constexpr const char *LOADING_STEP = "\xb1";
/* The anchor process moves the controlled NPC (AnchorProcess). */
inline constexpr bool ANCHOR_PROCESS = true;
/* The version the title screen and WORLD.C's Ctrl-V show (gameVersion). */
inline constexpr const char *GAME_VERSION = "V1.06";
#endif
