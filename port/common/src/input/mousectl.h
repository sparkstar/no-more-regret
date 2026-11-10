/* MOUSECTL.C: the mouse controller. A system process that
 * turns the mouse driver's events into MouseRecords for the gumps, holds a
 * press back for a few timer ticks to see whether the other button follows,
 * and draws the cursor over the screen. The cursor shapes come from a file of
 * their own; derived controllers pick the frame for a position. */
#ifndef PORT_MOUSECTL_H
#define PORT_MOUSECTL_H
#include "kprocess.h"
#include "queue.h"
#include "rect.h"
#include "shape.h"

class File;

/* MouseRecord::state */
constexpr int RECORD_EMPTY = -1;
constexpr int RECORD_PRESSED = 1;
constexpr int RECORD_CHORD = 2; /* a press while the other button is held back */
constexpr int RECORD_RELEASED = 3;
constexpr int RECORD_MOVED = 4;
/* Timer ticks a press is held back. */
constexpr int PRESS_DELAY = 13;
/* The cursors the code sets, frames of the mouse's shape (static\mouse.shp;
 * the load and save gump's is each game's, LOAD_SAVE_POINTER): the E shown
 * while the game starts, the arrows of the game's screen and its dialogs,
 * of the menus, of the difficulty dialog (in its own palette) and of the
 * help. */
constexpr int STARTUP_POINTER = 0x23;
constexpr int GAME_POINTER = 0x24;
constexpr int MENU_POINTER = 0x25;
constexpr int DIFFICULTY_POINTER = 0x26;
constexpr int HELP_POINTER = 0x27;

/* A mouse event of the GUI. action is the driver's event bits; 2Ah is both
 * buttons down and 54h both up. */
struct MouseRecord {
    s16 state;
    u32 ticks;
    u16 action;
    u16 buttons;
    u16 shift;
    u16 x;
    s16 y;
};
ASSERT_SIZE(MouseRecord, 0x10);
using MouseQueue = Queue<MouseRecord, 10>;
ASSERT_SIZE(MouseQueue, 0xa6);

class MouseCtl : public PeriodicProcess {
public:
    explicit MouseCtl(Loading loading) : PeriodicProcess(loading) {}
    Queue<MouseRecord, 10> events;
    MouseRecord current; /* the event getEvent took */
    MouseRecord chord;
    MouseRecord unused106;
    MouseRecord leftDown, leftUp; /* held back */
    MouseRecord rightDown, rightUp;
    MouseRecord moved;
    Rect bounds;          /* the largest cursor */
    s16 cursorX, cursorY; /* where the cursor frame is drawn */
    s16 pointerX, pointerY;
    s16 width, height; /* of the frame, less one */
    s16 hotX, hotY;
    u8 visible;
    u8 drawn; /* drawn into the draw viewport */
    u8 unknown180[4];
    u16 buttons;
    s16 shapes[8]; /* a stack of cursor shapes */
    s16 depth;

    explicit MouseCtl(char *path);
    void onKill() override; /* the original's destructor */
    void run() override;
    void load(File *file) override;
    void event(u32 kind, EventData data) override;
    /* The frame of the current shape to show at (x,y). */
    virtual i16 frameAt(s16 /*unused*/, s16 /*unused*/) { return 0; }
    /* UNCALLED: nothing calls it. */
    void close();
    /* The mouse ranges for a video mode; every mode gets 640x480. */
    void setLimits(u32 mode);
    void hide();
    void show();
    void setShape(i16 shape);
    void updateFrame();
    void pushShape(i16 shape);
    void resetShape(i16 shape);
    i16 popShape();
    /* UNCALLED: nothing calls it. */
    void loadShapes(char *path);
    /* Takes the next event into current; 0 when there is none. */
    bool getEvent();
    [[nodiscard]] MouseRecord lastRecord() const { return current; }
    /* Draws the cursor into the draw viewport, keeping what was under it, and
     * puts that back. */
    void drawCursor();
    void eraseCursor();
    i16 shapeCount();
    void moveTo(s16 x, s16 y);
};
ASSERT_SIZE(MouseCtl, 0x198);

/* Turn the controller's mouse events off and on. */
void disableMouseEvents();
/* Deletes the cursor's shapes and viewports, which MouseCtl's process leaves when the kernel
 * frees it (the port's, for gameRelease). */
void closeCursor();
/* No Remorse's PLAYER.C reads it far. */
extern MouseCtl *&mouseCtl;
#endif
