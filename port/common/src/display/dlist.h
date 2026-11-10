/* DLIST.C (Code54): the display list. Every item on the map and every
 * 128 by 128 cell of the screen's 6 by 5 grid has a node; an edge joins two
 * nodes that overlap on the screen, from the one drawn first to the other.
 * The nodes that changed are drawn again with the nodes over them. A node or
 * an edge is its offset in its pool, as in the original (its NODE and EDGE are nodeOf and edgeOf).
 * The item animations are here too (animateItems). The names DL_Node and
 * DL_ItemNode are the original's (DLITEM.C's message). */
#ifndef PORT_DLIST_H
#define PORT_DLIST_H
#include "common.h"
#include "rect.h"
#include "viewport.h"
#include "type.h"

/* DL_Node::kind: the grid cells sort before the items. */
constexpr int NODE_GRID = 1;
constexpr int NODE_ITEM = 2;

/* DL_Node::flags */
constexpr int NODE_CHANGED = 0x1;   /* on changedNodes */
constexpr int NODE_MARKED = 0x8;    /* on a chain through marked */
constexpr int NODE_RELINKED = 0x10; /* linked again by moveCamera */
constexpr int NODE_DRAWN = 0x20;    /* on drawNodes */
constexpr int NODE_CELL = 0x40;     /* a grid cell */
constexpr int NODE_COVERED = 0x80;  /* over a node with covers set */

constexpr int NODE_POOL = 1500;
constexpr int EDGE_POOL = 4000;
constexpr int GRID_COLUMNS = 6;
constexpr int GRID_ROWS = 5;
constexpr int GRID_CELL = 128;

/* A node. The derived classes' constructors and the virtual functions are
 * inline; the pool gives the memory. */
class DL_Node {
public:
    /* The next node on a chain of marked nodes. */
    u16 marked;
    /* The list the node is on (NodeList). */
    u16 next;
    u16 previous;
    /* The edges to this node and from it. */
    u16 edgesIn;
    u16 edgesOut;
    /* On the map's screen, before the scroll. */
    Rect bounds;
    char flags;
    i16 kind;
    /* Nodes linked after this one get NODE_COVERED. */
    u8 covers;

    DL_Node() : bounds(0, 0, 0, 0)
    {
        kind = 0;
        next = previous = 0;
        edgesIn = edgesOut = 0;
        flags = 0;
    }
    /* Virtual in the port, not in the original: nodes are deleted through
     * DL_Node. */
    virtual ~DL_Node() = default;
    /* From the pool; 0 when it is empty. noexcept, so that new
     * gives 0 without the constructor, as BCC's new does (GCC constructs
     * at what an operator new without it gives, 0 too); DL_Edge's alike. */
    void *operator new(size_t size) noexcept;
    void operator delete(void *memory);
    /* Puts the node between previous and next. */
    void insert(u16 before, u16 after);
    void unlink() const;
    [[nodiscard]] u8 changed() const { return flags & NODE_CHANGED; }
    /* 1 when no changed node is drawn over the node; else it marks itself
     * changed and drawChanged draws it later. */
    bool readyToDraw();
    virtual void draw() {}
    /* 1 when point x,y of the screen hits the node. */
    virtual bool hit([[maybe_unused]] i16 x, [[maybe_unused]] i16 y) { return false; }
    virtual void release() {}
};
ASSERT_SIZE(DL_Node, 0x18);

/* A cell of the grid. Its edges lead to the item nodes over it. */
class DL_GridNode : public DL_Node {
public:
    DL_GridNode() = default;
    /* Code57, the whole module. */
    void draw() override;
    /* Nothing calls it. */
    virtual i16 unknown3() { return 0; }
};
ASSERT_SIZE(DL_GridNode, 0x18);

/* An edge from first, drawn first, to second, over it. */
class DL_Edge {
public:
    u16 first;
    u16 second;
    /* The edges to second and from first. */
    u16 nextIn;
    u16 previousIn;
    u16 nextOut;
    u16 previousOut;

    /* Puts the edge at the head of second's edgesIn and first's edgesOut,
     * without setting them. */
    DL_Edge(u16 from, u16 to);
    void *operator new(size_t size) noexcept;
    void operator delete(void *memory);
    void unlink() const;
};
ASSERT_SIZE(DL_Edge, 0xc);

/* A shape's box in the display list's units, halved like the map point: 8
 * a step in x, y and z. The shape comes last, so its pointer goes through a
 * temporary (as COLLIDE.C's shapeBox). */
inline void halfBox(i16 &width, i16 &depth, i16 &height, ShapeInfo *info)
{
    width = info->x << 3;
    depth = info->y << 3;
    height = info->z << 3;
    if (height == 0 && !info->fixed) {
        height = 1;
    }
}

/* A node or an edge is its offset in its pool as the original lays the
 * pools out: slots of the original's sizes, 2Ah bytes for a node (the
 * largest, DL_ItemNode) and 0Ch for an edge, with offset 0 for none. The
 * original reaches it through the pool's selector (displayNodes,
 * displayEdges). The port's slots are larger (pointers, alignment) and 1500
 * nodes of its size would not fit 16-bit offsets, so an offset gives the
 * slot and the slot the address. */
constexpr int NODE_SLOT = 0x2a;
constexpr int EDGE_SLOT = 0x0c;
extern u8 *&nodePool;
extern u8 *&edgePool;
/* The size of the port's node slots, a DL_ItemNode's (dlitem.h). */
extern const u32 NODE_SIZE;

/* The node at offset node and the edge at offset edge. */
inline DL_Node *nodeOf(u16 node)
{
    return reinterpret_cast<DL_Node *>(nodePool + (static_cast<u32>(node / NODE_SLOT) * NODE_SIZE));
}
inline DL_Edge *edgeOf(u16 edge)
{
    return reinterpret_cast<DL_Edge *>(edgePool + (static_cast<u32>(edge / EDGE_SLOT) * sizeof(DL_Edge)));
}
/* The offset of a node or an edge, 0 for none (FP_OFF in the original). */
inline u16 nodeOffset(const void *node)
{
    return (node != nullptr) ? static_cast<u16>((static_cast<const u8 *>(node) - nodePool) / NODE_SIZE * NODE_SLOT) : 0;
}
inline u16 edgeOffset(const void *edge)
{
    return (edge != nullptr)
               ? static_cast<u16>((static_cast<const u8 *>(edge) - edgePool) / sizeof(DL_Edge) * EDGE_SLOT)
               : 0;
}

/* A list of nodes. */
class NodeList {
public:
    u16 first;
    u16 last;

    NodeList() { first = last = 0; }
    /* Takes the first node off, 0 when there is none. */
    u16 pop();
    void append(u16 node);
    void remove(u16 node);
    /* The callers go through this inline: its argument takes a frame slot
     * (Item::setFrameNow). */
    void add(u16 node) { append(node); }
};
ASSERT_SIZE(NodeList, 4);

/* The screen's map area. */
extern Rect &screenRect;
/* A pass of drawChanged links the nodes again. */
extern u8 &relink;
/* The items on the screen and the items whose place changed, a bit for each
 * item number, cleared when an item goes; ITEMCACH.C sets the second when it
 * places an item. They are plain pointers: BCC moves a structure member's
 * address out of findScreenItems's loop, and the original does not. */
extern u32 *&screenItems;
extern u32 *&movedItems;

/* The pointer goes by reference: a copy would take a temporary. */
inline void setItemBit(u32 *&bits, u16 item)
{
    bits[item >> 5] |= static_cast<s32>(1L) << (item & 0x1f);
}
inline void clearItemBit(u32 *&bits, u16 item)
{
    bits[item >> 5] &= ~(static_cast<s32>(1L) << (item & 0x1f));
}

/* The screen point of the grid's corner, which the camera sets. */
extern i16 &viewX;
extern i16 &viewY;
/* The counts of the free nodes and edges. */
extern i16 &freeNodeCount;
extern i16 &freeEdgeCount;
/* The nodes whose place changed, and the nodes to draw. */
extern NodeList &changedNodes;
/* The part of the screen being drawn, and a copy of drawViewport clipped to
 * it. */
extern Rect &drawArea;
extern Viewport &drawView;
/* The node of each cell. */
extern u16 (&gridCells)[GRID_ROWS][GRID_COLUMNS];

/* Clips rect to area. */
void clipRect(Rect *rect, Rect *area);
/* Grows rect to hold other. */
void mergeRect(Rect *rect, Rect *other);
/* Draws the changed nodes and the nodes over them in drawArea. */
void drawChanged();
void markChanged(u16 node);
/* Deletes the node's edges. */
void unlinkNode(u16 node);
/* Links a new item node with the nodes it overlaps in the
 * order of orderNodes. */
u8 linkNewNode(u16 node);
u8 relinkNode(u16 node);
void initDisplayList();
void deleteNode(u16 node);
void deleteEdge(u16 edge);
/* The node at screen point x,y, or 0. */
u16 nodeAt(i16 x, i16 y);
/* Puts the nodes in area on changedNodes and their items in movedItems. */
void changeArea(Rect *area);
/* Steps the animated items on the screen. */
void animateItems();
/* Releases every node. */
void releaseNodes();

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void redrawArea(Rect area);
/* UNCALLED: nothing calls it. */
void unused0eb1(i16 /*unused*/);
/* UNCALLED: nothing calls it. */
void initDisplayList2();
void closeDisplayList2();
#endif
