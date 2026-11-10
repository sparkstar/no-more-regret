/* Code11 (1050:00a6-039d, file name unknown): thrown and shot items. */
#include <cstdlib>
#include "missile.h"
#include "gravity.h"
#include "kernel.h"

MissileTracker::MissileTracker(ItemPoint from, ItemPoint to, i16 speed, i16 missileGravity)
{
    i16 dx;
    i16 dy;
    i16 dz;
    i16 frames;

    this->gravity = missileGravity;
    if (this->gravity < 0) {
        this->gravity = 0;
    }
    if (this->gravity == 0) {
        maxRange = 3000;
    } else {
        maxRange = speed * (static_cast<i16>(speed * 2) / this->gravity);
    }
    dx = to.x - from.x;
    dy = to.y - from.y;
    dz = to.z - from.z;
    range = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    range += abs(dx) + abs(dy);
    range >>= 1;
    frames = static_cast<i16>(range + (speed >> 1)) / speed;
    if (frames != 0) {
        speedZ = static_cast<i16>(dz + (static_cast<i16>(this->gravity * frames * (frames - 1)) >> 1)) / frames;
        if (speedZ > speed >> 2) {
            if ((this->gravity != 0) && (speed >> 2) / this->gravity > frames) {
                frames = (speed >> 2) / this->gravity;
            } else if (speed > 3 && dz / (speed >> 2) > frames) {
                frames = dz / (speed >> 2);
            }
            speedZ = static_cast<i16>(dz + (static_cast<i16>(this->gravity * frames * (frames - 1)) >> 1)) / frames;
        }
        speedX = static_cast<i16>(dx + (frames >> 1)) / frames;
        speedY = static_cast<i16>(dy + (frames >> 1)) / frames;
    } else {
        speedX = speedY = 0;
        speedZ = dz >= 0 ? speed >> 2 : -(speed >> 2);
    }
    reaches = static_cast<u8>(speedZ <= speed >> 2 && range <= maxRange);
}

void MissileTracker::launch(u16 item) const
{
    hurl(&lvalue(Item(item)), speedX, speedY, speedZ, gravity);
}

/* UNCALLED: nothing calls it. */
bool MissileTracker::test(u16 item, ItemPoint target) const
{
    Process *process = firstProcess(item, GRAVITY_TYPE);

    if (process != nullptr) {
        process->fail(0);
    }
    process = new GravityProcess(item, speedX, speedY, speedZ, gravity, 200, &target);
    process->status |= STATUS_CREATED_CHILD;
    process->run();
    const ItemPoint at = Item(item).point();
    const i16 distance =
        abs(at.z - target.z) + (abs(static_cast<i16>(at.x - target.x)) + abs(static_cast<i16>(at.y - target.y)));
    return distance < 50;
}
