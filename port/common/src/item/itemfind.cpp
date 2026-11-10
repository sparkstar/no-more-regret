/* ..\\ITEM\\ITEMFIND.C (Code27, 10d0:0000-1975): the item searches and the
 * loop script interpreter. */
#include "itemfind.h"
#include "item.h"
#include "type.h"
#include "itemcach.h"
#include "scrit.h"

void *ItemSearch::operator new(size_t size)
{
    /* The original's heap held what it held; the port's is 0. A search
     * leaves some fields unset (a fast one its script), and the usecode's
     * loops keep its bytes on their stack (inter.cpp), which a save holds:
     * they are the same each run. */
    void *memory = ::operator new(size);
    memset(memory, 0, size);
    return memory;
}

void ItemSearch::operator delete(void *memory)
{
    ::operator delete(memory);
}

/* UNCALLED: nothing calls it. */
ItemSearch::ItemSearch(u16 firstItem, const char *loopScript, i16 length) : Item(0)
{
    if (length == -1) {
        length = *loopScript++;
    }
    this->first = firstItem;
    memmove(this->script, loopScript, length);
    start();
}

void ItemSearch::start()
{
    id = first;
    if (isValid() && !matches()) {
        next();
    }
}

void ItemSearch::next()
{
    while (isValid()) {
        id = itemNext[id];
        if (isValid() && matches()) {
            return;
        }
    }
}

namespace {
/* A search script's stack (ItemSearch::matches): 32 words; a push beyond
 * them overflows, an operation without its operands underflows (the search
 * halts at the script's end). */
struct ScriptStack {
    i16 words[0x20];
    i16 depth = 0;
    bool overflow = false;
    bool underflow = false;

    [[nodiscard]] bool full() const { return depth >= 0x20; }
    void push(i16 value)
    {
        if (depth < 0x20) {
            words[depth++] = value;
        } else {
            overflow = true;
        }
    }
    /* The two top words become op of them. */
    template <typename Op> void combine(Op op)
    {
        if (depth > 1) {
            words[static_cast<i16>(depth - 2)] =
                static_cast<i16>(op(words[static_cast<i16>(depth - 2)], words[static_cast<i16>(depth - 1)]));
            depth--;
        } else {
            underflow = true;
        }
    }
};

/* Whether value is among the count words at list. */
bool listHas(const u16 *list, i16 count, u16 value)
{
    for (i16 i = 0; count > i; i++) {
        if (list[i] == value) {
            return true;
        }
    }
    return false;
}
}

/* The item matches the search's script (item/scrit.h): its tokens push
 * words (an integer, the item's shape, family, frame, status, quality and
 * number, true and false, whether its shape or frame is in a list) and
 * compare or combine the two on top; the item matches when the last word
 * is not 0, or the script leaves none. */
bool ItemSearch::matches()
{
    ScriptStack stack;
    i16 pos = 0;
    do {
        const i16 op = script[pos++];
        switch (op) {
        case SCRIPT_TRUE:
            stack.push(1);
            break;
        case SCRIPT_FALSE:
            stack.push(0);
            break;
        case SCRIPT_INT:
            if (stack.full()) {
                stack.overflow = true;
            } else {
                const i16 value = *reinterpret_cast<i16 *>(script + pos);
                pos = pos + 2;
                stack.push(value);
            }
            break;
        case SCRIPT_SHAPE:
            stack.push(shapeOf(id));
            break;
        case SCRIPT_FAMILY:
            stack.push((itemShape[id] < SHAPES) ? typeFlags.infos[itemShape[id]].family : 0xe);
            break;
        case SCRIPT_FRAME:
            stack.push(frame());
            break;
        case SCRIPT_STATUS:
            stack.push(itemFlags[id]);
            break;
        case SCRIPT_Q:
            stack.push(itemQuality[id]);
            break;
        case SCRIPT_NPCNUM:
            stack.push(id);
            break;
        case SCRIPT_EQUAL:
            stack.combine([](i16 a, i16 b) { return a == b; });
            break;
        case SCRIPT_GREATER:
            stack.combine([](i16 a, i16 b) { return a > b; });
            break;
        case SCRIPT_LESS:
            stack.combine([](i16 a, i16 b) { return a < b; });
            break;
        case SCRIPT_GEQUAL:
            stack.combine([](i16 a, i16 b) { return a >= b; });
            break;
        case SCRIPT_LEQUAL:
            stack.combine([](i16 a, i16 b) { return a <= b; });
            break;
        case SCRIPT_AND:
            stack.combine([](i16 a, i16 b) { return a & b; });
            break;
        case SCRIPT_OR:
            stack.combine([](i16 a, i16 b) { return a | b; });
            break;
        case SCRIPT_NOT:
            /* The original complements the bits: the NOT of a true 1 is
             * -2, still true, so only a word test is turned around. */
            if (stack.depth != 0) {
                stack.words[static_cast<i16>(stack.depth - 1)] = ~stack.words[static_cast<i16>(stack.depth - 1)];
            } else {
                stack.underflow = true;
            }
            break;
        default:
            break;
        }
        if (op == SCRIPT_END) {
            break;
        }
        if (op > SCRIPT_SHAPE && op < SCRIPT_FRAME) {
            const i16 count = op - SCRIPT_SHAPE;
            stack.push(listHas(reinterpret_cast<u16 *>(script + pos), count, itemShape[id]) ? 1 : 0);
            pos = pos + (count << 1);
        } else if (op > SCRIPT_FRAME && op < 0x80) {
            const i16 count = op - SCRIPT_FRAME;
            stack.push(listHas(reinterpret_cast<u16 *>(script + pos), count, frame()) ? 1 : 0);
            pos = pos + (count << 1);
        }
    } while (pos < 0x20);
    if (stack.overflow || stack.underflow) {
        halt();
    }
    if (stack.depth != 0) {
        return stack.words[--stack.depth] != 0;
    }
    return true;
}

/* UNCALLED: nothing calls it. */
ChunkSearch::ChunkSearch(u16 x, u16 y, const char *loopScript, i16 length)
{
    if (length == -1) {
        length = *loopScript++;
    }
    setChunk(x, y);
    memmove(this->script, loopScript, length);
    start();
}

void ChunkSearch::setChunk(u16 x, u16 y)
{
    u16 column;
    u16 row;

    mapChunk(x, y, column, row);
    first = mapChunks[column][row];
}

void AreaSearch::init(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom, const char *loopScript, i16 length)
{
    if (length == -1) {
        length = *loopScript++;
    }
    if (areaLeft < 0) {
        areaLeft = 0;
    }
    if (areaTop < 0) {
        areaTop = 0;
    }
    if (areaRight < 0) {
        areaRight = 0x7fff;
    }
    if (areaBottom < 0) {
        areaBottom = 0x7fff;
    }
    x = this->left = areaLeft;
    y = this->top = areaTop;
    this->right = areaRight;
    this->bottom = areaBottom;
    selector = 0;
    if (*loopScript == SCRIPT_END) {
        /* Every item: no fast search. */
    } else if (*loopScript > SCRIPT_SHAPE && *loopScript < SCRIPT_FRAME) {
        const i16 count = *loopScript - SCRIPT_SHAPE;
        if (loopScript[static_cast<i16>(count + count + 1)] == SCRIPT_END) {
            if (count == 1) {
                selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_SHAPE;
            } else if (count == 2) {
                memmove(this->script, loopScript, length);
                selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_EITHER;
            } else {
                memmove(this->script, loopScript, length);
                selector = count + SEARCH_SHAPES;
            }
        }
    } else if (length == 6) {
        if (loopScript[0] == SCRIPT_SHAPE && loopScript[1] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
            loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 2) + SEARCH_SHAPE;
        } else if (loopScript[3] == SCRIPT_SHAPE && loopScript[0] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
                   loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_SHAPE;
        } else if (loopScript[0] == SCRIPT_FAMILY && loopScript[1] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
                   loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 2) + SEARCH_FAMILY;
        } else if (loopScript[3] == SCRIPT_FAMILY && loopScript[0] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
                   loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_FAMILY;
        }
    }
    if (selector != 0) {
        id = 0x7fff;
        next();
        return;
    }
    memmove(this->script, loopScript, length);
    setChunk(x, y);
    id = first;
    if (!isValid()) {
        nextChunk();
    }
    if (isValid()) {
        if (!contains() || !matches()) {
            next();
        }
    }
}

/* UNCALLED: nothing calls it. */
void AreaSearch::initChunk(i16 column, i16 row, const char *loopScript, i16 length)
{
    const u16 chunkLeft = column << 9;
    const u16 chunkTop = row << 9;
    const u16 chunkRight = chunkLeft + CHUNK_SIZE - 1;
    const u16 chunkBottom = chunkTop + CHUNK_SIZE - 1;

    init(chunkLeft, chunkTop, chunkRight, chunkBottom, loopScript, length);
}

bool AreaSearch::contains()
{
    const u16 candidateX = itemX[id];
    const u16 candidateY = itemY[id];

    return left <= candidateX && right >= candidateX && top <= candidateY && bottom >= candidateY;
}

void AreaSearch::next()
{
    if (selector != 0) {
        if (id == 0x7fff) {
            x /= CHUNK_SIZE;
            y /= CHUNK_SIZE;
            lastColumn = right >> CHUNK_SHIFT;
            lastRow = bottom >> CHUNK_SHIFT;
            id = mapChunks[x][y];
        } else {
            if (id == 0) {
                return;
            }
            id = itemNext[id];
        }
        switch (selector >> 12) {
        case SEARCH_SHAPE >> 12:
            nextOfShape(selector & 0xfff);
            return;
        case SEARCH_FAMILY >> 12:
            nextOfFamily(selector & 0xfff);
            return;
        case SEARCH_SHAPES >> 12:
            nextOfShapes(selector & 0xfff);
            return;
        case SEARCH_EITHER >> 12:
            nextOfEither(selector & 0xfff);
            return;
        case SEARCH_ALL >> 12:
            nextInArea();
            return;
        }
    }
    while (isValid()) {
        id = itemNext[id];
        if (!isValid()) {
            nextChunk();
        }
        if (isValid() && contains() && matches()) {
            return;
        }
    }
}

void AreaSearch::nextChunk()
{
    while (!isValid()) {
        x += CHUNK_SIZE;
        if ((x & CHUNK_MASK) > (right & CHUNK_MASK)) {
            x = left;
            y += CHUNK_SIZE;
            if ((y & CHUNK_MASK) > (bottom & CHUNK_MASK)) {
                return;
            }
        }
        setChunk(x, y);
        id = first;
    }
}

void RecursiveAreaSearch::next()
{
    while (isValid()) {
        if (contents() != 0U) {
            id = contents();
        } else {
            while (((itemFlags[id] & ITEM_CONTAINED) != 0) && itemNext[id] == 0) {
                id = container();
            }
            id = itemNext[id];
            if (!isValid()) {
                nextChunk();
            }
        }
        if (isValid() && contains() && matches()) {
            return;
        }
    }
}

bool RecursiveAreaSearch::contains()
{
    const u16 candidateX = itemX[id];
    const u16 candidateY = itemY[id];

    return candidateX >= left && candidateX <= right && candidateY >= top && candidateY <= bottom;
}

void ContainerSearch::init(u16 container, const char *loopScript, i16 length)
{
    if (length == -1) {
        length = *loopScript++;
    }
    first = Item(container).contents();
    memmove(this->script, loopScript, length);
    start();
}

void RecursiveContainerSearch::init(u16 searchedContainer, const char *loopScript, i16 length)
{
    if (length == -1) {
        length = *loopScript++;
    }
    this->container = searchedContainer;
    first = Item(searchedContainer).contents();
    memmove(this->script, loopScript, length);
    start();
}

void RecursiveContainerSearch::next()
{
    while (isValid()) {
        u16 after = itemNext[id];
        if (after == 0 && Item::container() != container) {
            after = ItemRef(Item::container()).next();
        }
        id = contents();
        if (!isValid()) {
            id = after;
        }
        if (isValid() && matches()) {
            return;
        }
    }
}

void SurfaceSearch::init(u16 surfaceItem, const char *loopScript, i16 length, u8 searchMode)
{
    i16 itemLeft;
    i16 itemTop;
    i16 width;
    i16 depth;
    i16 height;

    this->item = surfaceItem;
    this->mode = searchMode;
    side = 0;
    if (surfaceItem != 0U) {
        ItemRef base(this->item);
        ShapeInfo *info = &typeFlags.infos[base.shape()];

        info->box(width, depth, height);
        baseX = itemX[base.id];
        baseY = itemY[base.id];
        itemLeft = baseX - width + 1;
        itemTop = baseY - depth + 1;
        surface = itemZ[base.id] + height;
        initArea(itemLeft, itemTop, baseX + 0x100, baseY + 0x100, loopScript, length);
    } else {
        id = 0;
    }
}

void SurfaceSearch::initArea(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom, const char *loopScript,
                             i16 length)
{
    if (length == -1) {
        length = *loopScript++;
    }
    if (areaLeft < 0) {
        areaLeft = 0;
    }
    if (areaTop < 0) {
        areaTop = 0;
    }
    if (areaRight < 0) {
        areaRight = 0x7fff;
    }
    if (areaBottom < 0) {
        areaBottom = 0x7fff;
    }
    x = this->left = areaLeft;
    y = this->top = areaTop;
    this->right = areaRight;
    this->bottom = areaBottom;
    selector = 0;
    if (*loopScript == SCRIPT_END) {
        selector = SEARCH_ALL;
    } else if (*loopScript > SCRIPT_SHAPE && *loopScript < SCRIPT_FRAME) {
        const i16 count = *loopScript - SCRIPT_SHAPE;
        if (loopScript[static_cast<i16>(count + count + 1)] == SCRIPT_END) {
            if (count == 1) {
                selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_SHAPE;
            } else if (count == 2) {
                memmove(this->script, loopScript, length);
                selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_EITHER;
            } else {
                memmove(this->script, loopScript, length);
                selector = count + SEARCH_SHAPES;
            }
        }
    } else if (length == 6) {
        if (loopScript[0] == SCRIPT_SHAPE && loopScript[1] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
            loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 2) + SEARCH_SHAPE;
        } else if (loopScript[3] == SCRIPT_SHAPE && loopScript[0] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
                   loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_SHAPE;
        } else if (loopScript[0] == SCRIPT_FAMILY && loopScript[1] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
                   loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 2) + SEARCH_FAMILY;
        } else if (loopScript[3] == SCRIPT_FAMILY && loopScript[0] == SCRIPT_INT && loopScript[4] == SCRIPT_EQUAL &&
                   loopScript[5] == SCRIPT_END) {
            selector = *reinterpret_cast<const u16 *>(loopScript + 1) + SEARCH_FAMILY;
        }
    }
    if (selector != 0) {
        id = 0x7fff;
        next();
        return;
    }
    memmove(this->script, loopScript, length);
    setChunk(x, y);
    id = first;
    if (!isValid()) {
        nextChunk();
    }
    if (isValid()) {
        if (!contains() || !matches()) {
            next();
        }
    }
}

bool SurfaceSearch::contains()
{
    i16 width;
    i16 depth;
    i16 height;

    if (isValid() && item != id && AreaSearch::contains()) {
        ShapeInfo *info = &typeFlags.infos[itemShape[id]];

        info->box(width, depth, height);
        if (((mode & 1) != 0) && itemZ[id] == surface && static_cast<ui16>(itemX[id] - width + 1) <= baseX &&
            static_cast<ui16>(itemY[id] - depth + 1) <= baseY) {
            side = 1;
            return true;
        }
        if ((mode & 2) != 0) {
            if (ItemRef(item).level() == static_cast<i16>(itemZ[id] + height) &&
                static_cast<ui16>(itemX[id] - width + 1) <= baseX &&
                static_cast<ui16>(itemY[id] - depth + 1) <= baseY) {
                side = 2;
                return true;
            }
        }
    }
    side = 0;
    return false;
}

bool SurfaceSearch::accepts()
{
    i16 width;
    i16 depth;
    i16 height;

    if (id != 0U) {
        ShapeInfo *info = &typeFlags.infos[itemShape[id]];

        info->box(width, depth, height);
        if (((mode & 1) != 0) && itemZ[id] == surface && static_cast<ui16>(itemX[id] - width + 1) <= baseX &&
            static_cast<ui16>(itemY[id] - depth + 1) <= baseY) {
            side = 1;
            return true;
        }
        if ((mode & 2) != 0) {
            if (ItemRef(item).level() == static_cast<i16>(itemZ[id] + height) &&
                static_cast<ui16>(itemX[id] - width + 1) <= baseX &&
                static_cast<ui16>(itemY[id] - depth + 1) <= baseY) {
                side = 2;
                return true;
            }
        }
    } else {
        side = 0;
        return true;
    }
    side = 0;
    return false;
}

void SurfaceSearch::next()
{
    if (selector != 0) {
        if (id == 0x7fff) {
            x /= CHUNK_SIZE;
            y /= CHUNK_SIZE;
            lastColumn = right >> CHUNK_SHIFT;
            lastRow = bottom >> CHUNK_SHIFT;
            id = mapChunks[x][y];
        } else {
            if (id == 0) {
                return;
            }
            id = itemNext[id];
        }
        switch (selector >> 12) {
        case SEARCH_SHAPE >> 12:
            for (;;) {
                nextOfShape(selector & 0xfff);
                if (accepts()) {
                    return;
                }
                id = itemNext[id];
            }
        case SEARCH_FAMILY >> 12:
            for (;;) {
                nextOfFamily(selector & 0xfff);
                if (accepts()) {
                    return;
                }
                id = itemNext[id];
            }
        case SEARCH_SHAPES >> 12:
            for (;;) {
                nextOfShapes(selector & 0xfff);
                if (accepts()) {
                    return;
                }
                id = itemNext[id];
            }
        case SEARCH_EITHER >> 12:
            for (;;) {
                nextOfEither(selector & 0xfff);
                if (accepts()) {
                    return;
                }
                id = itemNext[id];
            }
        case SEARCH_ALL >> 12:
            for (;;) {
                nextInArea();
                if (accepts()) {
                    return;
                }
                id = itemNext[id];
            }
        }
    }
    while (isValid()) {
        id = itemNext[id];
        if (!isValid()) {
            nextChunk();
        }
        if (isValid() && contains() && matches()) {
            return;
        }
    }
}

void AreaList::init(i16 areaLeft, i16 areaTop, i16 areaRight, i16 areaBottom)
{
    if (areaLeft < 0) {
        areaLeft = 0;
    }
    if (areaTop < 0) {
        areaTop = 0;
    }
    if (areaRight < 0) {
        areaRight = 0x7fff;
    }
    if (areaBottom < 0) {
        areaBottom = 0x7fff;
    }
    column = this->left = areaLeft;
    row = this->top = areaTop;
    this->right = areaRight;
    this->bottom = areaBottom;
    column /= CHUNK_SIZE;
    row /= CHUNK_SIZE;
    lastColumn = this->right >> CHUNK_SHIFT;
    lastRow = this->bottom >> CHUNK_SHIFT;
    fill();
    index = 0;
    id = foundItems[index++];
}
