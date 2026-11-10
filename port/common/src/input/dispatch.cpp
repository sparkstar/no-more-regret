/* DISPATCH.C. See dispatch.h. */
#include "dispatch.h"
#include "gamestate.h"
#include "bindings.h"
#include "devices.h"
#include "basecam.h"
#include "control.h"
#include "gumpshap.h"
#include "kernel.h"
#include "mousectl.h"
#include "system.h"
#include "file.h"
#include "world.h"
#include "clock.h"
#include "host.h"
#include "waits.h"

#include <memory>

const char *&eventFile = gameGlobal<const char *>("events.dat"); /* not used */
/* The step dispatch() is at, for the debugger. */
const char *&phase = gameGlobal<const char *>(nullptr);
GumpDispatcher *&guiDispatcher = gameGlobal<GumpDispatcher *>(nullptr);
Dispatcher *&dispatcher = gameGlobal<Dispatcher *>(nullptr);
u8 &dispatcherReady = gameGlobal<u8>(0);
u32 &dispatchFrame = gameGlobal<u32>(0);
i16 &eventHead = gameGlobal<i16>(0);
i16 &eventTail = gameGlobal<i16>(0);
i16 &alternateHead = gameGlobal<i16>(0);
i16 &alternateTail = gameGlobal<i16>(0);
i16 &modalTop = gameGlobal<i16>(-1);
i16 &recordMode = gameGlobal<i16>(RECORD_OFF);
File *&recordFile = gameGlobal<File *>(nullptr);
NewGumpEvent *&events = gameGlobal<NewGumpEvent *>(nullptr);
NewGumpEvent *&alternates = gameGlobal<NewGumpEvent *>(nullptr);
NewGumpEvent *&currentEvent = gameGlobal<NewGumpEvent *>(nullptr);
RecordedEvent *&recordBuffer = gameGlobal<RecordedEvent *>(nullptr);
s32 &recorded = gameGlobal<s32>(0);
NewGump *&capturedGump = gameGlobal<NewGump *>(nullptr);
NewGump *&parkedGump = gameGlobal<NewGump *>(nullptr);
NewGump **&modals = gameGlobal<NewGump **>(nullptr);
s32 &playbackCount = gameGlobal<s32>(0);
s32 &recordFileSize = gameGlobal<s32>(0);
u8 &playbackStarted = gameGlobal<u8>(0);

u32 &unusedDispatch = gameGlobal<u32>();
KeyQueue &gumpKeys = gameGlobal<KeyQueue>();
/* The last arrow key, which poll() takes before the queue. */
KeyEvent &lastArrow = gameGlobal<KeyEvent>();

void initDispatcher()
{
    new GumpDispatcher(0);
    new Dispatcher;
}

void closeDispatcherEvent()
{
    delete currentEvent;
    currentEvent = nullptr;
}

void closeDispatcher()
{
    if (guiDispatcher != nullptr) {
        guiDispatcher->shutdown();
        delete guiDispatcher;
        guiDispatcher = nullptr;
    }
}

Dispatcher::Dispatcher()
{
    nameProcess(pid, "Dispatcher");
    makeSystem();
    listenKeyboard();
    hold();
    eventMask |= EVENT_KEYBOARD;
    dispatcher = this;
}

void Dispatcher::onKill()
{
    dispatcher = nullptr;
}

void Dispatcher::load(File *file)
{
    Process::load(file);
    dispatcher = this;
}

void Dispatcher::run()
{
    guiDispatcher->dispatch();
}

i16 keyCode(const KeyEvent &key)
{
    i16 code = 0;
    u16 ascii;

    if ((key.shift & (KEY_RIGHT_SHIFT | KEY_LEFT_SHIFT | KEY_CONTROL | KEY_ALT)) != 0) {
        if ((key.shift & KEY_ALT) != 0) {
            code = static_cast<i16>(key.scan + KEYCODE_ALT);
        }
        if ((key.shift & KEY_CONTROL) != 0) {
            code = static_cast<i16>(key.scan + KEYCODE_CONTROL);
        }
        if ((key.shift & (KEY_RIGHT_SHIFT | KEY_LEFT_SHIFT)) != 0) {
            if ((ascii = key.ascii) == 0) {
                code = static_cast<i16>(key.scan + KEYCODE_SHIFT);
            } else {
                code = static_cast<i16>(ascii);
            }
        }
    } else if (key.ascii != 0U) {
        code = key.ascii;
    } else {
        code = static_cast<i16>(key.scan | KEYCODE_SCAN);
    }
    return code;
}

namespace {
/* Whether the key gives the world's gump Escape: the key the action of
 * Escape is bound to (bindings.h). */
bool escapeKey(const KeyEvent &key)
{
    i16 bound;
    return worldCode(keyCode(key), &bound) && bound == ASCII_ESCAPE;
}
}

void Dispatcher::event(u32 kind, EventData data)
{
    switch (kind) {
    case EVENT_KEYBOARD: {
        KeyEvent *key = data.pointer<KeyEvent>();
        /* The key settings' wait for an input takes the keys, pressed and
         * let go (bindings.h). */
        if (captureKey(*key)) {
            return;
        }
        if (key->pressed != 0U) {
            if (key->scan == SCAN_DELETE && ((keyShift & KEY_CONTROL) != 0) && ((keyShift & KEY_ALT) != 0)) {
                gumpKeys.clear();
                return;
            }
            /* Escape, or the key it is bound to (bindings.h). */
            if ((key->scan == SCAN_ESCAPE || escapeKey(*key)) && (key->pressed != 0U)) {
                unknown00a2 = controlledActor;
            }
            if (key->ascii == '?' && (key->extended != 0U)) {
                return;
            }
            if ((key->ascii != 0U) && key->repeat == 1) {
                gumpKeys.push(*key);
                return;
            }
            if (key->scan == SCAN_UP || key->scan == SCAN_DOWN || key->scan == SCAN_LEFT || key->scan == SCAN_RIGHT) {
                lastArrow = *key;
                return;
            }
            if (key->repeat == 1) {
                gumpKeys.push(*key);
            }
        }
    } break;
    }
}

GumpDispatcher::GumpDispatcher(i16 mode)
{
    i16 i;

    if (guiDispatcher != nullptr) {
        hostDebugBreak();
    }
    if (dispatcherReady == 0U) {
        dispatcherReady = 1;
        guiColors = &guiColorSets[0];
        currentEvent = new NewGumpEvent;
        currentEvent->kind = EVENT_EXECUTE;
        currentEvent->sender = nullptr;
        events = new NewGumpEvent[QUEUED_EVENTS];
        alternates = new NewGumpEvent[QUEUED_EVENTS];
        for (i = 0; i < QUEUED_EVENTS; i++) {
            events[i].kind = EVENT_EXECUTE;
            events[i].sender = nullptr;
            alternates[i].kind = EVENT_EXECUTE;
            alternates[i].sender = nullptr;
        }
        modals = new NewGump *[MODAL_DEPTH];
        for (i = 0; i < MODAL_DEPTH; i++) {
            modals[i] = nullptr;
        }
        recordMode = mode;
        if (recordMode != 0) {
            recordBuffer = new RecordedEvent[RECORD_EVENTS];
            recordFile = new File;
        }
    }
    lastArrow.scan = 0;
    guiDispatcher = this;
    NewGumpEvent start;
    start.kind = EVENT_MOTION;
    start.x = 0;
    start.y = 0;
    start.argument = 0;
    start.sender = nullptr;
    post(&start);
}

GumpDispatcher::~GumpDispatcher()
{
    shutdown();
}

void GumpDispatcher::shutdown()
{
    if (dispatcherReady != 0U) {
        while (modalTop >= 0) {
            popModal();
        }
        if (parkedGump != nullptr) {
            delete parkedGump;
        }
        delete[] modals;
        delete[] events;
        delete[] alternates;
        modals = nullptr;
        events = nullptr;
        alternates = nullptr;
        if (recordBuffer != nullptr) {
            recordFile->write(recordBuffer, recorded * static_cast<ui16>(sizeof(RecordedEvent)));
            recordFile->commit();
            delete[] recordBuffer;
        }
        if (recordFile != nullptr) {
            delete recordFile;
        }
        dispatcherReady = 0;
    }
}

void GumpDispatcher::poll()
{
    bool posted = false;
    s32 code;
    KeyEvent key;

    currentEvent->kind = EVENT_EXECUTE;
    currentEvent->sender = nullptr;
    phase = "PHASE3";
    while (mouseCtl->getEvent()) {
        NewGumpEvent event;
        translate(mouseCtl->lastRecord(), &event);
        event.sender = nullptr;
        post(&event);
        posted = true;
    }
    phase = "PHASE4";
    if ((static_cast<unsigned int>(gumpKeys.isEmpty()) == 0U) || (lastArrow.scan != 0U)) {
        if (lastArrow.scan != 0U) {
            key = lastArrow;
            lastArrow.scan = 0;
        } else {
            key = gumpKeys.pop();
        }
        /* The world's gump, alone on the screen, gets the code of the
         * action the key is bound to (bindings.h), the selection's while it
         * selects items; a key that was an action's and is bound to none it
         * does not get. */
        i16 bound = keyCode(key);
        if ((key.pressed != 0U) && modalTop == 0 &&
            !((selectingItems != 0U) ? selectionCode(bound, &bound) : worldCode(bound, &bound))) {
            key.pressed = 0;
        }
        code = bound;
        if (key.pressed != 0U) {
            NewGumpEvent event;
            if (recordMode == RECORD_PLAYBACK) {
                if (code == ASCII_ESCAPE && (capturedGump == nullptr)) {
                    recordMode = RECORD_OFF;
                }
            } else {
                event.argument = code;
                event.input = static_cast<s32>(key.shift) << 16;
                post(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, code)));
                posted = true;
            }
        }
    } else {
        /* In a key's place, the code of an action the other devices gave
         * (devices.h), to the world's gump alone on the screen. */
        i16 device;
        if (deviceCode(&device) && modalTop == 0 && recordMode != RECORD_PLAYBACK) {
            post(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, device)));
            posted = true;
        }
    }
    if (!posted) {
        NewGumpEvent event;
        translate(mouseCtl->lastRecord(), &event);
        event.kind = EVENT_EXECUTE;
        event.sender = nullptr;
        post(&event);
    }
}

void GumpDispatcher::post(NewGumpEvent *event)
{
    events[eventTail] = *event;
    if (static_cast<i16>(eventTail + 1) % QUEUED_EVENTS == eventHead) {
        /* the queue is full: not reported */
    }
    eventTail++;
    if (eventTail == QUEUED_EVENTS) {
        eventTail = 0;
    }
}

void GumpDispatcher::translate(const MouseRecord &record, NewGumpEvent *event)
{
    event->x = record.x;
    event->y = record.y;
    switch (record.action) {
    case MOUSE_MOVED:
        event->kind = EVENT_MOTION;
        event->input = record.state;
        return;
    }
    event->input = record.state;
    event->input |= static_cast<u32>(record.shift) << 16;
    switch (record.action & 0x7f) {
    case MOUSE_LEFT_DOWN:
    case MOUSE_LEFT_UP:
        event->kind = EVENT_LEFT_BUTTON;
        return;
    case MOUSE_RIGHT_DOWN:
    case MOUSE_RIGHT_UP:
        event->kind = EVENT_RIGHT_BUTTON;
        return;
    case MOUSE_BOTH_DOWN:
    case MOUSE_BOTH_UP:
        event->kind = EVENT_BOTH_BUTTONS;
        return;
    default:
        event->kind = EVENT_EXECUTE;
        return;
    }
}

void GumpDispatcher::dispatch()
{
    NewGumpEvent alternate;

    if (modals[modalTop] == nullptr) {
        return;
    }
    phase = "PHASE1";
    while (nextAlternate(&alternate)) {
        post(&alternate);
    }
    dispatchFrame++;
    modals[modalTop]->validate();
    phase = "PHASE2";
    dispatchEvents(0);
}

void GumpDispatcher::dispatchEvents(u8 afterEvent)
{
    NewGump *target;

    for (;;) {
        if (afterEvent == 0U) {
            poll();
            phase = "PHASE2";
        }
        /* After a wait, the end of the event whose gump opened it first. */
        while ((afterEvent != 0U) || nextEvent()) {
            if (afterEvent == 0U) {
                if (recordMode == RECORD_ON) {
                    record(currentEvent);
                }
                if ((capturedGump != nullptr) && currentEvent->kind != EVENT_COMMAND) {
                    if ((capturedGump->eventMask & currentEvent->kind) != 0U) {
                        target = capturedGump;
                        if (target->parent != nullptr) {
                            currentEvent->x -= target->parent->getAbsoluteX();
                            currentEvent->y -= target->parent->getAbsoluteY();
                        }
                    } else {
                        dispatchFrame++;
                        continue;
                    }
                } else if (currentEvent->target != nullptr) {
                    target = currentEvent->target;
                } else if (modals[modalTop]->findEventTarget(currentEvent, &target, 0) == 0U) {
                    continue;
                }
                target->validate();
                target->dispatchEvent(currentEvent);
                if (waitOpened()) {
                    giveRest([this]() {
                        dispatchEvents(1);
                        return false;
                    });
                    return;
                }
            }
            afterEvent = 0;
            if (capturedGump != nullptr) {
                camera->flush();
                dispatchFrame++;
            }
        }
        /* A gump that holds the input (a slot's description being typed)
         * keeps the loop until it lets go: the original's keys come in
         * its interrupts meanwhile, the port's as its time goes on. The
         * wait for them is a wait of the steps, the loop's next round its
         * rest. */
        if (capturedGump == nullptr) {
            return;
        }
        if (!waitsBlocked()) {
            openClockWait([this]() {
                dispatchEvents(0);
                return false;
            });
            return;
        }
        clockWait();
    }
}

bool GumpDispatcher::nextEvent()
{
    if (eventHead == eventTail) {
        return false;
    }
    *currentEvent = events[eventHead];
    eventHead++;
    if (eventHead == QUEUED_EVENTS) {
        eventHead = 0;
    }
    return true;
}

/* Copies the alternate at eventHead, not alternateHead, as the original does. */
bool GumpDispatcher::nextAlternate(NewGumpEvent *event)
{
    if (alternateHead == alternateTail) {
        return false;
    }
    *event = alternates[eventHead];
    alternateHead++;
    if (alternateHead == QUEUED_EVENTS) {
        alternateHead = 0;
    }
    return true;
}

void GumpDispatcher::enqueue(NewGumpEvent *event)
{
    if ((event->sender == nullptr) || (event->target == nullptr)) {
        invalidEvent(event->sender, event);
    }
    post(event);
}

/* UNCALLED: nothing calls it. */
void GumpDispatcher::enqueueAlternate(NewGumpEvent *event)
{
    if ((event->sender == nullptr) || (event->target == nullptr)) {
        invalidEvent(event->sender, event);
    }
    alternates[alternateTail] = *event;
    if (static_cast<i16>(alternateTail + 1) % QUEUED_EVENTS == alternateHead) {
        /* the queue is full: not reported */
    }
    alternateTail++;
    if (alternateTail == QUEUED_EVENTS) {
        alternateTail = 0;
    }
}

void GumpDispatcher::invalidEvent(NewGump * /*unused*/, NewGumpEvent * /*unused*/)
{
    halt();
}

void GumpDispatcher::paint(Rect area)
{
    i16 i;

    paintDepth = 0;
    for (i = 0; i <= modalTop; i++) {
        if (modals[i] != nullptr) {
            modals[i]->paintTree(0, 0, area);
        }
    }
}

/* UNCALLED: nothing calls it. */
void GumpDispatcher::unused0e09() {}

void GumpDispatcher::capture(NewGump *gump)
{
    capturedGump = gump;
}

void GumpDispatcher::release()
{
    capturedGump = nullptr;
}

void GumpDispatcher::pushModal(NewGump *gump)
{
    if (modalTop < MODAL_DEPTH - 1) {
        modalTop++;
        modals[modalTop] = gump;
        return;
    }
    halt();
}

void GumpDispatcher::popModal()
{
    if (modalTop >= 0) {
        delete modals[modalTop];
        modals[modalTop] = nullptr;
        modalTop--;
        return;
    }
    halt();
}

NewGump *GumpDispatcher::find(NewGumpId *path, i16 depth)
{
    i16 i;
    i16 j;

    for (i = 0; i <= modalTop; i++) {
        if (modals[i]->id == path[0]) {
            NewGump *gump = modals[i];
            for (j = 1; j <= depth; j++) {
                gump = gump->children.findByIdParts(path[j].classId(), path[j].instance());
                if (gump == nullptr) {
                    return nullptr;
                }
            }
            return gump;
        }
    }
    return nullptr;
}

/* UNCALLED: nothing calls it. */
void GumpDispatcher::swapModal()
{
    NewGump *parked = parkedGump;

    if (modals[modalTop] != nullptr) {
        parkedGump = modals[modalTop];
        modalTop--;
    }
    if (parked != nullptr) {
        pushModal(parked);
        enqueue(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 1, parked, parked)));
    }
}

/* UNCALLED: nothing calls it. */
void GumpDispatcher::restoreModal()
{
    pushModal(parkedGump);
    parkedGump = nullptr;
}

/* UNCALLED: nothing calls it. */
bool GumpDispatcher::playback(NewGumpEvent *event)
{
    u32 frame;

    if (playbackStarted == 0) {
        playbackStarted++;
        recordFileSize = recordFile->size();
    }
    if (recorded >= playbackCount) {
        playbackCount = (recordFileSize - recordFile->tell()) / static_cast<ui16>(sizeof(RecordedEvent));
        if (playbackCount == 0) {
            recordMode = RECORD_OFF;
            debugPrintfWait(50, "End of script! (press any key)");
            return false;
        }
        if (playbackCount > RECORD_EVENTS) {
            playbackCount = RECORD_EVENTS;
        }
        recordFile->read(recordBuffer, playbackCount * static_cast<ui16>(sizeof(RecordedEvent)));
        recorded = 0;
    }
    frame = recordBuffer[recorded].frame;
    if (frame == dispatchFrame) {
        event->kind = recordBuffer[recorded].kind;
        event->x = recordBuffer[recorded].x;
        event->y = recordBuffer[recorded].y;
        event->argument = recordBuffer[recorded].argument;
        recorded++;
        return true;
    }
    if (frame < dispatchFrame) {
        halt();
    }
    return false;
}

void GumpDispatcher::record(NewGumpEvent *event)
{
    if (event->sender != nullptr) {
        return;
    }
    if (recorded >= RECORD_EVENTS) {
        recordFile->write(recordBuffer, recorded * static_cast<ui16>(sizeof(RecordedEvent)));
        recordFile->commit();
        recorded = 0;
    }
    recordBuffer[recorded].kind = event->kind;
    recordBuffer[recorded].x = event->x;
    recordBuffer[recorded].y = event->y;
    recordBuffer[recorded].argument = event->argument;
    recordBuffer[recorded].frame = dispatchFrame;
    if (event->kind == EVENT_EXECUTE) {
        recordBuffer[recorded].argument = 0;
    }
    recorded++;
}

/* UNCALLED: nothing calls it. */
bool GumpDispatcher::unused12d2()
{
    return false;
}

/* UNCALLED: nothing calls it. */
void GumpDispatcher::unused12d9() {}

void ModalGump::execute(NewGumpEvent *event)
{
    NewGump *child = firstChild();

    if ((child != nullptr) && ((child->eventMask & EVENT_EXECUTE) != 0U)) {
        child->execute(event);
    }
}

void ModalGump::handleCommand(NewGumpEvent *event)
{
    *result = event->argument;
}

inline ModalGump::ModalGump(s32 *resultAddress)
    : NewGump(NewGumpId(GUMP_MODAL, 0), nullptr, EVENT_EXECUTE | EVENT_COMMAND)
{
    this->result = resultAddress;
    bounds.set(0, 0, drawViewport->right(), drawViewport->bottom());
}

ModalState::ModalState(NewGump *gump, s32 *result)
{
    ModalGump *modal;

    if (gump == nullptr) {
        return;
    }
    captured = capturedGump;
    capturedGump = nullptr;
    event = currentEvent;
    currentEvent = new NewGumpEvent;
    modal = new ModalGump(result);
    guiDispatcher->pushModal(modal);
    gump->parent = modal;
    modal->addChild(gump);
    modal->paint(2);
}

ModalState::~ModalState()
{
    guiDispatcher->popModal();
    capturedGump = captured;
    delete currentEvent;
    currentEvent = event;
}

u8 ModalState::run()
{
    camera->run();
    guiDispatcher->dispatch();
    if (waitOpened()) {
        /* The pass's end comes after the wait (ModalWait::pass). */
        return 1;
    }
    return passEnd();
}

u8 ModalState::passEnd()
{
    NewGump *child = nullptr;
    const bool again = modals[modalTop]->children.forward(&child);
    /* The pass's time on the original's machine, and its interrupts: the
     * input and the timer come in them. */
    clockPass();
    return static_cast<u8>(again);
}

ModalWait::ModalWait(NewGump *gump, u32 mode) : mode_(mode)
{
    if (gump->parent != nullptr) {
        halt();
    }
    previous_ = getVideoMode();
    if (mode != static_cast<u32>(-1) && previous_ != mode) {
        currentSystem->setMode(mode);
        mouseCtl->setLimits(mode);
    }
    state_ = std::make_unique<ModalState>(gump, &result_);
}

void ModalWait::pass()
{
    const u8 again = state_->run();
    if (waitOpened()) {
        giveRest([this]() {
            if (state_->passEnd() == 0U) {
                setOver();
            }
            return true;
        });
        return;
    }
    if (again == 0U) {
        setOver();
    }
}

bool ModalWait::run()
{
    state_.reset();
    if (mode_ != static_cast<u32>(-1) && previous_ != mode_) {
        currentSystem->setMode(previous_);
        mouseCtl->setLimits(previous_);
    }
    modals[modalTop]->paint(2);
    setRestResult(result_);
    return false;
}

s32 runModalGump(NewGump *gump, u32 mode)
{
    if (gump == nullptr) {
        return 0;
    }
    ModalWait wait(gump, mode);
    runWaitHere(wait);
    return wait.modalResult();
}

void GumpDispatcher::remove(NewGump *gump)
{
    i16 i = eventHead;

    while (i != eventTail) {
        if (events[eventHead].target == gump) {
            events[eventHead].target = nullptr;
        }
        i++;
        if (eventHead == QUEUED_EVENTS) {
            eventHead = 0;
        }
    }
}
