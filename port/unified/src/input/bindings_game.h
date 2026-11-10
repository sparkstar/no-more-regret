/* Written by port/tools/make_unified_values.py from regret/src/input/bindings_game.h and
 * remorse/src/input/bindings_game.h: the one program's run-time values; do not edit. */

#ifndef PORT_BINDINGS_GAME_H
#define PORT_BINDINGS_GAME_H
#include <span>

extern std::span<const ActionDefault> GAME_ACTION_DEFAULTS;
extern std::size_t GAME_ACTION_DEFAULT_COUNT;
extern std::span<const ActionDefault> GAME_MODERN_BINDINGS;
extern std::size_t GAME_MODERN_BINDING_COUNT;
#endif
