/* ..\CACHE\SHAPHAND.C (No Regret's 1080:0000-1ca0, No Remorse's
 * 1070:0000-1c67; the addresses are No Regret's): the shapes in the cache,
 * both games', the games' differences as each game's values
 * (shaphand_game.h). See shaphand.h. */
#include "gamestate.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "shaphand.h"
#include "cache.h"
#include "cache_game.h"
#include "clock.h"
#include "host.h"
#include "kernel.h"
#include "world.h"
#include "x32code.h"
#include "cprotmem.h"
#include "filespec.h"
#include "gamefile.h"
#include "handler.h"
#include "itemdam.h"
#include "type.h"
#include "utility.h"
#include "files.h"

/* The first of the shapes' handles; the handler uses the shape number. */
u16 &shapeHandles = gameGlobal<u16>(0);
EditableFlex *&shapeFlex = gameGlobal<EditableFlex *>(nullptr);
FastFlex *&swapFlex = gameGlobal<FastFlex *>(nullptr);

/* trimSwap keeps the NPCs' shapes. */
u8 &keepNpcShapes = gameGlobal<u8>(0);

ShapeCacheHandler *&shapeCache = gameGlobal<ShapeCacheHandler *>(nullptr);
u8 &allShapes = gameGlobal<u8>(ALL_SHAPES);
u8 &keepSwap = gameGlobal<u8>(1);
ShapeFrameCacheHandler *&frameCache = gameGlobal<ShapeFrameCacheHandler *>();

void initShapeCache()
{
    if (SHAPES_FROM_SWAP) {
        shapeCache = new ShapeHandler;
    } else {
        shapeCache = new ShapeCacheHandler(1);
    }
    frameCache = new ShapeFrameCacheHandler;
}

ShapeFrame *cachedShapeFrame(u16 shape, u16 frame)
{
    if (SHAPES_FROM_SWAP) {
        return static_cast<ShapeHandler *>(shapeCache)->getFrame(shape, frame);
    }
    return shapeCache->getFrame(shape, frame);
}

/* A 1x1 frame drawn for a shape that cannot be read: a ShapeFrame with its line
 * table and one pixel. */
u8 (&missingFrame)[25] = gameGlobal<u8[25]>({0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 1});

ShapeFlex::ShapeFlex()
    : EditableFlex(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr), FILE_READ, -1)
{
}

ShapeCacheHandler::ShapeCacheHandler(u8 open) : CacheHandler(SHAPE_HANDLER)
{
    if (open == 0U) {
        return;
    }
    shapeHandles = allocateHandles(SHAPES);
    registerCacheHandler(this, SHAPE_HANDLER);
    shapeFlex = new ShapeFlex;
}

void ShapeCacheHandler::frameMetrics(u16 shape, u16 frame, u16 *width, u16 *height, s16 *xOffset, s16 *yOffset)
{
    /* The original's behaviour: the frame comes from shapeCache, not this;
     * only shape 1, the stand-in, comes from this. */
    ShapeFrame *data = cachedShapeFrame(shape, frame);
    if (data == nullptr) {
        data = getFrame(1, 0);
    }
    *width = data->width;
    *height = data->height;
    *xOffset = data->xOffset;
    *yOffset = data->yOffset;
}

void ShapeCacheHandler::shapeHeader(u16 shape, u16 *width, u16 *height, u16 *frames)
{
    if (cacheHandles[shape] == 0) {
        ShapeFrame *frame = getFrame(shape, 0);
        if (frame == nullptr) {
            shape = 0;
            getFrame(0, 0);
        }
    }
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(cacheHandles[shape]));
    *width = header->width;
    *height = header->height;
    *frames = header->frameCount;
}

void ShapeCacheHandler::frameBounds(u16 shape, u16 frame, FrameRect *bounds, s16 x, s16 y)
{
    u16 width;
    u16 height;
    s16 xOffset;
    s16 yOffset;

    frameMetrics(shape, frame, &width, &height, &xOffset, &yOffset);
    bounds->left = x - xOffset;
    bounds->top = y - yOffset;
    bounds->right = bounds->left + width - 1;
    bounds->bottom = bounds->top + height - 1;
}

/* The frame's data follows its FrameBlock in the shape file, so the block
 * is read whole and its start filled in after. */
FrameBlock *ShapeCacheHandler::loadFrame(u16 shape, u16 frame, u32 data, u32 entry)
{
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(data));
    const s32 size = header->frames[frame].size;
    if (size == 0) {
        header->frames[frame].offset = 0;
        return nullptr;
    }
    if (memoryProfiler != 0U) {
        drawViewport->clear(borlandRandom(240) + 16, 0, 5, 9, 15);
        screenViewport->blit(drawViewport, 0, 5, 9, 15, 0, 5);
    }
    CacheBlock(cacheHandles[shape] - static_cast<ui16>(sizeof(BlockHeader))).touch();
    const u32 address = cacheAllocate(size, NO_HANDLE, FRAME_HANDLER);
    if (address == 0 || cacheHandles[shape] == 0) {
        return nullptr;
    }
    CacheBlock(address - static_cast<ui16>(sizeof(BlockHeader))).touch();
    CacheBlock(cacheHandles[shape] - static_cast<ui16>(sizeof(BlockHeader))).touch();
    FlexEntry flexEntry;
    shapeFlex->readEntry(shape, &flexEntry);
    const s32 position = entry & ~FRAME_ON_DISK;
    FrameBlock *block = static_cast<FrameBlock *>(cachePointer(address));
    shapeFlex->readObject(shape, block, size, position);
    header = static_cast<ShapeHeader *>(cachePointer(cacheHandles[shape]));
    block->entry = header->frames[frame].offset;
    header->frames[frame].offset = address;
    block->shape = shape;
    block->frame = frame;
    return block;
}

/* The original's behaviour: a frame that cannot be read gives missingFrame
 * when the shape was in the cache but 0 when the shape was just read;
 * ShapeHandler::getFrame has it the other way round. */
ShapeFrame *ShapeCacheHandler::getFrame(u16 shape, u16 frame)
{
    const u32 data = cacheHandles[shape];
    if (data != 0) {
        ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(data));
        if (static_cast<ui16>(header->frameCount) <= frame) {
            return nullptr;
        }
        u32 entry = header->frames[frame].offset;
        if (entry == 0) {
            return nullptr;
        }
        entry &= ~FRAME_KEPT;
        if ((entry & FRAME_ON_DISK) != 0U) {
            u8 *block = reinterpret_cast<u8 *>(loadFrame(shape, frame, data, entry));
            if (block != nullptr) {
                block += sizeof(FrameBlock);
                return reinterpret_cast<ShapeFrame *>(block);
            }
            return reinterpret_cast<ShapeFrame *>(missingFrame);
        }
        CacheBlock(entry - static_cast<ui16>(sizeof(BlockHeader))).touch();
        CacheBlock(data - static_cast<ui16>(sizeof(BlockHeader))).touch();
        return static_cast<ShapeFrame *>(cachePointer(entry + static_cast<ui16>(sizeof(FrameBlock))));
    }
    FlexEntry flexEntry;
    shapeFlex->readEntry(shape, &flexEntry);
    if (flexEntry.isEmpty() != 0) {
        return nullptr;
    }
    ShapeSize head;
    shapeFlex->readObject(shape, &head, sizeof(head), 0);
    i16 bytes = (head.frameCount * sizeof(ShapeFrameEntry)) + sizeof(head);
    const u32 address = cacheAllocate(bytes, shape, SHAPE_HANDLER);
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(address));
    memcpy(header, &head, sizeof(head));
    bytes = bytes - sizeof(head);
    shapeFlex->readObject(shape, header->frames, bytes, sizeof(head));
    ShapeFrameEntry *frames = header->frames;
    for (i16 i = 0; i < head.frameCount; i++) {
        frames->offset |= FRAME_ON_DISK;
        frames++;
    }
    if (static_cast<ui16>(header->frameCount) <= frame) {
        return nullptr;
    }
    const u32 entry = header->frames[frame].offset;
    u8 *block = reinterpret_cast<u8 *>(loadFrame(shape, frame, address, entry));
    if (block != nullptr) {
        block += sizeof(FrameBlock);
    }
    return reinterpret_cast<ShapeFrame *>(block);
}

/* UNCALLED: nothing calls it. */
ShapeFrame *ShapeCacheHandler::getFrame(u16 shape, u16 frame, u16 *size)
{
    *size = 0;
    ShapeFrame *data = getFrame(shape, frame);
    if (data == nullptr) {
        return nullptr;
    }
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(cacheHandles[shape]));
    *size = header->frames[frame].size;
    return data;
}

bool ShapeCacheHandler::check(u32 block)
{
    BlockHeader *header = static_cast<BlockHeader *>(cachePointer(block));
    const u16 handle = header->handle;
    ShapeHeader *shape = static_cast<ShapeHeader *>(cachePointer(block + static_cast<ui16>(sizeof(BlockHeader))));
    const u16 count = shape->frameCount;
    for (u16 i = 0; i < count; i++) {
        shape = static_cast<ShapeHeader *>(cachePointer(block + static_cast<ui16>(sizeof(BlockHeader))));
        u32 entry = shape->frames[i].offset;
        entry &= ~FRAME_KEPT;
        if ((entry & FRAME_ON_DISK) == 0U) {
            FrameBlock *frame = static_cast<FrameBlock *>(cachePointer(entry));
            if (frame->shape != handle || frame->frame != i) {
                return false;
            }
        }
    }
    return true;
}

void ShapeCacheHandler::freeShape(u16 shape)
{
    const u32 data = cacheHandles[shape];
    if (data == 0) {
        return;
    }
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(data));
    const u16 count = header->frameCount;
    for (u16 i = 0; i < count; i++) {
        header = static_cast<ShapeHeader *>(cachePointer(data));
        if ((header->frames[i].offset & FRAME_ON_DISK) == 0U) {
            freeFrame(data, i);
        }
    }
    CacheBlock(data - static_cast<ui16>(sizeof(BlockHeader))).release();
}

void ShapeCacheHandler::freeFrame(u32 data, u16 frame)
{
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(data));
    if (static_cast<ui16>(header->frameCount) <= frame) {
        return;
    }
    u32 entry = header->frames[frame].offset;
    if (entry == 0) {
        return;
    }
    entry &= ~FRAME_KEPT;
    if ((entry & FRAME_ON_DISK) != 0U) {
        return;
    }
    const CacheBlock block(entry - static_cast<ui16>(sizeof(BlockHeader)));
    freeBlock(block);
}

/* The swap file is the shape file in the static directory of the data
 * drive; the shape file itself is made in the game's static directory. */
ShapeHandler::ShapeHandler() : ShapeCacheHandler(0)
{
    shapeHandles = allocateHandles(SHAPES);
    registerCacheHandler(this, SHAPE_HANDLER);
    FileSpec swap(makePath(gameDrive, staticDirectory, gameFiles[GAME_SHAPES], nullptr), 0);
    strcat(swap.dir, "\\");
    if (swap.exists() != 0) {
        swapFlex = new FastFlex(swap.path(), FILE_READ, -1);
    } else {
        swapFlex = nullptr;
    }
    createSwap();
    FileSpec shapes(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr), 0);
    if (shapes.exists() == 0) {
        if (swapFlex == nullptr) {
            halt();
        }
        swapFlex->seek(100, SEEK_SET);
        s32 size;
        s32 value;
        swapFlex->read(&value, 4);
        size = value;
        File file(shapes.path(), FILE_READWRITE, 0);
        file.copy(0, 0, size, swapFlex);
        file.close();
        shapeFlex = new EditableFlex(shapes.path(), FILE_READWRITE, -1);
        trimSwap();
    } else {
        shapeFlex = new EditableFlex(shapes.path(), FILE_READWRITE, -1);
    }
}

ShapeHandler::~ShapeHandler()
{
    closeSwap();
    if (shapeFlex != nullptr) {
        delete shapeFlex;
        shapeFlex = nullptr;
    }
    if (swapFlex != nullptr) {
        delete swapFlex;
        swapFlex = nullptr;
    }
}

void ShapeHandler::trimSwap() const
{
    if (shapeFlex != nullptr && allShapes == 0) {
        FlexHeader header;
        shapeFlex->readHeader(&header);
        const s32 size = swapSize;
        FlexEntry entry;
        for (i16 i = 0; i < SHAPES; i++) {
            const ShapeInfo info = typeFlags.info(i);
            shapeFlex->readEntry(i, &entry);
            /* The original tests entry.offset twice. */
            if (!info.preload && entry.offset != 0 && ((keepNpcShapes == 0U) || !info.npc || entry.offset == 0)) {
                entry.offset = 0;
                entry.size = 0;
                shapeFlex->writeEntry(i, &entry);
            }
        }
        shapeFlex->setSize(size);
    }
}

/* The original's behaviour: a frame that cannot be read gives 0 when the
 * shape was in the cache but missingFrame when the shape was just read, the
 * other way round from ShapeCacheHandler::getFrame. */
ShapeFrame *ShapeHandler::getFrame(u16 shape, u16 frame)
{
    const u32 data = cacheHandles[shape];
    if (data != 0) {
        ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(data));
        if (static_cast<ui16>(header->frameCount) <= frame) {
            return nullptr;
        }
        u32 entry = header->frames[frame].offset;
        if (entry == 0) {
            return nullptr;
        }
        entry &= ~FRAME_KEPT;
        if ((entry & FRAME_ON_DISK) != 0U) {
            u8 *block = reinterpret_cast<u8 *>(loadFrame(shape, frame, data, entry));
            if (block != nullptr) {
                block += sizeof(FrameBlock);
            }
            return reinterpret_cast<ShapeFrame *>(block);
        }
        CacheBlock(entry - static_cast<ui16>(sizeof(BlockHeader))).touch();
        CacheBlock(data - static_cast<ui16>(sizeof(BlockHeader))).touch();
        return static_cast<ShapeFrame *>(cachePointer(entry + static_cast<ui16>(sizeof(FrameBlock))));
    }
    FlexEntry flexEntry;
    shapeFlex->readEntry(shape, &flexEntry);
    if (flexEntry.isEmpty() != 0) {
        if (allShapes != 0U) {
            return reinterpret_cast<ShapeFrame *>(missingFrame);
        }
        FlexEntry swapEntry;
        swapFlex->readEntry(shape, &swapEntry);
        if (swapEntry.isEmpty() != 0) {
            return nullptr;
        }
        if (cdTransferDisplay != 0U) {
            /* The original writes at the BIOS's text cursor 1,1, keeping
             * the SVGA window; the port at the host's text place. BCC
             * passes the size's low word for %d. The keys come as the
             * time passes (clock.h). */
            hostTextAt(1, 1);
            hostPrint("Loading from CD:  %4d (%4dK)", shape, static_cast<i16>(swapEntry.size / 1024));
            while (pendingKeys != 0U) {
                clockWait();
            }
            while (pendingKeys == 0U) {
                clockWait();
            }
        }
        u8 *buffer = new u8[0x1000];
        s32 copied = 0;
        swapFlex->seekObject(shape);
        shapeFlex->seek(0, SEEK_END);
        flexEntry.offset = shapeFlex->tell();
        flexEntry.size = swapEntry.size;
        shapeFlex->writeEntry(shape, &flexEntry);
        shapeFlex->seek(0, SEEK_END);
        while (copied + 0x1000 < swapEntry.size) {
            swapFlex->read(buffer, 0x1000);
            shapeFlex->write(buffer, 0x1000);
            copied += 0x1000;
        }
        swapFlex->read(buffer, swapEntry.size - copied);
        shapeFlex->write(buffer, swapEntry.size - copied);
        if (buffer != nullptr) {
            delete[] buffer;
        }
    }
    ShapeSize head;
    shapeFlex->readObject(shape, &head, sizeof(head), 0);
    i16 bytes = (head.frameCount * sizeof(ShapeFrameEntry)) + sizeof(head);
    const u32 address = cacheAllocate(bytes, shape, SHAPE_HANDLER);
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(address));
    memcpy(header, &head, sizeof(head));
    bytes = bytes - sizeof(head);
    shapeFlex->readObject(shape, header->frames, bytes, sizeof(head));
    ShapeFrameEntry *frames = header->frames;
    for (i16 i = 0; i < head.frameCount; i++) {
        frames->offset |= FRAME_ON_DISK;
        frames++;
    }
    if (static_cast<ui16>(header->frameCount) <= frame) {
        return nullptr;
    }
    const u32 entry = header->frames[frame].offset;
    u8 *block = reinterpret_cast<u8 *>(loadFrame(shape, frame, address, entry));
    if (block != nullptr) {
        block += sizeof(FrameBlock);
        return reinterpret_cast<ShapeFrame *>(block);
    }
    return reinterpret_cast<ShapeFrame *>(missingFrame);
}

void ShapeHandler::closeSwap() const
{
    if (keepSwap == 0U) {
        /* No Regret's halts: the rest of this branch, which makes the shape
         * file anew, runs in No Remorse only. The port's keepSwap is always 1. */
        if (SWAP_REBUILD_HALTS) {
            haltMessage("keepSwap=FALSE");
        }
        if (shapeFlex != nullptr) {
            delete shapeFlex;
            shapeFlex = nullptr;
        }
        fileRemove(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr));
        Flex flex;
        flex.create(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr), SHAPES);
    } else {
        trimSwap();
    }
    if (allShapes == 0U) {
        fileRemove(makePath(nullptr, staticDirectory, "dl.dat", nullptr));
    }
}

ShapeFrameCacheHandler::ShapeFrameCacheHandler() : CacheHandler(FRAME_HANDLER)
{
    registerCacheHandler(this, FRAME_HANDLER);
}

void ShapeFrameCacheHandler::relocated(u32 data)
{
    FrameBlock *block = static_cast<FrameBlock *>(cachePointer(data));
    const u32 shape = cacheHandles[block->shape];
    if (shape == 0) {
        halt();
    }
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(shape));
    const u32 kept = header->frames[block->frame].offset & FRAME_KEPT;
    header->frames[block->frame].offset = data | kept;
}

void ShapeFrameCacheHandler::evicted(u32 data, u16 /*handle*/)
{
    FrameBlock *block = static_cast<FrameBlock *>(cachePointer(data));
    const u32 shape = cacheHandles[block->shape];
    if (shape == 0) {
        return;
    }
    ShapeHeader *header = static_cast<ShapeHeader *>(cachePointer(shape));
    header->frames[block->frame].offset = block->entry;
}

void closeShapeCache()
{
    if (shapeCache != nullptr) {
        delete shapeCache;
        shapeCache = nullptr;
    }
}

void closeShapeFiles()
{
    delete shapeFlex;
    shapeFlex = nullptr;
    delete frameCache;
    frameCache = nullptr;
}

/* Copies the shapes the game keeps from the swap file into a new shape file,
 * with a progress bar. dl.dat marks a copy in progress: a shape file found
 * with it is left from a copy that did not finish. */
void ShapeHandler::createSwap()
{
    bool created = false;
    if (allShapes == 0U) {
        u8 copying = fileExists(makePath(nullptr, staticDirectory, "dl.dat", nullptr));
        if (copying != 0U) {
            fileRemove(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr));
            copying = 0;
        }
        File marker(makePath(nullptr, staticDirectory, "dl.dat", nullptr), FILE_READWRITE, 1);
        marker.write(&copying, 1);
        marker.close();
    }
    if (fileExists(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr)) == 0) {
        debugPrintf(0xff, "]            \n\r");
        debugPrintf(0xff, "Creating Swap file [");
        debugPrintf(0xff, "                                        ]\r");
        debugPrintf(0xff, "Creating Swap file [");
        Flex flex;
        flex.create(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr), SHAPES);
        created = true;
    }
    FastFlex shapes(makePath(nullptr, staticDirectory, gameFiles[GAME_SHAPES], nullptr), FILE_READWRITE, -1);
    u8 *used = new u8[SHAPES];
    memset(used, 0, SHAPES);
    i16 i;
    for (i = 0; i < DAMAGE_SHAPES; i++) {
        DamageEntry *damage = &damageTable->entries[i];
        if (damage->replaceItem() != 0U) {
            used[damage->replacement.shape] = 1;
        }
    }
    if (created) {
        u32 count = 0;
        u32 step;
        u32 progress = 0;
        u32 shown = 0;
        const u32 width = 40L << 16;
        for (i = 1; i < SHAPES; i++) {
            const ShapeInfo info = typeFlags.info(i);
            if (info.preload || info.npc || (allShapes != 0U) || (used[i] != 0U)) {
                count++;
            }
        }
        step = width / count;
        debugPrintf(0xff, "\xb1");
        for (i = 1; i < SHAPES; i++) {
            ShapeInfo info;
            u8 *buffer = new u8[0x1000];
            info = typeFlags.info(i);
            FlexEntry entry;
            shapes.readEntry(i, &entry);
            if (info.preload || info.npc || (allShapes != 0U) || (used[i] != 0U)) {
                progress += step;
                if (progress >> 16 != shown) {
                    debugPrintf(0xff, "\xb1");
                    shown = progress >> 16;
                }
                FlexEntry swapEntry;
                swapFlex->readEntry(i, &swapEntry);
                if (swapEntry.size != 0) {
                    s32 copied = 0;
                    swapFlex->seekObject(i);
                    shapes.seek(0, SEEK_END);
                    entry.offset = shapes.tell();
                    entry.size = swapEntry.size;
                    shapes.writeEntry(i, &entry);
                    shapes.seek(0, SEEK_END);
                    while (copied + 0x1000 < swapEntry.size) {
                        swapFlex->read(buffer, 0x1000);
                        shapes.write(buffer, 0x1000);
                        copied += 0x1000;
                    }
                    swapFlex->read(buffer, swapEntry.size - copied);
                    shapes.write(buffer, swapEntry.size - copied);
                }
            }
            if (buffer != nullptr) {
                delete[] buffer;
                buffer = nullptr;
            }
        }
        debugPrintf(0xff, "\n\r[");
        debugPrintf(0xff, "       ]\r[");
    }
    if (used != nullptr) {
        delete[] used;
    }
    swapSize = shapes.size();
}
