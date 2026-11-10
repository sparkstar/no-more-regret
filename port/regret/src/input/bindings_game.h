/* No Regret's actions' inputs (common/src/input/bindings.h). */
#ifndef PORT_BINDINGS_GAME_H
#define PORT_BINDINGS_GAME_H

/* The game's actions with their original inputs, in the order enhanced.cfg
 * writes them (bindings_game.cpp). */
extern const ActionDefault GAME_ACTION_DEFAULTS[];
extern const std::size_t GAME_ACTION_DEFAULT_COUNT;
/* The modern layout's inputs of the actions it changes (WASD, the mouse to
 * fire and turn, its buttons and the letters around WASD for the rest), the
 * others the original's. */
extern const ActionDefault GAME_MODERN_BINDINGS[];
extern const std::size_t GAME_MODERN_BINDING_COUNT;
#endif
