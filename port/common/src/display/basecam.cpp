/* 1300:0000-0daf, file name unknown: dirty rectangles and BaseCamera. See
 * basecam.h. */
#include "gamestate.h"
#include "basecam.h"
#include "kernel.h"
#include "viewport.h"
#include "mousectl.h"
#include "input.h"
#include "screen.h"

i16 &dirtyCount = gameGlobal<i16>(0);
u8 &flushRequested = gameGlobal<u8>(0); /* cleared by flushScreen; nothing sets it */
BaseCamera *&camera = gameGlobal<BaseCamera *>();
RectList &dirtyRects = gameGlobal<RectList>();
u8 &screenDirty = gameGlobal<u8>();

bool rectsOverlap(Rect *a, Rect *b)
{
    s16 left;
    s16 top;
    s16 right;
    s16 bottom;

    if (a->right < b->left || b->top > a->bottom || b->right < a->left || b->bottom < a->top) {
        return false;
    }
    left = a->left > b->left ? a->left : b->left;
    top = a->top > b->top ? a->top : b->top;
    right = a->right < b->right ? a->right : b->right;
    bottom = a->bottom < b->bottom ? a->bottom : b->bottom;
    if (static_cast<i16>(right - left + 1) > 0 && static_cast<i16>(bottom - top + 1) > 0) {
        return true;
    }
    if (b->left >= a->left && b->top >= a->top && b->right <= a->right && b->bottom <= a->bottom) {
        return true;
    }
    if (a->left >= b->left && a->top >= b->top && a->right <= b->right && a->bottom <= b->bottom) {
        return true;
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool RectList::overlaps(Rect area)
{
    RectLink *link = static_cast<RectLink *>(first);

    while (link != nullptr) {
        if (rectsOverlap(&area, &link->bounds)) {
            return true;
        }
        link = static_cast<RectLink *>(link->next);
    }
    return false;
}

void RectList::add(Rect area)
{
    RectLink *link = nullptr;

    if (area.left < 0) {
        area.left = 0;
    }
    if (area.top < 0) {
        area.top = 0;
    }
    if (area.right > SCREEN_RIGHT) {
        area.right = SCREEN_RIGHT;
    }
    if (area.bottom > SCREEN_BOTTOM) {
        area.bottom = SCREEN_BOTTOM;
    }
    link = new RectLink(area);
    append(link);
}

Rect RectList::pop()
{
    RectLink *link = static_cast<RectLink *>(last);
    Rect area(1, 1, 0, 0);

    if (link != nullptr) {
        area = link->bounds;
        destroy(link);
    }
    return area;
}

void RectList::merge()
{
    RectLink *link = nullptr;
    RectLink *upper;
    RectLink *lower;
    RectLink *other;

    while (forward(reinterpret_cast<GumpLink **>(&link))) {
        other = link;
        if (RECT_WIDTH(link->bounds) <= 0 || RECT_HEIGHT(link->bounds) <= 0) {
            continue;
        }
        while (forward(reinterpret_cast<GumpLink **>(&other))) {
            if (RECT_WIDTH(link->bounds) <= 0 || RECT_HEIGHT(link->bounds) <= 0) {
                break;
            }
            if (RECT_WIDTH(other->bounds) < 0 || RECT_HEIGHT(other->bounds) <= 0) {
                continue;
            }
            if (!rectsOverlap(&link->bounds, &other->bounds)) {
                continue;
            }
            if (link->bounds.top < other->bounds.top) {
                upper = link;
                lower = other;
            } else if (other->bounds.top < link->bounds.top) {
                upper = other;
                lower = link;
            } else if (link->bounds.bottom > other->bounds.bottom) {
                upper = link;
                lower = other;
            } else if (link->bounds.bottom < other->bounds.bottom) {
                upper = other;
                lower = link;
            } else {
                if (link->bounds.left < other->bounds.left) {
                    upper = link;
                    lower = other;
                } else {
                    upper = other;
                    lower = link;
                }
                if (lower->bounds.right > upper->bounds.right) {
                    upper->bounds.right = lower->bounds.right;
                }
                lower->bounds.set(1, 1, 0, 0);
                continue;
            }
            if (upper->bounds.left <= lower->bounds.left) {
                if (upper->bounds.right < lower->bounds.right) {
                    if (upper->bounds.bottom < lower->bounds.bottom) {
                        append(new RectLink(
                            Rect(upper->bounds.left, lower->bounds.top, lower->bounds.right, upper->bounds.bottom)));
                        {
                            const s32 top = lower->bounds.top;
                            lower->bounds.top = upper->bounds.bottom + 1;
                            upper->bounds.bottom = top - 1;
                        }
                    } else {
                        lower->bounds.left = upper->bounds.right + 1;
                    }
                } else if (upper->bounds.bottom < lower->bounds.bottom) {
                    lower->bounds.top = upper->bounds.bottom + 1;
                } else {
                    lower->bounds.set(1, 1, 0, 0);
                }
            } else if (upper->bounds.right < lower->bounds.right) {
                if (upper->bounds.bottom <= lower->bounds.bottom) {
                    upper->bounds.bottom = lower->bounds.top - 1;
                } else {
                    append(new RectLink(
                        Rect(upper->bounds.left, lower->bounds.bottom + 1, upper->bounds.right, upper->bounds.bottom)));
                    upper->bounds.bottom = lower->bounds.top - 1;
                }
            } else if (upper->bounds.bottom < lower->bounds.bottom) {
                append(new RectLink(
                    Rect(lower->bounds.left, lower->bounds.top, upper->bounds.right, upper->bounds.bottom)));
                {
                    const s32 top = lower->bounds.top;
                    lower->bounds.top = upper->bounds.bottom + 1;
                    upper->bounds.bottom = top - 1;
                }
            } else {
                lower->bounds.right = upper->bounds.left - 1;
            }
        }
    }
    link = nullptr;
    while (forward(reinterpret_cast<GumpLink **>(&link))) {
        if (RECT_WIDTH(link->bounds) <= 0 || RECT_HEIGHT(link->bounds) <= 0) {
            other = link;
            link = static_cast<RectLink *>(link->previous);
            destroy(other);
        }
    }
}

BaseCamera::BaseCamera()
{
    if (camera != nullptr) {
        return;
    }
    nameProcess(pid, "BaseCamera");
    makeSystem();
    camera = this;
    dirtyCount = 0;
}

void BaseCamera::onKill()
{
    camera = nullptr;
}

void BaseCamera::run()
{
    if (screenDirty != 0U) {
        flushScreen();
    }
}

namespace {
/* UNCALLED: nothing calls it. */
void startCamera()
{
    if (camera == nullptr) {
        camera = new BaseCamera;
        screenDirty = 0;
    }
}

/* UNCALLED: nothing calls it. */
void stopCamera() {}
}

void flushScreen()
{
    RectLink *link;

    if (screenDirty == 0U) {
        return;
    }
    flushRequested = 0;
    link = nullptr;
    screenDirty = 0;
    if (dirtyRects.first == nullptr) {
        return;
    }
    dirtyRects.merge();
    while (dirtyRects.forward(reinterpret_cast<GumpLink **>(&link))) {
        mouseCtl->drawCursor();
        screenViewport->blit(drawViewport, link->bounds.left, link->bounds.top, link->bounds.right, link->bounds.bottom,
                             link->bounds.left, link->bounds.top);
        mouseCtl->eraseCursor();
    }
    dirtyRects.clear();
    dirtyCount = 0;
}

void addDirtyRect(Rect area)
{
    if (dirtyCount < 0) {
        return;
    }
    Rect clip(area);
    if (clip.right < screenViewport->leftBound() || clip.left > screenViewport->rightBound()) {
        return;
    }
    if (clip.bottom < screenViewport->topBound() || clip.top > screenViewport->bottomBound()) {
        return;
    }
    if (clip.left < screenViewport->leftBound()) {
        clip.left = screenViewport->leftBound();
    }
    if (clip.top < screenViewport->topBound()) {
        clip.top = screenViewport->topBound();
    }
    if (clip.right > screenViewport->rightBound()) {
        clip.right = screenViewport->rightBound();
    }
    if (clip.bottom > screenViewport->bottomBound()) {
        clip.bottom = screenViewport->bottomBound();
    }
    if (RECT_WIDTH(clip) < 0 || RECT_HEIGHT(clip) < 0) {
        return;
    }
    screenDirty = 1;
    dirtyRects.add(clip);
    dirtyCount++;
}

Rect getClipRect()
{
    if (dirtyCount < 0) {
        return screenViewport->bounds();
    }
    dirtyCount--;
    dirtyRects.pop();
    return Rect(0, 0, 1, 1);
}

void clearDirtyRects()
{
    dirtyRects.clear();
    dirtyCount = 0;
}

/* UNCALLED: nothing calls it. */
void initCamera()
{
    startCamera();
}

/* UNCALLED: nothing calls it. */
void closeCamera()
{
    stopCamera();
}

/* Not called: three overlapping rectangles on a cleared screen, flushed. */
/* UNCALLED: nothing calls it. */
void testDirtyRects()
{
    drawViewport->clear(0);
    clearDirtyRects();
    addDirtyRect(Rect(110, 110, 115, 120));
    addDirtyRect(Rect(112, 115, 125, 125));
    addDirtyRect(Rect(120, 110, 130, 120));
    flushScreen();
    inputKey();
}
