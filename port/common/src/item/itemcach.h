/* ..\\ITEM\\ITEMCACH.C (Code24): the items of the maps. Each map chunk
 * (512 units square, 64 by 64 of them) keeps a list of its items; the chunks
 * near the camera (the fast area) have theirs active. The free item numbers
 * and the ethereal stack, where an item waits while it moves, are lists
 * through itemNext too. */
#ifndef PORT_ITEMCACH_H
#define PORT_ITEMCACH_H
#include "common.h"
#include "itemdata.h"

class Flex;
class FixedFlex;

/* The item numbers below it are NPCs (and the Silencer, 1). */
constexpr int FIRST_ITEM = 0x100;
/* itemZ of a free item number. */
constexpr int FREE_Z = 0xfe;

/* A diamond of map chunks: the bounds of column+row and of column-row. */
class FastArea {
public:
    i16 sumLow;
    i16 sumHigh;
    i16 differenceLow;
    i16 differenceHigh;

    explicit FastArea(i16 newSumLow = 0, i16 newSumHigh = 0, i16 newDifferenceLow = 0, i16 newDifferenceHigh = 0)
        : sumLow(newSumLow), sumHigh(newSumHigh), differenceLow(newDifferenceLow), differenceHigh(newDifferenceHigh)
    {
    }
    /* 1 when the chunk is on the map and in the area. */
    [[nodiscard]] bool contains(i16 column, i16 row) const;
    /* The area around the chunk of map point x,y. */
    void centre(u16 x, u16 y);
};
ASSERT_SIZE(FastArea, 8);

/* Code7 sets it while a new game starts; nothing reads it. */
extern u8 &unknown1453;
/* The current map number, FFFFh before a map is loaded. */
extern u16 &currentMap;
/* The map is CHUNKS by CHUNKS chunks of CHUNK_SIZE units: a map point's
 * chunk is x >> CHUNK_SHIFT, and x & CHUNK_MASK the chunk's edge. */
constexpr int CHUNKS = 64;
constexpr int CHUNK_SIZE = 0x200;
constexpr int CHUNK_SHIFT = 9;
constexpr int CHUNK_MASK = 0xfe00;
/* The first item of each map chunk: mapChunks[column][row]. */
extern u16 (*&mapChunks)[CHUNKS];

extern FastArea &fastArea;

/* The count of the free item numbers from FIRST_ITEM. */
extern i16 &freeItemCount;
/* The top of the ethereal stack and its size. */
extern u16 &etherealTop;

/* The map chunk of map point x,y, and the chunk's corner. */
inline void mapChunk(u16 x, u16 y, u16 &column, u16 &row)
{
    column = x >> CHUNK_SHIFT;
    row = y >> CHUNK_SHIFT;
}
inline void chunkCorner(u16 column, u16 row, u16 &x, u16 &y)
{
    x = column << CHUNK_SHIFT;
    y = row << CHUNK_SHIFT;
}

/* Keeps count item records of chunk column,row to create when the chunk
 * comes into the fast area. */
void addChunkRecords(u16 column, u16 row, ItemRecord *records, i16 count);
/* Writes and reads the item tables and the lists to objects index and
 * index+1 of flex. */
void saveItems(Flex *flex, i16 index);
bool loadItems(Flex *flex, i16 index);
/* Moves the fast area to map point x,y of map, 0 for the current one. */
void setFastArea(u16 x, u16 y, i16 map);
void initItemCache();
/* All NPC numbers become free. */
void resetNpcNumbers();
void closeItemCache();
/* Frees the records' buffer of a map whose loading a wait stopped, at the
 * run's end (the port's, gameRelease). */
void releaseMapLoad();
/* Frees the item's number. */
void removeItem(u16 item);
/* The chunk of map point x,y goes to column and row; 1 when it is in the
 * fast area. */
bool pointInFastArea(u16 x, u16 y, i16 *column, i16 *row);
/* Takes the item off the map or out of its container onto the ethereal
 * stack. */
void pushItem(u16 item);
/* Places the top of the ethereal stack at x,y,z. */
void popToCoords(u16 x, u16 y, u8 z);
/* Puts the top of the ethereal stack into container. */
void popToContainer(u16 container);
void popItem();
void popToEnd(u16 container);
/* Takes the top of the ethereal stack off it without placing it; with
 * clear its map becomes 0. */
void popToNowhere(u8 clear);
/* A free item number, a free NPC number. */
u16 newItem();
u16 newNpc();
void startItemCache();

u16 topEthereal();

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void reloadNpcs();
/* UNCALLED: nothing calls it. */
void reloadMap();
/* UNCALLED: nothing calls it. */
void resetFastArea();
/* UNCALLED: nothing calls it. */
i16 spareItems();
/* UNCALLED: nothing calls it. */
i16 countFreeNpcs();
/* UNCALLED: nothing calls it. */
void verifyFailed(i16 map);
/* UNCALLED: nothing calls it. */
void dumpItems();
/* UNCALLED: nothing calls it. */
void itemCacheNothing();
#endif
