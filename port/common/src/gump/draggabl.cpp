/* 1328:0000-0651, file name unknown: DraggableGump. See draggabl.h. */
#include "draggabl.h"
#include "basecam.h"

/* UNCALLED: nothing calls it. */
DraggableGump::DraggableGump(NewGumpId gumpId, NewGump *owner) : NewGump(gumpId, owner, 0L)
{
    eventMask |= EVENT_LEFT_BUTTON;
    dragging = 0;
}

DraggableGump::~DraggableGump()
{
    if (dragging != 0U) {
        delete image;
        delete background;
    }
}

void DraggableGump::handleMouseMotion(NewGumpEvent *event)
{
    if (dragging == 0) {
        return;
    }
    Rect area(&bounds);
    drawViewport->blit(background, 0, 0, area.right - area.left, area.bottom - area.top, area.left, area.top);
    addDirtyRect(area);
    bounds.moveTo(event->x - grabX, event->y - grabY);
    area.moveTo(event->x - grabX, event->y - grabY);
    background->blit(drawViewport, area.left, area.top, area.right, area.bottom, 0, 0);
    drawViewport->blit(image, 0, 0, area.right - area.left, area.bottom - area.top, area.left, area.top);
    addDirtyRect(area);
}

void DraggableGump::handleLeftButton(NewGumpEvent *event)
{
    if (event->isMouseRelease()) {
        if (dragging == 1) {
            delete image;
            delete background;
            if (parent != nullptr) {
                parent->children.raiseChild(this);
            }
            paintState >>= 2;
            dragging = 0;
            eventMask &= ~EVENT_MOTION;
            release();
            paint(0);
        }
    } else if (event->isMousePress()) {
        capture(this);
        pushPaintState(3);
        invalidate();
        paintState >>= 2;
        background = new Viewport(drawViewport->flags(), 0, 0, bounds.right - bounds.left, bounds.bottom - bounds.top);
        Rect area(&bounds);
        area.moveTo(getAbsoluteX(), getAbsoluteY());
        background->blit(drawViewport, area.left, area.top, area.right, area.bottom, 0, 0);
        image = new Viewport(drawViewport->flags(), 0, 0, bounds.right - bounds.left, bounds.bottom - bounds.top);
        image->clear(TRANSPARENT);
        {
            const DrawTarget redirect(image);
            const Rect saved(&bounds);
            bounds.moveTo(0, 0);
            paintTree(0, 0, Rect(&bounds));
            bounds = saved;
            pushPaintState(3);
        }
        grabX = event->x - bounds.left;
        grabY = event->y - bounds.top;
        eventMask |= EVENT_MOTION;
        dragging = 1;
        handleMouseMotion(event);
    }
}

void DraggableGump::paintSelf(i16 /*x*/, i16 /*y*/) {}
