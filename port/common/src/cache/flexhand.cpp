/* ..\CACHE\FLEXHAND.C (1078:0000-02d0): FlexCacheHandler. See flexhand.h. */
#include "flexhand.h"
#include "cache.h"
#include "handler.h"

FlexCacheHandler::FlexCacheHandler(u16 entries, u16 handlerId, char *path) : CacheHandler(handlerId)
{
    firstHandle = allocateHandles(entries);
    registerCacheHandler(this, handlerId);
    archive = new FastFlex(path, FILE_READ, -1);
}

FlexCacheHandler::~FlexCacheHandler()
{
    if (archive != nullptr) {
        delete archive;
        archive = nullptr;
    }
}

/* handle becomes the slot in the handle table; the archive's index is worked
 * back from it. The original's behaviour: the block always goes to handler
 * 1, the fonts' handler, whatever the handler's own number (the gump shapes'
 * handler is 3). */
ShapeHeader *FlexCacheHandler::loadEntry(u16 handle) const
{
    handle += firstHandle;
    const u32 address = cacheHandles[handle];
    if (address != 0) {
        CacheBlock(address - static_cast<ui16>(sizeof(BlockHeader))).touch();
    } else {
        FlexEntry entry;
        const u16 index = handle - firstHandle;
        archive->readEntry(index, &entry);
        if (entry.size == 0) {
            halt();
        }
        const u32 data = cacheAllocate(entry.size, handle, 1);
        void *buffer = cachePointer(data);
        archive->readObject(index, buffer, -1, 0);
    }
    return static_cast<ShapeHeader *>(cachePointer(cacheHandles[handle]));
}

ShapeFrame *FlexCacheHandler::getFrame(u16 index, u16 frame) const
{
    ShapeHeader *shape = loadEntry(index);
    const u32 offset = shape->frames[frame].offset;
    /* The original makes a window at its selector's base plus the offset;
     * DESC.C's windows start at their base (linearPointer), so the frame
     * lies offset bytes into the shape. */
    return reinterpret_cast<ShapeFrame *>(reinterpret_cast<u8 *>(shape) + offset);
}
