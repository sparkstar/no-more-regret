/* Written by port/tools/make_unified_values.py from regret/src/world/savefields_game.h and
 * remorse/src/world/savefields_game.h: the one program's run-time values; do not edit. */

#ifndef PORT_SAVEFIELDS_GAME_H
#define PORT_SAVEFIELDS_GAME_H
#include <span>

extern std::span<const SaveName> SAVE_NAMES;
extern u16 SAVE_NAME_COUNT;
extern std::span<const SaveClass> SAVE_CLASSES;
extern u16 SAVE_CLASS_COUNT;
extern u16 ORIGINAL_DS;
extern u16 ORIGINAL_UNIT_FILE;
extern u16 ORIGINAL_STACK;
extern u16 ORIGINAL_FIRST_TACTIC;
extern u32 ORIGINAL_BIOS_VECTOR;
extern u16 SUBSCRIBER_LIST_VTABLE;
extern std::span<const SaveGlobal> CONTROLLER_GLOBALS;
extern u16 CONTROLLER_GLOBAL_COUNT;
#endif
