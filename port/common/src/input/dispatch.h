/* DISPATCH.C: the GUI event dispatcher. GumpDispatcher turns
 * the mouse controller's records and the keys into NewGumpEvents, queues them
 * and hands each to the gump it is for; a stack of modal gumps says which gumps
 * take events. It can record the events to a file and play them back. Its
 * state is global; the members never read the object. The Dispatcher process
 * runs it and takes the keys. */
#ifndef PORT_DISPATCH_H
#define PORT_DISPATCH_H
#include "newgump.h"
#include "keyboard.h"
#include "queue.h"
#include "kprocess.h"
#include "waits.h"

#include <memory>

struct MouseRecord;

constexpr int QUEUED_EVENTS = 25;
constexpr int MODAL_DEPTH = 5;
/* GumpDispatcher::recordMode */
constexpr int RECORD_OFF = 0;
constexpr int RECORD_ON = 1;
constexpr int RECORD_PLAYBACK = 2;
/* Events of the record buffer, which the file takes in one write. */
constexpr int RECORD_EVENTS = 3275;

/* An event of the record file. */
struct __attribute__((packed)) RecordedEvent {
    s16 kind;
    u16 x, y;
    s32 argument;
    u16 unused0a;
    u32 frame; /* the dispatcher's frame when it came */
    u8 unused10[4];
};
ASSERT_FILE_SIZE(RecordedEvent, 0x14);

/* The root gump of a modal gump: it takes the command that closes it. */
class ModalGump : public NewGump {
public:
    s32 *result;

    explicit ModalGump(s32 *resultAddress);
    void execute(NewGumpEvent *event) override;
    void handleCommand(NewGumpEvent *event) override;
};
ASSERT_SIZE(ModalGump, 0x38);

/* What runModalGump saves while the modal gump runs. */
class ModalState {
public:
    NewGump *captured;
    NewGumpEvent *event;
    u8 unused08[4];

    ModalState(NewGump *gump, s32 *result);
    ~ModalState();
    /* A pass of the original's loop, which dispatched until the modal gump
     * had no children: the camera, the dispatch and the pass's time; whether
     * the gump has children still. */
    u8 run();
    /* run's after the dispatch: the children and the pass's time (the
     * port's: after a wait opened in the dispatch, its rest). */
    u8 passEnd();
};
ASSERT_SIZE(ModalState, 0xc);

/* A modal gump as a wait (the port's): runModalGump's work around its
 * ModalState, whose run is a pass. */
class ModalWait final : public GameWait {
public:
    /* runModalGump's start: the gump's check and the video mode, then the
     * ModalState (a gump, not none). */
    ModalWait(NewGump *gump, u32 mode);
    void pass() override;
    /* runModalGump's end: the ModalState's, the video mode back and the top
     * gump painted; the gump's result is the rest's (restResult). */
    [[nodiscard]] bool run() override;
    [[nodiscard]] s32 modalResult() const { return result_; }

private:
    /* The port's owner of the state: the run's end (hostEnd) frees the
     * wait, and the state gives the dispatcher its event back then too. */
    std::unique_ptr<ModalState> state_;
    s32 result_ = 0;
    u32 mode_;
    u32 previous_ = 0;
};

class GumpDispatcher {
public:
    explicit GumpDispatcher(i16 mode);
    ~GumpDispatcher();
    void shutdown();
    /* Queues the mouse controller's records and a key. */
    void poll();
    void post(NewGumpEvent *event);
    void translate(const MouseRecord &record, NewGumpEvent *event);
    /* Hands the queued events to their gumps. */
    void dispatch();
    /* dispatch's loop of the events (the port's): from its poll, or, after a
     * wait an event's gump opened (afterEvent), from that event's end. */
    void dispatchEvents(u8 afterEvent);
    bool nextEvent();
    bool nextAlternate(NewGumpEvent *event);
    void enqueue(NewGumpEvent *event);
    /* UNCALLED: nothing calls it. */
    void enqueueAlternate(NewGumpEvent *event);
    /* Halts: dispatchEvent met an event kind it does not know. */
    void invalidEvent(NewGump *gump, NewGumpEvent *event);
    void paint(Rect area);
    /* UNCALLED: nothing calls it. */
    void unused0e09();
    /* Sends all mouse events to gump until release(). */
    void capture(NewGump *gump);
    void release();
    void pushModal(NewGump *gump);
    void popModal();
    /* The gump at the end of a path of ids, from a modal gump down. */
    NewGump *find(NewGumpId *path, i16 depth);
    /* Parks the top modal gump, or brings the parked one back. */
    /* UNCALLED: nothing calls it. */
    void swapModal();
    /* UNCALLED: nothing calls it. */
    void restoreModal();
    /* UNCALLED: nothing calls it. */
    bool playback(NewGumpEvent *event);
    void record(NewGumpEvent *event);
    /* UNCALLED: nothing calls it. */
    bool unused12d2();
    /* UNCALLED: nothing calls it. */
    void unused12d9();
    void remove(NewGump *gump);
};

class Dispatcher : public EventProcess {
public:
    explicit Dispatcher(Loading loading) : EventProcess(loading) {}
    Dispatcher();
    void onKill() override; /* the original's destructor */
    void run() override;
    void load(File *file) override;
    void event(u32 kind, EventData data) override;
};
ASSERT_SIZE(Dispatcher, 0x36);

void initDispatcher();
void closeDispatcher();
/* Deletes the dispatcher's event, which its shutdown leaves; the next dispatcher makes
 * it again (the port's, for gameRelease). */
void closeDispatcherEvent();
/* Runs a modal gump until it closes and returns its result. A mode other than
 * -1 is the video mode to run it in. The original's way, the loop where it
 * is (waits.h's runWaitHere), for the callers that do not go on after it
 * as a wait (openModalGump). */
s32 runModalGump(NewGump *gump, u32 mode);
/* runModalGump as a wait: opens the modal gump and returns; then, a lambda
 * on the gump's result, is the caller's code after the original's
 * runModalGump, which runs when the gump has closed. Inside a loop that
 * waits where it is (waitsBlocked), and for no gump, as runModalGump, then
 * at once. */
template <typename Then> void openModalGump(NewGump *gump, u32 mode, Then then)
{
    if (gump == nullptr || waitsBlocked()) {
        then(runModalGump(gump, mode));
        return;
    }
    auto rest = [then = std::move(then)]() mutable {
        then(restResult());
        return false;
    };
    openWait(std::make_unique<ModalWait>(gump, mode), std::make_unique<CodeRest<decltype(rest)>>(std::move(rest)));
}

extern GumpDispatcher *&guiDispatcher;
/* The modal gumps, MODAL_DEPTH of them; the world's gump is the first. */
extern NewGump **&modals;
/* The top modal gump's index in modals. */
extern i16 &modalTop;
extern Dispatcher *&dispatcher;
/* Keys waiting for the gumps. */
/* A key's code for the gumps (newgump.h's KEYCODE_*) as poll gives it: the
 * character, or the scan code with Alt, Ctrl or Shift (the last held of
 * them, Shift giving the character when there is one), or KEYCODE_SCAN. */
i16 keyCode(const KeyEvent &key);
using KeyQueue = Queue<KeyEvent, 10>;
ASSERT_SIZE(KeyQueue, 0x7e);
extern KeyQueue &gumpKeys;
#endif
