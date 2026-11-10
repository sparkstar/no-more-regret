/* NEWGUMP.C (1360:0000-1423): NewGump, its events and child lists, and the
 * static no-target gump. See newgump.h. */
#include "gamestate.h"
#include <cstdarg>
#include <cstring>
#include <unordered_set>
#include <vector>
#include "newgump.h"
#include "basecam.h"
#include "dispatch.h"
#include "viewport.h"

NewGump &noEventTarget = gameGlobal<NewGump>(NewGumpId(), nullptr, 0L);
NewGump *&focusedGump = gameGlobal<NewGump *>(nullptr);
namespace {
/* The gumps operator new made and operator delete has not freed: the
 * original tells a gump on the stack by its selector, SS. */
std::unordered_set<void *> &heapGumps = gameGlobal<std::unordered_set<void *>>();

void rememberGump(void *gump)
{
    heapGumps.insert(gump);
}

/* Whether operator new made gump, which it forgets. */
bool forgetGump(void *gump)
{
    return heapGumps.erase(gump) != 0;
}
}
u8 &paintDepth = gameGlobal<u8>(0);
u8 &paintStarted = gameGlobal<u8>(0);
s16 &liveGumps = gameGlobal<s16>(0);
s16 &peakGumps = gameGlobal<s16>(0);
u16 &inactivePaintDepth = gameGlobal<u16>(0);

NewGumpEvent::NewGumpEvent(s16 k, s16 px, s16 py, s32 arg, NewGump *from, NewGump *to, u32 value)
{
    kind = k;
    x = px;
    y = py;
    argument = static_cast<s16>(arg);
    target = to;
    sender = from;
    extra = value;
}

bool NewGumpEvent::isMousePress() const
{
    return static_cast<u16>(input) == 1;
}

bool NewGumpEvent::isMouseRepeat() const
{
    return static_cast<u16>(input) == 2;
}

bool NewGumpEvent::isMouseRelease() const
{
    return static_cast<u16>(input) == 3;
}

/* UNCALLED: nothing calls it. */
i16 NewGumpEvent::isInitialPress() const
{
    return static_cast<i16>(static_cast<u16>(input) == 1);
}

NewGumpChildList::NewGumpChildList() = default;

NewGumpChildList::~NewGumpChildList()
{
    clear();
}

void NewGumpChildList::appendChild(NewGump *child)
{
    append(child);
}

void NewGumpChildList::raiseChild(NewGump *child)
{
    NewGump *current = nullptr;
    while (forward(&current)) {
        if (current == child) {
            raise(current);
            return;
        }
    }
}

bool NewGumpChildList::destroyChild(NewGump *child)
{
    NewGump *current = nullptr;
    while (forward(&current)) {
        if (current == child) {
            destroy(current);
            return true;
        }
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool NewGumpChildList::detachChild(NewGump *child)
{
    NewGump *current = nullptr;
    while (forward(&current)) {
        if (current == child) {
            detach(current);
            return true;
        }
    }
    return false;
}

/* UNCALLED: nothing calls it. */
NewGump *NewGumpChildList::at(i16 target)
{
    i16 index = -1;
    NewGump *result = nullptr;
    while (forward(&result) && index != target) {
        ++index;
    }
    return result;
}

NewGump *NewGumpChildList::findByInstanceId(u8 wanted)
{
    NewGump *current = nullptr;
    while (forward(&current)) {
        if (current->id.instance() == wanted) {
            break;
        }
    }
    return current;
}

NewGump *NewGumpChildList::findNextByClassId(u8 wanted, NewGump **current)
{
    while (GumpList::forward(reinterpret_cast<GumpLink **>(current))) {
        if (static_cast<u8>((*current)->id.value >> 8) == wanted) {
            break;
        }
    }
    return *current;
}

NewGump *NewGumpChildList::findByIdParts(u8 classId, u8 instanceId)
{
    NewGump *current = nullptr;
    while (forward(&current)) {
        if (current->id.instance() == instanceId && static_cast<u8>(current->id.value >> 8) == classId) {
            break;
        }
    }
    return current;
}

NewGump::NewGump(NewGumpId identity, NewGump *owner, u32 mask)
{
    id = identity;
    parent = owner;
    eventMask = mask;
    paintState = 0;
    active = 0;
    enabled2e = 1;
    clipChildren = 1;
    if (parent != nullptr) {
        parent->addChild(this);
    }
}

NewGump::~NewGump()
{
    if (this == focusedGump) {
        focusedGump = nullptr;
    }
}

void *NewGump::operator new(size_t size)
{
    void *result = ::operator new(size);
    if (result == nullptr) {
        debugPrintfWait(0x32, "\rOut of Memory! [%u]", static_cast<unsigned>(size));
        halt();
    }
    ++liveGumps;
    if (liveGumps > peakGumps) {
        ++peakGumps;
    }
    rememberGump(result);
    return result;
}

void NewGump::operator delete(void *pointer)
{
    /* Objects on the stack are not freed: the original compares the
     * pointer's selector with the stack's, the port looks for it among the
     * gumps operator new made. */
    if (forgetGump(pointer)) {
        ::operator delete(pointer);
    }
    --liveGumps;
}

bool NewGump::isHidden() const
{
    return (static_cast<i16>(paintState) & 3) == 1 || (static_cast<i16>(paintState) & 3) == 3;
}

void NewGump::hide()
{
    if ((static_cast<i16>(paintState) & 3) != 1 && (static_cast<i16>(paintState) & 3) != 3) {
        pushPaintState(3);
    }
}

void NewGump::paintTree(s16 dx, s16 dy, Rect clipRect)
{
    NewGump *child = nullptr;
    Rect moved(&bounds);
    [[maybe_unused]] const Rect clip = drawViewport->bounds(); /* not read */
    moved.moveBy(dx, dy);
    if (clipChildren == 0U) {
        ++inactivePaintDepth;
    }
    paintStarted = 0;
    if ((paintState & static_cast<s32>(1L)) == 0) {
        if ((paintDepth == 0U) || (clipChildren == 0U)) {
            if (clipChildren != 0U) {
                addDirtyRect(clipRect);
            } else {
                addDirtyRect(moved);
            }
        }
        paintStarted = 1;
        paintSelf(moved.left, moved.top);
    }
    if ((paintState & static_cast<s32>(2L)) == 0) {
        while (children.forward(&child)) {
            paintDepth++;
            child->paintTree(bounds.left + dx, bounds.top + dy, clipRect);
            paintDepth--;
        }
    }
    if (clipChildren == 0U) {
        --inactivePaintDepth;
    }
}

void NewGump::paint(u16 notify)
{
    Rect area;
    s16 x;
    s16 y;
    x = parent != nullptr ? parent->getAbsoluteX() : 0;
    y = parent != nullptr ? parent->getAbsoluteY() : 0;
    area = bounds;
    area.moveBy(x, y);
    paintDepth = 0;
    paintTree(x, y, area);
    if (notify != 0U) {
        camera->run();
    }
}

/* UNCALLED: nothing calls it. */
NewGump *NewGump::findDeepestAtPoint(NewGumpEvent event)
{
    const bool inside = eventPointInBounds(&event);
    NewGump *child;
    NewGump *result;
    if (!inside || (paintState & static_cast<u32>(1UL)) != 0) {
        return nullptr;
    }
    if (active == 0U) {
        if ((paintState & static_cast<u32>(2UL)) == 0) {
            child = nullptr;
            result = nullptr;
            event.x -= bounds.left;
            event.y -= bounds.top;
            while (children.backward(&child)) {
                result = child->findDeepestAtPoint(event);
                if (result != nullptr) {
                    return result;
                }
            }
        }
    }
    return this;
}

u8 NewGump::findEventTarget(NewGumpEvent *event, NewGump **output, u16 options)
{
    u8 recursiveResult;
    bool hit;
    NewGump *current;
    if (event->kind != EVENT_EXECUTE && event->kind != EVENT_MOTION && (eventMask & static_cast<s32>(0x400L)) != 0) {
        return 0;
    }
    /* Keys, broadcasts and kind 0x80 need no hit. */
    hit = acceptsEvent(event) || ((event->kind & 0xe0) != 0);
    *output = nullptr;
    if (((options & 1) == 0) && !hit) {
        return 0;
    }
    if ((active == 0U) && ((paintState & static_cast<s32>(2L)) == 0)) {
        current = nullptr;
        event->x -= bounds.left;
        event->y -= bounds.top;
        while (children.backward(&current)) {
            current->validate();
            recursiveResult = current->findEventTarget(event, output, options);
            if (recursiveResult != 0 && *output != &noEventTarget) {
                if (current->parent != nullptr) {
                    current->parent->children.raiseChild(current);
                }
                return recursiveResult;
            }
        }
        event->x += bounds.left;
        event->y += bounds.top;
    }
    if (((paintState & static_cast<s32>(1L)) != 0) && ((options & 4) == 0)) {
        return 0;
    }
    if ((event->kind & EVENT_KEY) != 0 && !acceptsEventArgument(event)) {
        return 0;
    }
    if ((event->kind & eventMask) != 0 || (options & 2) != 0) {
        *output = this;
    } else {
        *output = &noEventTarget;
    }
    return 1;
}

bool NewGump::eventPointInBounds(NewGumpEvent *event) const
{
    return bounds.contains(event->x, event->y);
}

void NewGump::pushPaintState(i16 bits)
{
    if (paintState == 0) {
        paintState = bits;
    } else {
        paintState <<= 2;
        paintState |= bits;
    }
}

/* UNCALLED: nothing calls it. */
bool NewGump::reparent(NewGump *child, NewGump *oldParent)
{
    if (oldParent->children.detachChild(child)) {
        children.appendChild(child);
        child->parent = this;
        return true;
    }
    return false;
}

s16 NewGump::getAbsoluteX() const
{
    if (parent == nullptr) {
        return bounds.left;
    }
    return bounds.left + parent->getAbsoluteX();
}

s16 NewGump::getAbsoluteY() const
{
    if (parent == nullptr) {
        return bounds.top;
    }
    return bounds.top + parent->getAbsoluteY();
}

s16 NewGump::eventX(NewGumpEvent *event) const
{
    return event->x - bounds.left;
}

s16 NewGump::eventY(NewGumpEvent *event) const
{
    return event->y - bounds.top;
}

void NewGump::paintSelf(i16 /*unused*/, i16 /*unused*/)
{
    if (paintDepth == 0U) {
        getClipRect();
        paintDepth--;
    }
}

void NewGump::dispatchEvent(NewGumpEvent *event)
{
    if (event->kind == EVENT_LEFT_BUTTON || event->kind == EVENT_RIGHT_BUTTON || event->kind == EVENT_BOTH_BUTTONS ||
        event->kind == EVENT_MOTION) {
        /* A click or motion takes the focus; the old focus hears of it. */
        if (focusedGump != nullptr && this != focusedGump && ((focusedGump->eventMask & EVENT_LOSE_FOCUS) != 0U)) {
            NewGumpEvent copy(*event);
            copy.sender = this;
            copy.target = focusedGump;
            focusedGump->focusLost(&copy);
        }
        focusedGump = this;
    }
    switch (event->kind) {
    case EVENT_EXECUTE:
        execute(event);
        return;
    case EVENT_LEFT_BUTTON:
        handleLeftButton(event);
        return;
    case EVENT_RIGHT_BUTTON:
        handleRightButton(event);
        return;
    case EVENT_BOTH_BUTTONS:
        handleBothButtons(event);
        return;
    case EVENT_MOTION:
        handleMouseMotion(event);
        return;
    case EVENT_BROADCAST: {
        NewGump *current = nullptr;
        while (children.forward(&current)) {
            current->dispatchEvent(event);
        }
        if ((eventMask & EVENT_BROADCAST) != 0U) {
            handleBroadcast(event);
        }
        return;
    }
    case EVENT_COMMAND:
        handleCommand(event);
        return;
    case EVENT_KEY:
        handleKeyboard(event);
        return;
    default:
        guiDispatcher->invalidEvent(this, event);
        return;
    }
}

void NewGump::addAcceptedArguments(int first, ...)
{
    u16 total;
    u16 oldCount;
    s16 argCount;
    std::vector<s16> arguments;
    std::unique_ptr<s16[]> fresh;
    s16 *scan;
    va_list list;

    oldCount = 0;
    argCount = 0;
    /* The original reads the arguments where they lie on the stack, as an
     * array that its 0 ends. */
    va_start(list, first);
    for (int argument = first;; argument = va_arg(list, int)) {
        arguments.push_back(static_cast<s16>(argument));
        if (argument == 0) {
            break;
        }
    }
    va_end(list);
    while (arguments[argCount] != 0) {
        argCount++;
    }
    if (argCount == 0) {
        accepted.reset();
        eventMask &= ~EVENT_KEY;
        return;
    }
    if (accepted != nullptr) {
        scan = accepted.get();
        while (*scan++ != 0) {
            oldCount++;
        }
    }
    total = argCount + oldCount + 1;
    fresh = std::make_unique_for_overwrite<s16[]>(total);
    if (fresh == nullptr) {
        allocationFailure();
    }
    if (accepted != nullptr) {
        memcpy(fresh.get(), accepted.get(), oldCount * sizeof(s16));
    }
    memcpy(fresh.get() + oldCount, arguments.data(), (argCount + 1) * sizeof(s16));
    /* The old ones go as the new ones come. */
    accepted = std::move(fresh);
    eventMask |= EVENT_KEY;
}

bool NewGump::acceptsEventArgument(NewGumpEvent *event) const
{
    s16 *p;
    if (accepted == nullptr) {
        return false;
    }
    p = accepted.get();
    if (*p == -1) {
        return true;
    }
    while (*p != 0) {
        if (event->argument != *p++) {
            continue;
        }
        return true;
    }
    return false;
}

void NewGump::invalidate()
{
    const s16 x = parent != nullptr ? parent->getAbsoluteX() : 0;
    const s16 y = parent != nullptr ? parent->getAbsoluteY() : 0;
    Rect area(&bounds);
    area.moveBy(x, y);
    invalidateRect(area);
}

void NewGump::invalidateRect(Rect value)
{
    value.intersect(drawViewport->bounds());
    drawViewport->clear(0, value.left, value.top, value.right, value.bottom);
    camera->invalidate(value, 0);
    addDirtyRect(value);
}

void NewGump::capture(NewGump *other)
{
    if (other == nullptr) {
        other = this;
    }
    guiDispatcher->capture(other);
}

void NewGump::release()
{
    guiDispatcher->release();
}

void NewGump::sendEvent(NewGumpEvent *event)
{
    guiDispatcher->enqueue(event);
}

/* UNCALLED: nothing calls it. */
void NewGump::sendAlternateEvent(NewGumpEvent *event)
{
    guiDispatcher->enqueueAlternate(event);
}

/* UNCALLED: nothing calls it. */
void NewGump::invalidEvent(NewGump *gump, NewGumpEvent *event)
{
    guiDispatcher->invalidEvent(gump, event);
}

void NewGump::close()
{
    if (!isHidden()) {
        hide();
        invalidate();
    }
    guiDispatcher->remove(this);
    parent->removeChild(this);
}

void NewGump::validate(std::source_location place) const
{
    /* The original passed its arguments in the wrong order (the format as
     * the file name, the id as the line); the port halts at the caller's
     * place with the id. */
    if (!id.isValid()) {
        haltMessage(haltFormat("Bad NewGumpId [%04X]", id.value), place);
    }
}

/* UNCALLED: nothing calls it. */
bool NewGump::hasChildMatchingQuery(void *query)
{
    NewGump *child = nullptr;
    while (children.forward(&child)) {
        if (child->matchesQuery(query)) {
            return true;
        }
    }
    return false;
}
