/* ..\\ITEM\\TYPE.C (Code35, 1110): the type flags of the shapes (typeflag.dat),
 * 9 bytes a shape, and the shapes' names (editor\typename.dat). */
#ifndef PORT_TYPE_H
#define PORT_TYPE_H
#include "common.h"
#include "shaphand.h"

/* ShapeInfo::family. */
constexpr int FAMILY_GENERIC = 0;
constexpr int FAMILY_QUALITY = 1;
constexpr int FAMILY_QUANTITY = 2;
constexpr int FAMILY_GLOB_EGG = 3;
constexpr int FAMILY_EGG = 4;
constexpr int FAMILY_BREAKABLE = 5;
constexpr int FAMILY_CONTAINER = 6;
constexpr int FAMILY_MONSTER_EGG = 7;
constexpr int FAMILY_TELEPORT_EGG = 8;
constexpr int FAMILY_REAGENT = 9;
constexpr int FAMILY_WEAPON = 10;
constexpr int FAMILY_AMMO = 11;
constexpr int FAMILY_BOMB = 12;
constexpr int FAMILY_INVENTORY = 13;

struct __attribute__((packed)) ShapeInfo {
    ui16 fixed : 1;
    ui16 solid : 1;
    ui16 sea : 1;
    ui16 land : 1;
    ui16 occludes : 1;
    ui16 bag : 1;
    ui16 damaging : 1;
    ui16 noisy : 1;
    ui16 draw : 1;
    ui16 ignore : 1;
    ui16 roof : 1;
    ui16 translucent : 1;
    /* What kind of item the shape is (4: eggs, Item::family). */
    ui16 family : 5;
    ui16 equipType : 4;
    /* The size of the shape's box. */
    ui16 x : 5;
    ui16 y : 5;
    ui16 z : 5;
    ui16 animType : 4;
    ui16 animData : 4;
    ui16 animSpeed : 4;
    ui16 editor : 1;
    ui16 selectable : 1;
    /* Kept in the shape file when the swap file is trimmed (SHAPHAND.C). */
    ui16 preload : 1;
    ui16 sound : 1;
    ui16 targetable : 1;
    ui16 npc : 1;
    /* Crates, barrels and containers and their wrecks: gravity moves them
     * without weight, and what lies on them falls when they go. */
    ui16 crate : 1;
    /* An NPC that cries out when it dies (CRUSADER.C's death sounds). */
    ui16 deathCry : 1;
    u8 weight;
    u8 volume;

    /* The flags of a new shape: solid, 1 by 1, the rest clear but the two
     * unknown bits (1110:0000). Nothing calls it. */
    void reset();
    /* The size of the shape's box in map units, 16 a step in x and y and 8
     * in z; a box without height is 1 high unless the shape is fixed. */
    void box(i16 &width, i16 &depth, i16 &height) const
    {
        width = x << 4;
        depth = y << 4;
        height = z << 3;
        if (height == 0 && !fixed) {
            height = 1;
        }
    }
    /* The same in half units, 8 a step in x and y too (Code117). */
    void halfBox(i16 &width, i16 &depth, i16 &height) const
    {
        width = x << 3;
        depth = y << 3;
        height = z << 3;
        if (height == 0 && !fixed) {
            height = 1;
        }
    }
};
ASSERT_FILE_SIZE(ShapeInfo, 9);

/* ShapeInfo::box with the shape last: its arguments go through temporaries
 * in this order (COLLIDE.C 10f0:00d9, 032e; Code52 1198:049e). */
inline void shapeBox(i16 &width, i16 &depth, i16 &height, ShapeInfo *info)
{
    info->box(width, depth, height);
}

class TypeFlags {
public:
    /* The name of each shape but shape 0, 32 characters each. */
    char (*names)[32];
    ShapeInfo *infos;

    /* Loads both files (1110:0099). */
    void init();
    void close(); /* 1110:00ca */
    /* Reads typeflag.dat (1110:0114). */
    void loadInfos();
    /* Reads editor\typename.dat when it is there (1110:0208). */
    void loadNames();
    [[nodiscard]] ShapeInfo info(i16 shape) const; /* 1110:01d5 */
    /* The shape's name, 0 for shape 0 (1110:02d6). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    [[nodiscard]] char *name(i16 shape) const;
    /* The first shape from start whose name is text, or with lower, whose
     * name holds text in any case; -1 when none is (1110:02fa). Nothing
     * calls it. */
    /* UNCALLED: nothing calls it. */
    i16 find(char *text, u8 lower, i16 start) const;
};
ASSERT_SIZE(TypeFlags, 8);

extern TypeFlags &typeFlags; /* 1480:89ac */

i16 familyOfType(i16 shape);
#endif
