/* WPNCACHE.C (Code48, 1178:0000-03b9): the weapon overlays, the frames drawn
 * over the controlled NPC's animation for the weapon in its hands, read from
 * wpnovlay.dat. */
#ifndef PORT_WPNCACHE_H
#define PORT_WPNCACHE_H
#include "common.h"

/* 1 when the overlays of weapon have a table for the animation; loads the
 * weapon's overlays first (1178:033b). */
bool hasWeaponOverlay(u16 weapon, u16 animation);
/* The overlay shape and frame of weapon for the animation, direction and
 * frame, and where it goes from the NPC's point; shape and frame are 0 when
 * there is none (1178:0246). */
void weaponOverlay(u16 weapon, u16 animation, char direction, u8 frame, u16 *shape, char *dx, char *dy,
                   u16 *overlayFrame);
constexpr int OVERLAY_WEAPONS = 0xf;

/* A frame of a weapon overlay: where it goes from the NPC and its frame. */
struct OverlayFrame {
    char dx;
    char dy;
    u16 frame;
};
ASSERT_SIZE(OverlayFrame, 4);

/* A weapon and its overlay type. */
struct OverlayWeapon {
    u16 shape;
    u8 type;
};
ASSERT_SIZE(OverlayWeapon, 3);

/* The overlay shape of the weapon of shape (1178:0374). */
u16 overlayShapeOf(u16 shape);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void loadType(u16 type);
void closeOverlays();
/* Each weapon's overlay type (1480:3434), the game's (wpncache_game.h). */
#include "wpncache_game.h"
#endif
