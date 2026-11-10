/* No Remorse's values of crusader.cfg (common/src/world/options.cpp). */
#ifndef PORT_OPTIONS_GAME_H
#define PORT_OPTIONS_GAME_H
#include "common.h"

/* The end of crusader.cfg's lines and of the messages about it (No Regret's \r\n). */
inline constexpr const char *CFG_LINE_END = "\n\r";
/* crusader.cfg as GOG's installer writes it (No Remorse 1.21), which
 * readOptions takes without the file. */
inline constexpr const char *GOG_OPTIONS =
    "irq=5\ndma=1\nport=0x220\nsoundcard=3\nsound=on\nmusic=on\nfullinstall=true\n"
    "bigvideo=true\nsubtitles=true\ncdletter=D\nflicpath=\\flics\nnewvideo=true\n";
/* The movies' keys: bigvideo doubles the movies (true, 1 or yes),
 * moviehicolor and lowresmovies: No Regret's; No Remorse's bigvideo and
 * videosize pick the flic's mode, and it writes newvideo=true. */
inline constexpr bool XAN_MOVIE_OPTIONS = false;
/* noweasel, crupath and fullinstall, and the installation's message after
 * the file: No Remorse's; No Regret's keeps the swap there instead. */
inline constexpr bool INSTALL_OPTIONS = true;
/* violence: No Regret's. */
inline constexpr bool VIOLENCE_OPTION = false;
/* frameskip's word is stored over a byte, its high byte going to
 * limitBlasts: No Remorse's. */
inline constexpr bool FRAME_SKIP_SPILLS = true;
/* The tunes read (1 to one less) and written; the message names the
 * tune's mission: No Remorse's. */
inline constexpr i16 TUNES_READ = 17;
inline constexpr i16 TUNES_WRITTEN = 18;
inline constexpr bool TUNES_BY_MISSION = true;
#endif
