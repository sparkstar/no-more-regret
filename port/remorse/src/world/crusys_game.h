/* No Remorse's values of the system object (common/src/world/crusys.cpp). */
#ifndef PORT_CRUSYS_GAME_H
#define PORT_CRUSYS_GAME_H
#include <conio.h>

/* The title showTitle prints before the version (CruSystem::CruSystem). */
inline constexpr const char *GAME_TITLE = "CRUSADER:  No Remorse -";
/* checkInstall's message without the CD: No Remorse's does not look for it. */
inline constexpr const char *MISSING_CD_MESSAGE = nullptr;
/* What -? prints. */
inline constexpr const char *HELP_MESSAGE = "You DO need help!\n\r";
/* An option that starts with "-laurie" turns the cheats on (checkCheatOption). */
inline constexpr bool CHEAT_OPTION = true;
/* The start's title bar and the copyright line's colours, and the title's
 * space between its name and its version (showTitle). */
inline constexpr u8 TITLE_BAR_COLOUR = BLUE;
inline constexpr u8 COPYRIGHT_COLOUR = YELLOW;
inline constexpr bool TITLE_SPACED = true;
/* The line above the copyright (showTitle): No Regret's. */
inline constexpr bool PRODUCTION_LINE = false;
inline constexpr const char *PRODUCTION = "";
/* The last line, without and with the cheats (shutdown). */
inline constexpr const char *GOODBYE = "CRUSADER:  No Remorse\n\r";
inline constexpr const char *GOODBYE_WITH_CHEATS = "No pity.  No mercy.  No remorse.\n\r";
/* shutdown does nothing before startup got through (cruStarted), ends the
 * fades and drops the keys: No Regret's. */
inline constexpr bool SHUTDOWN_AFTER_START = false;
inline constexpr bool SHUTDOWN_ENDS_FADES = false;
/* The start checks the program's files and greets at Christmas before the
 * kernel starts (startup): No Regret's; No Remorse's starts the kernel after the first loading step. */
inline constexpr bool START_CHECKS_FILES = false;
/* A step of the loading bar. */
inline constexpr const char *LOADING_STEP = ".";
/* The anchor process moves the controlled NPC (AnchorProcess): No Regret's. */
inline constexpr bool ANCHOR_PROCESS = false;
/* The version the title screen and WORLD.C's Ctrl-V show (gameVersion). */
inline constexpr const char *GAME_VERSION = "V1.21";
#endif
