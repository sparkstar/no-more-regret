/* DLIST.C (Code54): the display list. See dlist.h. */
#include "dlist.h"
#include "gamestate.h"
#include "camera.h"
#include "dlitem.h"
#include "gametime.h"
#include "item.h"
#include "itemdata.h"
#include "random.h"
#include "shaphand.h"
#include "type.h"
#include "screen.h"

void changeNodesBefore(u16 node);
bool orderNodes(u16 first, u16 second);
bool orderNodes2(u16 first, u16 second);
void initEdges();
void closeEdges();

Rect &screenRect = gameGlobal<Rect>(0, 0, SCREEN_RIGHT, SCREEN_BOTTOM);
u8 &relink = gameGlobal<u8>(0);
/* The animation time of the last animateItems. */
s32 &lastAnimation = gameGlobal<s32>(0);
u32 *&screenItems = gameGlobal<u32 *>(nullptr);
u32 *&movedItems = gameGlobal<u32 *>(nullptr);
i16 &viewX = gameGlobal<i16>(0);
i16 &viewY = gameGlobal<i16>(0);
/* The free nodes and edges and their pools. */
u16 &freeNodes = gameGlobal<u16>(0);
u16 &freeEdges = gameGlobal<u16>(0);
u8 *&nodePool = gameGlobal<u8 *>(nullptr);
u8 *&edgePool = gameGlobal<u8 *>(nullptr);
extern const u32 NODE_SIZE = sizeof(DL_ItemNode);
i16 &freeNodeCount = gameGlobal<i16>(0);
i16 &freeEdgeCount = gameGlobal<i16>(0);
NodeList &changedNodes = gameGlobal<NodeList>();
NodeList &drawNodes = gameGlobal<NodeList>();
Rect &drawArea = gameGlobal<Rect>();
Viewport &drawView = gameGlobal<Viewport>();
u16 (&gridCells)[GRID_ROWS][GRID_COLUMNS] = gameGlobal<u16[GRID_ROWS][GRID_COLUMNS]>();

void clipRect(Rect *rect, Rect *area)
{
    if (rect->left < area->left) {
        rect->left = area->left;
    }
    if (rect->top < area->top) {
        rect->top = area->top;
    }
    if (rect->right > area->right) {
        rect->right = area->right;
    }
    if (rect->bottom > area->bottom) {
        rect->bottom = area->bottom;
    }
}

void mergeRect(Rect *rect, Rect *other)
{
    if (rect->left > other->left) {
        rect->left = other->left;
    }
    if (rect->top > other->top) {
        rect->top = other->top;
    }
    if (rect->right < other->right) {
        rect->right = other->right;
    }
    if (rect->bottom < other->bottom) {
        rect->bottom = other->bottom;
    }
}

void *DL_Node::operator new(size_t /*unused*/) noexcept
{
    DL_Node *node = nullptr;

    if (freeNodes != 0U) {
        node = nodeOf(freeNodes);
        freeNodes = node->next;
        freeNodeCount--;
    }
    return node;
}

void DL_Node::operator delete(void *memory)
{
    DL_Node *node = static_cast<DL_Node *>(memory);

    node->next = freeNodes;
    freeNodes = nodeOffset(node);
    freeNodeCount++;
}

namespace {
void initNodes()
{
    if (nodePool == nullptr) {
        nodePool = new u8[NODE_POOL * sizeof(DL_ItemNode)];
    }
    /* The pool starts at offset 0, as the original's heap blocks do; offset 0
     * means no node, so the first slot is not used. */
    u16 node = 0;
    freeNodeCount = NODE_POOL;
    freeNodes = NODE_SLOT;
    freeNodeCount--;
    for (i16 i = 0; i < NODE_POOL - 1; i++) {
        nodeOf(node)->next = node + NODE_SLOT;
        node += NODE_SLOT;
    }
    nodeOf(node)->next = 0;
}

void closeNodes()
{
    if (nodePool != nullptr) {
        delete[] nodePool;
        nodePool = nullptr;
    }
}
}

u16 NodeList::pop()
{
    if (first != 0U) {
        DL_Node *node = nodeOf(first);
        first = node->next;
        if (first == 0U) {
            last = first;
        }
        node->unlink();
        return nodeOffset(node);
    }
    return 0;
}

void NodeList::append(u16 node)
{
    if (node != 0U) {
        nodeOf(node)->insert(last, 0);
        if (first == 0U) {
            first = node;
        }
        last = node;
    }
}

void NodeList::remove(u16 node)
{
    if (first == node) {
        first = nodeOf(node)->next;
    }
    if (last == node) {
        last = nodeOf(node)->previous;
    }
    nodeOf(node)->unlink();
}

void drawChanged()
{
    Rect area(0, 0, 0, 0);

    area.left = drawArea.left + scrollX;
    area.top = drawArea.top + scrollY;
    area.right = drawArea.right + scrollX;
    area.bottom = drawArea.bottom + scrollY;
    u16 node;
    u16 last = 0;
    while (changedNodes.first != 0U) {
        node = changedNodes.pop();
        DL_Node *entry = nodeOf(node);
        if (last == node || entry->readyToDraw()) {
            entry->flags &= ~NODE_CHANGED;
            last = 0;
            if (entry->bounds.touches(area)) {
                drawNodes.append(node);
                entry->flags |= NODE_DRAWN;
                changeNodesBefore(node);
            }
        } else {
            if (last == 0U) {
                last = node;
            }
            changedNodes.add(node);
        }
    }
    while (drawNodes.first != 0U) {
        node = drawNodes.pop();
        DL_Node *entry = nodeOf(node);
        entry->flags &= ~NODE_DRAWN;
        if (relink != 0U) {
            changedNodes.add(node);
            entry->flags |= NODE_CHANGED;
        } else {
            entry->draw();
        }
    }
    if (relink != 0U) {
        relink = 0;
        drawChanged();
    }
}

/* Puts the nodes before node on changedNodes. */
void changeNodesBefore(u16 node)
{
    DL_Node *entry = nodeOf(node);

    if (entry->kind != NODE_GRID) {
        u16 edge = entry->edgesIn;
        while (edge != 0U) {
            const u16 before = edgeOf(edge)->first;
            if ((nodeOf(before)->flags & (NODE_CHANGED | NODE_DRAWN)) == 0) {
                changedNodes.add(before);
                nodeOf(before)->flags |= NODE_CHANGED;
            }
            edge = edgeOf(edge)->nextIn;
        }
    }
}

/* Draws area of the map again. */
/* UNCALLED: nothing calls it. */
void redrawArea(Rect area)
{
    area.left -= scrollX;
    area.top -= scrollY;
    area.right -= scrollX;
    area.bottom -= scrollY;
    if (area.touches(screenRect)) {
        clipRect(&area, &screenRect);
        drawArea.left = area.left;
        drawArea.top = area.top;
        drawArea.right = area.right;
        drawArea.bottom = area.bottom;
        drawView = *drawViewport;
        drawView.setBounds(Rect(area.left, area.top, area.right, area.bottom));
        drawChanged();
    }
}

namespace {
/* An edge from first to second; 0 when the pool is empty. */
bool linkNodes(u16 first, u16 second)
{
    if (first != second) {
        const u16 edge = edgeOffset(new DL_Edge(first, second));
        if (edge != 0U) {
            DL_Node *from = nodeOf(first);
            DL_Node *to = nodeOf(second);
            from->edgesOut = edge;
            to->edgesIn = edge;
            if (from->covers != 0U) {
                to->flags |= NODE_COVERED;
            }
        } else {
            return false;
        }
    }
    return true;
}
}

void markChanged(u16 node)
{
    if (node != 0U) {
        nodeOf(node)->flags |= NODE_CHANGED;
        changedNodes.add(node);
    }
}

void unlinkNode(u16 node)
{
    DL_Node *entry = nodeOf(node);
    u16 edge = entry->edgesOut;

    while (edge != 0U) {
        const u16 after = edgeOf(edge)->second;
        if (nodeOf(after)->changed() == 0U) {
            changedNodes.add(after);
            nodeOf(after)->flags |= NODE_CHANGED;
        }
        edgeOf(edge)->unlink();
        const u16 done = edge;
        edge = edgeOf(edge)->nextOut;
        delete edgeOf(done);
    }
    entry->edgesOut = 0;
    edge = entry->edgesIn;
    while (edge != 0U) {
        edgeOf(edge)->unlink();
        const u16 done = edge;
        edge = edgeOf(edge)->nextIn;
        delete edgeOf(done);
    }
    entry->edgesIn = 0;
    if ((entry->flags & NODE_CHANGED) != 0) {
        entry->flags &= ~NODE_CHANGED;
        changedNodes.remove(node);
    }
}

u8 linkNewNode(u16 node)
{
    const bool result = true;
    Rect bounds(0, 0, 0, 0);
    i16 left;
    i16 top;
    i16 right;
    i16 bottom;
    i16 column;
    i16 row;
    u16 cell;
    u16 marked;
    u16 before;
    DL_Node *entry;
    DL_Node *other;

    entry = nodeOf(node);
    bounds = entry->bounds;
    left = static_cast<i16>(bounds.left - viewX) >> 7;
    top = static_cast<i16>(bounds.top - viewY) >> 7;
    right = static_cast<i16>(bounds.right - viewX) >> 7;
    bottom = static_cast<i16>(bounds.bottom - viewY) >> 7;
    marked = 0;
    if (left < GRID_COLUMNS && top < GRID_ROWS && right >= 0 && bottom >= 0) {
        if (left < 0) {
            left = 0;
        }
        if (top < 0) {
            top = 0;
        }
        if (right >= GRID_COLUMNS) {
            right = GRID_COLUMNS - 1;
        }
        if (bottom >= GRID_ROWS) {
            bottom = GRID_ROWS - 1;
        }
        for (row = top; row <= bottom; row++) {
            for (column = left; column <= right; column++) {
                cell = gridCells[row][column];
                for (u16 edge = nodeOf(cell)->edgesIn; edge != 0U; edge = edgeOf(edge)->nextIn) {
                    before = edgeOf(edge)->first;
                    other = nodeOf(before);
                    if ((other->flags & NODE_MARKED) == 0) {
                        if (entry->bounds.touches(other->bounds)) {
                            if (!orderNodes(node, before)) {
                                return 0;
                            }
                        }
                        other->flags |= NODE_MARKED;
                        other->marked = marked;
                        marked = before;
                    }
                }
                if (!orderNodes(node, cell)) {
                    return 0;
                }
            }
        }
        while (marked != 0U) {
            before = marked;
            marked = nodeOf(before)->marked;
            nodeOf(before)->flags &= ~NODE_MARKED;
        }
    }
    return static_cast<u8>(result);
}

bool orderNodes(u16 first, u16 second)
{
    bool result = true;
    DL_Node *a = nodeOf(first);
    DL_Node *b = nodeOf(second);

    if (second != first) {
        i16 order;
        if (a->kind < b->kind) {
            order = 2;
        } else if (a->kind > b->kind) {
            order = 1;
        } else {
            order = (static_cast<DL_ItemNode *>(a))->order(second);
        }
        if (order == 1) {
            if (!linkNodes(first, second)) {
                result = false;
            }
        } else if (order == 2) {
            if (!linkNodes(second, first)) {
                result = false;
            }
        }
    }
    return result;
}

u8 relinkNode(u16 node)
{
    const bool result = true;
    Rect bounds(0, 0, 0, 0);
    i16 left;
    i16 top;
    i16 right;
    i16 bottom;
    i16 column;
    i16 row;
    u16 cell;
    u16 marked;
    u16 over;
    DL_Node *cellNode;
    DL_Node *other;
    DL_Node *entry;
    DL_Node *before;

    marked = 0;
    entry = nodeOf(node);
    bounds = entry->bounds;
    left = static_cast<i16>(bounds.left - viewX) >> 7;
    top = static_cast<i16>(bounds.top - viewY) >> 7;
    right = static_cast<i16>(bounds.right - viewX) >> 7;
    bottom = static_cast<i16>(bounds.bottom - viewY) >> 7;
    if (left < GRID_COLUMNS && top < GRID_ROWS && right >= 0 && bottom >= 0) {
        u16 edge;
        for (edge = entry->edgesIn; edge != 0U; edge = edgeOf(edge)->nextIn) {
            const u16 linked = edgeOf(edge)->first;
            other = nodeOf(linked);
            other->flags |= NODE_MARKED;
            other->marked = marked;
            marked = linked;
        }
        for (edge = entry->edgesOut; edge != 0U; edge = edgeOf(edge)->nextOut) {
            const u16 linked = edgeOf(edge)->second;
            other = nodeOf(linked);
            other->flags |= NODE_MARKED;
            other->marked = marked;
            marked = linked;
        }
        if (left < 0) {
            left = 0;
        }
        if (top < 0) {
            top = 0;
        }
        if (right >= GRID_COLUMNS) {
            right = GRID_COLUMNS - 1;
        }
        if (bottom >= GRID_ROWS) {
            bottom = GRID_ROWS - 1;
        }
        for (row = top; row <= bottom; row++) {
            for (column = left; column <= right; column++) {
                cell = gridCells[row][column];
                cellNode = nodeOf(cell);
                if ((cellNode->flags & NODE_MARKED) == 0) {
                    for (edge = cellNode->edgesIn; edge != 0U; edge = edgeOf(edge)->nextIn) {
                        over = edgeOf(edge)->first;
                        before = nodeOf(over);
                        /* Unlike linkNewNode, a node that does not overlap
                         * stays unmarked. */
                        if (((before->flags & NODE_MARKED) == 0) && entry->bounds.touches(before->bounds)) {
                            if (!orderNodes2(node, over)) {
                                return 0;
                            }
                            before->flags |= NODE_MARKED;
                            before->marked = marked;
                            marked = over;
                        }
                    }
                    if (!orderNodes2(node, cell)) {
                        return 0;
                    }
                }
            }
        }
        while (marked != 0U) {
            over = marked;
            marked = nodeOf(over)->marked;
            nodeOf(over)->flags &= ~NODE_MARKED;
        }
    }
    return static_cast<u8>(result);
}

bool orderNodes2(u16 first, u16 second)
{
    bool result = true;
    DL_Node *a = nodeOf(first);
    DL_Node *b = nodeOf(second);

    if (second != first) {
        i16 order;
        if (a->kind < b->kind) {
            order = 2;
        } else if (a->kind > b->kind) {
            order = 1;
        } else {
            order = (static_cast<DL_ItemNode *>(a))->order(second);
        }
        if (order == 1) {
            if (!linkNodes(first, second)) {
                result = false;
            }
        } else if (order == 2) {
            if (!linkNodes(second, first)) {
                result = false;
            }
        }
    }
    return result;
}

void initDisplayList()
{
    initNodes();
    initEdges();
    relink = 0;
    drawArea.left = 0;
    drawArea.top = 0;
    drawArea.right = SCREEN_RIGHT;
    drawArea.bottom = screenRect.bottom;
    i16 column;
    i16 row;
    for (column = 0; column < GRID_COLUMNS; column++) {
        for (row = 0; row < GRID_ROWS; row++) {
            const u16 cell = nodeOffset(new DL_GridNode);
            if (cell != 0U) {
                nodeOf(cell)->kind = NODE_GRID;
                nodeOf(cell)->flags |= NODE_CELL;
                gridCells[row][column] = cell;
                const i16 x = column * GRID_CELL;
                const i16 y = row * GRID_CELL;
                nodeOf(cell)->bounds = Rect(x, y, x + GRID_CELL - 1, y + GRID_CELL - 1);
                markChanged(cell);
            } else {
                debugPrintf(50, "dl init ");
                gridCells[row][column] = 0;
            }
        }
    }
    if (itemNodes == nullptr) {
        itemNodes = new u16[0x2800];
    }
    memset(itemNodes, 0, 0x2800 * sizeof(u16));
    if (screenItems == nullptr) {
        screenItems = new u32[0x140];
    }
    memset(screenItems, 0, 0x140 * sizeof(u32));
    if (movedItems == nullptr) {
        movedItems = new u32[0x140];
    }
    memset(movedItems, 0, 0x140 * sizeof(u32));
}

namespace {
void closeDisplayList()
{
    closeNodes();
    closeEdges();
    if (itemNodes != nullptr) {
        delete[] itemNodes;
        itemNodes = nullptr;
    }
    if (screenItems != nullptr) {
        delete[] screenItems;
        screenItems = nullptr;
    }
    if (movedItems != nullptr) {
        delete[] movedItems;
        movedItems = nullptr;
    }
}
}

void deleteNode(u16 node)
{
    unlinkNode(node);
    delete nodeOf(node);
}

void deleteEdge(u16 edge)
{
    edgeOf(edge)->unlink();
    delete edgeOf(edge);
}

/* Nothing calls it. */
/* UNCALLED: nothing calls it. */
void unused0eb1(i16 /*unused*/) {}

u16 nodeAt(i16 x, i16 y)
{
    Rect point(0, 0, 0, 0);
    u16 found = 0;
    u16 cell;

    x += scrollX;
    y += scrollY;
    point = Rect(x, y);
    cell = gridCells[static_cast<i16>(y - viewY) >> 7][static_cast<i16>(x - viewX) >> 7];
    DL_Node *entry = nodeOf(cell);
    DL_Node *over;
    DL_Edge *link;
    u16 edge;
    for (edge = entry->edgesIn; edge != 0U; edge = link->nextIn) {
        link = edgeOf(edge);
        over = nodeOf(link->first);
        if (over->hit(x, y)) {
            found = link->first;
            over->flags |= NODE_MARKED;
        }
    }
    const u16 first = edge = entry->edgesIn;
    if (found != 0U) {
        for (edge = first; edge != 0U; edge = link->nextIn) {
            link = edgeOf(edge);
            found = link->first;
            over = nodeOf(found);
            if ((over->flags & NODE_MARKED) != 0) {
                DL_Edge *upper;
                for (u16 above = over->edgesIn; above != 0U; above = upper->nextIn) {
                    upper = edgeOf(above);
                    if ((nodeOf(upper->first)->flags & NODE_MARKED) != 0) {
                        found = 0;
                        break;
                    }
                }
                if (found != 0U) {
                    break;
                }
            }
        }
        for (edge = first; edge != 0U; edge = link->nextIn) {
            link = edgeOf(edge);
            nodeOf(link->first)->flags &= ~NODE_MARKED;
        }
    }
    return found;
}

void changeArea(Rect *area)
{
    i16 left = static_cast<i16>(area->left - viewX) >> 7;
    i16 top = static_cast<i16>(area->top - viewY) >> 7;
    i16 right = static_cast<i16>(area->right - viewX) >> 7;
    i16 bottom = static_cast<i16>(area->bottom - viewY) >> 7;

    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right >= GRID_COLUMNS) {
        right = GRID_COLUMNS - 1;
    }
    if (bottom >= GRID_ROWS) {
        bottom = GRID_ROWS - 1;
    }
    i16 column;
    i16 row;
    DL_ItemNode *entry;
    for (row = top; row <= bottom; row++) {
        for (column = left; column <= right; column++) {
            const u16 cell = gridCells[row][column];
            for (u16 edge = nodeOf(cell)->edgesIn; edge != 0U; edge = edgeOf(edge)->nextIn) {
                const u16 over = edgeOf(edge)->first;
                entry = static_cast<DL_ItemNode *>(nodeOf(over));
                if ((entry->changed() == 0U) && entry->bounds.touches(*area)) {
                    changedNodes.add(over);
                    entry->flags |= NODE_CHANGED;
                }
                setItemBit(movedItems, entry->item);
            }
        }
    }
}

namespace {
/* The items of movedItems on the screen go into screenItems. */
void findScreenItems()
{
    u16 base;
    u32 mask;
    u32 value;
    u32 *bits;
    i16 x;
    i16 y;
    i16 z;
    i16 word;
    Item item;
    ShapeInfo *info;
    u16 number;
    i16 bit;

    base = 0;
    bits = movedItems;
    for (word = 0; word < 0x140; word++) {
        if (*bits != 0U) {
            value = *bits;
            number = base;
            for (bit = 0, mask = 1; bit < 32; number++, bit++, mask <<= 1) {
                item = ItemRef(number);
                if ((value & mask) != 0U) {
                    if (typeFlags.infos[itemShape[item.id]].animType != 0U) {
                        i16 screenX;
                        i16 screenY;
                        i16 left;
                        i16 top;
                        i16 right;
                        i16 bottom;
                        i16 width;
                        i16 depth;
                        i16 height;
                        info = &typeFlags.infos[itemShape[item.id]];
                        x = itemX[item.id] >> 1;
                        y = itemY[item.id] >> 1;
                        z = itemZ[item.id];
                        screenX = x - y;
                        screenY = (x >> 1) + (y >> 1) - z;
                        if ((itemFlags[item.id] & ITEM_FLIPPED) != 0) {
                            halfBox(depth, width, height, info);
                        } else {
                            halfBox(width, depth, height, info);
                        }
                        left = screenX - width - scrollX;
                        top = screenY - (width >> 1) - (depth >> 1) - height - scrollY;
                        right = screenX + depth - scrollX;
                        bottom = screenY - scrollY;
                        if (left < SCREEN_WIDTH && top < SCREEN_HEIGHT && right >= 0 && bottom >= 0) {
                            screenItems[word] |= mask;
                        }
                    }
                }
            }
            *bits = 0;
        }
        base += 32;
        bits++;
    }
}

/* The animated items of screenItems whose frame is due at time. */
i16 dueItems(s32 time, u16 *items)
{
    u16 base;
    u32 mask;
    u32 value;
    u32 *bits;
    i16 x;
    i16 y;
    i16 z;
    i16 word;
    i16 count;
    Item item;
    ShapeInfo *info;
    u16 number;
    i16 bit;

    count = 0;
    base = 0;
    bits = screenItems;
    for (word = 0; word < 0x140; word++) {
        if (*bits != 0U) {
            value = *bits;
            number = base;
            for (bit = 0, mask = 1; bit < 32; number++, bit++, mask <<= 1) {
                item = ItemRef(number);
                if ((value & mask) != 0U) {
                    if ((itemFlags[item.id] & ITEM_DRAWN) != 0) {
                        i16 screenX;
                        i16 screenY;
                        i16 left;
                        i16 top;
                        i16 right;
                        i16 bottom;
                        i16 width;
                        i16 depth;
                        i16 height;
                        info = &typeFlags.infos[itemShape[item.id]];
                        x = itemX[item.id] >> 1;
                        y = itemY[item.id] >> 1;
                        z = itemZ[item.id];
                        screenX = x - y;
                        screenY = (x >> 1) + (y >> 1) - z;
                        if ((itemFlags[item.id] & ITEM_FLIPPED) != 0) {
                            halfBox(depth, width, height, info);
                        } else {
                            halfBox(width, depth, height, info);
                        }
                        left = screenX - width - scrollX;
                        top = screenY - (width >> 1) - (depth >> 1) - height - scrollY;
                        right = screenX + depth - scrollX;
                        bottom = screenY - scrollY;
                        if (left < SCREEN_WIDTH && top < SCREEN_HEIGHT && right >= 0 && bottom >= 0) {
                            const i16 speed = info->animSpeed;
                            if (speed < 2 || lastAnimation / speed != time / speed) {
                                items[count++] = number;
                            }
                        } else {
                            *bits &= ~mask;
                        }
                    } else {
                        *bits &= ~mask;
                    }
                }
            }
        }
        base += 32;
        bits++;
    }
    return count;
}
}

void animateItems()
{
    u16 width;
    u16 height;
    i16 frames;
    i16 i;
    i16 count;
    ShapeInfo *info;

    if (animationsHeld != 0U || animationsStopped != 0U) {
        return;
    }
    Item item;
    flushDirty();
    u16 *items = new u16[0x2800];
    const s32 time = animationProcess->time;
    findScreenItems();
    count = dueItems(time, items);
    lastAnimation = time;
    flushDirty();
    for (i = 0; i < count; i++) {
        item = ItemRef(items[i]);
        info = &typeFlags.infos[itemShape[item.id]];
        shapeCache->shapeHeader(itemShape[item.id], &width, &height, reinterpret_cast<u16 *>(&frames));
        switch (info->animType) {
        case 2:
            if (randomNumber(2) != 0U) {
                item.setFrame(randomNumber(frames));
            }
            break;
        case 1:
        case 3: {
            i16 frame = item.frame();
            if (info->animData == 0 || (info->animData == 1 && (randomNumber(2) != 0U))) {
                if (++frame >= frames) {
                    frame = 0;
                }
                item.setFrame(frame);
            } else {
                frame =
                    static_cast<ui16>(frame + 1) % info->animData + (frame - static_cast<ui16>(frame) % info->animData);
                if (frame >= frames) {
                    frame -= static_cast<ui16>(frame) % info->animData;
                }
                item.setFrame(frame);
            }
            break;
        }
        case 4: {
            i16 frame = item.frame();
            if ((frame != 0) || (randomNumber(info->animData + 2) == 0U)) {
                if (++frame >= frames) {
                    frame = 0;
                }
                item.setFrame(frame);
            }
            break;
        }
        case 5:
            item.anim();
            break;
        case 6: {
            i16 frame = item.frame();
            if (info->animData == 0 || (info->animData == 1 && (randomNumber(2) != 0U))) {
                if (frame != 0) {
                    if (++frame >= frames) {
                        frame = 1;
                    }
                    item.setFrame(frame);
                }
            } else if ((static_cast<ui16>(frame) % info->animData) != 0) {
                frame =
                    static_cast<ui16>(frame + 1) % info->animData + (frame - static_cast<ui16>(frame) % info->animData);
                if (static_cast<ui16>(frame) % info->animData == 0) {
                    frame++;
                }
                if (frame >= frames) {
                    frame -= static_cast<ui16>(frame) % info->animData;
                }
                item.setFrame(frame);
            }
            break;
        }
        }
    }
    delete[] items;
}

void releaseNodes()
{
    i16 column;
    i16 row;
    DL_Node *entry;
    relink = 0;
    for (row = 0; row < GRID_ROWS; row++) {
        for (column = 0; column < GRID_COLUMNS; column++) {
            const u16 cell = gridCells[row][column];
            u16 edge = nodeOf(cell)->edgesIn;
            while (edge != 0U) {
                const u16 node = edgeOf(edge)->first;
                entry = nodeOf(node);
                entry->release();
                edge = edgeOf(edge)->nextIn;
                deleteNode(node);
            }
        }
    }
    memset(itemNodes, 0, 0x2800 * sizeof(u16));
    memset(screenItems, 0, 0x140 * sizeof(u32));
    memset(movedItems, 0, 0x140 * sizeof(u32));
}

void DL_Node::insert(u16 before, u16 after)
{
    previous = before;
    next = after;
    if (before != 0U) {
        nodeOf(before)->next = nodeOffset(this);
    }
    if (after != 0U) {
        nodeOf(after)->previous = nodeOffset(this);
    }
}

void DL_Node::unlink() const
{
    if (previous != 0U) {
        nodeOf(previous)->next = next;
    }
    if (next != 0U) {
        nodeOf(next)->previous = previous;
    }
}

bool DL_Node::readyToDraw()
{
    bool result = true;
    DL_Edge *link;

    for (u16 edge = edgesOut; edge != 0U; edge = link->nextOut) {
        link = edgeOf(edge);
        const char after = nodeOf(link->second)->flags;
        if (((after & NODE_CHANGED) != 0) && ((after & NODE_CELL) == 0)) {
            if ((flags & NODE_DRAWN) == 0) {
                flags |= NODE_CHANGED;
                result = false;
            }
            break;
        }
    }
    return result;
}

void *DL_Edge::operator new(size_t /*unused*/) noexcept
{
    DL_Edge *edge = nullptr;

    if (freeEdges != 0U) {
        edge = edgeOf(freeEdges);
        freeEdges = edge->nextIn;
        freeEdgeCount--;
    }
    return edge;
}

void DL_Edge::operator delete(void *memory)
{
    DL_Edge *edge = static_cast<DL_Edge *>(memory);

    edge->nextIn = freeEdges;
    freeEdges = edgeOffset(edge);
    freeEdgeCount++;
}

void initEdges()
{
    if (edgePool == nullptr) {
        edgePool = new u8[EDGE_POOL * sizeof(DL_Edge)];
    }
    /* As initNodes: the first slot is not used. */
    u16 edge = 0;
    freeEdgeCount = EDGE_POOL;
    freeEdges = EDGE_SLOT;
    freeEdgeCount--;
    for (i16 i = 0; i < EDGE_POOL - 1; i++) {
        edgeOf(edge)->nextIn = edge + EDGE_SLOT;
        edge += EDGE_SLOT;
    }
    edgeOf(edge)->nextIn = 0;
}

void closeEdges()
{
    if (edgePool != nullptr) {
        delete[] edgePool;
        edgePool = nullptr;
    }
}

DL_Edge::DL_Edge(u16 from, u16 to)
{
    first = from;
    second = to;
    nextIn = nodeOf(to)->edgesIn;
    if (nextIn != 0U) {
        edgeOf(nextIn)->previousIn = edgeOffset(this);
    }
    previousIn = 0;
    nextOut = nodeOf(from)->edgesOut;
    if (nextOut != 0U) {
        edgeOf(nextOut)->previousOut = edgeOffset(this);
    }
    previousOut = 0;
}

void DL_Edge::unlink() const
{
    if (previousOut != 0U) {
        edgeOf(previousOut)->nextOut = nextOut;
    } else {
        nodeOf(first)->edgesOut = nextOut;
    }
    if (nextOut != 0U) {
        edgeOf(nextOut)->previousOut = previousOut;
    }
    if (previousIn != 0U) {
        edgeOf(previousIn)->nextIn = nextIn;
    } else {
        nodeOf(second)->edgesIn = nextIn;
    }
    if (nextIn != 0U) {
        edgeOf(nextIn)->previousIn = previousIn;
    }
}

/* Nothing calls them. */
/* UNCALLED: nothing calls it. */
void initDisplayList2()
{
    initDisplayList();
}

void closeDisplayList2()
{
    closeDisplayList();
}
