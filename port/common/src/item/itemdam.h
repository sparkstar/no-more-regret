/* ..\\ITEM\\ITEMDAM.C (Code26, 10c8): how the items of each shape take damage
 * (static\damage.flx), 6 bytes a shape. */
#ifndef PORT_ITEMDAM_H
#define PORT_ITEMDAM_H
#include "common.h"
#include "fastflex.h"

#include <memory>

struct __attribute__((packed)) DamageEntry {
    ui16 damageable : 1;
    ui16 explosion : 2;
    ui16 explosionDamages : 1;
    /* The item takes several hits: without it, one that takes damage breaks at
     * the first (isDamageable). */
    ui16 durable : 1;
    ui16 explosionDestroys : 1;
    ui16 replace : 1;
    ui16 absoluteFrames : 1;
    u8 sound;
    /* What a destroyed item becomes, with replace. */
    struct __attribute__((packed)) Replacement {
        u16 shape;
        u8 frame;
    };
    union {
        Replacement replacement;
        /* Without replace, the frames a destroyed item may turn to (0 for
         * none): frame numbers with absoluteFrame, else steps from its
         * frame. */
        u8 frames[3];
    };
    u8 damagePoints;

    [[nodiscard]] u8 takesDamage() const { return damageable; }
    [[nodiscard]] u8 explosionType() const { return explosion; }
    [[nodiscard]] u8 explodeWithDamage() const { return explosionDamages; }
    [[nodiscard]] u8 explodeDestroysItem() const { return explosionDestroys; }
    /* A destroyed item becomes an item of replacement.shape. */
    [[nodiscard]] u8 replaceItem() const { return replace; }
    [[nodiscard]] u8 frameDataIsAbsolute() const { return absoluteFrames; }
};
ASSERT_FILE_SIZE(DamageEntry, 6);

/* The shapes' entries, one per shape. */
constexpr int DAMAGE_SHAPES = 0x800;

class DamageTable {
public:
    /* Its own. */
    std::unique_ptr<FastFlex> flex;
    std::unique_ptr<DamageEntry[]> entries;

    /* Opens damage.flx, reads the entries and becomes damageTable
     * (10c8:0000). */
    DamageTable();
    ~DamageTable(); /* 10c8:00e8 */
    /* Reads the entries; all 0 when the object has the wrong size
     * (10c8:0169). */
    void load() const;
    /* 0 when a hit breaks the item at once: its shape takes damage and is not
     * durable (10c8:01dc). */
    [[nodiscard]] u8 isDamageable(u16 item) const;
    [[nodiscard]] u8 takesDamage(u16 shape) const; /* 10c8:022d */
    /* points of damage would break item (10c8:025d). */
    [[nodiscard]] bool wouldBreak(u16 item, i16 points) const;
    /* Takes points of damage from item and, when it breaks, explodes it,
     * plays its sound and replaces it or changes its frame; 1 when it
     * broke (10c8:030c). */
    [[nodiscard]] bool breakItem(u16 item, i16 points) const;
};
ASSERT_SIZE(DamageTable, 8);

/* static\fixed.dat, the items of the maps, through the game's file names
 * (10c8:0616, the module after ITEMDAM.C: fixedflx.cpp). ITEMCACH.C opens it.
 * Its destructor is the compiler's (10c8:06a2). */
class FixedFlex : public FastFlex {
public:
    FixedFlex();
};
ASSERT_SIZE(FixedFlex, 0x10);

extern DamageTable *&damageTable; /* 1480:15f6 */
#endif
