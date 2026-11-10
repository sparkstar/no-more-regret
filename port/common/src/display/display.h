/* DISPLAY.C (Code53): the pickup display. The items the player picks up are
 * listed at the top left of the screen, eight at most, each with its gump
 * shape, name and count, until four seconds pass without another of the
 * kind. */
#ifndef PORT_DISPLAY_H
#define PORT_DISPLAY_H
#include "common.h"
#include "rect.h"
#include "kprocess.h"
#include "gumpshap.h"

constexpr int PICKUPS = 8;

/* A line of the display. The gump shape and frame tell the kinds apart. */
struct Pickup {
    const char *name; /* 0 for a free line */
    GumpShape *gump;
    i16 count;
    u16 shape;
    u16 frame;
    i16 timeLeft; /* in seconds, 4 after each pickup */
    u8 redraw;
    Rect area; /* on the screen */
};
ASSERT_SIZE(Pickup, 0x19);

/* Ages the lines once a second (60 ticks of the system timer). */
class PickupProcess : public Process {
public:
    /* UNCALLED: nothing calls it. */
    explicit PickupProcess(Loading loading) : Process(loading) {}
    u32 lastTime;

    PickupProcess();
    void run() override;
};
ASSERT_SIZE(PickupProcess, 0x36);

/* Set while the world is frozen (WORLD.C): the lines are not drawn. */
extern u8 &pickupsFrozen;

/* Empties the lines and starts the process. */
void initPickups();
void clearPickups();
/* Deletes initPickups' viewports and the pickups' shapes (the port's, for gameRelease). */
void closePickups();
/* Draws the lines that need it, saving the screen under them first. */
void drawPickups();
/* Puts back the screen under the lines drawn. */
void erasePickups();
/* Adds item to the display, or counts it on the line of its kind. */
void addPickup(u16 id);
#endif
