/* No Remorse's values of the dragged item (common/src/gump/dragitem.cpp). */
#ifndef PORT_DRAGITEM_GAME_H
#define PORT_DRAGITEM_GAME_H

/* No Remorse's controlled NPC holds the item in slot 7: picking an item up
 * and dropping one into a container empty the slot, and an item dropped on
 * an NPC moves it into the pack, slot 6 (DraggedItem). */
inline constexpr bool DRAG_USES_SLOT_7 = true;
#endif
