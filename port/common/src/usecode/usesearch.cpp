/* The searches of the usecode's loops on its stack. See usesearch.h.
 *
 * BCC's layout, which the original's code reads (10d0:0771, 0e4c, 0f73) and
 * INTER.C's inline constructors write (13f0:3468-37a6): an ItemSearch is
 * its item at 0, the near vtable at 2, first at 4, the script's 32 bytes at
 * 6 and the selector at 26h; an AreaSearch adds left, top, right, bottom, x,
 * y, lastColumn and lastRow at 28h-36h, a SurfaceSearch then baseX 38h,
 * baseY 3Ah, surface 3Ch, item 3Dh, mode 3Fh and side 40h; a
 * RecursiveContainerSearch adds its container at 28h. */
#include "usesearch.h"
#include "searches.h"
#include <crash.h>

#include <cstring>
#include <typeinfo>

namespace {

enum { AREA = 2, RECURSIVE_AREA = 3, CONTAINER = 4, RECURSIVE_CONTAINER = 5, SURFACE = 6 };

/* Each kind's size; its vtable is the game's original's (searches.h, in
 * this order). */
struct Kind {
    i16 kind;
    u16 size;
};
const Kind KINDS[] = {
    {AREA, 0x38}, {RECURSIVE_AREA, 0x38}, {CONTAINER, 0x28}, {RECURSIVE_CONTAINER, 0x2a}, {SURFACE, 0x41}};

u16 vtableOf(const Kind &k)
{
    return SEARCH_VTABLES[&k - KINDS];
}

const Kind *byKind(i16 kind)
{
    for (const Kind &k : KINDS) {
        if (k.kind == kind) {
            return &k;
        }
    }
    return nullptr;
}

const Kind *byVtable(u16 vtable)
{
    for (const Kind &k : KINDS) {
        if (vtableOf(k) == vtable) {
            return &k;
        }
    }
    return nullptr;
}

[[noreturn]] void unknown(const char *what, unsigned value)
{
    fatalError(3, "a usecode loop's search of %s %X", what, value);
}

u16 word(const u8 *image, u16 at)
{
    return static_cast<u16>(image[at] | image[at + 1] << 8);
}

void putWord(u8 *image, u16 at, u16 value)
{
    image[at] = static_cast<u8>(value);
    image[at + 1] = static_cast<u8>(value >> 8);
}

bool isArea(i16 kind)
{
    return kind == AREA || kind == RECURSIVE_AREA || kind == SURFACE;
}

}

u16 searchImageSize(i16 kind)
{
    const Kind *k = byKind(kind);
    if (k == nullptr) {
        unknown("kind", static_cast<unsigned>(kind));
    }
    return k->size;
}

void putSearchImage(u8 *image, const ItemSearch *search, i16 kind)
{
    const Kind *k = byKind(kind);
    if (k == nullptr) {
        unknown("kind", static_cast<unsigned>(kind));
    }
    std::memset(image, 0, k->size);
    putWord(image, 0, search->id);
    putWord(image, 2, vtableOf(*k));
    putWord(image, 4, search->first);
    std::memcpy(image + 6, search->script, sizeof(search->script));
    putWord(image, 0x26, static_cast<u16>(search->selector));
    if (isArea(kind)) {
        const AreaSearch *area = static_cast<const AreaSearch *>(search);
        const u16 fields[] = {area->left, area->top, area->right,      area->bottom,
                              area->x,    area->y,   area->lastColumn, area->lastRow};
        for (int i = 0; i < 8; i++) {
            putWord(image, static_cast<u16>(0x28 + (2 * i)), fields[i]);
        }
    }
    if (kind == RECURSIVE_CONTAINER) {
        putWord(image, 0x28, (static_cast<const RecursiveContainerSearch *>(search))->container);
    }
    if (kind == SURFACE) {
        const SurfaceSearch *surface = static_cast<const SurfaceSearch *>(search);
        putWord(image, 0x38, surface->baseX);
        putWord(image, 0x3a, surface->baseY);
        image[0x3c] = surface->surface;
        putWord(image, 0x3d, surface->item);
        image[0x3f] = surface->mode;
        image[0x40] = surface->side;
    }
}

bool getSearchImage(const u8 *image, ItemSearch *search)
{
    const Kind *k = byVtable(word(image, 2));
    if (k == nullptr) {
        unknown("vtable", word(image, 2));
    }
    bool right = false;
    switch (k->kind) {
    case AREA:
        right = typeid(*search) == typeid(AreaSearch);
        break;
    case RECURSIVE_AREA:
        right = typeid(*search) == typeid(RecursiveAreaSearch);
        break;
    case CONTAINER:
        right = typeid(*search) == typeid(ContainerSearch);
        break;
    case RECURSIVE_CONTAINER:
        right = typeid(*search) == typeid(RecursiveContainerSearch);
        break;
    case SURFACE:
        right = typeid(*search) == typeid(SurfaceSearch);
        break;
    }
    if (!right) {
        return false;
    }
    search->id = word(image, 0);
    search->first = word(image, 4);
    std::memcpy(search->script, image + 6, sizeof(search->script));
    search->selector = static_cast<i16>(word(image, 0x26));
    if (isArea(k->kind)) {
        AreaSearch *area = static_cast<AreaSearch *>(search);
        u16 *fields[] = {&area->left, &area->top, &area->right,      &area->bottom,
                         &area->x,    &area->y,   &area->lastColumn, &area->lastRow};
        for (int i = 0; i < 8; i++) {
            *fields[i] = word(image, static_cast<u16>(0x28 + (2 * i)));
        }
    }
    if (k->kind == RECURSIVE_CONTAINER) {
        (static_cast<RecursiveContainerSearch *>(search))->container = word(image, 0x28);
    }
    if (k->kind == SURFACE) {
        SurfaceSearch *surface = static_cast<SurfaceSearch *>(search);
        surface->baseX = word(image, 0x38);
        surface->baseY = word(image, 0x3a);
        surface->surface = image[0x3c];
        surface->item = word(image, 0x3d);
        surface->mode = image[0x3f];
        surface->side = image[0x40];
    }
    return true;
}

u16 nextSearchImage(u8 *image)
{
    AreaSearch area;
    RecursiveAreaSearch recursiveArea;
    ContainerSearch container;
    RecursiveContainerSearch recursiveContainer;
    SurfaceSearch surface;
    ItemSearch *searches[] = {&area, &recursiveArea, &container, &recursiveContainer, &surface};
    for (ItemSearch *search : searches) {
        if (getSearchImage(image, search)) {
            const Kind *k = byVtable(word(image, 2));
            search->next();
            putSearchImage(image, search, k->kind);
            return search->id;
        }
    }
    unknown("vtable", word(image, 2));
}
