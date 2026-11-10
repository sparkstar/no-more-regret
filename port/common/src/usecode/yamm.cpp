/* YAMM.C (1400:0000-0eef): the usecode's memory. See yamm.h. */
#include "gamestate.h"
#include <cstring>
#include "yamm.h"
#include "usemem.h"
#include "input.h"

u16 &yammChunks = gameGlobal<u16>();
u8 *&yammHeap = gameGlobal<u8 *>();
YammChunk *&yammFirst = gameGlobal<YammChunk *>();
u16 &yammOwner = gameGlobal<u16>();

void initYamm()
{
    YammChunk *last;

    yammOwner = 0;
    yammHeap = new u8[YAMM_SIZE];
    usecodeMemory.setYamm(yammHeap, YAMM_SIZE);
    memset(yammHeap, 0, YAMM_SIZE);
    const u16 offset = 9;
    last = YAMM_CHUNK(offset);
    yammFirst = YAMM_CHUNK(1);
    yammFirst->prev = yammFirst->next = offset;
    last->prev = last->next = 1;
    yammFirst->size = 0;
    yammFirst->markUsed();
    yammFirst->owner = 0;
    last->size = YAMM_SIZE - 16;
    last->markFree();
    last->owner = 0;
    yammChunks = 2;
}

void freeYamm()
{
    if (yammHeap != nullptr) {
        usecodeMemory.remove(yammHeap);
        delete[] yammHeap;
        yammHeap = nullptr;
    }
}

u16 yammString(const char *text)
{
    const u16 length = strlen(text) + 1;
    const u16 offset = yammAlloc(length);

    if (offset != 0U) {
        strcpy(reinterpret_cast<char *>(yammHeap + offset), text);
    }
    return offset;
}

u16 yammAlloc(u16 size)
{
    YammChunk *chunk;

    for (u16 offset = yammFirst->next; offset != 1; offset = chunk->next) {
        chunk = YAMM_CHUNK(offset);
        u16 length = chunk->length();
        if ((chunk->isFree() != 0U) && length >= size) {
            const u16 spare = length - size;
            u16 extra = 0;

            if (spare > 8) {
                YammChunk *next = YAMM_CHUNK(chunk->next);
                const u16 split = offset + size + 8;
                YammChunk *rest = YAMM_CHUNK(split);

                rest->next = chunk->next;
                rest->prev = offset;
                next->prev = chunk->next = split;
                length = spare - 8;
                rest->size = length;
                rest->markFree();
                yammChunks++;
            } else {
                extra = spare;
            }
            length = size + extra;
            chunk->size = length;
            chunk->owner = yammOwner;
            chunk->markUsed();
            return offset + 8;
        }
    }
    halt();
    return 0;
}

void yammFreeOwner(u16 owner)
{
    YammChunk *chunk;

    for (u16 offset = yammFirst->next; offset != 1; offset = chunk->next) {
        chunk = YAMM_CHUNK(offset);
        if (chunk->owner == owner && (chunk->isFree() == 0U)) {
            yammFree(offset + 8);
        }
    }
}

void yammFree(u16 offset)
{
    if (offset != 0U) {
        offset -= 8;
        YammChunk *chunk = YAMM_CHUNK(offset);
        YammChunk *prev = YAMM_CHUNK(chunk->prev);
        YammChunk *next = YAMM_CHUNK(chunk->next);

        chunk->markFree();
        if (next->isFree() != 0U) {
            chunk->next = next->next;
            YammChunk *after = YAMM_CHUNK(next->next);
            after->prev = offset;
            chunk->grow(next->length() + 8);
            next = after;
            yammChunks--;
        }
        if (prev->isFree() != 0U) {
            prev->next = chunk->next;
            next->prev = chunk->prev;
            prev->grow(chunk->length() + 8);
            yammChunks--;
        }
    }
}

/* UNCALLED: nothing calls it. */
void yammDump()
{
    debugPrintf(0xff, "pos  next prev free size pid\r\n");
    u16 offset = 1;
    for (u16 i = 0; i < yammChunks; i++) {
        YammChunk *chunk = YAMM_CHUNK(offset);
        debugPrintf(0xff, "%04X %04X %04X %04X %04X %04X\r\n", offset, chunk->next, chunk->prev, chunk->isFree(),
                    chunk->length(), chunk->owner);
        offset = chunk->next;
    }
}

void yammFailure(i16 check, std::source_location place)
{
    haltMessage(haltFormat("Yamm damn @%d", check), place);
}

/* Walks the ring both ways and halts when it is broken. */
void yammCheck(std::source_location place)
{
    YammChunk *forward = yammFirst;
    YammChunk *backward = yammFirst;
    u32 total = 0;

    for (u16 i = 0; i < yammChunks; i++) {
        forward = YAMM_CHUNK(forward->next);
        backward = YAMM_CHUNK(backward->prev);
        total += static_cast<ui16>(forward->length() + 8 + forward->length() + 8);
    }
    if (forward != backward || forward != yammFirst) {
        yammFailure(239, place);
    }
    if (total / 2 != YAMM_SIZE) {
        yammFailure(243, place);
    }
}

void loadYamm(Flex *flex, i16 object)
{
    flex->readObject(object, &yammChunks, 2L, 0L);
    if (yammHeap != nullptr) {
        usecodeMemory.remove(yammHeap);
        delete[] yammHeap;
        yammHeap = nullptr;
    }
    yammHeap = new u8[YAMM_SIZE];
    usecodeMemory.setYamm(yammHeap, YAMM_SIZE);
    object++;
    flex->readObject(object, yammHeap, YAMM_SIZE, 0L);
    yammFirst = YAMM_CHUNK(1);
}

void saveYamm(Flex *flex, i16 object)
{
    flex->writeObject(object, &yammChunks, 2L, 0L, 1);
    flex->writeObject(object + 1, yammHeap, YAMM_SIZE, 0L, 1);
}

void YammList::clear() const
{
    u16 node = head;

    while (node != 0U) {
        u16 *data = reinterpret_cast<u16 *>(yammHeap + node);
        const u16 next = data[0];
        if (strings != 0U) {
            yammFree(data[1]);
        }
        yammFree(node);
        node = next;
    }
}

u8 YammList::next(u16 *node) const
{
    if (*node != 0U) {
        *node = *reinterpret_cast<u16 *>(yammHeap + *node);
    } else {
        *node = head;
    }
    return *node == 0 ? 0 : 1;
}

u16 YammList::copy(YammList *from)
{
    clear();
    u16 node = 0;
    u16 *last = nullptr;

    size = from->size;
    strings = from->strings;
    while (from->next(&node) != 0U) {
        const u16 offset = yammAlloc(size + 2);
        u16 *target = reinterpret_cast<u16 *>(yammHeap + offset);
        u16 *source = reinterpret_cast<u16 *>(yammHeap + node);

        *target = 0;
        if (strings != 0U) {
            char *text = reinterpret_cast<char *>(yammHeap + source[1]);
            const u16 length = strlen(text) + 1;
            const u16 string = yammAlloc(length);
            if (string == 0U) {
                halt();
            }
            memcpy(yammHeap + string, text, length);
            target[1] = string;
        } else {
            memcpy(target + 1, source + 1, size);
        }
        if (last != nullptr) {
            *last = offset;
        } else {
            head = offset;
        }
        last = target;
    }
    return head;
}

void *YammList::at(i16 index) const
{
    u16 node = 0;
    i16 i = 0;

    while (i++ <= index) {
        if (next(&node) == 0U) {
            return nullptr;
        }
    }
    if (strings != 0U) {
        return yammHeap + *reinterpret_cast<u16 *>(NODE_DATA(node));
    }
    return NODE_DATA(node);
}

char *YammList::string(u16 node)
{
    return reinterpret_cast<char *>(yammHeap + *reinterpret_cast<u16 *>(NODE_DATA(node)));
}

void YammList::append(u16 node)
{
    u16 cursor = 0;
    u16 last = 0;

    while (next(&cursor) != 0U) {
        last = cursor;
    }
    if (last != 0U) {
        *reinterpret_cast<u16 *>(yammHeap + last) = node;
    } else {
        head = node;
    }
}

void YammList::merge(u16 first)
{
    u16 *target;
    char *text;
    u16 length;
    u16 offset;
    const YammList other(first, size, strings);
    u16 node = 0;

    while (other.next(&node) != 0U) {
        u16 mine = 0;
        u16 *value = reinterpret_cast<u16 *>(NODE_DATA(node));

        while (next(&mine) != 0U) {
            u16 *existing = reinterpret_cast<u16 *>(NODE_DATA(mine));

            if (strings != 0U) {
                char *a = reinterpret_cast<char *>(yammHeap + *value);
                char *b = reinterpret_cast<char *>(yammHeap + *existing);
                if (strcmp(a, b) == 0) {
                    goto found;
                }
            } else if (memcmp(value, existing, size) == 0) {
                goto found;
            }
        }
        offset = yammAlloc(size + 2);
        target = reinterpret_cast<u16 *>(yammHeap + offset);
        *target = 0;
        target++;
        if (strings != 0U) {
            text = reinterpret_cast<char *>(yammHeap + *value);
            length = strlen(text) + 1;
            const u16 string = yammAlloc(length);
            memcpy(yammHeap + string, text, length);
            *target = string;
        } else {
            memcpy(target, value, size);
        }
        append(offset);
        /* A value the list already holds: on to the next. */
    found:;
    }
}

void YammList::remove(u16 first)
{
    const YammList other(first, size, strings);
    u16 node = 0;

    while (other.next(&node) != 0U) {
        u16 mine = 0;
        u16 *value = reinterpret_cast<u16 *>(NODE_DATA(node));
        u8 found = 0;
        u16 *previous = nullptr;
        /* Read only when found, which comes after it is set. */
        u16 *existing = nullptr;

        while (next(&mine) != 0U) {
            existing = reinterpret_cast<u16 *>(NODE_DATA(mine));
            if (strings != 0U) {
                char *a = reinterpret_cast<char *>(yammHeap + *value);
                char *b = reinterpret_cast<char *>(yammHeap + *existing);
                if (strcmp(a, b) == 0) {
                    found++;
                    break;
                }
            } else if (memcmp(value, existing, size) == 0) {
                found++;
                break;
            }
            previous = existing;
        }
        if (found != 0U) {
            existing--;
            if (previous != nullptr) {
                previous--;
                *previous = *existing;
            } else {
                head = *existing;
            }
            if (strings != 0U) {
                existing++;
                yammFree(*existing);
            }
            yammFree(mine);
        }
    }
}

i16 YammList::contains(void *value)
{
    if (strings != 0U) {
        value = yammHeap + *static_cast<u16 *>(value);
    }
    u16 node = 0;
    while (next(&node) != 0U) {
        if ((strings != 0U) && (strcmp(string(node), static_cast<char *>(value)) == 0)) {
            return 1;
        }
        if (memcmp(NODE_DATA(node), value, size) == 0) {
            return 1;
        }
    }
    return 0;
}

void YammList::set(i16 index, void *value) const
{
    u16 node = 0;
    i16 i = 0;

    while (i++ < index) {
        if (next(&node) == 0U) {
            return;
        }
    }
    if (strings != 0U) {
        u16 *data = reinterpret_cast<u16 *>(yammHeap + node);
        yammFree(data[1]);
    }
    memcpy(NODE_DATA(node), value, size);
}

void YammList::setOwner(u16 owner) const
{
    u16 node = 0;

    while (next(&node) != 0U) {
        if (strings != 0U) {
            setChunkOwner(*reinterpret_cast<u16 *>(NODE_DATA(node)), owner);
        }
        setChunkOwner(node, owner);
    }
}

/* UNCALLED: nothing calls it. */
void printLongs(u16 head)
{
    const YammList list(head, 4, 0);
    u16 node = 0;

    while (list.next(&node) != 0U) {
        debugPrintf(50, "%d\r\n", *reinterpret_cast<s32 *>(NODE_DATA(node)));
    }
}

/* UNCALLED: nothing calls it. */
void printInts(u16 head)
{
    const YammList list(head, 2, 0);
    u16 node = 0;

    while (list.next(&node) != 0U) {
        debugPrintf(50, "%d\r\n", *reinterpret_cast<s32 *>(NODE_DATA(node)));
    }
}

/* UNCALLED: nothing calls it. */
void printStrings(u16 head)
{
    YammList list(head, 2, 0);
    u16 node = 0;

    while (list.next(&node) != 0U) {
        char *text = list.string(node);
        debugPrintf(50, "%s\r\n", text);
    }
}

/* UNCALLED: nothing calls it. */
void showYamm()
{
    yammDump();
}

/* UNCALLED: nothing calls it. */
void showYammWait()
{
    yammDump();
    inputKey();
}
