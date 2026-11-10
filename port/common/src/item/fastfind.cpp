/* ITEM\fastfind.asm, Code28 (10d8:0000-04dd) in C++: the fast item searches
 * of ITEMFIND.C (see itemfind.h).
 *
 * Each walks the item chains of the map chunks in the rectangle, chunk by
 * chunk, row after row. The original handles an item as its number doubled
 * in 16 bits, so a chain also ends at item 8000h and an item's number keeps
 * its low 15 bits; the same arithmetic is kept here. The script's shapes
 * are read as the callers leave them: nextOfShapes with 1 to 15 shapes
 * (with 0 the original compares on past the script, 65536 times; here it
 * finds none). AreaList::fill reads mapChunks from its selector's start,
 * dropping the pointer's offset, which is 0 for the game's table. */
#include "itemfind.h"
#include "itemcach.h"
#include "itemdata.h"
#include "type.h"

namespace {

/* The item as the original holds it: its number doubled in 16 bits, 0 at
 * the end of a chain. */
u16 slotOf(u16 item)
{
    return static_cast<u16>(item << 1);
}

u16 chunkHead(u16 column, u16 row)
{
    return (&mapChunks[0][0])[static_cast<u16>(static_cast<u16>(column << 7) + static_cast<u16>(row << 1)) >> 1];
}

bool inRectangle(u16 left, u16 top, u16 right, u16 bottom, u16 slot)
{
    const u16 x = itemX[slot >> 1];
    const u16 y = itemY[slot >> 1];
    return !(left > x) && !(right < x) && !(top > y) && !(bottom < y);
}

/* The walk: the search's item first, then its chain, then the chunks after
 * it; accept(slot) tests an item before the rectangle does. */
template <class Accept> void search(AreaSearch &search, Accept accept)
{
    u16 slot = slotOf(search.id);
    for (;;) {
        for (; slot != 0; slot = slotOf(itemNext[slot >> 1])) {
            if (accept(slot) && inRectangle(search.left, search.top, search.right, search.bottom, slot)) {
                search.id = slot >> 1;
                return;
            }
        }
        search.x++;
        if (search.x > search.lastColumn) {
            search.x = search.left >> 9;
            search.y++;
            if (search.y > search.lastRow) {
                search.id = 0;
                return;
            }
        }
        slot = slotOf(chunkHead(search.x, search.y));
    }
}

u16 word(const u8 *p)
{
    return static_cast<u16>(p[0] | p[1] << 8);
}

}

void AreaSearch::nextOfShape(s16 shape)
{
    search(*this, [shape](u16 slot) { return itemShape[slot >> 1] == static_cast<u16>(shape); });
}

/* The original reads the family as bits 4-8 of the word at byte 1 of the
 * shape's 9-byte ShapeInfo in typeFlags.infos; the port reads the field. */
void AreaSearch::nextOfFamily(s16 family)
{
    const u16 wanted = static_cast<u16>(family << 4);
    search(*this, [wanted](u16 slot) {
        const u16 shape = itemShape[slot >> 1];
        return static_cast<u16>(typeFlags.infos[shape].family << 4) == wanted;
    });
}

void AreaSearch::nextOfShapes(s16 count)
{
    const u8 *shapes = script + 1;
    search(*this, [shapes, count](u16 slot) {
        const u16 shape = itemShape[slot >> 1];
        for (s16 k = 0; k < count; k++) {
            if (word(shapes + (2 * k)) == shape) {
                return true;
            }
        }
        return false;
    });
}

void AreaSearch::nextOfEither(s16 shape)
{
    const u16 other = word(script + 3);
    search(*this, [shape, other](u16 slot) {
        const u16 found = itemShape[slot >> 1];
        return found == static_cast<u16>(shape) || found == other;
    });
}

void AreaSearch::nextInArea()
{
    search(*this, [](u16) { return true; });
}

void AreaList::fill()
{
    u16 *out = foundItems;
    for (;;) {
        for (u16 slot = slotOf(chunkHead(column, row)); slot != 0; slot = slotOf(itemNext[slot >> 1])) {
            if (inRectangle(left, top, right, bottom, slot)) {
                *out++ = slot >> 1;
            }
        }
        column++;
        if (column > lastColumn) {
            column = left >> 9;
            row++;
            if (row > lastRow) {
                break;
            }
        }
    }
    *out = 0;
}
