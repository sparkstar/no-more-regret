/* Code10 (file name unknown; No Regret's 1048, No Remorse's 1038; the addresses
 * are No Regret's): what an item stands on, and items falling and thrown. */
#ifndef PORT_GRAVITY_H
#define PORT_GRAVITY_H
#include "common.h"
#include "item.h"
#include "kprocess.h"
#include "gravity_game.h"

class Npc;

/* The type of the process of a falling item. */
constexpr int GRAVITY_TYPE = 0x203;

/* The process of a falling or thrown item (1048:0395, vtable 1480:08da). */
class GravityProcess : public Process {
public:
    explicit GravityProcess(Loading loading) : Process(loading) {}
    i16 speedX;
    i16 speedY;
    i16 speedZ;
    i16 gravity;
    /* Sliding on a low friction item after a landing. */
    u8 sliding;
    /* Runs left; 0 for no limit. */
    i16 lifetime;
    u8 unknown3d;
    /* Where a thrown item should land; 0 for a fall. Then the process runs
     * steps until the item gets there or stops. */
    ItemPoint *target;
    /* The first run's checks are done (No Regret's only). */
    u8 started;

    GravityProcess(u16 processItem, i16 dx, i16 dy, i16 dz, i16 newGravity, i16 newLifetime, ItemPoint *targetPoint);
    void onKill() override; /* the original's destructor, 1048:04c1 */
    void run() override;    /* 1048:06d7 */
    /* run's steps (the port's): a block (bumped), the landing on the floor
     * (landOnFloor), a low-friction thing's turn by chance (turnByChance). */
    bool bumped(const Npc &moving, u16 blocked, bool onTop, bool controlled, u8 &landed);
    bool landOnFloor(const Npc &moving, bool onTop, bool controlled, u8 &landed);
    u16 turnByChance(const Npc &moving, u16 blocked);
    /* 1 when the item cannot move one step of its fall (1048:11da). */
    bool cannotFall();
    void accelerate(i16 dx, i16 dy, i16 dz)
    {
        speedX += dx;
        speedY += dy;
        speedZ += dz;
    }
};
/* BCC's: No Regret 43h, No Remorse 42h (without started). */
ASSERT_SIZE(GravityProcess, GRAVITY_PROCESS_SIZE);

/* The item starts to fall (1048:13eb). */
void fall(Item *item);
/* Puts the falling item where it lands and ends process (1048:12a6). */
void landNow(Process *process);
/* Support::flags: the corners of the item's base that stand on something
 * (north is y-1, east x+1), and SUPPORT_STANDING when two opposite corners
 * do. ANIMPRIM.C tests for a base held on one side only. */
constexpr int SUPPORT_NORTHEAST = 1;
constexpr int SUPPORT_SOUTHEAST = 2;
constexpr int SUPPORT_SOUTHWEST = 4;
constexpr int SUPPORT_NORTHWEST = 8;
constexpr int SUPPORT_STANDING = 0x10;
constexpr int SUPPORT_NORTH = (SUPPORT_NORTHEAST | SUPPORT_NORTHWEST);
constexpr int SUPPORT_SOUTH = (SUPPORT_SOUTHEAST | SUPPORT_SOUTHWEST);
constexpr int SUPPORT_EAST = (SUPPORT_NORTHEAST | SUPPORT_SOUTHEAST);
constexpr int SUPPORT_WEST = (SUPPORT_NORTHWEST | SUPPORT_SOUTHWEST);

/* What testSupport finds under an item. One byte (1048:0000 news 1). */
struct Support {
    u8 flags;

    Support() = default;
    /* Tests item where it is, as testSupport (1048:0000). */
    Support(u16 item, u8 close);
    /* The item stands on something. */
    [[nodiscard]] u8 standing() const { return flags & SUPPORT_STANDING; }
    /* The parts on stand on something and the parts off do not. */
    [[nodiscard]] bool only(i16 on, i16 off) const { return ((flags & on) != 0) && ((flags & off) == 0); }
};
ASSERT_SIZE(Support, 1);

/* Tests an item of shape at point, leaving item itself out (1048:0072).
 * With close, tops up to 8 below the base count too, but only when one top
 * meets the base. */
void testSupport(Support *support, i16 shape, const ItemPoint &point, u8 close, u16 item);
/* Adds dx,dy,dz to the speed of item's fall, starting one; the falling
 * process's pid (1048:134f). */
u16 hurl(Item *item, i16 dx, i16 dy, i16 dz, i16 gravity);
#endif
