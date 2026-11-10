/* The port's enhanced.cfg: the port's own settings in a file of their own
 * beside crusader.cfg, which both originals read and write: the language of
 * the game's text (language.h), the sound's depth in place of crusader.cfg's
 * DMA channel's (asylum.h's ASS_SetDepth; none: the channel's), the window
 * over the whole screen (host.h's hostSetFullscreen), the experimental wide
 * screen (screen.h's screenSetWide), the game speed (clock.h's
 * clockSetMachineSpeed), the keys' preset, whether the mouse rolls and each
 * action's inputs (bindings.h), whether the port's debug features are on
 * (which the system layer reads itself as the run starts: host.h's
 * hostDebugFeatures), and those to come. A player may share crusader.cfg
 * with an original, whose options gump writes it again without lines it does
 * not know. readOptions reads enhanced.cfg after crusader.cfg, and
 * writeOptions writes it after crusader.cfg. */
#ifndef PORT_ENHANCED_H
#define PORT_ENHANCED_H
#include "common.h"

#include <string>
#include <vector>

/* The file's name in the game's directory, both games'. */
inline constexpr const char *ENHANCED_FILE = "enhanced.cfg";

/* The file's text: a comment, game=, language=, sound_bits= when the
 * player chose the depth, fullscreen=1 when the window is so, widescreen=
 * when it is on, game_speed= always, debug_feature=true when the file had
 * it, then bindingsText's keys= and bind_ lines, each ending in CR LF. */
[[nodiscard]] std::string enhancedText();
/* The settings from such a text: language=, sound_bits= (8 or 16, another
 * the DMA channel's), fullscreen= (1, another a window), widescreen= (1 the
 * sides, 2 dimmed, another off), game_speed= (slow, normal or fast, another
 * leaves it), debug_feature= (true or 1, another off: kept for the file
 * only) and readBindingsText's lines; comments and other keys are passed
 * over.
 * Returns the lines it cannot read, which it leaves. */
[[nodiscard]] std::vector<std::string> readEnhancedText(const std::string &text);

/* The sound's depth the player chose (the settings' SOUND, sound_bits=):
 * 8 or 16, set at once (asylum.h's ASS_SetDepth) and kept in the file;
 * another none, the DMA channel's again at the sound's next start. */
void chooseSoundBits(u16 bits);
/* The settings from enhanced.cfg when it is there, the lines it cannot read
 * told (debugPrintf); the window's changes of the full screen written back
 * (host.h's hostFullscreenAction). */
void readEnhancedOptions();
/* enhanced.cfg written again with the settings. */
void writeEnhancedOptions();
#endif
