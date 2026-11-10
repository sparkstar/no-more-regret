/* ..\\ITEM\\ITEM.C (Code23, No Regret's 10b0, No Remorse's 10a0; the addresses
 * are No Regret's): an Item is an item number, and the item tables (itemdata.h)
 * are indexed by it. NPCs are items 1-255. Most members are also usecode
 * intrinsics: INTER.C passes the address of the item number on the usecode
 * stack as this. */
#ifndef PORT_ITEM_H
#define PORT_ITEM_H
#include <array>
#include <span>
#include <cstddef>
#include "common.h"
#include "kprocess.h"
#include "itemdata.h"
#include "type.h"

/* Item::flags. */
constexpr int ITEM_DRAWN = 0x1; /* in the display list */
/* Discarded when the map changes; created NPCs have it, and their usecode
 * class is their shape's (UPROCESS.C classOf). */
constexpr int ITEM_DISPOSABLE = 0x2;
constexpr int ITEM_OWNED = 0x4;
/* In a container: itemX is the container, itemY the place in its gump. */
constexpr int ITEM_CONTAINED = 0x8;
constexpr int ITEM_INVISIBLE = 0x10;
constexpr int ITEM_FLIPPED = 0x20;
/* Crusader also sets it on an item that explodes, so that it explodes only
 * once (explode); BoboBoomer clears it with ITEM_GUMP_OPEN. */
constexpr int ITEM_IN_NPC_LIST = 0x40;
/* Discarded when it leaves the fast area: the items that explosions,
 * sprites and globs create. */
constexpr int ITEM_FAST_ONLY = 0x80;
/* Crusader marks a broken item with it, so that it takes no more damage
 * (DamageTable::breakItem, explode). */
constexpr int ITEM_GUMP_OPEN = 0x100;
constexpr int ITEM_EQUIPPED = 0x200;
constexpr int ITEM_BOUNCING = 0x400;
constexpr int ITEM_ETHEREAL = 0x800;
constexpr int ITEM_HANGING = 0x1000;
constexpr int ITEM_FAST_AREA = 0x2000;
constexpr int ITEM_LOW_FRICTION = 0x4000;
/* Broken items cannot be targeted. */
constexpr int ITEM_BROKEN = 0x8000;

/* The z of the items in the ethereal void. */
constexpr int ETHEREAL_Z = 0xfe;
/* The most a quantity item holds. */
constexpr int MAX_QUANTITY = 666;

class Item {
public:
    u16 id;

    Item() { id = 0; }
    // NOLINTNEXTLINE(google-explicit-constructor): an item is its number, as the original's
    Item(u16 itemId) : id(itemId) {}
    /* The copy the compiler made (C++ no longer makes it by itself next to a
     * copy assignment of the class's own). */
    Item(const Item &item) = default;
    /* Takes the item by value, so the copy goes through a temporary. */
    Item &operator=(Item item)
    {
        id = item.id;
        return *this;
    }
    /* The item is not 0 (a search that found one). */
    [[nodiscard]] bool isValid() const { return id != 0; }
    /* The item is the Silencer (item 1). */
    [[nodiscard]] bool isSilencer() const { return id == 1; }
    /* The item is in the ethereal void. */
    [[nodiscard]] bool inVoid() const { return z() == ETHEREAL_Z; }
    [[nodiscard]] bool isAvatar() const; /* 10b0:00ad */
    /* The item is the NPC the player controls (10b0:00c1). */
    [[nodiscard]] bool isControlled() const;
    /* Where the item is drawn on the screen (10b0:00d8). */
    void screenPosition(i16 *x, i16 *y) const;
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] u8 isFlipped() const; /* 10b0:0122 */
    /* The item's place, or that of its outermost container (10b0:013e,
     * 0176, 0266). */
    [[nodiscard]] u16 x() const;
    [[nodiscard]] u16 y() const;
    [[nodiscard]] u8 z() const;
    /* The place of a contained item in its container's gump (10b0:01ae,
     * 01e1, 0215). */
    [[nodiscard]] u16 gumpX() const;
    [[nodiscard]] u16 gumpY() const;
    void setGumpPosition(i16 x, i16 y) const;
    /* The centre of the item's box (10b0:029a, 02de, 0322). */
    [[nodiscard]] u16 centreX() const;
    [[nodiscard]] u16 centreY() const;
    [[nodiscard]] u8 centreZ() const;
    [[nodiscard]] u8 damagePoints() const; /* 10b0:0361 */
    void setDamagePoints(u8 points) const; /* 10b0:037b */
    void setX(u16 x) const;                /* 10b0:0397 */
    void setY(u16 y) const;                /* 10b0:03b0 */
    void setZ(u8 z) const;                 /* 10b0:03c9 */
    [[nodiscard]] ItemPoint point() const; /* 10b0:03fa */
    [[nodiscard]] u16 next() const;        /* 10b0:0471 */
    [[nodiscard]] i16 weight() const;      /* 10b0:0487 */
    /* The weight with the contents' (10b0:04ae). */
    [[nodiscard]] i16 totalWeight() const;
    [[nodiscard]] i16 volume() const;   /* 10b0:04f2 */
    [[nodiscard]] i16 capacity() const; /* 10b0:05b5 */
    /* The weight of the items on this one (10b0:0602). */
    [[nodiscard]] u16 surfaceWeight() const;
    void setNext(u16 item) const;    /* 10b0:0676 */
    [[nodiscard]] u16 shape() const; /* 10b0:068f */
    /* The item's shape is a crate: its ShapeInfo::crate, or one of No
     * Remorse's CRATE_SHAPES (10b0:06a5); hasDeathCry: its
     * ShapeInfo::deathCry (06de; No Regret's). */
    [[nodiscard]] bool isCrate() const;
    [[nodiscard]] u8 hasDeathCry() const;
    void setShape(u16 shape) const;  /* 10b0:0717 */
    [[nodiscard]] u16 frame() const; /* 10b0:0826 */
    void setFrame(u16 frame) const;  /* 10b0:0882 */
    /* setFrame that updates the screen itself. Nothing calls it
     * (10b0:094a). */
    /* UNCALLED: nothing calls it. */
    void setFrameNow(u16 frame) const;
    void setNpc(u8 npc) const;           /* 10b0:0c7a */
    void setNpcNumber(u8 npc) const;     /* 10b0:0c96 */
    [[nodiscard]] u16 npcNumber() const; /* 10b0:0cb2 */
    void setMap(i16 map) const;          /* 10b0:0cce */
    [[nodiscard]] i16 map() const;       /* 10b0:0cea */
    [[nodiscard]] u16 q() const;         /* 10b0:0d06 */
    /* The quality of an item of the quality family, else 0 (10b0:0d1c,
     * 0d60). */
    [[nodiscard]] i16 familyQuality() const;
    void setFamilyQuality(u16 quality) const;
    /* An egg's type, the low byte of its quality; 0 for other items
     * (10b0:0da3). */
    [[nodiscard]] u16 eggType() const;
    void setEggQuality(u16 quality) const; /* 10b0:0dea */
    [[nodiscard]] i16 quality() const;     /* 10b0:0e2d */
    /* Sets the quality and calls the combine event (10b0:0e43). */
    void setQuality(u16 quality);
    /* The first item in a container (10b0:0e68). */
    [[nodiscard]] u16 contents() const;
    /* The container the item is in, or 0 (10b0:0ea4, 0ed2). */
    [[nodiscard]] u16 container() const;
    [[nodiscard]] u16 outerContainer() const;
    [[nodiscard]] u16 globQuality() const;  /* 10b0:0f4a */
    void setGlobQuality(u16 quality) const; /* 10b0:0f8e */
    void setQ(u16 quality) const;           /* 10b0:0fd1 */
    void setContents(u16 item) const;       /* 10b0:0fea */
    /* The family of the item's shape (FAMILY_*, type.h). */
    [[nodiscard]] i16 family() const; /* 10b0:1036 */
    /* A bit of the shape's ShapeInfo (10b0:1061). */
    [[nodiscard]] u8 typeFlag(i16 bit) const;
    [[nodiscard]] u16 flags() const; /* 10b0:10aa */
    /* 1 when a flag of mask is set. */
    [[nodiscard]] u8 hasFlags(u16 mask) const { return ((flags() & mask) != 0) ? 1 : 0; }
    bool legalCreateAtPoint(u16 shape, u16 frame, ItemPoint *point);    /* 10b0:10c0 */
    bool legalCreateAtCoords(u16 shape, u16 frame, u16 x, u16 y, u8 z); /* 10b0:114d */
    /* UNCALLED: nothing calls it. */
    bool legalCreateInContainer(u16 shape, u16 frame, u16 container, i16 flag); /* 10b0:11f8 */
    /* Makes a new item of shape and frame into id (10b0:1259). */
    bool create(u16 shape, u16 frame);
    /* The item goes on and off the ethereal stack (10b0:1383-13c4). */
    void popToCoords(u16 x, u16 y, u8 z);
    void popToPoint(ItemPoint &point) { popToCoords(point.x, point.y, point.z); }
    void popToContainer(u16 container);
    void pop();
    void popToEnd(u16 container);
    void push() const;
    void destroy();               /* 10b0:13d7 */
    void destroyContents() const; /* 10b0:14f3 */
    /* UNCALLED: nothing calls it. */
    void removeContents() const;                                         /* 10b0:1534 */
    void move(u16 x, u16 y, u8 z);                                       /* 10b0:1575 */
    void moveToContainer(u16 container);                                 /* 10b0:1767 */
    bool legalMoveToPoint(ItemPoint *point, u16 flags, u16 force) const; /* 10b0:1791 */
    /* UNCALLED: nothing calls it. */
    bool legalMoveToContainer(u16 *container); /* 10b0:1835 */
    /* Moves the item toward point, hitting what is in the way; blocked
     * tells whether target stopped it (10b0:1885). */
    bool collideMove(ItemPoint *point, u16 flags, u16 moveFlags, u16 target, u8 *blocked, u16 *hit) const;
    void moveTo(const ItemPoint &point) const; /* 10b0:1989 */
    /* Moves the item 16 units in direction (0-7), 10b0:19b3. */
    /* UNCALLED: nothing calls it. */
    void step(char direction) const;
    [[nodiscard]] char directionTo(u16 x, u16 y) const;  /* 10b0:1a2d */
    [[nodiscard]] i16 directionFrom(u16 x, u16 y) const; /* 10b0:1a61 */
    [[nodiscard]] i16 directionFromItem(u16 item) const; /* 10b0:1a83 */
    [[nodiscard]] char directionToItem(u16 item) const;  /* 10b0:1ab8 */
    /* The size of the item's box in 16 (x, y) and 8 (z) units, x and y
     * swapped for a flipped item (10b0:1aed). */
    void footpad(i16 *x, i16 *y, i16 *z) const;
    [[nodiscard]] bool touches(u16 item) const;        /* 10b0:1b90 */
    [[nodiscard]] bool overlaps(u16 item) const;       /* 10b0:1d09 */
    [[nodiscard]] bool overlapsXY(u16 item) const;     /* 10b0:1e44 */
    [[nodiscard]] bool isOn(u16 item) const;           /* 10b0:1f2e */
    [[nodiscard]] bool isCompletelyOn(u16 item) const; /* 10b0:1fa3 */
    [[nodiscard]] bool isAbove(u16 item) const;        /* 10b0:20ba */
    [[nodiscard]] bool isBelow(u16 item) const;        /* 10b0:212f */
    /* Moves the item and those on it up or down by delta (10b0:219a). */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] bool lift(i16 delta) const;
    void getPoint(ItemPoint *point) const; /* 10b0:247a */
    [[nodiscard]] bool isNpc() const;      /* 10b0:24ca */
    [[nodiscard]] bool isInNpc() const;    /* 10b0:24f9 */
    /* Redraws the item (10b0:2558). */
    void touch() const;
    void setFlags(u16 flags) const; /* 10b0:2589 */
    void orFlags(u16 flags) const;  /* 10b0:25b6 */
    void andFlags(u16 flags) const; /* 10b0:25cf */
    /* The usecode events (UPROCESS.C callEvent), each with its number. */
    u16 look();                        /* 10b0:25e8 */
    void setBroken() const;            /* 10b0:2614 */
    void clearBroken() const;          /* 10b0:2653 */
    [[nodiscard]] u8 isBroken() const; /* 10b0:2690 */
    u16 use();                         /* 10b0:26ab */
    /* The item hit hitter (10b0:275c). */
    u16 hit(u16 hitter, i16 force);
    /* hitter hit the item (10b0:27a3). */
    u16 gotHit(u16 hitter, i16 force, u8 *handled);
    u16 anim(); /* 10b0:2926 */
    /* The items on this one are released (10b0:2959). */
    void grab() const;
    u16 release();                        /* 10b0:29ef */
    u16 equip(i16 value);                 /* 10b0:2a1e */
    u16 unequip(i16 value);               /* 10b0:2a51 */
    u16 cacheIn();                        /* 10b0:2a84 */
    u16 combine();                        /* 10b0:2abb */
    [[nodiscard]] i16 isEthereal() const; /* 10b0:2aea, No Regret's */
    u16 calledFromAnim();                 /* 10b0:2b05 */
    u16 enterFastArea();                  /* 10b0:2b34 */
    u16 leaveFastArea();                  /* 10b0:2dcd */
    u16 cast(u16 value);                  /* 10b0:2f5e */
    u16 justMoved();                      /* 10b0:2f91 */
    /* Usecode event 14h, whose use is not known (10b0:2fc0). */
    /* UNCALLED: nothing calls it. */
    u16 unknownEvent14(u16 value);
    u16 unknown2ff3();                   /* 10b0:2ff3 */
    u16 avatarStoleSomething(u16 value); /* 10b0:2ffa */
    /* Stubs that nothing calls (10b0:30a9-30c1). */
    u16 unknown30a9();
    u16 unknown30b0();
    void unknown30b7();
    void unknown30bc();
    bool unknown30c1();
    [[nodiscard]] u8 isOccluding() const;                                   /* 10b0:30c8 */
    [[nodiscard]] u8 isTargetable() const;                                  /* 10b0:30f2 */
    [[nodiscard]] u8 isFixed() const;                                       /* 10b0:311d */
    bool canReach(u16 item, i16 margin, ItemPoint *point, u8 unused) const; /* 10b0:3144 */
    [[nodiscard]] bool canReach(u16 item, i16 margin) const;                /* 10b0:3551 */
    /* Loads the item's frame into the shape cache (10b0:356c). */
    /* UNCALLED: nothing calls it. */
    void loadFrame() const;
    [[nodiscard]] i16 distanceTo(u16 item) const;                       /* 10b0:3596 */
    [[nodiscard]] i16 distanceToShapeAt(u16 shape, i16 x, i16 y) const; /* 10b0:370e */
    [[nodiscard]] u16 qualityHigh() const;                              /* 10b0:385e */
    void setQualityHigh(u16 value) const;                               /* 10b0:387a */
    [[nodiscard]] u16 qualityLow() const;                               /* 10b0:38a9 */
    void setQualityLow(u16 value) const;                                /* 10b0:38c2 */
    void shoot(ItemPoint *target, u16 speed, u16 gravity) const;        /* 10b0:38f1 */
    /* The item's sounds (SNDSRC.C), when the effects are on (10b0:3954-3a33). */
    void playSound(i16 sound) const;
    void playLoop(i16 sound) const;
    void cancelSound(i16 sound) const;
    void restartSound(i16 sound) const;
    void stopSound(i16 sound) const;
    void stopSounds() const;
    [[nodiscard]] u8 isSoundPlaying(i16 sound) const;
    void unknown3a2e();
    void setSoundVolume(i16 sound, char volume) const;
    /* The usecode's movies (No Remorse's, 10a0:3a36, 3a5b, 3a80, 3a9f, 3abb,
     * 3ad7). */
    /* UNCALLED: nothing calls it. */
    u16 playMovie(char *name, i16 x, i16 y, i16 a, i16 b, i16 c, i16 d);
    u16 playMovieCutscene(char *name, i16 x, i16 y, i16 /*unused*/);
    u16 playMovieCutsceneAlt(char *name, i16 x, i16 y, i16 /*unused*/);
    u16 playMovieCutsceneAlt(char *name, i16 x, i16 y);
    u16 playMovieCutscene(char *name, i16 x, i16 y);
    u16 playMovieOverlay(char *name, i16 x, i16 y);
    [[nodiscard]] u8 inFastArea() const; /* 10b0:3a56 */
    /* A new NPC from this egg and the NPC of number npcEgg (10b0:3cd8). */
    [[nodiscard]] u16 createNpc(u16 npcEgg) const;
    bool fits(u16 shape, ItemPoint *point);      /* 10b0:4057 */
    [[nodiscard]] bool isOnScreen() const;       /* 10b0:41c1 */
    [[nodiscard]] bool isMostlyOnScreen() const; /* 10b0:42e9 */
    /* The item stands on the centre of item (10b0:446e). */
    [[nodiscard]] bool isCentreOn(u16 item) const;
    u16 nothing(); /* 10b0:2952, No Regret's */
    /* Says text over the item (No Remorse's, 10a0:290e); the notifier's
     * pid. */
    /* UNCALLED: nothing calls it. */
    u16 bark(char *text) const;
    /* Code19 (1090): takes item's quantity into this one when both have
     * the same quantity or reagent shape (and frame, for reagents), up to
     * MAX_QUANTITY, and destroys item when all of it fits. It always
     * returns 0 (1090:0000). */
    bool absorb(u16 item);
    /* Puts the item in container when it fits there, then lets it absorb
     * the container's items. Nothing calls it (1090:00fb). */
    /* UNCALLED: nothing calls it. */
    bool putInContainer(u16 container);
    /* Code21 (10a0): eggs. The NPC number holds the ranges (high nibble x, low
     * nibble y, in 32-unit steps) and the map number the egg's id. */
    [[nodiscard]] i16 eggXRange() const; /* 10a0:0513 */
    [[nodiscard]] i16 eggYRange() const; /* 10a0:0532 */
    void setEggXRange(u8 range) const;   /* 10a0:0551 */
    void setEggYRange(u8 range) const;   /* 10a0:0586 */
    [[nodiscard]] u16 eggId() const;     /* 10a0:05bb */
    void setEggId(u8 newEggId) const;    /* 10a0:05d7 */
    /* A teleport egg starts a Teleporter and gives its process; the others
     * call the hatch event (10a0:05f3). */
    u16 hatch();
    u16 unhatch(); /* 10a0:0652 */
};
ASSERT_SIZE(Item, 2);

/* The shape of item, read inline. BCC evaluates an inline call before the
 * rest of an expression (ITEMDAM.C, ITEMFIND.C). */
inline u16 shapeOf(u16 item)
{
    return itemShape[item];
}

/* An item whose accessors read the item tables inline instead of calling
 * ITEM.C. Its constructor stores 0 (Item()) before the number. */
class ItemRef : public Item {
public:
    explicit ItemRef(u16 item) { id = item; }
    u16 shape() { return itemShape[id]; }
    u16 next() { return itemNext[id]; }
    u16 flags() { return itemFlags[id]; }
    /* z(), read inline (SurfaceSearch). ITEM.C's z() stays a call. */
    u8 level() { return itemZ[id]; }
};
ASSERT_SIZE(ItemRef, 2);

/* An item made from another Item, also reading the tables inline. A number
 * becomes an Item temporary first, so the original stores the argument and
 * that temporary before the 0 (Code20 Container::contains, GLOB.C
 * 10a8:0339, Code21 10a0:072e). */
class TableItem : public Item {
public:
    explicit TableItem(const Item &item) { id = item.id; }
    u16 x() { return itemX[id]; }
    u16 y() { return itemY[id]; }
    u8 z() { return itemZ[id]; }
    u16 flags() { return itemFlags[id]; }
};
ASSERT_SIZE(TableItem, 2);

/* An item number read straight from the tables. The number initializes Item
 * without the 0 of Item() (Code52 followedItemMoved). */
class MapItem : public Item {
public:
    /* UNCALLED: nothing calls it. */
    explicit MapItem(u16 itemId) : Item(itemId) {}
    /* UNCALLED: nothing calls it. */
    u16 x() { return itemX[id]; }
    /* UNCALLED: nothing calls it. */
    u16 y() { return itemY[id]; }
    /* UNCALLED: nothing calls it. */
    u8 z() { return itemZ[id]; }
};
ASSERT_SIZE(MapItem, 2);

/* The item under screen point x,y (10b0:3062). */
bool itemAt(u16 *item, i16 x, i16 y);
/* A row of a game's STRONGER_WEAPONS: shape's NPC takes weapon type first at
 * one chance in chance (always when chance is 0), else second. */
struct StrongerWeapon {
    u16 shape;
    u16 chance;
    i16 first;
    i16 second;
};
/* Weapon type for an NPC of shape on the hardest level (10b0:3a71). */
i16 strongerWeaponType(u16 shape);
/* At least half of frame of shape at x,y,z is on the screen (10b0:40b4, No
 * Regret's). */
bool isVisible(u16 x, u16 y, u8 z, u16 shape, u16 frame);

/* The process that calls an item's release event (10b0:0020, type F1h). */
constexpr int RELEASE_TYPE = 0xf1;
/* Declared in item.cpp in the original; here so that the saved games'
 * schema reaches it. */
/* Calls an item's release event (Item::grab). */
class ReleaseProcess : public Process {
public:
    /* UNCALLED: nothing calls it. */
    explicit ReleaseProcess(Loading loading) : Process(loading) {}
    explicit ReleaseProcess(u16 releasedItem);
    void run() override;
};

u16 moveKeyDownRecently();
u8 isNpcType(u16 shape);

/* Whether value is one of values (the <name>_game.h files' lists of shapes
 * and animations, which the program holds as spans where the two games'
 * differ in size). */
inline bool isOneOf(u16 value, std::span<const u16> values)
{
    for (const u16 one : values) {
        if (one == value) {
            return true;
        }
    }
    return false;
}
#endif
