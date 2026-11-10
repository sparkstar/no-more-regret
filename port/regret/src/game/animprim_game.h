/* No Regret's values of the animation processes (common/src/game/animprim.h). */
#ifndef PORT_ANIMPRIM_GAME_H
#define PORT_ANIMPRIM_GAME_H
#include "common.h"

/* AnimPrimitive's BCC size. */
inline constexpr u16 ANIM_PRIMITIVE_SIZE = 0x5e;
/* A step onto nothing (AnimPrimitive::canStep): No Regret's controlled NPC falls
 * there, after a landing, only when its edge holds it on no side, else it
 * does not step; another NPC falls. */
inline constexpr bool LEDGE_HOLDS_CONTROLLED_NPC = true;
/* AnimPrimitive::init's search beside a blocked move of the controlled NPC:
 * No Regret's tries the slides, the kneeling moves and the roll forward
 * too, waits on the anchor process, scores the cells it gets to and goes to
 * the best; No Remorse's waits on a test anchor of its own and steps there
 * by a sweep. The two searches as a whole. */
inline constexpr bool BLOCKED_MOVE_SEARCH = true;
#endif
