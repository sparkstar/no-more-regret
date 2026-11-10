/* Code58 (file name unknown, after SPMGR.C): the status display at the bottom
 * of the screen, five parts showing the controlled NPC's weapon, its
 * ammunition, the active item, the health and the energy. While it is shown the
 * mouse stays above it. */
#ifndef PORT_STATUS_H
#define PORT_STATUS_H
#include "common.h"
#include "rect.h"
#include "gumpshap.h"

/* The parts of the status display. */
constexpr int STATUS_WEAPON = 0;
constexpr int STATUS_AMMO = 1;
constexpr int STATUS_ITEM = 2;
constexpr int STATUS_HEALTH = 3;
constexpr int STATUS_ENERGY = 4;
constexpr int STATUS_PARTS = 5;

struct StatusPart {
    u8 changed; /* drawn again in the next frame */
    Rect area;  /* on the screen */
    GumpShape *background;
};
ASSERT_SIZE(StatusPart, 0xd);

/* The display is on. */
extern u8 &statusShown;

void initStatus();
/* Deletes initStatus' backgrounds and viewports (the port's, for gameRelease). */
void closeStatus();
/* Draws the parts that need it, saving the screen under them first. */
void drawStatus();
/* Puts back the screen under the parts drawn. */
void eraseStatus();
void setStatusShown(u8 shown);
/* Shows the display and keeps the mouse above it; hides it. */
void showStatus();
void hideStatus();
/* Redraws part of the status display. */
void updateStatus(i16 part);

void hideStatusGump();
void showStatusGump();
#endif
