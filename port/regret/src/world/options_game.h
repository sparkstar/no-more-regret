/* No Regret's values of crusader.cfg (common/src/world/options.cpp). */
#ifndef PORT_OPTIONS_GAME_H
#define PORT_OPTIONS_GAME_H
#include "common.h"

/* The end of crusader.cfg's lines and of the messages about it. */
inline constexpr const char *CFG_LINE_END = "\r\n";
/* crusader.cfg as GOG's installer writes it (No Regret 1.06), which readOptions takes
 * without the file. */
inline constexpr const char *GOG_OPTIONS =
    "irq=5\ndma=5\nport=0x220\nsoundcard=3\nsound=on\nmusic=on\nmoviehicolor=true\n"
    "bigvideo=true\nsubtitles=true\ncontrol=0\ncdletter=D\nflicpath=flics\n";
/* The movies' keys: bigvideo doubles the movies (true, 1 or yes),
 * moviehicolor and lowresmovies: No Regret's; No Remorse's bigvideo and
 * videosize pick the flic's mode, and it writes newvideo=true. */
inline constexpr bool XAN_MOVIE_OPTIONS = true;
/* noweasel, crupath and fullinstall, and the installation's message after
 * the file: No Remorse's; No Regret's keeps the swap there instead. */
inline constexpr bool INSTALL_OPTIONS = false;
/* violence: No Regret's. */
inline constexpr bool VIOLENCE_OPTION = true;
/* frameskip's word is stored over a byte, its high byte going to
 * limitBlasts: No Remorse's. */
inline constexpr bool FRAME_SKIP_SPILLS = false;
/* The tunes read (1 to one less) and written; the message names the
 * tune's mission: No Remorse's. */
inline constexpr i16 TUNES_READ = 22;
inline constexpr i16 TUNES_WRITTEN = 22;
inline constexpr bool TUNES_BY_MISSION = false;
#endif
