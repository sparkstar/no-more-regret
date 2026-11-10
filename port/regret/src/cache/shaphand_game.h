/* No Regret's values of the shapes' cache (common/src/cache/shaphand.cpp). */
#ifndef PORT_SHAPHAND_GAME_H
#define PORT_SHAPHAND_GAME_H

/* The shapes' handler copies a shape missing from the shape file out of the
 * swap file on the CD (a ShapeHandler): No Remorse's; No Regret's is a
 * ShapeCacheHandler. All shapes are in the shape file at the start
 * (allShapes): No Regret's. */
inline constexpr bool SHAPES_FROM_SWAP = false;
inline constexpr u8 ALL_SHAPES = 1;
#endif
