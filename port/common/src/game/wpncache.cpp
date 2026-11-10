/* WPNCACHE.C (Code48, No Regret's 1178:0000-03b9, No Remorse's
 * 1158:03ff-07af; No Regret's addresses): the weapon overlays, the frames
 * drawn over the Silencer's animations for the weapon in his hands, read
 * from wpnovlay.dat for the weapon's overlay type. The weapons' types and
 * the archive's kind are the game's (wpncache_game.h, wpncache_game.cpp). */
#include "wpncache.h"
#include "gamestate.h"
#include "animcach.h"
#include "gamefile.h"
#include "filespec.h"
#include "npc.h"
#include "fastflex.h"

void freeOverlays();
u16 overlayTypeOf(u16 shape);

/* wpnovlay.dat, then the names of the overlay types (1480:33d2). */
const char *(&overlayNames)[16] = gameGlobal<const char *[16]>({"wpnovlay.dat", "Small Pist", "Large Pist",
                                                                "Small Rifl", "Large Rifl", "Shotgun   ", "", "", "",
                                                                "", "", "", "", "", "", ""});
/* The weapon whose overlays are loaded, and its overlay type (1480:3412,
 * 3414). */
u16 &loadedWeapon = gameGlobal<u16>(0);
u16 &overlayType = gameGlobal<u16>(0);
/* The overlay shape of each type (1480:3416); each weapon's type (3434) is
 * the game's (wpncache_game.cpp). */
u16 (&overlayShapes)[OVERLAY_WEAPONS] = gameGlobal<u16[OVERLAY_WEAPONS]>({0x1a1, 0x36d, 0x36f, 0x36e, 0x33b});
/* The overlays of the loaded weapon by animation (1480:8a3c), read from
 * overlayFlex (8e3c). */
OverlayFrame *(&overlays)[0x100] = gameGlobal<OverlayFrame *[0x100]>();
/* wpnovlay.dat's archive (8e3c in No Regret, 82d2 in No Remorse). No
 * Remorse's original reads it with a Flex, the index from the file each
 * time; the port keeps the index in memory, as No Regret's FastFlex does,
 * and reads the same bytes. */
FastFlex &overlayFlex = gameGlobal<FastFlex>();

namespace {
/* Loads the overlays of weapon for its type (1178:0000) and frees them
 * (0132). */
void loadOverlays(u16 weapon)
{
    i16 anim;
    AnimInfo info;
    loadedWeapon = weapon;
    freeOverlays();
    overlayFlex.open(makePath(nullptr, staticDirectory, overlayNames[0], nullptr), FILE_READ, -1);
    overlayType = overlayTypeOf(weapon);
    for (anim = 0; anim < ANIM_ACTIONS; anim++) {
        info = animInfo(1, anim);
        i16 frames;
        if ((frames = info.frames) != 0 && info.attack) {
            i16 size;
            i16 directions;
            if (info.sixteenDirections) {
                directions = 16;
            } else {
                directions = 8;
            }
            size = frames * directions * sizeof(OverlayFrame);
            overlays[anim] = reinterpret_cast<OverlayFrame *>(new char[size]);
            if (overlays[anim] == nullptr) {
                halt();
            }
            overlayFlex.readObject(anim, overlays[anim], size, static_cast<ui16>(size * overlayType));
        }
    }
    overlayFlex.close();
}
}

void freeOverlays()
{
    for (i16 i = 0; i < 0x100; i++) {
        if (overlays[i] != nullptr) {
            delete[] reinterpret_cast<char *>(overlays[i]);
            overlays[i] = nullptr;
        }
    }
}

namespace {
/* The index of shape among the overlay weapons; halts when it is not one
 * (1178:0175). Returns nothing when halt comes back. */
i16 overlayIndex(u16 shape)
{
    for (i16 i = 0; i < OVERLAY_WEAPONS; i++) {
        if (overlayWeapons[i].shape == shape) {
            return i;
        }
    }
    haltMessage(haltFormat("%d.\n", shape));
}
}

/* The overlay type of the weapon of shape (1178:01ba). */
u16 overlayTypeOf(u16 shape)
{
    return overlayWeapons[overlayIndex(shape)].type;
}

namespace {
/* Loads the overlays of weapon unless they are (1178:01d7). */
/* UNCALLED: nothing calls it. */
void loadWeapon(u16 weapon)
{
    if (loadedWeapon != weapon) {
        loadOverlays(weapon);
    }
}
}

/* Loads the overlays of the first weapon of type; nothing calls it
 * (1178:01ee). */
/* UNCALLED: nothing calls it. */
void loadType(u16 type)
{
    if (overlayType != type) {
        for (i16 i = 0; i < OVERLAY_WEAPONS; i++) {
            if (static_cast<ui16>(overlayWeapons[i].type) == type) {
                loadWeapon(overlayWeapons[i].shape);
                return;
            }
        }
        halt();
    }
}

void weaponOverlay(u16 weapon, u16 animation, char direction, u8 frame, u16 *shape, char *dx, char *dy,
                   u16 *overlayFrame)
{
    AnimInfo info;
    OverlayFrame entry;
    if (loadedWeapon != weapon) {
        loadOverlays(weapon);
    }
    if (overlays[animation] == nullptr) {
        *shape = 0;
        *overlayFrame = 0;
        return;
    }
    if (controlledNpc.shape() != 1) {
        *shape = 0;
        *overlayFrame = 0;
        return;
    }
    info = animInfo(1, animation);
    if (!info.sixteenDirections) {
        direction >>= 1;
    }
    entry = overlays[animation][static_cast<ui16>((direction * info.frames) + frame)];
    *shape = overlayShapes[overlayType];
    *dx = entry.dx;
    *dy = entry.dy;
    *overlayFrame = entry.frame;
}

bool hasWeaponOverlay(u16 weapon, u16 animation)
{
    if (loadedWeapon != weapon) {
        loadOverlays(weapon);
    }
    return overlays[animation] != nullptr;
}

/* Frees the overlays and forgets the weapon (1178:0369). */
void closeOverlays()
{
    freeOverlays();
    loadedWeapon = 0;
}

u16 overlayShapeOf(u16 shape)
{
    const i16 type = overlayTypeOf(shape);
    return overlayShapes[type];
}
