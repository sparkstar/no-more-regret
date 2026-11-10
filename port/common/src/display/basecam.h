/* 1300:0000-0daf, file name unknown: the screen's dirty rectangles and the
 * camera process that copies them from the draw viewport to the screen. The
 * gumps and the display modules add rectangles (addDirtyRect); the camera
 * flushes them. The process registers as "BaseCamera"; the camera module
 * (1198) derives from it. */
#ifndef PORT_BASECAM_H
#define PORT_BASECAM_H
#include "common.h"
#include "rect.h"
#include "gumplist.h"
#include "kprocess.h"

/* A rectangle on a RectList. */
class RectLink : public GumpLink {
public:
    Rect bounds;
    explicit RectLink(Rect area) : bounds(area) {}
};
ASSERT_SIZE(RectLink, 0x12);

/* A list of screen rectangles. merge() cuts overlapping rectangles apart so
 * that no pixel is copied twice. SPMGR.C (11b8) keeps lists of its own; the
 * vtable and the implicit destructor in use are its copies (1480:36a1). */
class RectList : public GumpList {
public:
    /* 1 when area overlaps a rectangle of the list. */
    /* UNCALLED: nothing calls it. */
    bool overlaps(Rect area);
    /* Appends area, clipped to the 640x480 screen. */
    void add(Rect area);
    /* Removes the last rectangle; an empty rectangle when there is none. */
    Rect pop();
    virtual void merge();
};
ASSERT_SIZE(RectList, 0xa);

/* 1 when the two share a pixel or one holds the other (as Rect::overlaps). */
bool rectsOverlap(Rect *a, Rect *b);

/* Copies the dirty rectangles to the screen, the mouse cursor drawn over
 * them. */
void flushScreen();
/* Queues the part of area on the screen; nothing while dirtyCount < 0. */
void addDirtyRect(Rect area);
/* Drops the last dirty rectangle. The callers ignore the result: the screen
 * bounds while dirtyCount < 0, else an empty rectangle. */
Rect getClipRect();
void clearDirtyRects();

class BaseCamera : public Process {
public:
    explicit BaseCamera(Loading loading) : Process(loading) {}
    BaseCamera();
    void onKill() override; /* the original's destructor */
    /* Flushes the screen when a rectangle was added; the GUI calls it
     * directly. */
    void run() override;
    virtual void flush() { flushScreen(); }
    virtual void invalidate(Rect area, u8 /*unused*/) { addDirtyRect(area); }
};
ASSERT_SIZE(BaseCamera, 0x32);

/* The camera process; there is one at a time. */
extern BaseCamera *&camera;
extern RectList &dirtyRects;

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void initCamera();
/* UNCALLED: nothing calls it. */
void closeCamera();
/* UNCALLED: nothing calls it. */
void testDirtyRects();
#endif
