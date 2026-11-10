/* ..\\ITEM\\ITEMFIND.C (Code27, 10d0): the item searches of the usecode
 * loop opcode and of the game. A search is the item it has reached (0 at
 * the end); next() moves it on to the next item that the loop script
 * accepts. The script is a LoopScript (scrit.h): its first byte is the
 * length when a search is given length -1. The vtables are made where the
 * inline constructors are expanded (WORLD.C, Code19, INTER.C). */
#ifndef PORT_ITEMFIND_H
#define PORT_ITEMFIND_H
#include "common.h"
#include "item.h"

/* AreaSearch::selector: the kind of fast search in the high nibble and its
 * argument in the low 12 bits (Code28, fastfind.asm). */
constexpr int SEARCH_SHAPE = 0x1000;  /* one shape */
constexpr int SEARCH_FAMILY = 0x2000; /* one family */
constexpr int SEARCH_SHAPES = 0x3000; /* the shapes of the script, their count */
constexpr int SEARCH_EITHER = 0x4000; /* one of two shapes */
constexpr int SEARCH_ALL = 0x5000;    /* every item (SurfaceSearch) */

class ItemSearch : public Item {
public:
    /* The item where the search starts. */
    u16 first;
    /* The loop script, which an item must pass. */
    u8 script[0x20];
    /* The fast search, 0 for none (AreaSearch::init). */
    i16 selector;

    ItemSearch() : Item(0) {}
    /* Virtual in the port, not in the original: the interpreter deletes
     * its searches through ItemSearch. */
    virtual ~ItemSearch() = default;
    /* The memory of a search, 0, and its freeing (itemfind.cpp). */
    static void *operator new(size_t size);
    static void operator delete(void *memory);
    /* A search from first (10d0:0000). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    ItemSearch(u16 firstItem, const char *loopScript, i16 length);
    /* Goes to the first item, or the first that passes (10d0:0077). */
    void start();
    virtual void next(); /* 10d0:00b9 */
    /* The item passes the loop script (10d0:010c). */
    bool matches();
};
ASSERT_SIZE(ItemSearch, 0x28);

/* The items of one map chunk. */
class ChunkSearch : public ItemSearch {
public:
    ChunkSearch() = default;
    /* The items of the chunk of map point x,y (10d0:06aa). Nothing calls
     * it. */
    /* UNCALLED: nothing calls it. */
    ChunkSearch(u16 x, u16 y, const char *loopScript, i16 length);
    /* first becomes the first item of the chunk of x,y (10d0:072e). */
    void setChunk(u16 x, u16 y);
};
ASSERT_SIZE(ChunkSearch, 0x28);

/* The items in a rectangle of the map. */
class AreaSearch : public ChunkSearch {
public:
    u16 left;
    u16 top;
    u16 right;
    u16 bottom;
    /* The corner of the chunk the search is in; its column and row in the
     * fast searches. */
    u16 x;
    u16 y;
    /* The last column and row, for the fast searches. */
    u16 lastColumn;
    u16 lastRow;

    AreaSearch() = default;
    AreaSearch(u16 aroundX, u16 aroundY, u16 range, const char *loopScript, i16 length)
    {
        init(aroundX - range, aroundY - range, aroundX + range, aroundY + range, loopScript, length);
    }
    AreaSearch(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom, const char *loopScript, i16 length)
    {
        init(areaLeft, areaTop, areaRight, areaBottom, loopScript, length);
    }
    /* Negative bounds mean the map's (10d0:0771). A script that only tests
     * shapes or a family takes a fast search. */
    void init(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom, const char *loopScript, i16 length);
    /* The chunk of column,row (10d0:0a08). */
    /* UNCALLED: nothing calls it. */
    void initChunk(i16 column, i16 row, const char *loopScript, i16 length);
    /* 1 when the item is in the rectangle (10d0:0a46). */
    virtual bool contains();
    void next() override; /* 10d0:0a8e */
    /* Moves on to the next chunk with items (10d0:0c50). */
    void nextChunk();
    /* The fast searches (Code28, fastfind.asm, 10d8:0000-0385). */
    void nextOfShape(i16 shape);
    void nextOfFamily(i16 family);
    void nextOfShapes(i16 count);
    void nextOfEither(i16 shape);
    void nextInArea();
};
ASSERT_SIZE(AreaSearch, 0x38);

/* The items in a rectangle and the items they contain. */
class RecursiveAreaSearch : public AreaSearch {
public:
    /* The port's, for a search read from the usecode's stack (usesearch.h). */
    RecursiveAreaSearch() = default;
    RecursiveAreaSearch(u16 aroundX, u16 aroundY, u16 range, const char *loopScript, i16 length)
    {
        init(aroundX - range, aroundY - range, aroundX + range, aroundY + range, loopScript, length);
    }
    void next() override;     /* 10d0:0cca */
    bool contains() override; /* 10d0:0da7 */
};
ASSERT_SIZE(RecursiveAreaSearch, 0x38);

/* The items in a container. */
class ContainerSearch : public ItemSearch {
public:
    ContainerSearch() = default;
    ContainerSearch(u16 container, const char *loopScript, i16 length) { init(container, loopScript, length); }
    void init(u16 container, const char *loopScript, i16 length); /* 10d0:0def */
};
ASSERT_SIZE(ContainerSearch, 0x28);

/* The items in a container and in the containers in it. */
class RecursiveContainerSearch : public ContainerSearch {
public:
    u16 container;

    /* The port's, for a search read from the usecode's stack (usesearch.h). */
    RecursiveContainerSearch() = default;
    RecursiveContainerSearch(u16 searchedContainer, const char *loopScript, i16 length)
    {
        init(searchedContainer, loopScript, length);
    }
    void init(u16 searchedContainer, const char *loopScript, i16 length); /* 10d0:0e4c */
    void next() override;                                                 /* 10d0:0eb3 */
};
ASSERT_SIZE(RecursiveContainerSearch, 0x2a);

/* The items above or below an item. */
class SurfaceSearch : public AreaSearch {
public:
    /* The item's place. */
    u16 baseX;
    u16 baseY;
    /* The height of the item's top. */
    u8 surface;
    u16 item;
    /* 1: the items on the item, 2: the items it stands on, 3: both. */
    u8 mode;
    /* 1: the item found is on the item, 2: under it. */
    u8 side;

    /* The port's, for a search read from the usecode's stack (usesearch.h). */
    SurfaceSearch() = default;
    SurfaceSearch(u16 surfaceItem, const char *loopScript, i16 length, u8 searchMode)
    {
        init(surfaceItem, loopScript, length, searchMode);
    }
    void init(u16 surfaceItem, const char *loopScript, i16 length, u8 searchMode); /* 10d0:0f73 */
    /* AreaSearch::init, where "$" takes every item (10d0:1099). */
    void initArea(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom, const char *loopScript, i16 length);
    bool contains() override; /* 10d0:1338 */
    /* contains() for the fast searches: 1 at the end too (10d0:14c9). */
    bool accepts();
    void next() override; /* 10d0:1637 */
};
ASSERT_SIZE(SurfaceSearch, 0x41);

/* The items in a rectangle, found at once (foundItems) by Code28. */
class AreaList : public Item {
public:
    u16 left;
    u16 top;
    u16 right;
    u16 bottom;
    u16 column;
    u16 row;
    u16 lastColumn;
    u16 lastRow;
    i16 index;

    AreaList() = default;
    AreaList(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom)
    {
        init(areaLeft, areaTop, areaRight, areaBottom);
    }
    /* Negative bounds mean the map's (10d0:18c0). */
    void init(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom);
    /* Fills foundItems (Code28, 10d8:040f). */
    void fill();
    bool isValid() { return id != 0; }
    void next() { id = foundItems[index++]; }
};
ASSERT_SIZE(AreaList, 0x14);
#endif
