/* No Remorse's save tables (common/src/world/savefields.h): the generated
 * savefields.cpp's (generated) and control_game.cpp's. */
#ifndef PORT_SAVEFIELDS_GAME_H
#define PORT_SAVEFIELDS_GAME_H

/* The names the far pointers point at (SaveName). */
extern const SaveName SAVE_NAMES[];
extern const u16 SAVE_NAME_COUNT;
/* The classes a saved process can be (SaveClass). */
extern const SaveClass SAVE_CLASSES[];
extern const u16 SAVE_CLASS_COUNT;
/* What the original's saves hold where its load sets the values again, as
 * the game's original has them: its DS, which the far pointers to the
 * processes' names carry; the unit file's selector, where the usecode's
 * class names are; its stack's selector, where a usecode process's result
 * points; the selector of the first tactic (a tactic's is 8 times its
 * number on); BiosTimer's handler; SubscriberList's vtable. */
extern const u16 ORIGINAL_DS;
extern const u16 ORIGINAL_UNIT_FILE;
extern const u16 ORIGINAL_STACK;
extern const u16 ORIGINAL_FIRST_TACTIC;
extern const u32 ORIGINAL_BIOS_VECTOR;
extern const u16 SUBSCRIBER_LIST_VTABLE;
/* Controller's block of globals (control_game.cpp, 75 bytes). */
extern const SaveGlobal CONTROLLER_GLOBALS[];
extern const u16 CONTROLLER_GLOBAL_COUNT;
#endif
