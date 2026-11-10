/* MOUSECTL.C (1358:0000-113e). See mousectl.h. */
#include "gamestate.h"
#include "mousectl.h"
#include "kernel.h"
#include "file.h"
#include "input.h"
#include "clock.h"
#include "screen.h"

void enableMouseEvents();

Viewport *&cursorImage = gameGlobal<Viewport *>(nullptr); /* the cursor over what it hides */
Viewport *&cursorBack = gameGlobal<Viewport *>(nullptr);  /* what the cursor hides */
ShapeHeader *&cursorShapes = gameGlobal<ShapeHeader *>(nullptr);
ShapeFrame *&cursorFrame = gameGlobal<ShapeFrame *>(nullptr);
MouseCtl *&mouseCtl = gameGlobal<MouseCtl *>(nullptr);
i16 &unusedMouseCtl = gameGlobal<i16>(0);

MouseCtl::MouseCtl(char *path)
{
    if (mouseCtl != nullptr) {
        halt();
    }
    nameProcess(pid, "Mouse_Ctl");
    setRate(60);
    listenMouse();
    startTimer(1);
    pause();
    makeSystem();
    if (cursorShapes == nullptr) {
        loadShapes(path);
    }
    if (cursorImage == nullptr) {
        cursorImage = new Viewport(VIEWPORT_LARGE, bounds.left, bounds.top, bounds.right, bounds.bottom);
        cursorBack = new Viewport(VIEWPORT_LARGE, bounds.left, bounds.top, bounds.right, bounds.bottom);
    }
    memset(&current, 0, sizeof current);
    memset(&chord, 0, sizeof chord);
    memset(&unused106, 0, sizeof unused106);
    memset(&leftDown, 0, sizeof leftDown);
    memset(&leftUp, 0, sizeof leftUp);
    memset(&rightDown, 0, sizeof rightDown);
    memset(&rightUp, 0, sizeof rightUp);
    memset(&moved, 0, sizeof moved);
    unusedMouseCtl = 0;
    visible = 0;
    drawn = 0;
    pointerX = 0;
    pointerY = 0;
    cursorX = 0;
    cursorY = 0;
    hotX = 0;
    hotY = 0;
    buttons = 0;
    depth = 0;
    memset(shapes, 0, sizeof shapes);
    shapes[0] = 1;
    setShape(0);
    cursorX = pointerX - hotX;
    cursorY = pointerY - hotY;
    mouseCtl = this;
    chord.state = RECORD_EMPTY;
    leftDown.state = RECORD_EMPTY;
    rightDown.state = RECORD_EMPTY;
    moved.state = RECORD_EMPTY;
    inputMousePlace(0, 0);
    eventMask |= EVENT_MOUSE | EVENT_TIMER;
}

void MouseCtl::onKill()
{
    halt();
}

void MouseCtl::load(File *file)
{
    Process::load(file);
    mouseCtl = this;
}

/* UNCALLED: nothing calls it. */
void MouseCtl::close()
{
    eventMask &= ~(EVENT_MOUSE | EVENT_TIMER);
    delete cursorImage;
    delete cursorBack;
    if (isValid(mouseCtl)) {
        mouseCtl->terminate(0);
        mouseCtl = nullptr;
    }
    if (cursorShapes != nullptr) {
        delete[] cursorShapes;
        cursorShapes = nullptr;
    }
}

void MouseCtl::setLimits(u32 /*unused*/)
{
    inputMouseXRange(0, 1278);
    inputMouseYRange(0, SCREEN_BOTTOM);
}

void MouseCtl::run() {}

void MouseCtl::event(u32 kind, EventData data)
{
    u16 window;
    u16 x;
    s16 y;

    if ((kind & EVENT_TIMER) != 0U) {
        u16 settled = 1;

        if (leftDown.state != RECORD_EMPTY) {
            settled = 0;
            leftDown.ticks++;
            if (leftDown.ticks == PRESS_DELAY) {
                events.push(leftDown);
                if (leftUp.state != RECORD_EMPTY) {
                    events.push(leftUp);
                    leftUp.state = RECORD_EMPTY;
                }
                leftDown.state = RECORD_EMPTY;
                settled = 1;
            }
        }
        settled |= 2;
        if (rightDown.state != RECORD_EMPTY) {
            settled &= 1;
            rightDown.ticks++;
            if (rightDown.ticks == PRESS_DELAY) {
                events.push(rightDown);
                if (rightUp.state != RECORD_EMPTY) {
                    events.push(rightUp);
                    rightUp.state = RECORD_EMPTY;
                }
                rightDown.state = RECORD_EMPTY;
                settled |= 2;
            }
        }
        if (settled == 3) {
            eventMask &= ~EVENT_TIMER;
            return;
        }
    } else if ((kind & EVENT_MOUSE) != 0U) {
        MouseEvent *mouse = data.pointer<MouseEvent>();
        u16 mouseEvents;
        u16 newX;
        s16 newY;
        u16 shift;
        MouseRecord record;

        newX = x = mouse->x >> 1;
        newY = y = mouse->y;
        record.x = x;
        record.y = y;
        record.buttons = mouse->buttons;
        record.shift = 0;
        /* The original leaves a move's and a release's ticks as its stack
         * had them: only a press's are read (the timer counts them), but
         * the records are saved. */
        record.ticks = 0;
        buttons = record.buttons;
        mouseEvents = mouse->events;
        record.action = 0;
        if ((mouseEvents & MOUSE_MOVED) != 0) {
            record.state = RECORD_MOVED;
            record.action = MOUSE_MOVED;
            moved = record;
            if ((drawn == 0U) && (visible != 0U)) {
                window = getWindow();
                clockPushHold();
                clockHold();
                screenViewport->blit(cursorBack, 0, 0, width, height, cursorX, cursorY);
                cursorX = newX - hotX;
                cursorY = newY - hotY;
                updateFrame();
                cursorImage->blit(screenViewport, cursorX, cursorY, cursorX + width, cursorY + height);
                cursorBack->blit(cursorImage);
                cursorImage->drawShape(cursorFrame->xOffset, cursorFrame->yOffset, cursorFrame);
                screenViewport->blit(cursorImage, 0, 0, width, height, cursorX, cursorY);
                clockPopHold();
                setWindow(window);
            }
            pointerX = newX;
            pointerY = newY;
        }
        if ((mouseEvents & MOUSE_LEFT_UP) != 0) {
            record.state = RECORD_RELEASED;
            record.action = MOUSE_LEFT_UP;
            if (leftDown.state == RECORD_EMPTY) {
                if ((mouseEvents & MOUSE_RIGHT_UP) != 0) {
                    record.action = MOUSE_BOTH_UP;
                }
                events.push(record);
            } else {
                eventMask &= ~EVENT_TIMER;
                leftUp = record;
                eventMask |= EVENT_TIMER;
            }
        } else if ((mouseEvents & MOUSE_LEFT_DOWN) != 0) {
            shift = keyShift;
            record.ticks = 0;
            record.state = RECORD_PRESSED;
            record.action = MOUSE_LEFT_DOWN;
            record.shift = shift;
            if (leftDown.state != RECORD_EMPTY) {
                record.state = RECORD_CHORD;
                events.push(record);
                leftDown.state = RECORD_EMPTY;
            } else {
                eventMask &= ~EVENT_TIMER;
                leftUp.state = RECORD_EMPTY;
                leftDown = record;
                if (rightDown.state != RECORD_EMPTY) {
                    record.action = MOUSE_BOTH_DOWN;
                    chord = record;
                    leftDown.state = RECORD_EMPTY;
                    rightDown.state = RECORD_EMPTY;
                    events.push(record);
                } else {
                    eventMask |= EVENT_TIMER;
                }
            }
        }
        if ((mouseEvents & MOUSE_RIGHT_UP) != 0) {
            record.state = RECORD_RELEASED;
            record.action = MOUSE_RIGHT_UP;
            if (rightDown.state == RECORD_EMPTY) {
                if ((mouseEvents & MOUSE_LEFT_UP) != 0) {
                    return;
                }
                events.push(record);
                return;
            }
            eventMask &= ~EVENT_TIMER;
            rightUp = record;
            eventMask |= EVENT_TIMER;
            return;
        }
        if ((mouseEvents & MOUSE_RIGHT_DOWN) != 0) {
            shift = inputShiftState();
            record.ticks = 0;
            record.state = RECORD_PRESSED;
            record.action = MOUSE_RIGHT_DOWN;
            record.shift = shift;
            if (rightDown.state != RECORD_EMPTY) {
                record.state = RECORD_CHORD;
                events.push(record);
                rightDown.state = RECORD_EMPTY;
                return;
            }
            eventMask &= ~EVENT_TIMER;
            rightUp.state = RECORD_EMPTY;
            rightDown = record;
            if (leftDown.state != RECORD_EMPTY) {
                record.action = MOUSE_BOTH_DOWN;
                chord = record;
                leftDown.state = RECORD_EMPTY;
                rightDown.state = RECORD_EMPTY;
                events.push(record);
                return;
            }
            eventMask |= EVENT_TIMER;
        }
    }
}

void MouseCtl::hide()
{
    if (visible == 0U) {
        return;
    }
    visible = 0;
    screenViewport->blit(cursorBack, 0, 0, width, height, cursorX, cursorY);
}

void MouseCtl::show()
{
    if (visible != 0U) {
        return;
    }
    updateFrame();
    cursorImage->blit(screenViewport, cursorX, cursorY, cursorX + width, cursorY + height);
    cursorBack->blit(cursorImage);
    cursorImage->drawShape(cursorFrame->xOffset, cursorFrame->yOffset, cursorFrame);
    screenViewport->blit(cursorImage, 0, 0, width, height, cursorX, cursorY);
    visible = 1;
}

void MouseCtl::setShape(i16 shape)
{
    u8 wasVisible;

    if (cursorShapes == nullptr) {
        halt();
    }
    wasVisible = visible;
    if (wasVisible != 0U) {
        hide();
    }
    shapes[depth] = shape;
    updateFrame();
    if (wasVisible != 0U) {
        show();
    }
    if (cursorFrame == nullptr) {
        haltMessage("Mouse pointer corruption");
    }
}

void MouseCtl::updateFrame()
{
    const i16 frame = frameAt(cursorX, cursorY);
    u8 *base = reinterpret_cast<u8 *>(cursorShapes);
    const u32 offset = cursorShapes->frames[frame].offset;

    cursorFrame = reinterpret_cast<ShapeFrame *>(base + offset);
    hotX = cursorFrame->xOffset;
    hotY = cursorFrame->yOffset;
    width = cursorFrame->width - 1;
    height = cursorFrame->height - 1;
}

void MouseCtl::pushShape(i16 shape)
{
    depth++;
    if (depth == 8) {
        depth = 7;
    }
    setShape(shape);
}

void MouseCtl::resetShape(i16 shape)
{
    depth = 0;
    setShape(shape);
}

i16 MouseCtl::popShape()
{
    const i16 shape = shapes[depth];

    depth--;
    if (depth < 0) {
        depth = 0;
    }
    setShape(shapes[depth]);
    return shape;
}

/* UNCALLED: nothing calls it. */
void MouseCtl::loadShapes(char *path)
{
    File file(path, 0, 0);

    cursorShapes = reinterpret_cast<ShapeHeader *>(new u8[file.size()]);
    if (cursorShapes == nullptr) {
        allocationFailure();
    }
    file.read(cursorShapes, file.size());
    file.close();
    bounds.set(0, 0, cursorShapes->width, cursorShapes->height);
}

bool MouseCtl::getEvent()
{
    if (static_cast<unsigned int>(events.isEmpty()) == 0U) {
        current = events.pop();
        return true;
    }
    if (moved.state != RECORD_EMPTY) {
        current = moved;
        moved.state = RECORD_EMPTY;
        return true;
    }
    return false;
}

void MouseCtl::drawCursor()
{
    if (visible != 0U) {
        drawn = 1;
        updateFrame();
        cursorImage->blit(drawViewport, cursorX, cursorY, cursorX + width, cursorY + height);
        drawShape(cursorX + hotX, cursorY + hotY, cursorFrame);
    }
}

void MouseCtl::eraseCursor()
{
    if (visible != 0U) {
        drawViewport->blit(cursorImage, 0, 0, width, height, cursorX, cursorY);
        cursorBack->blit(drawViewport, cursorX, cursorY, cursorX + width, cursorY + height);
        drawn = 0;
    }
}

i16 MouseCtl::shapeCount()
{
    return cursorShapes->frameCount;
}

void MouseCtl::moveTo(s16 x, s16 y)
{
    u8 wasVisible;
    s16 oldX;
    s16 oldY;

    inputMousePlace(x * 2, y);
    disableMouseEvents();
    wasVisible = visible;
    if (wasVisible != 0U) {
        hide();
    }
    oldX = pointerX;
    oldY = pointerY;
    pointerX = x;
    pointerY = y;
    cursorX = x - hotX;
    cursorY = y - hotY;
    if (oldX != x || oldY != y) {
        MouseRecord record;
        record.x = pointerX;
        record.y = pointerY;
        record.buttons = 0;
        record.shift = 0;
        record.ticks = 0; /* as above: the original's are its stack's */
        record.action = 0;
        record.state = RECORD_MOVED;
        record.action = MOUSE_MOVED;
        moved = record;
    }
    cursorBack->blit(screenViewport, cursorX, cursorY, cursorX + width, cursorY + height);
    if (wasVisible != 0U) {
        show();
    }
    enableMouseEvents();
    inputMouseXRange(0, 1278);
    inputMouseYRange(0, SCREEN_BOTTOM);
}

void closeCursor()
{
    delete cursorImage;
    cursorImage = nullptr;
    delete cursorBack;
    cursorBack = nullptr;
    delete[] cursorShapes;
    cursorShapes = nullptr;
}

void disableMouseEvents()
{
    if (mouseCtl != nullptr) {
        mouseCtl->eventMask &= ~EVENT_MOUSE;
    }
}

void enableMouseEvents()
{
    if (mouseCtl != nullptr) {
        mouseCtl->eventMask |= EVENT_MOUSE;
    }
}
