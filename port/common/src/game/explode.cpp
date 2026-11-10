/* Code45 (No Regret's 1160, No Remorse's 1148, file name unknown; the
 * addresses are No Regret's): placing items, the frames the controlled NPC
 * fires from, the usecode's explosions, turning NPCs, the sprites and sounds
 * of fire and explosions, the games' differences as each game's values
 * (explode_game.h). */
#include <cstdlib>
#include "gamestate.h"
#include "explode.h"
#include "animcach.h"
#include "animprim.h"
#include "collide.h"
#include "control.h"
#include "crusader.h"
#include "dsfxman.h"
#include "dtable.h"
#include "firetype.h"
#include "itemdata.h"
#include "keyboard.h"
#include "kernel.h"
#include "npc.h"
#include "random.h"
#include "sprite.h"
#include "tracker.h"
#include "type.h"
#include "utility.h"
#include "sounds.h"
#include "shapes.h"

namespace {
/* Nothing reads these. */
[[maybe_unused]] u16 unused32c8[4] = {0, 0, 0, 0};
}

/* Creates an item of shape and frame at x,y,z with flag 80h when it fits
 * there; 1 when it did (1160:0000). */
/* UNCALLED: nothing calls it. */
bool unknown0000(u16 shape, u16 frame, u16 x, u16 y, u8 z)
{
    if (shapeFits(shape, x, y, z, MOVE_STANDS, 0, 0, nullptr)) {
        Item item;
        if (item.create(shape, frame)) {
            item.popToCoords(x, y, z);
            item.setFlags(item.flags() | ITEM_FAST_ONLY);
            return true;
        }
    }
    return false;
}

/* item is not used; its constructor's store is left (original). */
void mouseFire(u8 pressed, i16 /*unused*/, i16 /*unused*/)
{
    const Item item;
    Npc npc(movedActor);
    if (npc.isBusy()) {
        return;
    }
    mouseFiring = 1;
    KeyEvent key;
    key.ascii = ' ';
    key.scan = SCAN_SPACE;
    key.shift = 0;
    key.time = 0;
    key.pressed = pressed;
    key.extended = 0;
    key.repeat = 1;
    controller->syntheticKey(&key);
}

bool fireOffset(u8 direction, u16 *x, u16 *y, u16 *z)
{
    Npc self(controlledActor);
    AnimFrame frame;
    AnimInfo info;
    i16 frames;
    i16 i;
    const u16 anim = npcAnim(&self, (self.isKneeling() != 0U) ? 0x2a : 8);
    info = animInfo(self.shape(), anim);
    frames = info.frames;
    for (i = 0; i < frames; i++) {
        getAnimFrame(self.shape(), anim, direction, i, &frame);
        if (frame.moves()) {
            *x = frame.dx * 16;
            *y = frame.dy * 16;
            *z = frame.dz * 8;
            break;
        }
    }
    return i != frames;
}

namespace {
AnimFrame &frame = gameGlobal<AnimFrame>();
}

AnimFrame *controlledFrame()
{
    Npc self(controlledActor);
    AnimInfo info;
    i16 frames;
    i16 i;
    u16 anim;
    const char facing = self.direction();
    anim = npcAnim(&self, (self.isKneeling() != 0U) ? 0x2a : 8);
    info = animInfo(self.shape(), anim);
    frames = info.frames;
    for (i = 0; i < frames; i++) {
        getAnimFrame(self.shape(), anim, facing, i, &frame);
        if (frame.moves()) {
            break;
        }
    }
    if (i == frames) {
        return nullptr;
    }
    return &frame;
}

void explodeIntrinsic(Item *item, i16 type, u8 destroys)
{
    explode(item, type, destroys, 1);
}

/* explode without damage (1160:0a93). */
/* UNCALLED: nothing calls it. */
void unknown0a93(Item *item, i16 type, u8 destroys)
{
    explode(item, type, destroys, 0);
}

namespace {
/* The sounds of fireExplosion's hits, by the fire types that use them: the
 * blasts' (FIRE_GL_303, FIRE_EXPLOSION, FIRE_AR_7, FIRE_BOSS_GRENADE), the
 * bullets' (FIRE_RP, FIRE_SG_A1, FIRE_BA, FIRE_AC_88), the shocks'
 * (FIRE_ELECTRIC, FIRE_EM_4, 19), and No Regret's BK-16's (FIRE_BK_16, 18),
 * UV-9's and XP-5's. The blasts' shapes are each game's (GRENADE_BLASTS,
 * ROCKET_BLASTS); the boss's grenade takes ROCKET_BLASTS' last. */
i16 explosionSounds[3] = {0x1c, 0x6c, 0x3e};
i16 bulletHitSounds[6] = {0x26, 0x27, 0x41, 0x42, 0x45, 0x46};
i16 em4HitSounds[2] = {0x48, 5};
i16 bk16HitSounds[2] = {8, 0x202};
i16 uv9HitSounds[2] = {0x205, 0x204};
i16 xp5HitSounds[3] = {0x207, 0x208, 0x209};
}

void fireExplosion(u16 type, u16 x, u16 y, u16 z)
{
    u16 shape;
    i16 sound = 0;
    i16 first;
    /* randomBetween(0, 3) picks one of the four cases. */
    i16 last = 0;
    /* Fire types 10h-16h are No Regret's (No Remorse's end at 0Fh). */
    switch (type) {
    case FIRE_RP:
    case FIRE_BA:
        shape = SHAPE_BULLET_HIT;
        sound = bulletHitSounds[borlandRandom(6)];
        break;
    case FIRE_BOSS_GRENADE:
        shape = ROCKET_BLASTS[3];
        sound = explosionSounds[borlandRandom(3)];
        break;
    case FIRE_AR_7:
        shape = ROCKET_BLASTS[borlandRandom(3)];
        sound = explosionSounds[borlandRandom(3)];
        break;
    case FIRE_GL_303:
    case FIRE_EXPLOSION:
        shape = GRENADE_BLASTS[borlandRandom(static_cast<i16>(GRENADE_BLASTS.size()))];
        sound = explosionSounds[borlandRandom(3)];
        break;
    case FIRE_PA:
        shape = SHAPE_SMALL_HIT;
        if (MORE_HIT_SOUNDS) {
            sound = (borlandRandom(2) != 0) ? SOUND_LASER_HIT_1 : SOUND_LASER_HIT_2;
        }
        break;
    case FIRE_SG_A1:
        shape = SHAPE_BULLET_HIT;
        if (MORE_HIT_SOUNDS && oneIn(3)) {
            sound = bulletHitSounds[borlandRandom(6)];
        }
        break;
    case FIRE_PL_1:
        shape = SHAPE_PL_1_HIT;
        if (MORE_HIT_SOUNDS) {
            sound = SOUND_PL_1_HIT;
        }
        break;
    case 9:
        shape = SHAPE_SMALL_HIT;
        break;
    case FIRE_AC_88:
        shape = SHAPE_BULLET_HIT;
        if (!MORE_HIT_SOUNDS || oneIn(4)) {
            sound = bulletHitSounds[borlandRandom(6)];
        }
        break;
    case FIRE_UV_9:
        shape = SHAPE_UV_9_HIT;
        if (MORE_HIT_SOUNDS) {
            sound = uv9HitSounds[borlandRandom(2)];
        }
        break;
    case FIRE_ELECTRIC:
        shape = SHAPE_SMALL_HIT;
        sound = em4HitSounds[borlandRandom(2)];
        break;
    case FIRE_EM_4:
        shape = SHAPE_EM_4_HIT;
        sound = em4HitSounds[borlandRandom(2)];
        break;
    case FIRE_BK_16:
        shape = SHAPE_BK_16_HIT;
        sound = bk16HitSounds[borlandRandom(2)];
        break;
    case FIRE_LNR_81:
        shape = SHAPE_LNR_81_HIT;
        sound = SOUND_LNR_81_HIT;
        break;
    case 18:
        shape = SHAPE_EM_4_HIT;
        sound = bk16HitSounds[borlandRandom(2)];
        break;
    case 19:
        shape = SHAPE_EM_4_HIT;
        sound = em4HitSounds[borlandRandom(2)];
        break;
    case FIRE_XP_5:
        shape = SHAPE_XP_5_HIT;
        sound = xp5HitSounds[borlandRandom(3)];
        break;
    case 21:
        shape = SHAPE_XP_5_HIT;
        sound = SOUND_UV_9_SHOT;
        break;
    default:
        shape = SHAPE_SMALL_HIT;
    }
    first = 0;
    if (shape == SHAPE_UV_9_HIT) {
        first = borlandRandom(3) * 6;
        last = first + 5;
    } else if (shape == SHAPE_SMALL_HIT) {
        last = 10;
    } else if (shape == SHAPE_PL_1_HIT) {
        first = borlandRandom(3) * 5;
        last = first + 4;
    } else if (shape == SHAPE_EM_4_HIT) {
        first = borlandRandom(2) * 4;
        last = first + 3;
    } else if (shape == SHAPE_BK_16_HIT) {
        last = 3;
    } else if (shape == SHAPE_XP_5_HIT) {
        last = 3;
    } else if (shape == SHAPE_LNR_81_HIT) {
        first = borlandRandom(3) * 5;
        last = first + 4;
    } else if (shape != SHAPE_BULLET_HIT) {
        last = 0x27;
    } else {
        switch (randomBetween(0, 3)) {
        case 0:
            last = 4;
            break;
        case 1:
            first = 5;
            last = 8;
            break;
        case 2:
            first = 9;
            last = 0xc;
            break;
        case 3:
            first = 0xd;
            last = 0x10;
            break;
        }
    }
    createSpriteProcess(shape, first, last, 3, x, y, z);
    if (sound != 0) {
        playEffect(sound);
    }
}

namespace {
/* The sprite shapes of explode by explosion type: small, medium and large
 * (the damage and radius are divided by 3, 2 and 1). */
u16 smallBlastShapes[2] = {0x31c, 0x31f};
u16 mediumBlastShapes[3] = {0x326, 0x320, 0x321};
u16 largeBlastShapes[3] = {0x324, 0x323, 0x325};
}

/* The explosion type 0 has divisor 3 and type 2 divisor 1 (they are
 * swapped from type+1). */
i16 explode(Item *item, i16 type, u8 destroys, u8 damages)
{
    i16 radius;
    i16 sound;
    i16 divisor;
    i16 damage;
    /* The type is 0, 1 or 2: DAMAGE.FLX's two bits less 1, and the constants
     * of every usecode call. */
    u16 shape = 0;
    const Npc controlled(controlledActor);
    ItemPoint point;
    if (item->z() == ETHEREAL_Z) {
        return 0;
    }
    item->setBroken();
    /* An item explodes once (item.h). */
    if (item->hasFlags(ITEM_IN_NPC_LIST) != 0U) {
        return 0;
    }
    item->setFlags(item->flags() | ITEM_IN_NPC_LIST);
    item->setFlags(item->flags() | ITEM_GUMP_OPEN);
    item->stopSounds();
    divisor = type + 1;
    if (divisor == 1) {
        divisor = 3;
    } else if (divisor == 3) {
        divisor = 1;
    }
    radius = fireTypes[FIRE_EXPLOSION].range;
    damage = damageOf(FIRE_EXPLOSION);
    radius /= divisor;
    damage /= divisor;
    switch (type) {
    case 0:
        shape = smallBlastShapes[borlandRandom(2)];
        break;
    case 1:
        shape = mediumBlastShapes[borlandRandom(3)];
        break;
    case 2:
        shape = largeBlastShapes[borlandRandom(3)];
        break;
    }
    createSpriteProcess(shape, 0, 0x27, 1, item->centreX(), item->centreY(), item->centreZ());
    if (EXPLOSION_ALERTS && !recentlyAlerted()) {
        alertNpcs();
    }
    if (damages == 0U) {
        if (destroys != 0U) {
            item->destroy();
        }
        sound = oneIn(2) ? 0x1c : 0x6c;
        controlled.playSound(sound);
        return 0;
    }
    point.set(itemX[item->id], itemY[item->id], itemZ[item->id]);
    sound = oneIn(2) ? 0x1c : 0x6c;
    if (EXPLOSION_SOUND_AT_ITEM) {
        item->playSound(sound);
    } else {
        controlled.playSound(sound);
    }
    hurtArea(&point, radius, damage, FIRE_EXPLOSION, item->id, 0);
    if ((destroys != 0U) && item->isValid()) {
        item->destroy();
    }
    return 0;
}

/* Intrinsic 0A9h: whether shape fits at x,y,z (1160:0d96). The shape fits where
 * the dummy shape 31Ah with its size fits; 31Ah gets its own back afterwards.
 * The original gives the dummy its own solidity, not the shape's (original
 * bug). */
bool canExistAt(u16 shape, u16 x, u16 y, u16 z)
{
    const ShapeInfo info = typeFlags.info(shape);
    ShapeInfo saved;
    bool fits = false;
    saved = typeFlags.info(SHAPE_FIT_PROBE);
    typeFlags.infos[SHAPE_FIT_PROBE].x = info.x;
    typeFlags.infos[SHAPE_FIT_PROBE].y = info.y;
    typeFlags.infos[SHAPE_FIT_PROBE].z = info.z;
    typeFlags.infos[SHAPE_FIT_PROBE].solid = saved.solid;
    if (shapeFits(SHAPE_FIT_PROBE, x, y, z, MOVE_NO_FLOOR, 0, 0, nullptr)) {
        fits = true;
    }
    typeFlags.infos[SHAPE_FIT_PROBE].x = saved.x;
    typeFlags.infos[SHAPE_FIT_PROBE].y = saved.y;
    typeFlags.infos[SHAPE_FIT_PROBE].z = saved.z;
    typeFlags.infos[SHAPE_FIT_PROBE].solid = saved.solid;
    return fits;
}

/* Turns npc toward direction and returns the pid of the animation, 0 for
 * none (1160:0084, Actor::I_turnToward): turnFrom from its own direction. No
 * Remorse's has turnFrom's body in it (1148:009c). The originals' direction
 * and last parameter are a byte and a word in No Regret's, a char and a byte
 * in No Remorse's; the port takes turnFrom's, which only the low bytes reach
 * in both. */
u16 turnNpc(u16 npc, char direction, u8 speed, u8 unknown)
{
    Npc self(npc);
    return turnFrom(npc, self.direction(), direction, speed, unknown);
}

/* Turns npc from one direction to another, a turn animation a step; the pid of
 * the last, 0 for none (1160:00bf). The steps go the short way round; an NPC
 * with 8 directions turns two steps at a time from even directions. */
u16 turnFrom(u16 npc, char from, char to, u8 speed, u8 unknown)
{
    Npc self(npc);
    char facing = from;
    i16 i;
    i16 back;
    i16 around;
    i16 steps;
    const char left = (facing - 1) & 0xf;
    const char right = (facing + 1) & 0xf;
    bool clockwise;
    bool sixteen;
    i16 anim;
    AnimInfo info;
    u16 pid = 0;
    if (self.isKneeling() != 0U) {
        anim = ANIM_KNEELING_SMALL;
    } else if ((self.isInCombat() != 0U) && (unknown == 0U)) {
        const Item gun(self.activeWeapon());
        if (self.isSilencer()) {
            if (gun.isValid() && (smallWeapon(gun.shape()) != 0U)) {
                anim = ANIM_COMBAT_STAND_SMALL;
            } else {
                anim = ANIM_COMBAT_STAND_LARGE;
            }
        } else {
            anim = ANIM_COMBAT_STAND_SMALL;
        }
    } else if (self.isActivityLocked() != 0U) {
        anim = ANIM_SURRENDER_STAND;
    } else {
        anim = ANIM_STAND;
    }
    info = animInfo(self.shape(), anim);
    sixteen = info.sixteenDirections != 0;
    if (!sixteen) {
        if ((facing & 1) != 0) {
            facing = (facing + 1) & 0xf;
        }
        if ((to & 1) != 0) {
            to = (to + 1) & 0xf;
        }
    }
    if (facing != to && ((speed != 0U) || (to != left && to != right))) {
        if (facing > to) {
            around = facing - to;
        } else {
            around = facing + 16 - to;
        }
        back = 16 - around;
        if (back < around) {
            steps = back;
            clockwise = true;
        } else {
            steps = around;
            clockwise = false;
        }
        steps++;
        for (i = 0; i < steps; (sixteen) ? i++ : (i++, i++)) {
            pid = self.doAnim(anim, (clockwise) ? (facing + i) & 0xf : (facing - i) & 0xf, EVEN_IF_BUSY, 0);
        }
    }
    return pid;
}
