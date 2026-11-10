/* The one program's game values: the run-time value headers beside this
 * (written by port/tools/make_unified_values.py) declare a variable for
 * each value both games have; the game the program plays fills them before
 * anything else starts (values.cpp). */
#ifndef PORT_VALUES_H
#define PORT_VALUES_H

/* Fill the values with No Regret's or No Remorse's (values_regret.cpp,
 * values_remorse.cpp). */
void useRegretValues();
void useRemorseValues();
/* The game the program plays is No Remorse (values.cpp; else No Regret):
 * the functions each game's generated files define go to its (dispatch.cpp). */
extern bool playingRemorse;
#endif
