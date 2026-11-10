/* Code47 (No Regret's 1170, No Remorse's 1158, file name unknown; the
 * addresses are No Regret's): the NPC helpers of the animations, their shots
 * and sounds, and No Regret's Bobo boomer, the games' differences as each
 * game's values (boboboom_game.h). */
#include <cstdlib>
#include "boboboom.h"
#include "boboboom_game.h"
#include "activity.h"
#include "animprim.h"
#include "attack.h"
#include "control.h"
#include "crusader.h"
#include "crusys.h"
#include "dtable.h"
#include "explode.h"
#include "firetype.h"
#include "itemdata.h"
#include "kernel.h"
#include "npc.h"
#include "random.h"
#include "tracker.h"
#include "utility.h"
#include "shapes.h"

/* Creates an item of shape and frame with flag 80h, in container when it
 * is not 0; its number, 0 when none (1170:01a7). */
/* UNCALLED: nothing calls it. */
u16 unknown01a7(u16 shape, u16 frame, u16 container)
{
    Item item;
    if (item.create(shape, frame)) {
        item.setFlags(item.flags() | ITEM_FAST_ONLY);
        if (container != 0U) {
            item.popToContainer(container);
        }
        return item.id;
    }
    return 0;
}

/* Empty; nothing calls them (1170:0855, 085a). */
void unknown0855(u16 /*unused*/) {}

BoboBoomer::BoboBoomer(ItemPoint *blastPoint)
{
    this->point = *blastPoint;
    count = 0;
    nameProcess(pid, "BoboBoomer");
    setType(BOBO_BOOMER_TYPE);
}

void BoboBoomer::run()
{
    i16 dx;
    i16 dy;
    i16 radius;
    i16 damage;
    ItemPoint where = point;
    dx = randomBetween(-7, 7);
    dy = randomBetween(-7, 7);
    dx *= 16;
    dy *= 16;
    where.x += dx;
    where.y += dy;
    fireExplosion(4, where.x, where.y, where.z);
    radius = fireType(FIRE_EXPLOSION)->range;
    damage = damageOf(FIRE_EXPLOSION);
    if (radius != 0) {
        hurtArea(&where, radius, damage, FIRE_EXPLOSION, 0, 0);
    }
    count++;
    if (count >= 10) {
        terminate(0);
        return;
    }
    setTimeout(static_cast<i16>(borlandRandom(15) + 5));
}

/* The animation, shape and direction are read and only used in a disabled
 * test, which keeps their frame slots (original). */
void fireAtFrame(u16 npc, AnimFrame frame)
{
    Npc self(npc);
    u16 shape;
    const u16 anim = self.lastAnimSet();
    const u16 npcShape = self.shape();
    const char facing = self.direction();
    Item gun;
    i16 type;
    i16 sound;
    /* A test switched off with 0 (original): nothing else reads anim,
     * npcShape and facing. */
    /* NOLINTNEXTLINE(readability-simplify-boolean-expr): the original's disabled test */
    if ((false) && anim == npcShape && (facing != 0)) {
        /* empty in the original */
    }
    if ((self.activeItem2() != 0U) && self.lastAnimSet() == ANIM_FIRE_LARGE) {
        gun = Item(self.activeItem2());
    } else {
        gun = Item(self.activeWeapon());
    }
    if (gun.isValid()) {
        if (BOSS_GRENADES && itemShape[self.id] == BOSS_SHAPE && bossShots++ >= 4) {
            if (bossShots >= 6) {
                bossShots = 0;
            }
            shape = SHAPE_GL_303;
            type = FIRE_BOSS_GRENADE;
        } else {
            shape = itemShape[gun.id];
            type = weaponOf(shape);
        }
        if (self.isSilencer() && type == FIRE_PL_1) {
            if (autoFirer != nullptr) {
                if (fireDown == 0U) {
                    autoFirer->terminate(0);
                }
                if (autoFiring == 0U) {
                    return;
                }
            } else if (fireDown != 0U) {
                new AutoFirer;
            }
        }
        fireWeapon(&self, frame.dx * 16, frame.dy * 16, frame.dz * 8, self.direction(), type, 1);
        sound = fireSound(shape);
        if (sound != 0) {
            self.restartSound(sound);
        }
    }
}

namespace {
/* No Regret's dying shapes' sounds (makeNpcSound), men's and women's; the
 * plasma death's also SHAPE_EM_DEATH's. */
i16 maleGibsSounds[3] = {0x20b, 0x20c, 0x20d};
i16 femaleGibsSounds[2] = {0x20e, 0x20f};
i16 maleSkeletonDeathSounds[2] = {0x213, 0x214};
i16 femaleSkeletonDeathSounds[2] = {0x212, 0x211};
i16 maleLnrDeathSounds[2] = {0x217, 0x218};
i16 femaleLnrDeathSounds[2] = {0x219, 0x216};
i16 maleFrozenSounds[3] = {0x21b, 0x21a, 0x21c};
i16 femaleFrozenSounds[2] = {0x21d, 0x215};
i16 plasmaDeathSounds[2] = {0xd9, 0xda};
}

void makeNpcSound(u16 npc)
{
    Npc self(npc);
    i16 i;
    i16 sound;
    switch (self.lastAnimSet()) {
    case ANIM_LOOK_LEFT:
    case ANIM_LOOK_RIGHT:
        if (seesControlledNpc(npc, 1)) {
            if (oneIn(4)) {
                self.setActivity(ACTIVITY_AGGRESSIVE_ATTACK);
                return;
            }
            self.setActivity(ACTIVITY_ATTACK);
        }
        return;
    }
    if (SHAPE_SOUNDS) {
        /* No Regret's shapes' own sounds, and its boss's boomer. */
        switch (self.shape()) {
        case SHAPE_BOSS_ROBOT: {
            ItemPoint point;
            point = self.point();
            new BoboBoomer(&point);
            return;
        }
        case SHAPE_PLASMA_DEATH:
        case SHAPE_EM_DEATH:
            sound = plasmaDeathSounds[borlandRandom(2)];
            self.playSound(sound);
            return;
        case SHAPE_FROZEN:
            if (self.isFemale() != 0U) {
                sound = femaleFrozenSounds[borlandRandom(2)];
            } else {
                sound = maleFrozenSounds[borlandRandom(3)];
            }
            self.playSound(sound);
            return;
        case SHAPE_LNR_DEATH:
            if (self.isFemale() != 0U) {
                sound = femaleLnrDeathSounds[borlandRandom(2)];
            } else {
                sound = maleLnrDeathSounds[borlandRandom(2)];
            }
            self.playSound(sound);
            return;
        case SHAPE_SKELETON_DEATH:
            if (self.isFemale() != 0U) {
                sound = femaleSkeletonDeathSounds[borlandRandom(2)];
            } else {
                sound = maleSkeletonDeathSounds[borlandRandom(2)];
            }
            self.playSound(sound);
            return;
        case SHAPE_GIBS:
        case SHAPE_BURNING_GIBS:
            if (self.isFemale() != 0U) {
                sound = femaleGibsSounds[borlandRandom(2)];
            } else {
                sound = maleGibsSounds[borlandRandom(3)];
            }
            self.playSound(sound);
            return;
        }
    }
    switch (self.shape()) {
    case SHAPE_CHEDITRON:
    case SHAPE_VETRON:
        explodeIntrinsic(&self, 2, 0);
        self.setFlags(self.flags() & ~ITEM_IN_NPC_LIST);
        self.setFlags(self.flags() & ~ITEM_GUMP_OPEN);
        return;
    case SHAPE_BURNING:
    case SHAPE_SKELETON_DEATH: /* No Remorse's; No Regret's has its own sounds above */
        if (violenceEnabled()) {
            if (self.isFemale() != 0U) {
                sound = FEMALE_CRIES[borlandRandom(static_cast<i16>(FEMALE_CRIES.size()) - 1)];
                for (i = 0; FEMALE_CRIES[i] != -1; i++) {
                    if (self.isSoundPlaying(FEMALE_CRIES[i]) != 0U) {
                        break;
                    }
                }
                if (FEMALE_CRIES[i] == -1) {
                    self.playSound(sound);
                }
                return;
            }
            sound = MALE_CRIES[borlandRandom(static_cast<i16>(MALE_CRIES.size()) - 1)];
            for (i = 0; MALE_CRIES[i] != -1; i++) {
                if (self.isSoundPlaying(MALE_CRIES[i]) != 0U) {
                    break;
                }
            }
            if (MALE_CRIES[i] == -1) {
                self.playSound(sound);
            }
        }
    }
}

void unknown085a(u16 /*unused*/) {}
