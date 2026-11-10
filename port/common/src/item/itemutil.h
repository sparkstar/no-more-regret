/* Code20 (1098, file name unknown, among the ..\ITEM modules): containers,
 * what they hold and what fits in them, and the screen place of a map
 * point. */
#ifndef PORT_ITEMUTIL_H
#define PORT_ITEMUTIL_H
#include "common.h"
#include "item.h"

class Container : public Item {
public:
    explicit Container(u16 itemId) : Item(itemId) {}
    /* item is in the container or in a container in it (1098:0000). */
    bool contains(u16 item);
    /* item may go in the container (1098:0094). */
    bool canAdd(u16 item);
    /* count items of shape fit in the container (1098:0315). */
    bool canHold(u16 shape, i16 count);
    /* The weight check of canAdd and canHold. It finds the outermost
     * container and checks nothing: it always returns 0 (1098:0486). */
    bool tooHeavy(i16 weight);
    /* Returns 0. Nothing calls it (1098:04fe). */
    u16 unknown04fe();
    /* Moves the contents to the container's container, or to its place
     * on the map (1098:0505). */
    void removeContents();
    void destroyContents(); /* 1098:05cb */
    /* The weight of the contents, theirs included (1098:0603). */
    i16 contentsWeight();
    /* The volume the item takes: its own, or its contents' when they
     * take more (1098:0678). */
    i16 totalVolume();
    /* The volume the contents take (1098:06e5). */
    i16 contentsVolume();
    /* The number of items in the container, theirs included (1098:0734). */
    i16 contentsCount();
};
ASSERT_SIZE(Container, 2);

/* The screen point of map point x,y (1098:07bd). */
void mapToScreen(u16 x, u16 y, i16 *screenX, i16 *screenY);
#endif
