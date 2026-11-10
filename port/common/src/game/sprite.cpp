/* SPRITE.C (Code43, No Regret's 1150, No Remorse's 1138; the addresses are
 * No Regret's): sprites, the processes that show an animation at a map point,
 * and super sprites, the shots that fly across the map, the games' differences
 * as each game's values (sprite_game.h). */
#include <algorithm>
#include <array>
#include <cstdlib>
#include "gamestate.h"
#include "sprite.h"
#include "sprite_game.h"
#include "camera.h"
#include "control.h"
#include "crusader.h"
#include "distance.h"
#include "explode.h"
#include "firetype.h"
#include "itemdata.h"
#include "itemutil.h"
#include "kernel.h"
#include "mlist.h"
#include "npc.h"
#include "spmgr.h"
#include "systimer.h"
#include "tracker.h"
#include "type.h"
#include "utility.h"
#include "shapes.h"

namespace {
/* The sprites that run. */
u16 &runningSprites = gameGlobal<u16>(0);

/* The shapes of fire, the explosions' blasts (explode's tables,
 * GRENADE_BLASTS, ROCKET_BLASTS), in adjustPoint's order. */
constexpr std::array<u16, 10> FIRE_SHAPES = {0x326, 0x31c, 0x31f, 0x320, 0x321, 0x323, 0x324, 0x325, 0x322, 0x327};

/* No Regret's sprites that follow the Silencer when they start at him: the
 * usecode's (a red ring, a glowing body, white and yellow lightning) and the
 * sparks of a shock. */
constexpr std::array<u16, 5> FOLLOWING_SPRITES = {0x352, 0x609, 0x1f2, 0x3d5, SHAPE_SHOCK_SPARKS};
}

u16 createSpriteProcess(u16 shape, u16 firstFrame, u16 lastFrame, u16 repeats, u16 x, u16 y, u8 z)
{
    Sprite *sprite = nullptr;
    u16 fireX;
    u16 fireY;
    u8 fireZ;
    i16 handle;
    u16 pid;
    Process *process;
    bool atNpc = false;
    i16 distance;
    i16 count;
    i16 i;
    Point point;
    i16 screenY;
    i16 screenX;
    if (SILENCER_SPRITE && itemX[controlledNpc.id] == x && itemY[controlledNpc.id] == y &&
        itemZ[controlledNpc.id] == z) {
        atNpc = true;
    }
    if ((limitBlasts != 0U) && isFireShape(shape)) {
        fireX = x;
        fireY = y;
        fireZ = z;
        adjustPoint(shape, &fireX, &fireY, &fireZ);
        mapToScreen(fireX, fireY, &screenX, &screenY);
        screenY -= fireZ;
        if (spriteList.valid()) {
            count = spriteList.count - 1;
        } else {
            count = 0;
        }
        for (i = 0; i < count; i++) {
            handle = spriteList.get(i);
            if (spriteFrame(handle) < 0x1e) {
                point = spritePoint(handle);
                distance = mapDistance(point.x, point.y, screenX, screenY);
                if (distance <= 0x3c) {
                    break;
                }
            }
        }
        if (i == count) {
            sprite = new Sprite(shape, firstFrame, lastFrame, repeats, x, y, z);
            spriteList.add(sprite->handle);
        }
    } else {
        sprite = new Sprite(shape, firstFrame, lastFrame, repeats, x, y, z);
    }
    if (SILENCER_SPRITE && atNpc && std::ranges::find(FOLLOWING_SPRITES, shape) != FOLLOWING_SPRITES.end() &&
        (silencerSprite == 0U) && (sprite != nullptr)) {
        silencerSprite = pid = sprite->pid;
        process = findProcess(pid);
        process->listenTo(controller->pid);
    }
    if (sprite != nullptr) {
        return sprite->pid;
    }
    return 0;
}

/* Nothing calls it. */
/* UNCALLED: nothing calls it. */
u16 unknown0000(u16 /*unused*/)
{
    return 0;
}

/* The fire shapes start where the weapon's muzzle shows on the screen, 240
 * units up. */
void adjustPoint(u16 shape, u16 *x, u16 *y, u8 *z)
{
    i16 i;
    i16 screenX;
    i16 screenY;
    i16 rise;
    const i16 dx[10] = {152, 168, 89, 139, 152, 180, -8, 10, 145, 90};
    const i16 dy[10] = {162, 148, 78, 147, 83, 163, -23, -36, 58, 77};
    for (i = 0; i < static_cast<i16>(FIRE_SHAPES.size()); i++) {
        if (FIRE_SHAPES[i] == shape) {
            mapToScreen(*x, *y, &screenX, &screenY);
            rise = 0xf0 - *z;
            screenY += rise;
            screenToMap(screenX, screenY, x, y);
            *z = 0xf0;
            *x += dx[i];
            *y += dy[i];
            return;
        }
    }
}

Sprite::Sprite(u16 shape, u16 newFirstFrame, u16 newLastFrame, u16 newRepeats, u16 x, u16 y, u8 z)
{
    nameProcess(pid, "Sprite");
    setType(SPRITE_TYPE);
    handle = -1;
    const Item unused;
    ShapeInfo info;
    adjustPoint(shape, &x, &y, &z);
    info = typeFlags.info(shape);
    handle = createSprite(shape, newFirstFrame, info.translucent ? 0x1000 : 0, x, y, z);
    if (handle != -1 && shape == SHAPE_RETICLE) {
        targetSprite = handle;
        targetSpritePid = pid;
    }
    if (isFireShape(shape)) {
        fire = 1;
    } else {
        fire = 0;
    }
    this->firstFrame = newFirstFrame;
    frame = this->firstFrame;
    this->lastFrame = newLastFrame;
    this->repeats = newRepeats;
    runningSprites++;
    if ((fire != 0U) && (frameSkip != 0)) {
        if ((this->firstFrame & 1) != 0) {
            this->firstFrame--;
            frame = this->firstFrame;
        }
        if ((this->lastFrame & 1) != 0) {
            this->lastFrame--;
        }
    }
}

/* The frames the last ticks missed are skipped; the sprite shows the last
 * of them. */
void Sprite::run()
{
    i16 missed;
    i16 step;
    const u32 ticks = frameTicks;
    if (frameSkip != 0) {
        step = 2;
    } else {
        step = 1;
    }
    if (ticks >= static_cast<u32>(static_cast<i16>(step * 5))) {
        missed = 4;
    } else if (ticks >= static_cast<u32>(static_cast<i16>(step * 4))) {
        missed = 3;
    } else if (ticks >= static_cast<u32>(static_cast<i16>(step * 3))) {
        missed = 2;
    } else if (ticks >= static_cast<u32>(static_cast<i16>(step * 2))) {
        missed = 1;
    } else {
        missed = 0;
    }
    do {
        if (firstFrame <= lastFrame) {
            if ((fire != 0U) && (frameSkip != 0)) {
                frame += 2;
            } else {
                frame++;
            }
        } else {
            frame--;
        }
        if (missed == 0) {
            if (frame > lastFrame || frame < 0) {
                if (handle != -1 && static_cast<ui16>(handle) != targetSprite) {
                    freeSprite(handle);
                    handle = -1;
                }
                terminate(0);
            } else if (repeats != 0U) {
                if (handle != -1) {
                    setSpriteFrame(handle, frame);
                }
                setTimeout(static_cast<ui16>(repeats * (missed + 1) * (static_cast<int>(frameSkip != 0) + 1)));
            }
        }
    } while ((missed--) != 0);
}

void Sprite::event(u32 /*event*/, EventData /*data*/) {}

void Sprite::cleanup()
{
    Process::cleanup();
    runningSprites--;
}

void Sprite::moveBy(i16 dx, i16 dy, char dz) const
{
    u16 x;
    u16 y;
    u16 z;
    spritePosition(handle, &x, &y, &z);
    showSprite(handle, x + dx, y + dy, z + dz);
}

void Sprite::onKill()
{
    if (handle != -1) {
        if (static_cast<ui16>(handle) == targetSprite) {
            setSpriteFrame(handle, (reticleStyle * 6) + 5);
        } else {
            freeSprite(handle);
            handle = -1;
        }
    }
    if (pid == targetSpritePid) {
        targetSpritePid = 0;
    }
}

SuperSprite::SuperSprite()
{
    nameProcess(pid, "SuperSprite");
    setType(SUPER_SPRITE_TYPE);
    sprite = -1;
    vx = 0;
    vy = 0;
    vz = 0;
    dz = 0;
    hitItem = 0;
    distance = 0;
    steps = 0;
    shape = 0;
    counter = 1;
    bounces = 0;
    homing = 0;
    expired = 0;
    startTime = systemTimer->ticks;
    unknownb9 = 0;
    direction = NO_DIRECTION;
    targetItem = 0;
    frame = 0;
    splash.x = -1;
    launched = 0;
}

void SuperSprite::onKill()
{
    if (sprite != -1) {
        freeSprite(sprite);
        sprite = -1;
    }
}

void SuperSprite::place(u16 newShape, u16 newFrame, ItemPoint *point)
{
    place(newShape, point);
    this->frame = newFrame;
}

void SuperSprite::place(u16 newShape, ItemPoint *point)
{
    this->shape = newShape;
    now = *point;
    start = *point;
}

/* UNCALLED: nothing calls it. */
u16 SuperSprite::createItem(u16 createdShape, u16 createdFrame, ItemPoint *point)
{
    Item shot;
    shot.create(createdShape, createdFrame);
    if (shot.isValid()) {
        this->shape = createdShape;
        now = *point;
        shot.setFlags(shot.flags() | ITEM_FAST_ONLY);
        shot.popToPoint(*point);
        start = *point;
        setItem(shot.id);
    }
    return shot.id;
}

void SuperSprite::launchInDirection(char newDirection, u16 newDamage, u16 newFire, u16 newSource, u16 newTarget,
                                    u8 inexact)
{
    ItemPoint point;
    point.x = start.x + directionX16[static_cast<int>(newDirection)] * 16 * 16 * 5;
    point.y = start.y + directionY16[static_cast<int>(newDirection)] * 16 * 16 * 5;
    point.z = start.z;
    launch(newDirection, point, newDamage, newFire, newSource, newTarget, inexact);
}

/* An inexact shot spreads over a tenth of the distance (less for the
 * Silencer's weapons and for NPCs that are hurt or wait), at most 80 across
 * and 24 up. */
void SuperSprite::launch(char newDirection, ItemPoint point, u16 newDamage, u16 newFire, u16 newSource, u16 newTarget,
                         u8 inexact)
{
    static const i16 maxSpread = 80;
    static const i16 maxRise = 24;
    Npc controlled(controlledActor);
    Npc shooter(newSource);
    i16 nowX;
    i16 nowY;
    i16 spreadX;
    i16 spreadY;
    i16 spreadZ;
    i16 spread;
    i16 offset;
    targetItem = newTarget;
    this->direction = newDirection;
    this->source = newSource;
    this->damage = newDamage;
    this->fire = newFire;
    counter = 1;
    if (launched == 0U) {
        launched = 1;
        if (newFire == 9) {
            homing = 1;
            vx = directionX16[static_cast<int>(this->direction)] * 16;
            vy = directionY16[static_cast<int>(this->direction)] * 16;
        }
    }
    if (inexact != 0U) {
        inexact = static_cast<u8>(fireType(newFire)->accurate == 0U);
    }
    if (ROBOTS_AIM_EXACTLY && shooter.isValid() && shooter.isNpc() && !isOrganic(&shooter)) {
        inexact = 0;
    }
    if (inexact != 0U) {
        Npc firer(newSource);
        bool npc;
        spreadX = abs(static_cast<i16>(point.x - start.x));
        spreadY = abs(static_cast<i16>(point.y - start.y));
        spreadZ = abs(point.z - start.z);
        if (spreadX > spreadY) {
            spread = spreadX;
        } else {
            spread = spreadY;
        }
        if (spreadZ > spread) {
            spread = spreadZ;
        }
        if (newSource == controlledActor) {
            if (this->fire == FIRE_SG_A1 || this->fire == FIRE_AC_88) {
                spread /= 8;
            } else if (this->fire == FIRE_PA) {
                spread /= 0xc;
            } else if (this->fire == FIRE_AR_7) {
                spread /= 5;
            } else {
                spread /= 10;
            }
        } else {
            npc = firer.isNpc();
            if (npc && (npcs[firer.id].aimedBurst != 0U)) {
                spread /= 2;
            } else if (npc && dodgeOver(&firer)) {
                spread /= 8;
            } else if (OTHER_SHOOTERS_SPREAD && !npc && (controlled.isKneeling() != 0U)) {
                spread /= 5;
            } else if (OTHER_SHOOTERS_SPREAD && !npc) {
                spread /= static_cast<i16>(dodgeFactor * 3);
            } else {
                spread /= dodgeFactor;
            }
        }
        if (spread > maxSpread) {
            spread = maxSpread;
        }
        offset = randomBetween(-spread, spread);
        point.x += offset;
        offset = randomBetween(-spread, spread);
        point.y += offset;
        spread /= 3;
        if (spread > maxRise) {
            spread = maxRise;
        }
        offset = randomBetween(-spread, spread);
        spreadZ = point.z;
        spreadZ += offset;
        if (spreadZ > 0xfa) {
            spreadZ = 0xfa;
        } else if (spreadZ < 0) {
            spreadZ = 0;
        }
        point.z = spreadZ;
    }
    this->target.x = point.x;
    this->target.y = point.y;
    this->target.z = point.z;
    nowX = now.x;
    nowY = now.y;
    distance = mapDistance(point.x, point.y, point.z, nowX, nowY, now.z);
    dy = static_cast<i16>(point.y - nowY);
    dx = static_cast<i16>(point.x - nowX);
    distance /= 16;
    steps = distance / fireTypes[newFire].cellsPerRound;
    dz = point.z - now.z;
    if (dy == 0 && dx == 0) {
        halt();
    }
    if (this->fire == FIRE_SG_A1 || this->fire == FIRE_AC_88) {
        this->target.x += static_cast<i16>(distance * 16) / 5;
        this->target.y += static_cast<i16>(distance * 16) / 5;
        this->target.z += static_cast<i16>(distance * 8) / 5;
    }
    if ((distance % 16) != 0) {
        steps++;
    }
    if (distance == 0) {
        steps++;
    }
}

void SuperSprite::launchAtReticle(char newDirection, u16 newDamage, u16 newFire, u16 newSource, u16 newTarget, u8 z)
{
    ItemPoint point;
    const Item firer(newSource);
    i16 reticleDx;
    i16 reticleDy;
    u16 x;
    u16 y;
    u16 reticleX;
    u16 reticleY;
    x = itemX[firer.id];
    y = itemY[firer.id];
    reticleX = spriteX(reticleSprite);
    reticleY = spriteY(reticleSprite);
    if (AIM_PAST_RETICLE) {
        reticleDx = reticleX - x;
        reticleDy = reticleY - y;
        const u16 aimX = x + (reticleDx * 6);
        const u16 aimY = y + (reticleDy * 6);
        point.x = aimX;
        point.y = aimY;
    } else {
        point.x = reticleX;
        point.y = reticleY;
    }
    point.z = z;
    launch(newDirection, point, newDamage, newFire, newSource, newTarget, 1);
}

bool SuperSprite::move(ItemPoint from, ItemPoint to)
{
    bool moved;
    const Item self(item);
    sweep.reset();
    if (self.isValid()) {
        moved = sweep.move(item, &to, 4);
    } else {
        moved = sweep.test(shape, from, to, MOVE_NO_FLOOR, 0, 0);
    }
    hitItem = sweep.hitItem;
    return moved;
}

void SuperSprite::finish()
{
    Item self(item);
    if (self.isValid()) {
        if (self.z() != ETHEREAL_Z) {
            self.destroy();
        }
    } else {
        terminate(0);
    }
}

/* where is not used; only its frame slot is left. The Z half size is worked
 * out but never clamped (original). */
void SuperSprite::clampToItem(Item clampItem, ItemPoint *point)
{
    const ShapeInfo info = typeFlags.info(clampItem.shape());
    i16 halfX;
    i16 halfY;
    [[maybe_unused]] i16 halfZ;
    u16 sizeX;
    u16 sizeY;
    u16 sizeZ;
    if ((sizeX = info.x) >= 3 && (sizeY = info.y) >= 3 && (sizeZ = info.z) >= 3) {
        halfX = (sizeX - 2) * 16;
        halfY = (sizeY - 2) * 16;
        halfZ = (sizeZ - 2) * 8;
        const u16 clampX = clampItem.x();
        const u16 clampY = clampItem.y();
        if (point->x < static_cast<ui16>(clampX - halfX)) {
            point->x = clampX - halfX;
        }
        if (point->y < static_cast<ui16>(clampY - halfY)) {
            point->y = clampY - halfY;
        }
    }
}

/* 1 for the shapes of fire (1150:1fb1); the original's callers test the
 * shapes themselves, the port's call it. */
bool isFireShape(u16 shape)
{
    return std::ranges::find(FIRE_SHAPES, shape) != FIRE_SHAPES.end();
}

void SuperSprite::explodeAt(ItemPoint *point) const
{
    const i16 radius = fireType(fire)->range;
    if (radius != 0) {
        hurtArea(point, radius, damage, fire, hitItem, source);
    }
    fireExplosion(fire, point->x, point->y, point->z);
}

void SuperSprite::advance()
{
    const Item self(item);
    u16 spriteFrame;
    now = next;
    if (self.isValid()) {
        sweep.apply();
    }
    if (sprite != -1) {
        showSprite(sprite, now.x, now.y, now.z);
        if (fire == FIRE_UV_9) {
            spriteFrame = ::spriteFrame(sprite);
            spriteFrame++;
            if (spriteFrame >= 0x4c) {
                spriteFrame = 0x47;
            }
            setSpriteFrame(sprite, spriteFrame);
        } else if (fire == FIRE_LNR_81) {
            spriteFrame = ::spriteFrame(sprite);
            spriteFrame++;
            if (spriteFrame % 6 == 0) {
                spriteFrame = spriteFrame - 6;
            }
            setSpriteFrame(sprite, spriteFrame);
        } else if (fire == FIRE_XP_5) {
            spriteFrame = ::spriteFrame(sprite);
            spriteFrame++;
            if (static_cast<ui16>(spriteFrame - 0xdc) % 3 == 0) {
                spriteFrame = spriteFrame - 3;
            }
            setSpriteFrame(sprite, spriteFrame);
        }
    }
    if (fire == FIRE_GL_303) {
        if (splash.x != 0xffff) {
            createSpriteProcess(SHAPE_GRENADE_TRAIL, 0, 9, 3, splash.x, splash.y, splash.z);
        }
        splash = now;
    }
}

/* The move from from to to is swept again 16 units at a time to find where
 * the shot hit; the item there takes the hit and the shot explodes. self is
 * not used (original). */
void SuperSprite::hit(ItemPoint from, ItemPoint to)
{
    Item struck(hitItem);
    const Item self(item);
    s32 hitDx;
    s32 hitDy;
    s32 hitDz;
    s32 i = 1;
    s32 count;
    i16 hitDistance;
    char toward;
    bool hitResult;
    ItemPoint point;
    ItemPoint step;
    ItemPoint sweepFrom;
    ItemPoint sweepTo;
    s32 x;
    s32 y;
    s32 z;
    hitDistance = mapDistance(from.x, from.y, from.z, to.x, to.y, to.z);
    hitDx = static_cast<s32>(to.x) - from.x;
    hitDy = static_cast<s32>(to.y) - from.y;
    hitDz = static_cast<s32>(to.z) - static_cast<s32>(from.z);
    count = hitDistance / 16;
    if ((hitDistance % 16) != 0) {
        count++;
    }
    point = from;
    do {
        x = from.x + hitDx * i / count;
        y = from.y + hitDy * i / count;
        z = static_cast<s32>(from.z) + hitDz * i / count;
        step.x = x;
        step.y = y;
        step.z = z;
        sweep.reset();
        sweepFrom = point;
        sweepTo = step;
        hitResult = sweep.test(shape, sweepFrom, sweepTo, MOVE_NO_FLOOR, 0, 0);
        hitItem = sweep.hitItem;
        if (hitResult) {
            point = step;
            i++;
        } else {
            break;
        }
    } while (hitResult && i <= count);
    if (hitItem != 0U) {
        struck = Item(hitItem);
        clampToItem(Item(hitItem), &point);
        if (struck.isNpc()) {
            const i16 lastX = last.x;
            const i16 lastY = last.y;
            toward = ::direction(lastX, lastY, itemX[struck.id], itemY[struck.id]);
            takeHit(&struck, SHOT_HITS_AS_ITEM ? item : source, toward, damage, fire, &point);
        } else {
            const u16 hitter = 0x4000;
            const i16 force = fire << 8 | damage;
            u8 handled;
            struck.gotHit(hitter, force, &handled);
            if ((handled == 0U) && (struck.isFixed() == 0U)) {
                takeHit(&struck, 0, borlandRandom(8), 0, fire, &point);
            }
        }
    }
    explodeAt(&point);
    finish();
}

#define SIGN(x) ((x) < 0 ? -1 : (x) != 0)

void SuperSprite::run()
{
    const Item aim(targetItem);
    const Item self(item);
    s32 nextX;
    s32 nextY;
    s32 nextZ;
    char facing;
    if (!inFastArea(now)) {
        finish();
        return;
    }
    last = now;
    if (homing != 0U) {
        char toward;
        /* Set while the missile has not expired, and read only then. */
        i16 aimZ = 0;
        i16 step;
        if (fireTypes[fire].roundDuration <= counter) {
            expired = 1;
        } else if (expired == 0U) {
            if (aim.isValid()) {
                toward = ::direction(last.x, last.y, aim.x(), aim.y());
                aimZ = aim.centreZ() + 8;
            } else {
                toward = direction;
                aimZ = last.z;
            }
            step = directionX16[static_cast<int>(toward)];
            if (SIGN(vx) != SIGN(step)) {
                step <<= 1;
            }
            vx += step;
            step = directionY16[static_cast<int>(toward)];
            if (SIGN(vy) != SIGN(step)) {
                step <<= 1;
            }
            vy += step;
        }
        if (expired != 0U) {
            vz--;
        } else if (last.z < aimZ) {
            vz++;
        } else if (last.z > aimZ) {
            vz--;
        }
        if (vx > 32) {
            vx = 32;
        } else if (vx < -32) {
            vx = -32;
        }
        if (vy > 32) {
            vy = 32;
        } else if (vy < -32) {
            vy = -32;
        }
        if (vz > 16) {
            vz = 16;
        } else if (vz < -16) {
            vz = -16;
        }
        nextX = static_cast<s32>(last.x) + vx;
        nextY = static_cast<s32>(last.y) + vy;
        nextZ = static_cast<s32>(last.z) + vz;
        if (fire == 9 && (expired == 0U)) {
            facing = ::direction(last.x, last.y, nextX, nextY);
            if (facing != NO_DIRECTION) {
                if (self.isValid()) {
                    self.setFrame(facing + 0x11);
                } else {
                    setSpriteFrame(sprite, facing + 0x11);
                }
            }
        }
    } else {
        nextX = start.x + counter * dx / steps;
        nextY = start.y + counter * dy / steps;
        nextZ = static_cast<s32>(start.z) + dz * counter / steps;
    }
    next.x = nextX;
    next.y = nextY;
    next.z = nextZ;
    if (nextZ <= 0) {
        next.z = 0;
    } else if (nextZ >= 0xfa) {
        next.z = 0xfa;
    }
    counter++;
    if (sprite == -1 && counter >= 2) {
        const ShapeInfo info = typeFlags.info(shape);
        sprite = createSprite(shape, frame, info.translucent ? 0x1000 : 0, now.x, now.y, now.z);
    }
    /* The shot stops at the floor, at the top of the map and when its time
     * is up. */
    if (next.z == 0 || next.z == 0xfa || static_cast<i16>(fireTypes[fire].roundDuration + 25) <= counter) {
        goto stopped;
    }
    if (!inFastArea(next)) {
        finish();
        return;
    }
    if (move(last, next)) {
        advance();
        /* No Remorse's fire types end at 0Fh (FIRE_TYPES). */
        if (fire == FIRE_BOSS_GRENADE && counter >= 6 && counter % 3 == 0) {
            explodeAt(&next);
        }
        if (fire == 9) {
            setTimeout(3);
            return;
        }
        setTimeout(2);
        return;
    }
    if (hitItem != 0U) {
        const Item other(hitItem);
        const u16 otherShape = other.shape();
        const ShapeInfo info = typeFlags.infos[otherShape];
        if (info.roof && fire == FIRE_PA && bounces < 5) {
            ItemPoint bounce;
            fireExplosion(fire, next.x, next.y, next.z);
            const u16 left = itemX[other.id] - (info.x * 16) + 1;
            const u16 right = itemX[other.id];
            if (now.x < left || now.x > right) {
                dx = -dx;
            } else {
                dy = -dy;
            }
            start = last;
            bounce = last;
            bounce.x += dx;
            bounce.y += dy;
            bounces++;
            direction = ::direction(last.x, last.y, bounce.x, bounce.y);
            launch(direction, bounce, damage, fire, source, targetItem, 0);
            if (self.isValid()) {
                self.setFrame(direction + 1);
                return;
            }
            setSpriteFrame(sprite, direction + 1);
            return;
        }
    }
    /* The shot stopped (the test above) or hit something. */
stopped:
    if (hitItem == source && counter < 5) {
        Item firer(source);
        firer.push();
        run();
        firer.pop();
        return;
    }
    hit(last, next);
}
