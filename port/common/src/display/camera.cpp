/* Code52: the game's camera. See camera.h. */
#include "camera.h"
#include "gamestate.h"
#include "waits.h"
#include "collide.h"
#include "crusys.h"
#include "display.h"
#include "dispatch.h"
#include "dsfxman.h"
#include "egg.h"
#include "file.h"
#include "flicplay.h"
#include "gametime.h"
#include "gumpfont.h"
#include "host.h"
#include "item.h"
#include "itemcach.h"
#include "itemdata.h"
#include "itemutil.h"
#include "kernel.h"
#include "npc.h"
#include "snap.h"
#include "sndsrc.h"
#include "spmgr.h"
#include "status.h"
#include "systimer.h"
#include "type.h"
#include "viewport.h"
#include "widesides.h"
#include "trace.h"
#include "screen.h"

void drawMarker(Viewport *view);
void drawChunkGrid(Viewport *view);
void setLevelLimit(i16 level);

/* Nothing reads them. */
u16 &unknown34ee = gameGlobal<u16>(0);
u16 &unknown34f0 = gameGlobal<u16>(1);
u8 &unknown34f2 = gameGlobal<u8>(0);
i16 &scrollX = gameGlobal<i16>(0);
i16 &scrollY = gameGlobal<i16>(0);
/* The nodes are built and the screen is up to date. */
u8 &nodesBuilt = gameGlobal<u8>(0);
u8 &screenValid = gameGlobal<u8>(0);
/* The last showItemBoxes and unknown3631 flush saw. */
u8 &lastItemBoxes = gameGlobal<u8>(0);
u8 &lastUnknown3631 = gameGlobal<u8>(0);
i16 &levelLimit = gameGlobal<i16>(250);
/* Where findRoof saw the followed item. */
u16 &roofX = gameGlobal<u16>(0xffff);
u16 &roofY = gameGlobal<u16>(0xffff);
u8 &roofZ = gameGlobal<u8>(0xff);
u16 &followedItem = gameGlobal<u16>(0);
/* Code9 asks for the nodes to be built again. */
u8 &rebuildRequested = gameGlobal<u8>(0);
/* Cheats: the snap eggs' areas and the chunk grid. */
u8 &showSnaps = gameGlobal<u8>(0);
u8 &showChunks = gameGlobal<u8>(0);
/* Debug displays nothing sets: a marker at markerX,markerY, the free node
 * and edge counts, the uncovered strips painted first. */
u16 &showMarker = gameGlobal<u16>(0);
u16 &markerX = gameGlobal<u16>(0);
u16 &markerY = gameGlobal<u16>(0);
u8 &showNodeCounts = gameGlobal<u8>(0);
u8 &unknown350e = gameGlobal<u8>(0);
u8 &mapDrawing = gameGlobal<u8>(1);
u8 &unknown3510 = gameGlobal<u8>(1);
u8 &fillRepaints = gameGlobal<u8>(0);
/* Only loaded and saved. */
u8 &unknown3512 = gameGlobal<u8>(0);
u8 &clearingChunks = gameGlobal<u8>(0);
Camera *&gameCamera = gameGlobal<Camera *>(nullptr);
namespace {
/* The port's: the host's repaint (host.h), when a watched replay that went
 * fast draws again: the map area anew, as the game's own calls repaint it
 * (a map's jump, a menu's end). */
const bool REPAINT_ACTION = (hostRepaintAction([] {
                                 if (gameCamera != nullptr) {
                                     gameCamera->repaint();
                                 }
                             }),
                             true);
}
u32 &unknown3518 = gameGlobal<u32>(0xffffffffL);
u32 &lastSoundTime = gameGlobal<u32>(0);
/* The part of the map area the scroll left on the screen. */
ScreenArea &remainingArea = gameGlobal<ScreenArea>();
/* The map point the camera looks at. */
u16 &cameraMapX = gameGlobal<u16>();
u16 &cameraMapY = gameGlobal<u16>();
u8 &cameraMapZ = gameGlobal<u8>();
/* Only loaded and saved. */
u8 &unknown8ec7 = gameGlobal<u8>();

void createCamera()
{
    gameCamera = new Camera;
}

void repaintCamera()
{
    if (gameCamera != nullptr) {
        gameCamera->repaint();
        gameCamera->drawFrame();
    }
}

void Camera::unknown0040() {}

Camera::Camera()
{
    gameCamera = this;
    setType(CAMERA_TYPE);
    nameProcess(pid, "Camera");
}

void Camera::afterLoad()
{
    gameCamera = this;
    releaseNodes();
    rebuildScreen();
}

void Camera::fail(u32 failResult)
{
    gameCamera = nullptr;
    Process::fail(failResult);
}

void Camera::run()
{
    gameCamera->flush();
    if (showNodeCounts != 0U) {
        drawViewport->clear(0x18, 7, 40, 80, 75);
        systemFont->x = 10;
        systemFont->y = 55;
        systemFont->print("Nodes: %04d", freeNodeCount);
        systemFont->x = 10;
        systemFont->y = 70;
        systemFont->print("Edges: %04d", freeEdgeCount);
        screenViewport->blit(drawViewport, 7, 40, 80, 75, 7, 40);
    }
    u32 now;
    if (lastSoundTime >> 5 < (now = systemTimer->time()) >> 5) {
        lastSoundTime = now;
        if (effectsStarted != 0U) {
            updateSoundSources();
        }
    }
}

void Camera::load(File *file)
{
    Process::load(file);
    file->read(&scrollX, 2);
    file->read(&scrollY, 2);
    file->read(&remainingArea, 8);
    file->read(&cameraMapX, 2);
    file->read(&cameraMapY, 2);
    file->read(&cameraMapZ, 1);
    file->read(&levelLimit, 2);
    file->read(&roofX, 2);
    file->read(&roofY, 2);
    file->read(&roofZ, 1);
    file->read(&followedItem, 2);
    file->read(&unknown8ec7, 1);
    file->read(&rebuildRequested, 1);
    file->read(&unknown3512, 1);
    gameCamera = this;
    camera = this;
}

void Camera::save(File *file)
{
    Process::save(file);
    file->write(&scrollX, 2);
    file->write(&scrollY, 2);
    file->write(&remainingArea, 8);
    file->write(&cameraMapX, 2);
    file->write(&cameraMapY, 2);
    file->write(&cameraMapZ, 1);
    file->write(&levelLimit, 2);
    file->write(&roofX, 2);
    file->write(&roofY, 2);
    file->write(&roofZ, 1);
    file->write(&followedItem, 2);
    file->write(&unknown8ec7, 1);
    file->write(&rebuildRequested, 1);
    file->write(&unknown3512, 1);
}

/* The shifts by 0 are in the original. */
void screenToMap(i16 x, i16 y, u16 *mapX, u16 *mapY)
{
    const i16 mapPointX = x + scrollX;
    const i16 mapPointY = y + scrollY;
    *mapX = (mapPointX << 0) + (mapPointY << 1);
    *mapY = (mapPointY << 1) - (mapPointX << 0);
}

void mapPointAt(i16 x, i16 y, u16 *mapX, u16 *mapY, u8 *z, Item *item, u8 *inside)
{
    DL_ItemNode *entry;

    screenToMap(x, y, mapX, mapY);
    const u16 node = nodeAt(x, y);
    entry = static_cast<DL_ItemNode *>(nodeOf(node));
    if ((node != 0U) && entry->kind == NODE_ITEM) {
        ItemRef found(entry->item);
        if (found.isValid()) {
            i16 width;
            i16 depth;
            i16 height;
            shapeBox(width, depth, height, &typeFlags.infos[found.shape()]);
            *z = itemZ[found.id] + height;
            if (*z > 250) {
                *z = 250;
            }
            *mapX += *z * 2;
            *mapY += *z * 2;
            if (item != nullptr) {
                *item = found;
            }
            if (inside != nullptr) {
                *inside = static_cast<u8>(itemX[found.id] >= *mapX && itemY[found.id] >= *mapY);
            }
            if (itemX[found.id] <= *mapX || itemY[found.id] <= *mapY) {
                /* The distances into the footprint compare as signed. */
                i16 over;
                if (static_cast<i16>(static_cast<i16>(*mapX) - static_cast<i16>(itemX[found.id])) >
                    static_cast<i16>(static_cast<i16>(*mapY) - static_cast<i16>(itemY[found.id]))) {
                    over = *mapX - itemX[found.id];
                } else {
                    over = *mapY - itemY[found.id];
                }
                if (*z >= over >> 1) {
                    *mapX -= over;
                    *mapY -= over;
                    *z -= over >> 1;
                }
            }
            return;
        }
        *z = 0;
        return;
    }
    *z = 0;
}

namespace {
/* Builds the nodes of the fast area's items on the view. */
void buildNodes()
{
    Rect view(0, 0, 0, 0);
    Rect area(0, 0, 0, 0);
    Rect chunk(0, 0, 0, 0);
    Item item;
    i16 width;
    i16 depth;
    i16 height;
    i16 x;
    i16 y;
    i16 screenX;
    u16 chunkX;
    u16 chunkY;
    i16 chunkLeft;
    i16 chunkTop;

    updateNodes();
    i16 scrollTop;
    i16 scrollLeft;
    scrollLeft = scrollX & ~(GRID_CELL - 1);
    scrollTop = scrollY & ~(GRID_CELL - 1);
    area = Rect(scrollLeft, scrollTop, scrollLeft + (GRID_COLUMNS * GRID_CELL) - 1,
                scrollTop + (GRID_ROWS * GRID_CELL) - 1);
    i16 viewTop;
    i16 viewLeft;
    viewLeft = viewX;
    viewTop = viewY;
    view = Rect(viewLeft, viewTop, viewLeft + (GRID_COLUMNS * GRID_CELL) - 1, viewTop + (GRID_ROWS * GRID_CELL) - 1);
    for (i16 sum = fastArea.sumLow; sum <= fastArea.sumHigh; sum++) {
        for (i16 difference = fastArea.differenceLow; difference <= fastArea.differenceHigh; difference++) {
            if ((static_cast<i16>((sum + difference) & 1)) == 0) {
                const u16 column = static_cast<u16>(sum + difference) >> 1;
                const u16 row = static_cast<u16>(sum - difference) >> 1;
                if (!fastArea.contains(column, row)) {
                    continue;
                }
                /* Once the nodes are built, only the chunks the scroll brought
                 * onto the view. */
                if (nodesBuilt != 0U) {
                    chunkCorner(column, row, chunkX, chunkY);
                    chunkLeft = (chunkX >> 1) - (chunkY >> 1);
                    chunkTop = (chunkX >> 2) + (chunkY >> 2);
                    chunk = Rect(chunkLeft - 376, chunkTop - 250, chunkLeft + 375, chunkTop + 255);
                    if (!chunk.touches(view)) {
                        continue;
                    }
                    if (area.holds(chunk)) {
                        continue;
                    }
                }
                if (!fastArea.contains(column, row)) {
                    continue;
                }
                for (u16 number = mapChunks[column][row]; number != 0U; number = itemNext[item.id]) {
                    item = ItemRef(number);
                    ShapeInfo *info = &typeFlags.infos[itemShape[item.id]];
                    if ((itemFlags[item.id] & ITEM_DRAWN) != 0) {
                        continue;
                    }
                    if (info->editor) {
                        continue;
                    }
                    if (((itemFlags[item.id] & ITEM_INVISIBLE) == 0) || item.isSilencer() || (worldActive == 0U)) {
                        x = itemX[item.id] >> 1;
                        y = itemY[item.id] >> 1;
                        screenX = x - y;
                        const i16 screenY = (x >> 1) + (y >> 1) - itemZ[item.id];
                        halfBox(width, depth, height, info);
                        const Rect bounds(screenX - width, screenY - (width >> 1) - (depth >> 1) - height,
                                          screenX + depth, screenY);
                        if (view.touches(bounds)) {
                            DL_ItemNode *entry = new DL_ItemNode;
                            const u16 node = nodeOffset(entry);
                            if (node != 0U) {
                                entry->kind = NODE_ITEM;
                                entry->covers =
                                    static_cast<u8>(((itemFlags[item.id] & ITEM_INVISIBLE) != 0) ||
                                                    (typeFlags.infos[itemShape[item.id]].translucent != 0U));
                                itemNodes[number] = node;
                                entry->attach(number);
                                if (entry->bounds.touches(view)) {
                                    item.orFlags(ITEM_DRAWN);
                                    markChanged(node);
                                    if (linkNewNode(node) == 0U) {
                                        itemNodes[number] = 0;
                                        item.andFlags(~ITEM_DRAWN);
                                        unlinkNode(node);
                                        delete entry;
                                    }
                                } else {
                                    itemNodes[number] = 0;
                                    delete entry;
                                }
                            } else {
                                debugPrintf(50, "item node alloc");
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Puts the grid's corner at x,y. */
void placeGrid(i16 x, i16 y)
{
    i16 row;
    i16 column;
    i16 left;
    i16 top;

    for (row = 0; row < GRID_ROWS; row++) {
        for (column = 0; column < GRID_COLUMNS; column++) {
            left = x + column * GRID_CELL;
            top = y + row * GRID_CELL;
            nodeOf(gridCells[row][column])->bounds = Rect(left, top, left + GRID_CELL - 1, top + GRID_CELL - 1);
        }
    }
    viewX = x;
    viewY = y;
}

/* When the view moves by whole cells, the grid's cells are rotated and only
 * the nodes the move uncovers are built; the rest stay linked. */
void cameraMoved(u16 x, u16 y, u8 z);
}

void moveCamera(u16 x, u16 y, u8 z, u16 map)
{
    cameraMapX = x;
    cameraMapY = y;
    cameraMapZ = z;
    clearDirtyRects();
    setFastArea(x, y, map);
    if (waitOpened()) {
        /* The view after the map's loading's wait. */
        giveRest([x, y, z]() {
            cameraMoved(x, y, z);
            return false;
        });
        return;
    }
    cameraMoved(x, y, z);
}

namespace {
/* moveCamera's after the fast area: the view and the grid moved. */
void cameraMoved(u16 x, u16 y, u8 z)
{
    i16 column;
    i16 row;
    /* The grid's corner, then the cell a rotated cell comes from. */
    i16 top;
    i16 left;
    i16 newScrollX;
    i16 newScrollY;
    i16 columns;
    i16 rows;
    i16 newColumn;
    i16 newRow;
    i16 oldColumn;
    i16 oldRow;
    DL_Node *entry;
    u16 edge;
    u16 node;
    u16 cells[GRID_ROWS][GRID_COLUMNS];

    Rect newView(0, 0, 0, 0);
    Rect oldView(0, 0, 0, 0);
    if (worldActive != 0U) {
        setLevelLimit(0xff);
    }
    newScrollX = (x >> 1) - (y >> 1) - 320;
    newScrollY = (x >> 2) + (y >> 2) - z - 300;
    if (nodesBuilt == 0U) {
        releaseNodes();
        placeGrid(newScrollX & ~(GRID_CELL - 1), newScrollY & ~(GRID_CELL - 1));
        buildNodes();
    } else {
        columns = rows = 0;
        left = viewX;
        top = viewY;
        if (newScrollX < left) {
            columns = static_cast<i16>((newScrollX & ~(GRID_CELL - 1)) - left) >> 7;
        }
        if (newScrollY < top) {
            rows = static_cast<i16>((newScrollY & ~(GRID_CELL - 1)) - top) >> 7;
        }
        if (static_cast<i16>(newScrollX + SCREEN_RIGHT) >= static_cast<i16>(left + (GRID_COLUMNS * GRID_CELL))) {
            columns = static_cast<i16>(((newScrollX + SCREEN_RIGHT) & ~(GRID_CELL - 1)) -
                                       (left + ((GRID_COLUMNS - 1) * GRID_CELL))) >>
                      7;
        }
        if (static_cast<i16>(newScrollY + screenRect.bottom) >= static_cast<i16>(top + (GRID_ROWS * GRID_CELL))) {
            rows = static_cast<i16>(((newScrollY + screenRect.bottom) & ~(GRID_CELL - 1)) -
                                    (top + ((GRID_ROWS - 1) * GRID_CELL))) >>
                   7;
        }
        if ((columns != 0) || (rows != 0)) {
            newColumn = (viewX >> 7) + columns;
            newRow = (viewY >> 7) + rows;
            oldView = Rect(left, top, left + (GRID_COLUMNS * GRID_CELL) - 1, top + (GRID_ROWS * GRID_CELL) - 1);
            newView = Rect(newColumn << 7, newRow << 7, (newColumn << 7) + (GRID_COLUMNS * GRID_CELL) - 1,
                           (newRow << 7) + (GRID_ROWS * GRID_CELL) - 1);
            {
                const bool overlap = oldView.touches(newView);
                oldColumn = viewX >> 7;
                oldRow = viewY >> 7;
                /* The cells that leave the grid lose their nodes, all of them when the
                 * views do not overlap. */
                for (row = 0; row < GRID_ROWS; row++) {
                    for (column = 0; column < GRID_COLUMNS; column++) {
                        cells[row][column] = gridCells[row][column];
                        if (static_cast<i16>(oldColumn + column) >= newColumn &&
                            static_cast<i16>(oldColumn + column) < static_cast<i16>(newColumn + GRID_COLUMNS) &&
                            static_cast<i16>(oldRow + row) >= newRow &&
                            static_cast<i16>(oldRow + row) < static_cast<i16>(newRow + GRID_ROWS)) {
                            continue;
                        }
                        edge = nodeOf(gridCells[row][column])->edgesIn;
                        while (edge != 0U) {
                            const u16 link = edge;
                            node = edgeOf(edge)->first;
                            entry = nodeOf(node);
                            edge = edgeOf(edge)->nextIn;
                            if (!entry->bounds.touches(newView) || !overlap) {
                                unlinkNode(node);
                                if (entry->kind == NODE_ITEM) {
                                    (static_cast<DL_ItemNode *>(entry))->detach();
                                    delete entry;
                                }
                            } else {
                                deleteEdge(link);
                            }
                        }
                    }
                }
                if (rows >= 0) {
                    top = rows % GRID_ROWS;
                } else {
                    top = GRID_ROWS - 1 - static_cast<i16>(-rows - 1) % GRID_ROWS;
                }
                for (row = 0; row < GRID_ROWS; row++) {
                    if (columns >= 0) {
                        left = columns % GRID_COLUMNS;
                    } else {
                        left = GRID_COLUMNS - 1 - static_cast<i16>(-columns - 1) % GRID_COLUMNS;
                    }
                    for (column = 0; column < GRID_COLUMNS; column++) {
                        gridCells[row][column] = cells[top][left];
                        if (++left == GRID_COLUMNS) {
                            left = 0;
                        }
                    }
                    if (++top == GRID_ROWS) {
                        top = 0;
                    }
                }
                placeGrid(newColumn << 7, newRow << 7);
                /* The nodes the move brought partly onto the view are linked again. */
                for (row = 0; row < GRID_ROWS; row++) {
                    for (column = 0; column < GRID_COLUMNS; column++) {
                        edge = nodeOf(gridCells[row][column])->edgesIn;
                        while (edge != 0U) {
                            node = edgeOf(edge)->first;
                            entry = nodeOf(node);
                            edge = edgeOf(edge)->nextIn;
                            if (((entry->flags & NODE_RELINKED) == 0) && entry->bounds.touches(newView) &&
                                (!entry->bounds.within(oldView))) {
                                if (relinkNode(node) != 0U) {
                                    entry->flags |= NODE_RELINKED;
                                } else {
                                    if (entry->kind == NODE_ITEM) {
                                        const Item item = (static_cast<DL_ItemNode *>(entry))->item;
                                        item.andFlags(~ITEM_DRAWN);
                                        itemNodes[item.id] = 0;
                                    }
                                    unlinkNode(node);
                                    delete entry;
                                }
                            }
                        }
                    }
                }
                for (row = 0; row < GRID_ROWS; row++) {
                    for (column = 0; column < GRID_COLUMNS; column++) {
                        edge = nodeOf(gridCells[row][column])->edgesIn;
                        while (edge != 0U) {
                            node = edgeOf(edge)->first;
                            entry = nodeOf(node);
                            edge = edgeOf(edge)->nextIn;
                            if (entry->bounds.touches(newView) && !entry->bounds.within(oldView)) {
                                entry->flags &= ~NODE_RELINKED;
                            }
                        }
                    }
                }
            }
            buildNodes();
        }
    }
    /* Set and tested for nothing: the original's empty if keeps a compare. */
    const bool moved = false;
    if (screenValid != 0U) {
        screenValid = 0;
    }
    scrollX = newScrollX;
    scrollY = newScrollY;
    nodesBuilt = 1;
    if (moved) {
        /* empty in the original */
    }
    if (effectsStarted != 0U) {
        updateItemSounds();
    }
}
}

void Camera::repaint()
{
    const Rect area(screenRect.left, screenRect.top, screenRect.right, screenRect.bottom);
    invalidate(area, 0);
}

void Camera::invalidate(Rect area, u8 only)
{
    if (mapDrawing == 0U) {
        return;
    }
    const i16 left = area.left;
    const i16 top = area.top;
    const i16 right = area.right;
    const i16 bottom = area.bottom;
    updateNodes();
    flushDirty();
    if (worldActive == 0U) {
        drawViewport->clear(0x58, left, top, right, bottom);
    }
    if (only == 0U) {
        Rect map(left + scrollX, top + scrollY, right + scrollX, bottom + scrollY);
        changeArea(&map);
        drawArea.left = left;
        drawArea.top = top;
        drawArea.right = right;
        drawArea.bottom = bottom;
        drawView = *drawViewport;
        drawView.setBounds(Rect(left, top, right, bottom));
        drawChanged();
    }
    if ((showChunks != 0U) || (showEggs != 0U)) {
        drawChunkGrid(drawViewport);
    }
    if (showMarker != 0U) {
        drawMarker(drawViewport);
    }
    if (showSnaps != 0U) {
        snapProcess->showAreas();
    }
    guiDispatcher->paint(Rect(left, top, right, bottom));
    addDirtyRect(Rect(left, top, right, bottom));
}

/* The debug displays: the marker and the chunk grid with the eggs. */
void drawMarker(Viewport *view)
{
    const u16 x = markerX;
    const u16 y = markerY;
    i16 screenX;
    i16 screenY;

    mapToScreen(x, y, &screenX, &screenY);
    screenY -= 2;
    view->line(screenX - 256, screenY + 129, screenX - 1, screenY + 257, 14);
    view->line(screenX, screenY, screenX + 255, screenY + 128, 14);
    view->line(screenX - 1, screenY, screenX - 256, screenY + 128, 14);
    view->line(screenX + 255, screenY + 129, screenX, screenY + 257, 14);
}

void drawChunkGrid(Viewport *view)
{
    if (showChunks != 0U) {
        const i16 left = (cameraMapX & CHUNK_MASK) - CHUNK_SIZE;
        const i16 top = (cameraMapY & CHUNK_MASK) - CHUNK_SIZE;
        i16 screenX;
        i16 screenY;
        const u8 color = 14;
        for (i16 row = 0; row < 3; row++) {
            for (i16 column = 0; column < 3; column++) {
                mapToScreen(left + (column * CHUNK_SIZE), top + (row * CHUNK_SIZE), &screenX, &screenY);
                screenY -= 2;
                view->line(screenX - 256, screenY + 129, screenX - 1, screenY + 257, color);
                view->line(screenX - 1, screenY, screenX - 256, screenY + 128, color);
            }
        }
    }
    if (showEggs != 0U) {
        EggHatcher *hatcher = nullptr;
        for (hatcher = static_cast<EggHatcher *>(firstProcess(0, EGG_HATCHER_TYPE)); hatcher != nullptr;
             hatcher = static_cast<EggHatcher *>(nextProcess())) {
            hatcher->show();
        }
    }
}

namespace {
/* Moves the camera where it is when the nodes are not built. */
void ensureNodes()
{
    if (nodesBuilt == 0U) {
        moveCamera(cameraMapX, cameraMapY, cameraMapZ, 0);
    }
}
}

void Camera::flush()
{
    if (mapDrawing == 0U) {
        return;
    }
    ensureNodes();
    if (showItemBoxes != lastItemBoxes) {
        screenValid = 0;
        lastItemBoxes = showItemBoxes;
    }
    if (unknown3631 != lastUnknown3631) {
        screenValid = 0;
        lastUnknown3631 = unknown3631;
    }
    animationProcess->freeze();
    if (rebuildRequested != 0U) {
        rebuildRequested = 0;
        updateNodes();
        animateItems();
    }
    if (screenValid != 0U) {
        if (remainingArea.left > 0) {
            if (fillRepaints != 0U) {
                drawViewport->clear(0x58, 0, 0, remainingArea.left - 1, screenRect.bottom);
            }
            invalidate(Rect(0, 0, remainingArea.left - 1, screenRect.bottom), 0);
        }
        if (remainingArea.top > 0) {
            if (fillRepaints != 0U) {
                drawViewport->clear(0x58, remainingArea.left, 0, remainingArea.right, remainingArea.top - 1);
            }
            invalidate(Rect(remainingArea.left, 0, remainingArea.right, remainingArea.top - 1), 0);
        }
        if (remainingArea.right < SCREEN_RIGHT) {
            if (fillRepaints != 0U) {
                drawViewport->clear(0x58, remainingArea.right + 1, 0, SCREEN_RIGHT, screenRect.bottom);
            }
            invalidate(Rect(remainingArea.right + 1, 0, SCREEN_RIGHT, screenRect.bottom), 0);
        }
        if (remainingArea.bottom < screenRect.bottom) {
            if (fillRepaints != 0U) {
                drawViewport->clear(0x58, remainingArea.left, remainingArea.bottom + 1, remainingArea.right,
                                    screenRect.bottom);
            }
            invalidate(Rect(remainingArea.left, remainingArea.bottom + 1, remainingArea.right, screenRect.bottom), 0);
        }
    } else {
        if (fillRepaints != 0U) {
            drawViewport->clear(0x58, screenRect.left, screenRect.top, screenRect.right, screenRect.bottom);
        }
        invalidate(Rect(0, 0, SCREEN_RIGHT, screenRect.bottom), 0);
    }
    animationProcess->thaw();
    drawFrame();
    remainingArea = ScreenArea(0, 0, SCREEN_RIGHT, screenRect.bottom);
    screenValid = 1;
}

void Camera::drawFrame()
{
    updateNodes();
    flushDirty();
    paintUpdate();
    drawSprites();
    drawStatus();
    drawPickups();
    if ((screenViewport->flags() & VIEWPORT_LARGE) != 0) {
        flushScreen();
    } else if (flicPlaying == 0U) {
        dirtyRects.clear();
        if (worldActive != 0U) {
            i16 x;
            i16 y;
            mapToScreen(itemX[controlledNpc.id], itemY[controlledNpc.id], &x, &y);
            screenViewport->blit(drawViewport, x - 160, y - 100, x + 159, y + 99, 0, 0);
        } else {
            screenViewport->blit(drawViewport);
        }
    }
    erasePickups();
    eraseStatus();
    restoreBackground();
    /* The port's: the wide screen's sides (widesides.h). */
    drawWideSides();
}

void setFollowedItem(u16 item)
{
    traceFollowed(item);
    followedItem = item;
}

/* Moves the camera by dx,dy,dz. */
/* UNCALLED: nothing calls it. */
void moveCameraBy(i16 dx, i16 dy, char dz)
{
    moveCamera(cameraMapX + dx, cameraMapY + dy, cameraMapZ + dz, 0);
}

/* Items above level are not drawn; 0-250. */
void setLevelLimit(i16 level)
{
    if (level >= 0 && level <= 250 && level != levelLimit) {
        levelLimit = level;
        screenValid = 0;
        nodesBuilt = 0;
    }
}

/* The level limit under the roof over item. Nothing calls it. The
 * box it gets is not used. */
/* UNCALLED: nothing calls it. */
void findRoof(Item item)
{
    roofX = itemX[item.id];
    roofY = itemY[item.id];
    const u8 z = itemZ[item.id];
    roofZ = z;
    i16 width;
    i16 depth;
    i16 height;
    typeFlags.infos[item.shape()].box(width, depth, height);
    i16 level = 250;
    Sweep sweep;
    ItemPoint top = item.point();
    top.z = 250;
    if ((!sweep.test(item.shape(), lvalue(item.point()), top, MOVE_NO_FLOOR, 0, 0, 8)) && (sweep.hitItem != 0U)) {
        level = Item(sweep.hitItem).z();
    }
    setLevelLimit(level);
}

/* The followed item left the place findRoof saw. Nothing calls it. */
/* UNCALLED: nothing calls it. */
bool followedItemMoved()
{
    return MapItem(followedItem).x() != roofX || MapItem(followedItem).y() != roofY ||
           MapItem(followedItem).z() != roofZ;
}

void rebuildScreen()
{
    nodesBuilt = 0;
    screenValid = 0;
    updateNodes();
}

u16 cameraX()
{
    return cameraMapX;
}

u16 cameraY()
{
    return cameraMapY;
}

u8 cameraZ()
{
    return cameraMapZ;
}

i16 cameraLevel()
{
    return levelLimit;
}

void showBox(u16 x, u16 y, u8 z, i16 xsize, i16 ysize, u8 color)
{
    i16 screenX;
    i16 screenY;
    i16 left;
    i16 top;
    /* Two points it sets and never reads. */
    Point first;
    Point second;

    mapToScreen(x, y, &screenX, &screenY);
    screenY -= z;
    i16 right = screenX + (ysize << 4);
    i16 bottom = screenY - (ysize << 3);
    first.x = right;
    first.y = screenY;
    drawViewport->line(screenX, screenY, right, bottom, color);
    left = right - (xsize << 4);
    top = bottom - (xsize << 3);
    drawViewport->line(right, bottom, left, top, color);
    second.x = right;
    second.y = top;
    right = left - (ysize << 4);
    bottom = top + (ysize << 3);
    drawViewport->line(left, top, right, bottom, color);
    drawViewport->line(right, bottom, screenX, screenY, color);
}
