/* NEWGUMP.C (1360): gumps (NewGump), their ids, events and child lists, and
 * the gump system's globals. The virtual functions slot10, slot16 and slot17
 * are named by their places in the vtable: what they do is not known. */
#ifndef PORT_NEWGUMP_H
#define PORT_NEWGUMP_H
#include "common.h"
#include "gumplist.h"
#include "rect.h"

#include <memory>
/* Gump classes: the high byte of a NewGumpId. */
enum {
    GUMP_MODAL = 1, /* GumpDispatcher's ModalGump */
    GUMP_BUTTON = 2,
    GUMP_MENU_BAR = 3,
    GUMP_SCROLLBAR = 4,
    GUMP_LIST = 5,
    GUMP_EDITOR = 6,
    GUMP_COMBOBOX = 8,
    GUMP_WORLD = 0x20 /* WorldGump */
};
class NewGumpId {
public:
    u16 value, inverse;
    u8 zero;
    explicit NewGumpId(u16 n = 0)
    {
        value = n;
        inverse = ~value;
        zero = 0;
    }
    NewGumpId(u8 cls, u8 n)
    {
        value = (static_cast<u16>(cls) << 8) | n;
        inverse = ~value;
        zero = 0;
    }
    [[nodiscard]] u8 classId() const { return static_cast<u8>(value >> 8); }
    [[nodiscard]] u8 instance() const { return static_cast<u8>(value & 0xff); }
    /* inverse holds ~value while the object is alive. BCC's ~ keeps 16
     * bits, the port's int would set the high ones. */
    [[nodiscard]] bool isValid() const { return static_cast<ui16>(value ^ ~inverse) == 0U; }
    bool operator==(NewGumpId other) const { return value == other.value; }
};
class NewGump;
class NewGumpChildList : public GumpList {
public:
    NewGumpChildList();           /* 1360:00c7 */
    ~NewGumpChildList() override; /* 1360:0113 */
    /* GumpList::forward and backward for children. */
    bool forward(NewGump **child) { return GumpList::forward(reinterpret_cast<GumpLink **>(child)); }
    bool backward(NewGump **child) { return GumpList::backward(reinterpret_cast<GumpLink **>(child)); }
    void appendChild(NewGump *child);  /* 1360:0161 */
    void raiseChild(NewGump *child);   /* 1360:017a */
    bool destroyChild(NewGump *child); /* 1360:01c7 */
    /* UNCALLED: nothing calls it. */
    bool detachChild(NewGump *child); /* 1360:0218 */
    /* UNCALLED: nothing calls it. */
    NewGump *at(i16 target);                                  /* 1360:0269 */
    NewGump *findByInstanceId(u8 wanted);                     /* 1360:02a5 */
    NewGump *findNextByClassId(u8 wanted, NewGump **current); /* 1360:02e4 */
    NewGump *findByIdParts(u8 classId, u8 instance);          /* 1360:031f */
};
/* Event kinds. The same bits in NewGump::eventMask make a gump a target for
 * that kind; EVENT_LOSE_FOCUS is a mask bit only, and 0x400 is not
 * understood yet. */
enum {
    EVENT_EXECUTE = 0x01,
    EVENT_LEFT_BUTTON = 0x02,
    EVENT_RIGHT_BUTTON = 0x04,
    EVENT_MOTION = 0x08,
    EVENT_COMMAND = 0x10,      /* a notification to the parent; argument says which */
    EVENT_KEY = 0x20,          /* argument is the key code */
    EVENT_BROADCAST = 0x40,    /* goes to every child */
    EVENT_LOSE_FOCUS = 0x80,   /* the focused gump hears of the click that takes the focus (focusLost) */
    EVENT_BOTH_BUTTONS = 0x100 /* both mouse buttons at once (MOUSE_BOTH_DOWN, _UP) */
};
/* The key code of an EVENT_KEY (GumpDispatcher): the key's ASCII code or,
 * for a key without one or with Alt or Ctrl, its scan code (keyboard.h) with
 * one of these. */
constexpr int KEYCODE_SCAN = 0x100; /* no ASCII code */
constexpr int KEYCODE_ALT = 0x200;
constexpr int KEYCODE_CONTROL = 0x400;
constexpr int KEYCODE_SHIFT = 0x800; /* shift and a key without an ASCII code */
class NewGumpEvent {
public:
    s16 kind;
    s16 x, y;
    s16 argument;
    u32 extra;
    /* Mouse button transition (1 press, 2 repeat, 3 release) in the low word,
     * keyboard shift state in the high word. */
    s32 input;
    NewGump *target;
    NewGump *sender;
    explicit NewGumpEvent(s16 kind = EVENT_EXECUTE, s16 x = 0, s16 y = 0, s32 argument = 0, NewGump *from = nullptr,
                          NewGump *to = nullptr, u32 value = 0); /* 1360:0000 */
    [[nodiscard]] bool isMousePress() const;                     /* 1360:006f */
    [[nodiscard]] bool isMouseRepeat() const;                    /* 1360:0085 */
    [[nodiscard]] bool isMouseRelease() const;                   /* 1360:009b */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] i16 isInitialPress() const; /* 1360:00b1 */
};
class NewGump : public GumpLink {
public:
    NewGumpId id;
    NewGumpChildList children;
    NewGump *parent;
    u32 eventMask; /* EVENT_* kinds the gump takes */
    Rect bounds;
    /* A stack of two-bit states, current one lowest: bit 0 hides the gump,
     * bit 1 its children. */
    s32 paintState;
    u8 active, enabled2e, clipChildren;
    /* The key codes addAcceptedArguments collected, 0-terminated; -1 accepts
     * any. Its own. */
    std::unique_ptr<s16[]> accepted;
    void *operator new(size_t size);                 /* 1360:050c */
    void operator delete(void *pointer);             /* 1360:0562 */
    NewGump(NewGumpId id, NewGump *owner, u32 mask); /* 1360:036a */
    [[nodiscard]] u16 width() const { return bounds.right - bounds.left + 1; }
    [[nodiscard]] u16 height() const { return bounds.bottom - bounds.top + 1; }
    NewGump *firstChild() { return static_cast<NewGump *>(children.first); }
    void addChild(NewGump *child) { children.appendChild(child); }
    void removeChild(NewGump *child) { children.destroyChild(child); }
    [[nodiscard]] bool isHidden() const;       /* 1360:0580 */
    void hide();                               /* 1360:05a6 */
    void paintTree(s16 dx, s16 dy, Rect clip); /* 1360:05d4 */
    /* Repaints the gump and its children; notify also flushes the renderer
     * (1360:078d). */
    void paint(u16 notify);
    /* UNCALLED: nothing calls it. */
    NewGump *findDeepestAtPoint(NewGumpEvent event);                        /* 1360:084d */
    u8 findEventTarget(NewGumpEvent *event, NewGump **output, u16 options); /* 1360:0955 */
    bool eventPointInBounds(NewGumpEvent *event) const;                     /* 1360:0b43 */
    void pushPaintState(i16 bits);                                          /* 1360:0b65 */
    /* UNCALLED: nothing calls it. */
    bool reparent(NewGump *child, NewGump *oldParent);    /* 1360:0bb2 */
    [[nodiscard]] s16 getAbsoluteX() const;               /* 1360:0c00 */
    [[nodiscard]] s16 getAbsoluteY() const;               /* 1360:0c35 */
    s16 eventX(NewGumpEvent *event) const;                /* 1360:0c6a */
    s16 eventY(NewGumpEvent *event) const;                /* 1360:0c7d */
    void addAcceptedArguments(int first, ...);            /* 1360:0f2a */
    bool acceptsEventArgument(NewGumpEvent *event) const; /* 1360:107b */
    void invalidate();                                    /* 1360:10d8 */
    void invalidateRect(Rect value);                      /* 1360:118e */
    /* Sends all mouse events to object, or to this gump, until release() (1360:1253). */
    void capture(NewGump *other = nullptr);
    void release();                      /* 1360:1282 */
    void sendEvent(NewGumpEvent *event); /* 1360:1297 */
    /* UNCALLED: nothing calls it. */
    void sendAlternateEvent(NewGumpEvent *event); /* 1360:12b2 */
    /* UNCALLED: nothing calls it. */
    void invalidEvent(NewGump *gump, NewGumpEvent *event); /* 1360:12cd */
    /* Halts at the caller's place unless id is intact (1360:135d). */
    void validate(std::source_location place = CALL_PLACE) const;
    /* UNCALLED: nothing calls it. */
    bool hasChildMatchingQuery(void *query); /* 1360:1395 */
    /* Trivial handlers are inline. Each module whose vtable refers to one
     * emits a copy and the linker keeps the first: WORLD.C (Code2) holds slots
     * 3, 7-11, 14, 16 and 17, Code96 slots 1, 5 and 6, COMBOBOX.C slots 2 and 4. */
    ~NewGump() override; /* 1360:046e */
    virtual void execute(NewGumpEvent * /*unused*/) {}
    virtual void handleLeftButton(NewGumpEvent * /*unused*/) {}
    virtual void handleRightButton(NewGumpEvent * /*unused*/) {}
    virtual void handleMouseMotion(NewGumpEvent * /*unused*/) {}
    virtual void handleCommand(NewGumpEvent * /*unused*/) {}
    virtual void handleKeyboard(NewGumpEvent * /*unused*/) {}
    virtual void handleBroadcast(NewGumpEvent * /*unused*/) {}
    virtual void focusLost(NewGumpEvent * /*unused*/) {}
    virtual void handleBothButtons(NewGumpEvent * /*unused*/) {}
    virtual void slot10() {}
    virtual bool matchesQuery(void * /*unused*/) { return false; }
    virtual void paintSelf(i16 x, i16 y);            /* 1360:0c90 */
    virtual void dispatchEvent(NewGumpEvent *event); /* 1360:0cb2 */
    virtual bool acceptsEvent(NewGumpEvent *event) { return eventPointInBounds(event); }
    virtual void close(); /* 1360:12ee */
    virtual void slot16() {}
    virtual void slot17() {}
};
ASSERT_SIZE(NewGumpId, 0x5);
ASSERT_SIZE(NewGumpChildList, 0xa);
ASSERT_SIZE(NewGumpEvent, 0x18);
ASSERT_SIZE(NewGump, 0x34);
/* The nesting of NewGump::paint; the dispatcher clears it. */
extern u8 &paintDepth;
extern NewGump &noEventTarget;
#endif
