/* IRGUMP.C (13b8:0000-09ad, No Remorse only): gumps placed by an item, and
 * the process that closes one when its time is up. */
#ifndef PORT_IRGUMP_H
#define PORT_IRGUMP_H
#include "dialog.h"
#include "itemdata.h"
#include "systimer.h"

/* The gump class of the item relative gumps (NewGumpId). */
constexpr int GUMP_ITEM_RELATIVE = 0x43;
/* ItemRelativeGump::mode: what the gump follows. 0 and 1 an item, 2 the
 * controlled NPC, 3 the camera's point. */
constexpr int IRGUMP_ITEM = 0;
constexpr int IRGUMP_ITEM_MOVING = 1;
constexpr int IRGUMP_NPC = 2;
constexpr int IRGUMP_CAMERA = 3;

/* Waits seconds, then closes the gump it was given, which it finds again by
 * its path of ids from the modal gump (vtable 1478:5a69). 74h bytes. */
class GumpNotifyProcess : public WaitProcess {
public:
    NewGumpId path[10];
    i16 depth;
    u8 unknown72;
    /* The gump is still to be closed. */
    u8 waiting;

    explicit GumpNotifyProcess(Loading loading) : WaitProcess(loading) {}
    /* UNCALLED: nothing calls it. */
    GumpNotifyProcess(u16 seconds, NewGump *gump, u16 item, i16 type, u8 flag); /* 13b8:0000 */
    void onKill() override;  /* the original's destructor, 13b8:012e */
    void restart() override; /* 13b8:021a */
    /* Closes the gump when it is there (13b8:01ce). */
    virtual void closeGump();
};
ASSERT_SIZE(GumpNotifyProcess, 0x74);

/* A dialog kept at dx,dy from where its item (or the controlled NPC, or the
 * camera's point) is on the screen, with the first free instance of class
 * 43h among its parent's children (vtable 1478:5a1d). 5Ah bytes. */
class ItemRelativeGump : public DialogGump {
public:
    i16 dx, dy;
    /* Where the item was last seen. */
    ItemPoint point;
    i16 mode; /* IRGUMP_* */
    /* The pid of the GumpNotifyProcess that closes it, 0 for none. */
    u16 notifier;
    i16 ticks; /* seconds*4; nothing reads it */
    /* 2 for the gumps raiseItemGumps puts on top. */
    u16 layer;
    u16 item;

    /* UNCALLED: nothing calls it. */
    ItemRelativeGump(u16 layer, NewGump *parent, Rect area, u16 item, i16 mode, i16 dx, i16 dy,
                     u16 seconds); /* 13b8:0233 */
    ~ItemRelativeGump() override;  /* 13b8:04f7 */
    /* Nothing (13b8:096f, 0974). */
    void handleBothButtons(NewGumpEvent *event) override;
    void handleRightButton(NewGumpEvent *event) override;
    /* Ends the notifier too (13b8:0580). */
    void close() override;
    /* Follows the item again, or hides the gump when not show (13b8:07c7). */
    virtual void update(i16 show);
    /* Empty; nothing calls them (13b8:0979, 097e). */
    /* UNCALLED: nothing calls it. */
    void unknown0979();
    /* UNCALLED: nothing calls it. */
    void unknown097e();
};
ASSERT_SIZE(ItemRelativeGump, 0x5a);

/* Nothing calls these: updates the gumps of item (all with 0), raises the
 * layer 2 gumps, closes the gumps of item (13b8:05e8, 06b4, 0758). */
/* UNCALLED: nothing calls it. */
void updateItemGumps(u16 item, i16 show);
/* UNCALLED: nothing calls it. */
void raiseItemGumps();
/* UNCALLED: nothing calls it. */
void closeItemGumps(u16 item);
#endif
