/* ..\\ITEM\\NPC.C (Code32, No Regret's 10f8:0000-5815, No Remorse's 10e8):
 * the NPCs, their activities and animations, and the controlled NPC; both
 * games' ("NPC"), the addresses No Regret's, the game's values in
 * npc_game.h. */
#include <vector>
#include "gamestate.h"
#include <cstddef>
#include <cstdlib>
#include "npc.h"
#include "waits.h"
#include "item.h"
#include "type.h"
#include "itemcach.h"
#include "collide.h"
#include "kprocess.h"
#include "kernel.h"
#include "priority.h"
#include "uprocess.h"
#include "systimer.h"
#include "random.h"
#include "flex.h"
#include "animprim.h"
#include "activity.h"
#include "attack.h"
#include "pathfind.h"
#include "dtable.h"
#include "firetype.h"
#include "control.h"
#include "crusader.h"
#include "explode.h"
#include "camera.h"
#include "animcach.h"
#include "status.h"
#include "world.h"
#include "cprotmem.h"
#include "tracker.h"
#include "trace.h"
#include "shapes.h"

/* Set while the controlled NPC enters or leaves combat through
 * controlledEnterCombat and controlledLeaveCombat. Nothing here reads it. */
u8 &playerCombat = gameGlobal<u8>(0);
NpcData *&npcs = gameGlobal<NpcData *>();
ControlledNpc &controlledNpc = gameGlobal<ControlledNpc>();

void initNpcs()
{
    allocateNpcs();
}

void closeNpcs()
{
    freeNpcs();
}

void allocateNpcs()
{
    npcs = new NpcData[NPCS];
    memset(static_cast<void *>(npcs), 0, sizeof(NpcData) * NPCS);
    /* Item 1 is the Silencer. */
    itemShape[1] = 1;
    itemFrame[1] = 0;
    itemFlags[1] = 0;
    itemZ[1] = 0;
    itemQuality[1] = 0;
    itemNpc[1] = 1;
}

void clearNpcs()
{
    memset(static_cast<void *>(&npcs[1]), 0, sizeof(NpcData) * (NPCS - 1));
    resetNpcNumbers();
}

void freeNpcs()
{
    if (npcs != nullptr) {
        delete[] npcs;
        npcs = nullptr;
    }
}

namespace {
/* An NPC's record in the game's layout (NPC_DATA_SIZE, EQUIPMENT_SLOTS) from
 * the port's, whose equipment holds EQUIPMENT_SLOTS_CAPACITY slots, and
 * back: the bytes before the equipment, the game's slots, the bytes after
 * it. */
/* UNCALLED: nothing calls it. */
void packNpcData(const NpcData &npc, u8 *record)
{
    const auto *bytes = reinterpret_cast<const u8 *>(&npc);
    const std::size_t before = offsetof(NpcData, equipment) + (2 * EQUIPMENT_SLOTS);
    const std::size_t after = offsetof(NpcData, equipment) + (2 * EQUIPMENT_SLOTS_CAPACITY);
    memcpy(record, bytes, before);
    memcpy(record + before, bytes + after, sizeof(NpcData) - after);
}

/* UNCALLED: nothing calls it. */
void unpackNpcData(const u8 *record, NpcData *npc)
{
    auto *bytes = reinterpret_cast<u8 *>(npc);
    const std::size_t before = offsetof(NpcData, equipment) + (2 * EQUIPMENT_SLOTS);
    const std::size_t after = offsetof(NpcData, equipment) + (2 * EQUIPMENT_SLOTS_CAPACITY);
    memset(bytes, 0, sizeof(NpcData));
    memcpy(bytes, record, before);
    memcpy(bytes + after, record + before, sizeof(NpcData) - after);
}
}

void saveNpcs(Flex *flex, i16 index)
{
    if (sizeof(NpcData) == NPC_DATA_SIZE) {
        flex->writeObject(index, npcs, static_cast<ui16>(sizeof(NpcData) * NPCS), 0, 1);
        return;
    }
    std::vector<u8> records(static_cast<std::size_t>(NPC_DATA_SIZE) * NPCS);
    for (std::size_t i = 0; i < NPCS; i++) {
        packNpcData(npcs[i], &records[i * NPC_DATA_SIZE]);
    }
    flex->writeObject(index, records.data(), static_cast<ui16>(records.size()), 0, 1);
}

void loadNpcs(Flex *flex, i16 index)
{
    if (sizeof(NpcData) == NPC_DATA_SIZE) {
        flex->readObject(index, npcs, static_cast<ui16>(sizeof(NpcData) * NPCS), 0);
        return;
    }
    std::vector<u8> records(static_cast<std::size_t>(NPC_DATA_SIZE) * NPCS);
    flex->readObject(index, records.data(), static_cast<ui16>(records.size()), 0);
    for (std::size_t i = 0; i < NPCS; i++) {
        unpackNpcData(&records[i * NPC_DATA_SIZE], &npcs[i]);
    }
}

bool Npc::isBusy()
{
    return itemHasType(id, ANIM_TYPE);
}

/* The NPC is on the base's map (BASE_MAP: No Regret's 25, No Remorse's 40). */
bool Npc::onBaseMap()
{
    const u16 map = npcMap(&id);
    return map == BASE_MAP;
}

void Npc::applyType(i16 type)
{
    const NpcType info = npcTypes[type];
    const u16 weapon = npcTypes[type].weaponType;

    if (weapon != 0U) {
        Item item;
        item.create(weaponTypes[weapon].shape, 0);
        item.popToCoords(itemX[id], itemY[id], itemZ[id]);
        const i16 slot = weaponSlot();
        setEquip(slot, item.id);
        setActiveWeapon(slot);
    }
    setDefaultActivity0(info.defaultActivity0);
    setDefaultActivity1(info.defaultActivity1);
    setDefaultActivity2(info.defaultActivity2);
}

void Npc::setAttackTarget(u16 value)
{
    if (isDead() != 0U) {
        return;
    }
    if (!isSilencer() && !isControlled()) {
        if (isInCombat() == 0U) {
            killItemExcept(id, Priority(PRIORITY_TESTS), ATTACK_TYPE, TYPE_END);
            setActivity(ACTIVITY_ATTACK);
        }
        npcs[id].attackTarget = value;
    }
}

u8 Npc::isFalling()
{
    return npcs[id].falling;
}

u8 Npc::isInCombat()
{
    return npcs[id].inCombat;
}

bool Npc::isActing()
{
    const i16 anim = lastAnimSet();
    return anim != ANIM_WALK && anim != ANIM_STAND && anim != ANIM_QUICK_JUMP && anim != ANIM_UNREADY_SMALL &&
           anim != ANIM_UNREADY_LARGE;
}

bool Npc::isSliding()
{
    const i16 anim = lastAnimSet();
    return anim == ANIM_SLIDE_LEFT || anim == ANIM_SLIDE_RIGHT;
}

bool Npc::recentlyHit()
{
    const u32 now = systemTimer->ticks;
    const u32 hit = npcs[id].hitTime;
    return hit != 0 && hit + 0x78 >= now;
}

u8 Npc::isDead()
{
    if (id != 0 && id < NPCS) {
        return npcs[id].dead;
    }
    return 1;
}

void Npc::setDead()
{
    setBroken();
    npcs[id].dead = 1;
}

void Npc::clearDead()
{
    clearBroken();
    npcs[id].dead = 0;
}

/* UNCALLED: nothing calls it. */
u16 Npc::pathfind(u16 x, u16 y, u8 z, u16 steps)
{
    PathFinder *finder = new PathFinder(id, 0, ItemPoint(x, y, z), steps, 0, 0x40, 1);
    return finder->pid;
}

/* UNCALLED: nothing calls it. */
u16 Npc::pathfind(u16 item, u16 steps)
{
    PathFinder *finder = new PathFinder(id, item, steps, 0, 0x40, 1);
    return finder->pid;
}

/* A free NPC number, 0 for none (10f8:2380). */
u16 newNpcNumber()
{
    for (i16 npc = 1; npc < NPCS; npc++) {
        if (!npcs[npc].used) {
            npcs[npc].used = 1;
            return npc;
        }
    }
    return 0;
}

void freeNpcNumber(Item *npc)
{
    npcs[npc->id].used = 0;
}

u8 Npc::isImmortal()
{
    return npcs[id].immortal;
}

void Npc::setImmortal()
{
    npcs[id].immortal = 1;
}

void Npc::clearImmortal()
{
    npcs[id].immortal = 0;
}

bool Npc::isRunning()
{
    const i16 anim = lastAnimSet();
    return anim == ANIM_RUN || anim == ANIM_COMBAT_RUN_SMALL || anim == ANIM_COMBAT_RUN_LARGE ||
           anim == ANIM_JUMP_FORWARD || anim == ANIM_ROLL_FORWARD;
}

bool Npc::isStartingRun()
{
    const i16 anim = lastAnimSet();
    return anim == ANIM_START_RUN || anim == ANIM_START_RUN_SMALL || anim == ANIM_START_RUN_LARGE;
}

u8 Npc::isKneeling()
{
    return npcs[id].kneeling;
}

void Npc::kneel()
{
    if (isSilencer()) {
        typeFlags.infos[shape()].z = 6;
        npcs[id].kneeling = 1;
    }
}

void Npc::standUp()
{
    if (isSilencer()) {
        typeFlags.infos[shape()].z = 10;
        npcs[id].kneeling = 0;
    }
}

u8 Npc::unknown77Bit5()
{
    return npcs[id].unknown77_5;
}

void Npc::setUnknown77Bit5()
{
    npcs[id].unknown77_5 = 1;
}

void Npc::clearUnknown77Bit5()
{
    npcs[id].unknown77_5 = 0;
}

u8 Npc::isStopped()
{
    return npcs[id].stopped;
}

void Npc::setStopped()
{
    npcs[id].stopped = 1;
}

void Npc::clearStopped()
{
    npcs[id].stopped = 0;
}

bool Npc::allocate(u16 shape, u16 frame)
{
    id = newNpc();
    if (id != 0U) {
        itemShape[id] = shape;
        itemFrame[id] = frame;
        itemNpc[id] = id;
        itemMap[id] = currentMap;
        setAttackTarget(0);
        setFrameHigh(frame >> 8);
    } else {
        for (i16 npc = 2; npc < NPCS; npc++) {
            ItemRef item(npc);
            if (item.z() == ETHEREAL_Z) {
                continue;
            }
            /* The NPCs whose numbers are never taken (No Remorse's). */
            if (isOneOf(item.shape(), KEPT_NPC_SHAPES)) {
                continue;
            }
            if (item.inFastArea() == 0U) {
                id = npc;
                item.destroy();
                itemShape[id] = shape;
                itemFrame[id] = frame;
                itemNpc[id] = id;
                itemMap[id] = currentMap;
                setAttackTarget(0);
                setFrameHigh(frame >> 8);
                break;
            }
        }
    }
    return isValid();
}

bool Npc::create(u16 shape, i16 frame)
{
    if (allocate(shape, frame)) {
        reset();
        if (isOneOf(shape, FEMALE_SHAPES)) {
            npcs[id].female = 1;
        }
    }
    return isValid();
}

bool Npc::createActor(u16 shape, u16 frame)
{
    if (allocate(shape, frame)) {
        reset();
        applyType(npcType(this->shape()));
    }
    return isValid();
}

void Npc::reset()
{
    i16 type;
    const u16 frame = this->frame();

    memset(static_cast<void *>(&npcs[id]), 0, sizeof(NpcData));
    setFrame(frame);
    type = npcType(shape());
    setHp(npcTypes[type].maxHp);
}

/* UNCALLED: nothing calls it. */
bool Npc::createAtPoint(u16 shape, u16 frame, ItemPoint *point)
{
    if (shape >= SHAPES) {
        return false;
    }
    Sweep sweep;
    if (sweep.fits(shape, point, 0)) {
        createActor(shape, frame);
        popToCoords(point->x, point->y, point->z);
        return true;
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool Npc::createAtCoords(u16 shape, u16 frame, u16 x, u16 y, u8 z)
{
    if (shape >= SHAPES) {
        return false;
    }
    ItemPoint point(x, y, z);
    Sweep sweep;
    if (sweep.fits(shape, &point, 0)) {
        createActor(shape, frame);
        popToCoords(point.x, point.y, point.z);
        return true;
    }
    return false;
}

u16 Npc::equipment(i16 slot)
{
    return npcs[id].equipment[slot];
}

bool Npc::setEquip(u16 slot, u16 item)
{
    Item equip(item);

    if (id == item || (equipment(slot) != 0U)) {
        return false;
    }
    switch (equip.family()) {
    case FAMILY_WEAPON:
        if (activeWeapon() == 0U) {
            setActiveWeapon(slot);
        }
        break;
    case FAMILY_AMMO:
    case FAMILY_INVENTORY:
        updateStatus(STATUS_WEAPON);
        updateStatus(STATUS_AMMO);
        updateStatus(STATUS_HEALTH);
        updateStatus(STATUS_ENERGY);
        updateStatus(STATUS_ITEM);
        break;
    }
    equip.push();
    equip.popToContainer(id);
    npcs[id].equipment[slot] = item;
    ItemRef(item).setZ(slot);
    equip.orFlags(ITEM_EQUIPPED);
    return true;
}

void Npc::clearSlot(i16 slot)
{
    npcs[id].equipment[slot] = 0;
}

void Npc::removeEquip(u16 item)
{
    NpcData &npc = npcs[id];
    for (i16 slot = 0; slot < EQUIPMENT_SLOTS; slot++) {
        if (npc.equipment[slot] == item) {
            npc.equipment[slot] = 0;
        }
    }
}

i16 Npc::slotOf(u16 item)
{
    const NpcData &npc = npcs[id];
    i16 slot;
    for (slot = 0; slot < EQUIPMENT_SLOTS; slot++) {
        if (npc.equipment[slot] == item) {
            break;
        }
    }
    return slot;
}

void Npc::clearEquip()
{
    NpcData &npc = npcs[id];
    i16 slot;
    for (slot = 0; slot < EQUIPMENT_SLOTS; slot++) {
        npc.equipment[slot] = 0;
    }
}

namespace {
/* What an animation does besides its process (Npc::doAnim): its frames
 * need no fresh load (the reticle stays shown: loaded), it hands back to
 * the controls when it ends (handBack), and the controlled NPC's dodge it
 * starts: its ticks and factor (dodgeLength 0: none). */
struct AnimStart {
    bool loaded = false;
    bool handBack = false;
    u32 dodgeLength = 0;
    i16 dodgeFactor = 0;
};

AnimStart animStart(u16 anim)
{
    switch (anim) {
    case ANIM_RELOAD_LARGE:
    case ANIM_FIRE_SMALL:
    case ANIM_FIRE_LARGE:
    case ANIM_BRIGHT_KNEEL_FIRE:
    case ANIM_BRIGHT_FIRE_LARGE:
    case ANIM_READY_SMALL:
    case ANIM_READY_LARGE:
    case 6:
    case ANIM_COMBAT_STAND_SMALL:
    case ANIM_COMBAT_STAND_LARGE:
    case ANIM_ADVANCE_SMALL:
    case ANIM_ADVANCE_LARGE:
    case ANIM_RETREAT_LARGE:
    case ANIM_KNEEL_FIRE_SMALL:
    case ANIM_KNEEL_FIRE_LARGE:
    case ANIM_KNEELING_SMALL:
    case ANIM_KNEELING_LARGE:
        return {.loaded = true};
    case ANIM_RETREAT_SMALL:
        return {.loaded = true, .dodgeLength = 0x78, .dodgeFactor = 3};
    case ANIM_KNEEL_START:
    case ANIM_KNEEL_END:
        return {.loaded = true, .dodgeLength = 0x4b, .dodgeFactor = 3};
    case ANIM_QUICK_JUMP:
    case ANIM_JUMP_FORWARD:
    case ANIM_RUN:
    case ANIM_START_RUN:
    case ANIM_START_RUN_SMALL:
    case ANIM_START_RUN_LARGE:
        return {.dodgeLength = 0x78, .dodgeFactor = 2};
    case ANIM_WALK:
        return {.dodgeLength = 0x78, .dodgeFactor = 3};
    case ANIM_STOP_RUN_LARGE:
    case ANIM_STOP_RUN_SMALL:
    case ANIM_COMBAT_ROLL_LEFT:
    case ANIM_COMBAT_ROLL_RIGHT:
    case ANIM_KNEEL_ROLL_LEFT:
    case ANIM_KNEEL_ROLL_RIGHT:
        return {.handBack = true, .dodgeLength = 0x78, .dodgeFactor = 2};
    case ANIM_SLIDE_LEFT:
    case ANIM_SLIDE_RIGHT:
        return {.handBack = true, .dodgeLength = 0x3c, .dodgeFactor = 3};
    case ANIM_ROLL_FORWARD:
        /* No Remorse's keeps the reticle and does not dodge. */
        if (ROLL_FORWARD_DODGES) {
            return {.handBack = true, .dodgeLength = 0x78, .dodgeFactor = 2};
        }
        return {.loaded = true};
    case ANIM_KNEELING_SLIDE_RIGHT:
        return {.handBack = true};
    case ANIM_KNEELING_RETREAT:
    case ANIM_KNEELING_ADVANCE:
    case ANIM_KNEELING_SLIDE_LEFT:
        return {.handBack = KNEELING_MOVES_HAND_BACK};
    case ANIM_COMBAT_RUN_SMALL:
    case ANIM_COMBAT_RUN_LARGE:
        if (COMBAT_RUN_DODGES) {
            return {.dodgeLength = 0x78, .dodgeFactor = 2};
        }
        return {};
    default:
        return {};
    }
}

/* The process of animation anim of npc toward direction; nullptr for an
 * animation the game has none of. */
AnimPrimitive *newAnimProcess(u16 anim, u16 npc, u16 direction)
{
    switch (anim) {
    case 6:
        return new Anim06(npc, direction);
    case 0x11:
        return new Anim11(npc, direction);
    case 0x13:
        return new Anim13(npc, direction);
    case 0x16:
        return new Anim16(npc, direction);
    case ANIM_RELOAD_LARGE:
        return new ReloadLargeAnim(npc, direction);
    case ANIM_SURRENDER_STAND:
        return new SurrenderStandAnim(npc, direction);
    case ANIM_JUMP_LANDING:
        return new JumpLandingAnim(npc, direction);
    case ANIM_STOP_RUN_LARGE:
        return new StopRunLargeAnim(npc, direction);
    case ANIM_FINISH_FIRING:
        return new FinishFiringAnim(npc, direction);
    case ANIM_QUICK_JUMP:
        return new QuickJumpAnim(npc, direction);
    case ANIM_JUMP_FORWARD:
        return new JumpForwardAnim(npc, direction);
    case ANIM_UNREADY_SMALL:
        return new UnreadySmallAnim(npc, direction);
    case ANIM_SURRENDER:
        return new SurrenderAnim(npc, direction);
    case ANIM_UNREADY_LARGE:
        return new UnreadyLargeAnim(npc, direction);
    case ANIM_FIRE_SMALL:
        return new FireSmallAnim(npc, direction);
    case ANIM_FIRE_LARGE:
        return new FireLargeAnim(npc, direction);
    case ANIM_ELECTROCUTED:
        return new ElectrocutedAnim(npc, direction);
    case ANIM_BRIGHT_KNEEL_FIRE:
        return new BrightKneelFireAnim(npc, direction);
    case ANIM_BRIGHT_FIRE_LARGE:
        return new BrightFireLargeAnim(npc, direction);
    case ANIM_READY_SMALL:
        return new ReadySmallAnim(npc, direction);
    case ANIM_READY_LARGE:
        return new ReadyLargeAnim(npc, direction);
    case ANIM_WALK:
        return new WalkAnim(npc, direction);
    case ANIM_RETREAT_SMALL:
        return new RetreatSmallAnim(npc, direction);
    case ANIM_RUN:
        return new RunAnim(npc, direction);
    case ANIM_STAND:
        return new StandAnim(npc, direction);
    case ANIM_SLIDE_LEFT:
        return new SlideLeftAnim(npc, direction);
    case ANIM_SLIDE_RIGHT:
        return new SlideRightAnim(npc, direction);
    case ANIM_COMBAT_ROLL_LEFT:
        return new CombatRollLeftAnim(npc, direction);
    case ANIM_COMBAT_ROLL_RIGHT:
        return new CombatRollRightAnim(npc, direction);
    case ANIM_KNEEL_ROLL_LEFT:
        return new KneelRollLeftAnim(npc, direction);
    case ANIM_KNEEL_ROLL_RIGHT:
        return new KneelRollRightAnim(npc, direction);
    case ANIM_ROLL_FORWARD:
        return new RollForwardAnim(npc, direction);
    case ANIM_TELEPORT_IN:
        return new TeleportInAnim(npc, direction);
    case ANIM_TELEPORT_OUT:
        return new TeleportOutAnim(npc, direction);
    case ANIM_COMBAT_STAND_SMALL:
        return new CombatStandSmallAnim(npc, direction);
    case ANIM_COMBAT_STAND_LARGE:
        return new CombatStandLargeAnim(npc, direction);
    case ANIM_LOOK_LEFT:
        return new LookLeftAnim(npc, direction);
    case ANIM_LOOK_RIGHT:
        return new LookRightAnim(npc, direction);
    case ANIM_KNEELING_SLIDE_RIGHT:
        return new KneelingSlideRightAnim(npc, direction);
    case ANIM_KNEELING_RETREAT:
        return new KneelingRetreatAnim(npc, direction);
    case ANIM_KNEELING_ADVANCE:
        return new KneelingAdvanceAnim(npc, direction);
    case ANIM_KNEELING_SLIDE_LEFT:
        return new KneelingSlideLeftAnim(npc, direction);
    case ANIM_FALL_BACKWARDS:
        return new FallBackwardsAnim(npc, direction);
    case ANIM_FALL_FORWARDS:
        return new FallForwardsAnim(npc, direction);
    case ANIM_ADVANCE_SMALL:
        return new AdvanceSmallAnim(npc, direction);
    case ANIM_ADVANCE_LARGE:
        return new AdvanceLargeAnim(npc, direction);
    case ANIM_RETREAT_LARGE:
        return new RetreatLargeAnim(npc, direction);
    case ANIM_START_RUN:
        return new StartRunAnim(npc, direction);
    case ANIM_STOP_RUN_SMALL:
        return new StopRunSmallAnim(npc, direction);
    case ANIM_START_RUN_SMALL:
        return new StartRunSmallAnim(npc, direction);
    case ANIM_START_RUN_LARGE:
        return new StartRunLargeAnim(npc, direction);
    case ANIM_KNEEL_START:
        return new KneelStartAnim(npc, direction);
    case ANIM_KNEEL_END:
        return new KneelEndAnim(npc, direction);
    case ANIM_KNEEL_FIRE_SMALL:
        return new KneelFireSmallAnim(npc, direction);
    case ANIM_KNEEL_FIRE_LARGE:
        return new KneelFireLargeAnim(npc, direction);
    case ANIM_KNEELING_SMALL:
        return new KneelingSmallAnim(npc, direction);
    case ANIM_KNEELING_LARGE:
        return new KneelingLargeAnim(npc, direction);
    case ANIM_COMBAT_RUN_SMALL:
        return new CombatRunSmallAnim(npc, direction);
    case ANIM_COMBAT_RUN_LARGE:
        return new CombatRunLargeAnim(npc, direction);
    default:
        return nullptr;
    }
}

/* The NPC kneels in anim (KNEELING_ANIMS, ended by -1). */
bool kneelsIn(u16 anim)
{
    for (i16 i = 0; KNEELING_ANIMS[i] != -1; i++) {
        if (static_cast<ui16>(KNEELING_ANIMS[i]) == anim) {
            return true;
        }
    }
    return false;
}
}

u16 Npc::doAnim(u16 anim, u16 direction, u16 maxBusy, u8 kill)
{
    if (inVoid()) {
        return 0;
    }
    anim = npcAnim(this, anim);
    if (isControlled() && (selectingItems != 0U)) {
        avatarMoved();
    }
    if ((isDead() != 0U) || (isStopped() != 0U) || !hasAnim(shape(), anim)) {
        return 0;
    }
    const u16 busy = (maxBusy == 0) ? static_cast<u16>(isBusy()) : countProcesses(id, ANIM_TYPE);
    if (busy > maxBusy) {
        return 0;
    }
    if (kill != 0U) {
        killItem(id, ANIM_TYPE);
    }
    if (kneelsIn(anim)) {
        kneel();
    } else {
        standUp();
    }
    const u32 time = systemTimer->ticks;
    /* No Remorse's has no animation 16h nor the kneeling slide to the right
     * (kept). */
    if (!ANIM_16_AND_SLIDE_RIGHT && (anim == 0x16 || anim == ANIM_KNEELING_SLIDE_RIGHT)) {
        halt();
    }
    const AnimStart start = animStart(anim);
    if (start.dodgeLength != 0U && isControlled()) {
        dodgeStart = time;
        dodgeLength = start.dodgeLength;
        dodgeFactor = start.dodgeFactor;
    }
    if (anim == ANIM_UNREADY_SMALL || anim == ANIM_UNREADY_LARGE) {
        npcs[id].weaponDrawn = 0;
    } else if (anim == ANIM_READY_SMALL || anim == ANIM_READY_LARGE) {
        npcs[id].weaponDrawn = 1;
    }
    AnimPrimitive *process = newAnimProcess(anim, id, direction);
    if (process == nullptr) {
        halt();
    }
    if (isControlled() || isOneOf(shape(), RETICLE_NPC_SHAPES)) {
        if (start.loaded) {
            showReticle();
        } else {
            hideReticle();
            controller->stopAllFiring();
        }
        if (start.handBack) {
            process->holdReticle();
        }
    }
    return process->pid;
}

namespace {
void teleported(u16 npc, u8 map);
}

void Npc::teleport(u16 x, u16 y, u8 z, u8 map)
{
    traceTeleport(id, x, y, z, map);
    if (currentMap != 0 && static_cast<ui16>(itemMap[id]) == currentMap) {
        if (static_cast<ui16>(map) != currentMap) {
            leaveFastArea();
        }
        pushItem(id);
        popToNowhere(0);
    }
    if (TELEPORT_COMPACTS_MEMORY) {
        compactMemory(0);
    }
    itemX[id] = x;
    itemY[id] = y;
    itemZ[id] = z;
    itemMap[id] = map;
    if (TELEPORT_RESTARTS_TRACKER) {
        closeTracker();
        initTracker();
    }
    if (isSilencer() && followedItem == 1) {
        moveCamera(x, y, z, map);
        if (waitOpened()) {
            /* After the map's loading's wait. */
            const u16 npc = id;
            giveRest([npc, map]() {
                teleported(npc, map);
                return false;
            });
            return;
        }
    }
    teleported(id, map);
}

namespace {
/* Npc::teleport's after the camera: on the map it is, it is put back. */
void teleported(u16 npc, u8 map)
{
    if (static_cast<ui16>(map) == currentMap) {
        pushItem(npc);
        popItem();
    }
}
}

void Npc::setActivity(u16 activity)
{
    Attack *attack;
    u16 pid;
    u16 current;
    Process *anim;

    if (isDead() == 0 && isActivityLocked() == 0) {
        if (isControlled()) {
            return;
        }
        if (activity == ACTIVITY_DEFAULT0) {
            activity = defaultActivity0();
        } else if (activity == ACTIVITY_DEFAULT1) {
            activity = defaultActivity1();
        } else if (activity == ACTIVITY_DEFAULT2) {
            activity = defaultActivity2();
        }
        if (controlledNpc.difficulty == 4 && oneIn(2)) {
            if (activity == ACTIVITY_ATTACK) {
                activity = ACTIVITY_AGGRESSIVE_ATTACK;
            } else if (activity == ACTIVITY_KNEEL_ATTACK) {
                activity = ACTIVITY_AGGRESSIVE_KNEEL_ATTACK;
            }
        }
        current = this->activity();
        if (activity != 0U) {
            if (current == activity) {
                return;
            }
            if ((isInCombat() != 0U) && activity != ACTIVITY_FEINT_ATTACK &&
                (!FEINTING_NPC_MAY_CHANGE || current != ACTIVITY_FEINT_ATTACK)) {
                return;
            }
            npcs[id].lastActivity = npcs[id].activity;
            npcs[id].activity = activity;
            if (isInCombat() != 0U) {
                clrInCombat();
            }
            if (inFastArea() != 0U) {
                killItemTypes(id, Priority(PRIORITY_EQUAL | BASE_LEVEL), PATH_FINDER_TYPE, ANIM_TYPE, STAND_TYPE,
                              LOITER_TYPE, PACE_TYPE, ATTACK_TYPE, SURRENDER_TYPE, GUARD_TYPE, TYPE_END);
                if (NEW_ACTIVITY_PUTS_WEAPON_AWAY) {
                    npcs[id].weaponDrawn = 0;
                }
                switch (activity) {
                case ACTIVITY_STAND:
                    new Stand(id);
                    return;
                case ACTIVITY_PACE:
                    new Pace(id);
                    return;
                case ACTIVITY_LOITER:
                    new Loiter(id, -1);
                    return;
                case ACTIVITY_ROLLING_THUNDER:
                    if (!ROLLING_THUNDER) {
                        new Stand(id);
                        return;
                    }
                    setInCombat();
                    npcs[id].weaponDrawn = 1;
                    new RollingThunder(id);
                    return;
                case 4:
                    break;
                case ACTIVITY_ATTACK:
                case ACTIVITY_KNEEL_ATTACK:
                case ACTIVITY_AGGRESSIVE_ATTACK:
                case ACTIVITY_AGGRESSIVE_KNEEL_ATTACK:
                case ACTIVITY_FEINT_ATTACK:
                    setInCombat();
                    attack = new Attack(id);
                    if (current == ACTIVITY_GUARD) {
                        attack->fromGuard = 1;
                    }
                    if (activity == ACTIVITY_FEINT_ATTACK) {
                        attack->startFeint();
                    }
                    if (activity == ACTIVITY_KNEEL_ATTACK || activity == ACTIVITY_AGGRESSIVE_KNEEL_ATTACK) {
                        npcs[id].weaponDrawn = 1;
                        attack->kneeling = 1;
                        pid = doAnim(ANIM_READY_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
                    } else {
                        if (STANDING_PUTS_WEAPON_AWAY) {
                            npcs[id].weaponDrawn = 0;
                        }
                        pid = doAnim(ANIM_STAND, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
                    }
                    if (pid != 0U) {
                        anim = findProcess(pid);
                        anim->block(attack);
                    }
                    if (activity == ACTIVITY_AGGRESSIVE_ATTACK || activity == ACTIVITY_AGGRESSIVE_KNEEL_ATTACK) {
                        attack->aggressive = 1;
                        return;
                    }
                    break;
                case 6:
                    break;
                case ACTIVITY_SURRENDER:
                    new Surrender(id);
                    return;
                case ACTIVITY_GUARD:
                    new Guard(id);
                    return;
                default:
                    new Stand(id);
                }
            }
        }
    }
}

i16 Npc::weaponSlot()
{
    for (i16 slot = WEAPON_SLOTS.first; slot <= WEAPON_SLOTS.last; slot++) {
        if (equipment(slot) == 0U) {
            return slot;
        }
    }
    return 0;
}

i16 Npc::utilitySlot()
{
    for (i16 slot = UTILITY_SLOTS.first; slot <= UTILITY_SLOTS.last; slot++) {
        if (equipment(slot) == 0U) {
            return slot;
        }
    }
    return 0;
}

i16 Npc::noSlot()
{
    return 0;
}

i16 Npc::packSlot()
{
    for (i16 slot = PACK_SLOTS.first; slot <= PACK_SLOTS.last; slot++) {
        if (equipment(slot) == 0U) {
            return slot;
        }
    }
    return 0;
}

u16 npcMap(u16 *npc)
{
    return itemMap[*npc];
}

/* UNCALLED: nothing calls it. */
u16 Npc::activityEvent(u32 activity)
{
    u16 result;
    if (callEvent(this, 1L << 3, 3, &result, activity)) {
        return result;
    }
    setActivity(activity);
    return 0;
}

void Npc::setDefaultActivity0(u16 activity)
{
    npcs[id].defaultActivities[0] = activity;
}

void Npc::setDefaultActivity1(u16 activity)
{
    npcs[id].defaultActivities[1] = activity;
}

void Npc::setDefaultActivity2(u16 activity)
{
    npcs[id].defaultActivities[2] = activity;
}

void Npc::setUnknown12(u8 value)
{
    npcs[id].unknown12 = value;
}

u8 Npc::unknown12()
{
    return npcs[id].unknown12;
}

u16 Npc::defaultActivity0()
{
    return npcs[id].defaultActivities[0];
}

u16 Npc::defaultActivity1()
{
    return npcs[id].defaultActivities[1];
}

u16 Npc::defaultActivity2()
{
    return npcs[id].defaultActivities[2];
}

void Npc::setCurrentActivity(u16 activity)
{
    npcs[id].lastActivity = npcs[id].activity;
    npcs[id].activity = activity;
}

bool Npc::isActivity(u16 activity)
{
    if (activity == ACTIVITY_DEFAULT0) {
        activity = defaultActivity0();
    } else if (activity == ACTIVITY_DEFAULT1) {
        activity = defaultActivity1();
    } else if (activity == ACTIVITY_DEFAULT2) {
        activity = defaultActivity2();
    }
    if (npcs[id].activity == activity) {
        return true;
    }
    return false;
}

/* UNCALLED: nothing calls it. */
u16 Npc::schedule(u32 time)
{
    u16 result;
    if (isDead() != 0U) {
        return 0;
    }
    if ((inFastArea() == 0U) && (isInCombat() != 0U)) {
        clrInCombat();
    }
    if (callEvent(this, 1L << 8, 8, &result, time)) {
        return result;
    }
    return 0;
}

i16 Npc::weaponDamage(u8 second)
{
    i16 damage = 0;
    Item weapon;
    u16 shape;
    i16 type;

    if (second != 0U) {
        weapon = activeItem2();
    } else {
        weapon = activeWeapon();
    }
    if (weapon.isValid()) {
        shape = weapon.shape();
        type = weaponOf(shape);
        damage = damageOf(type);
    }
    return damage;
}

/* The process it returns is always 0. */
ProcessId Npc::setInCombat()
{
    ProcessId result;
    result.pid = 0;
    npcs[id].inCombat = 1;
    return result;
}

void Npc::clrInCombat()
{
    Process *combat;

    npcs[id].inCombat = 0;
    if (!isControlled() && (inFastArea() != 0U) && (isWeaponDrawn() != 0U)) {
        doAnim(ANIM_UNREADY_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 0);
    }
    if (isControlled()) {
        return;
    }
    if (inFastArea() != 0U) {
        combat = firstProcess(id, ATTACK_TYPE);
        if (combat != nullptr) {
            combat->terminate(0);
        }
    }
}

u16 Npc::turnToward(u8 direction, u8 speed)
{
    return turnNpc(id, direction, speed, 0);
}

void Npc::enterCombat()
{
    playerCombat = 1;
    [[maybe_unused]] const ProcessId pid = setInCombat();
    playerCombat = 0;
}

void Npc::leaveCombat()
{
    playerCombat = 1;
    clrInCombat();
    playerCombat = 0;
}

/* The controlled NPC enters and leaves combat (10f8:39b8, 39c8), and is in
 * combat (39d8). */
void controlledEnterCombat()
{
    controlledNpc.enterCombat();
}

/* UNCALLED: nothing calls it. */
void controlledLeaveCombat()
{
    controlledNpc.leaveCombat();
}

u16 controlledInCombat()
{
    return controlledNpc.isInCombat();
}

u16 Npc::hp()
{
    return npcs[id].hp;
}

u16 Npc::activity()
{
    return npcs[id].activity;
}

u16 Npc::activeWeaponSlot()
{
    return npcs[id].activeSlots[0];
}

u16 Npc::activeSlot2()
{
    return npcs[id].activeSlots[1];
}

u16 Npc::activeSlot3()
{
    return npcs[id].activeSlots[2];
}

u16 Npc::maxHp()
{
    if (id == 1) {
        return 150;
    }
    const i16 type = npcType(itemShape[id]);
    return npcTypes[type].maxHp;
}

u16 Npc::activeWeapon()
{
    const u16 slot = npcs[id].activeSlots[0];
    if (slot == 0U) {
        return 0;
    }
    return equipment(slot);
}

u16 Npc::activeItem2()
{
    const u16 slot = npcs[id].activeSlots[1];
    if (slot == 0U) {
        return 0;
    }
    return equipment(slot);
}

u16 Npc::activeItem3()
{
    const u16 slot = npcs[id].activeSlots[2];
    if (slot == 0U) {
        return 0;
    }
    return equipment(slot);
}

u16 Npc::mana()
{
    return npcs[id].mana;
}

void Npc::setMana(u16 mana)
{
    npcs[id].mana = mana;
}

u16 Npc::maxEnergy()
{
    if (id != 1) {
        return 0;
    }
    switch (controlledNpc.battery) {
    case 0:
        return 0;
    case 1:
        return 2500;
    case 2:
        return 5000;
    case 3:
        return 10000;
    }
    return 0;
}

void Npc::recharge(u8 unknown)
{
    if (unknown != 0U) {
        createBatteryCharger();
    } else {
        setMana(maxEnergy());
    }
}

i16 Npc::shieldType()
{
    return npcs[id].shieldType;
}

void Npc::setShieldType(u16 type)
{
    npcs[id].shieldType = type;
    updateStatus(STATUS_ITEM);
}

u16 Npc::batteryType()
{
    return controlledNpc.battery;
}

void Npc::setBatteryType(u16 type)
{
    controlledNpc.battery = type;
    setMana(maxEnergy());
}

void Npc::setHp(u16 hp)
{
    npcs[id].hp = hp;
}

u16 Npc::combatTactic()
{
    return npcs[id].combatTactic;
}

void Npc::setCombatTactic(u16 tactic)
{
    npcs[id].combatTactic = tactic;
}

void Npc::setHome(ItemPoint home)
{
    npcs[id].home = home;
}

ItemPoint Npc::home()
{
    return npcs[id].home;
}

void Npc::setActiveWeapon(u16 slot)
{
    npcs[id].activeSlots[0] = slot;
    updateStatus(STATUS_WEAPON);
}

void Npc::setActiveSlot2(u16 slot)
{
    npcs[id].activeSlots[1] = slot;
}

void Npc::setActiveSlot3(u16 slot)
{
    npcs[id].activeSlots[2] = slot;
}

/* The controlled NPC is held still (10f8:3d8e). */
u16 isHeld()
{
    return controlledNpc.held;
}

void setHeld(u16 held)
{
    controlledNpc.held = held;
    if ((controlledNpc.held != 0U) && (controlledNpc.isInCombat() != 0U) && (controlledNpc.isDead() == 0U)) {
        controlledNpc.leaveCombat();
    }
}

/* UNCALLED: nothing calls it. */
void holdControlledNpc()
{
    setHeld(1);
}

void releaseControlledNpc()
{
    setHeld(0);
}

void Npc::setDirection(u8 direction)
{
    npcs[id].direction = direction;
}

char Npc::direction()
{
    return npcs[id].direction;
}

u16 Npc::lastAnimSet()
{
    return npcs[id].lastAnimSet;
}

bool Npc::addHp(i16 delta)
{
    const i16 most = maxHp();
    i16 hp = this->hp();
    if (hp == most) {
        return false;
    }
    hp += delta;
    if (hp > most) {
        hp = most;
    }
    setHp(hp);
    updateStatus(STATUS_HEALTH);
    return true;
}

bool Npc::addMana(i16 delta)
{
    const i16 most = maxEnergy();
    i16 mana = this->mana();
    if (mana == most && delta >= 0) {
        return false;
    }
    mana += delta;
    if (mana > most) {
        mana = most;
    } else if (mana < 0) {
        mana = 0;
    }
    setMana(mana);
    updateStatus(STATUS_ENERGY);
    return true;
}

ControlledNpc::ControlledNpc() : Npc(1)
{
    difficulty = 3;
}

/* The credits of the controlled NPC (10f8:3f2b, 3f3c). */
s32 controlledCredits()
{
    return controlledNpc.credits();
}

void setControlledCredits(u16 credits)
{
    controlledNpc.setCredits(credits);
}

void ControlledNpc::reset()
{
    clearKeycards();
    held = 0;
    randomSeed = 0;
    unknown08 = 0;
    if (difficulty < 1 || difficulty > 4) {
        difficulty = 3;
    }
    setHp(150);
    battery = 0;
    clearDead();
    setFrame(0);
}

bool ControlledNpc::setUnknown08(u8 value)
{
    unknown08 = value;
    return true;
}

/* 10f8:3fcd */
i16 gameDifficulty()
{
    return controlledNpc.difficulty;
}

/* The object without its last two bytes. */
constexpr int CONTROLLED_SAVE_SIZE = 0x15;

void ControlledNpc::load(Flex *flex, i16 index)
{
    flex->readObject(index, &controlledNpc, CONTROLLED_SAVE_SIZE, 0);
    setRandomSeed(controlledNpc.randomSeed);
}

i16 ControlledNpc::save(Flex *flex, i16 index)
{
    controlledNpc.randomSeed = getRandomSeed();
    flex->writeObject(index, &controlledNpc, CONTROLLED_SAVE_SIZE, 0, 1);
    return gameDifficulty();
}

void ControlledNpc::writeState(Flex *flex, i16 index) const
{
    ControlledNpc state = *this;
    state.randomSeed = getRandomSeed();
    flex->writeObject(index, &state, CONTROLLED_SAVE_SIZE, 0, 1);
}

bool Npc::unknown4040()
{
    return false;
}

i16 Npc::unknown4047()
{
    return 0x65;
}

bool Npc::unknown404f()
{
    return false;
}

u16 Npc::findItem(u16 shape)
{
    /* The Silencer's slots, whatever this NPC is. */
    const NpcData &silencer = npcs[1];
    for (i16 slot = 0; slot < EQUIPMENT_SLOTS; slot++) {
        const Item item(silencer.equipment[slot]);
        if (item.isValid() && item.shape() == shape) {
            return item.id;
        }
    }
    return 0;
}

u16 Npc::findAmmo()
{
    const Item weapon = controlledNpc.activeWeapon();
    if (weapon.isValid()) {
        const u16 ammo = ammoShape(weapon.shape());
        if (ammo == 0U) {
            return 0;
        }
        return findItem(ammo);
    }
    return 0;
}

u16 Npc::ammo()
{
    u16 ammo;
    const Item weapon = activeWeapon();
    if (weapon.isValid()) {
        ammo = ammoShape(weapon.shape());
        if (ammo == 0) {
            const u16 energy = shotEnergy(weapon.shape());
            if (energy == 0U) {
                return 1;
            }
            return mana() / energy;
        }
        return weapon.quality();
    }
    return 0;
}

void Npc::useAmmo()
{
    Item weapon = activeWeapon();
    i16 rounds;
    if (weapon.isValid()) {
        const i16 ammo = ammoShape(weapon.shape());
        i16 energy;
        if (ammo == 0) {
            energy = shotEnergy(weapon.shape());
            addMana(-energy);
            updateStatus(STATUS_ENERGY);
        } else {
            rounds = weapon.quality();
            weapon.setQuality(rounds - 1);
            updateStatus(STATUS_AMMO);
        }
    }
}

bool Npc::usesSmallAnims()
{
    const Item weapon = activeWeapon();
    return !(weapon.isValid()) || (smallWeapon(weapon.shape()) != 0U);
}

void Npc::reload()
{
    if (ammo() == 0U) {
        Item weapon = activeWeapon();
        if (weapon.isValid()) {
            const i16 ammo = ammoShape(weapon.shape());
            if (ammo != 0) {
                updateStatus(STATUS_AMMO);
                Item clip = findAmmo();
                const i16 rounds = clipSize(weapon.shape());
                weapon.setQuality(rounds);
                clip.setQuality(clip.quality() - 1);
                if (clip.quality() == 0) {
                    clip.destroy();
                }
                const u8 sound = reloadSound(weapon.shape());
                if (sound != 0U) {
                    restartSound(sound);
                }
                if (isSilencer() && (isKneeling() == 0U)) {
                    doAnim(ANIM_RELOAD_SMALL, DIRECTION_CURRENT, EVEN_IF_BUSY, 1);
                }
            }
        }
    }
}

/* The controlled NPC's keycards (10f8:43e0-4443). */
bool hasKeycard(u8 card)
{
    const u32 bit = static_cast<s32>(1L) << card;
    return (controlledNpc.keycards & bit) != 0;
}

void setKeycard(i16 card)
{
    const u32 bit = static_cast<s32>(1L) << card;
    controlledNpc.keycards |= bit;
}

namespace {
void clearKeycard(u8 card)
{
    const u32 bit = static_cast<s32>(1L) << card;
    controlledNpc.keycards &= ~bit;
}
}

void clearKeycards()
{
    clearKeycard(0);
    setKeycard(0);
    hasKeycard(0);
    controlledNpc.keycards = 0;
}

s32 Npc::credits()
{
    const Item card = controlledNpc.findItem(SHAPE_CREDITS);
    if (card.isValid()) {
        return card.familyQuality();
    }
    return 0;
}

void Npc::setCredits(u32 credits)
{
    const Item card = controlledNpc.findItem(SHAPE_CREDITS);
    if (card.isValid()) {
        card.setFamilyQuality(credits);
    }
}

u16 Npc::waypointGroup()
{
    return npcs[id].waypointGroup;
}

void Npc::setWaypointGroup(u16 value)
{
    npcs[id].waypointGroup = value;
}

u16 Npc::lastActivity()
{
    return npcs[id].lastActivity;
}

namespace {
/* No Regret's NPCs' hurt sounds (playHurtSound), by the NPCs that make them
 * (hurtSoundsWorkers the office workers and the seated workers): a count,
 * then the sounds. */
i16 hurtSoundsLmcGuard[] = {6, 0xd2, 0xd3, 0xd4, 0xd5, 0xe5, 0x100};
i16 hurtSoundsStormTrooper[] = {6, 0x9, 0x79, 0x7a, 0x7b, 0x7c, 0x7d};
i16 hurtSoundsElite[] = {6, 0x7e, 0x7f, 0x90, 0xb6, 0xc2, 0xd0};
i16 hurtSoundsEnforcer[] = {6, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106};
i16 hurtSoundsHqGuard[] = {7, 0x108, 0x109, 0x1ab, 0x1ac, 0x1ad, 0x1af, 0x1ae};
i16 hurtSoundsCongress[] = {5, 0x1b0, 0x1b1, 0x1b2, 0x1b3, 0x1b4};
i16 hurtSoundsFreighter[] = {7, 0x1b5, 0x1b6, 0x1b7, 0x1b8, 0x1b9, 0x1ba, 0x1bb};
i16 hurtSoundsMiner[] = {6, 0x1c1, 0x1c0, 0x1bf, 0x1be, 0x1bd, 0x1bc};
i16 hurtSoundsMaintenanceMan[] = {6, 0x1c2, 0x1c3, 0x1c4, 0x1c5, 0x1c6, 0x1c7};
i16 hurtSoundsSeatedLady[] = {6, 0x1c8, 0x1c9, 0x1ca, 0x1cb, 0x1cc, 0x1cd};
i16 hurtSoundsWorkers[] = {6, 0x1d0, 0x1d1, 0x1d2, 0x1d3, 0x1d4, 0x1d5};
i16 hurtSoundsChemsuitWorker[] = {6, 0x1d7, 0x1d8, 0x1d9, 0x1da, 0x1db, 0x1dc};
i16 hurtSoundsScientist[] = {7, 0x1dd, 0x1de, 0x1df, 0x1e0, 0x1e1, 0x1e2, 0x1e3};
i16 hurtSoundsAndroid[] = {5, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f};
i16 hurtSoundsBossRobot[] = {5, 0x1e7, 0x1e8, 0x1e9, 0x1ea, 0x1ed};
/* The last two hurt sounds of any NPC. */
i16 (&lastHurtSounds)[2] = gameGlobal<i16[2]>({-1, -1});
}

u16 Npc::busyAnim()
{
    const bool busy = itemHasType(id, ANIM_TYPE);
    s32 time;
    s32 progress;
    AnimPrimitive *process;

    if (busy) {
        process = static_cast<AnimPrimitive *>(findItemProcess(id, ANIM_TYPE));
        const i16 anim = process->anim;
        if (anim == ANIM_COMBAT_ROLL_LEFT || anim == ANIM_COMBAT_ROLL_RIGHT || anim == ANIM_KNEEL_ROLL_LEFT ||
            anim == ANIM_KNEEL_ROLL_RIGHT || anim == ANIM_STAND || anim == ANIM_COMBAT_STAND_SMALL ||
            anim == ANIM_COMBAT_STAND_LARGE || anim == ANIM_ROLL_FORWARD || anim == ANIM_READY_SMALL ||
            anim == ANIM_READY_LARGE || anim == ANIM_FIRE_SMALL || anim == ANIM_FIRE_LARGE ||
            anim == ANIM_UNREADY_SMALL || anim == ANIM_UNREADY_LARGE || anim == ANIM_STOP_RUN_SMALL ||
            anim == ANIM_STOP_RUN_LARGE || anim == ANIM_SLIDE_LEFT || anim == ANIM_SLIDE_RIGHT ||
            anim == ANIM_KNEEL_START || anim == ANIM_KNEEL_END || anim == ANIM_KNEEL_FIRE_SMALL ||
            anim == ANIM_KNEEL_FIRE_LARGE || anim == ANIM_BRIGHT_KNEEL_FIRE || anim == ANIM_BRIGHT_FIRE_LARGE ||
            anim == ANIM_QUICK_JUMP || anim == ANIM_KNEELING_SMALL || anim == ANIM_KNEELING_LARGE) {
            if (anim == ANIM_KNEEL_START || anim == ANIM_KNEEL_END) {
                time = process->info.frames * static_cast<s32>(300L);
            } else {
                time = process->info.frames * static_cast<s32>(700L);
            }
            progress = npcs[id].animFrame * static_cast<s32>(1000L);
            if (time == 700 || progress >= time) {
                return anim;
            }
        }
    }
    return ANIMS;
}

void Npc::playHurtSound()
{
    i16 sound;

    npcs[id].hurt = 1;
    if (controlledActor != 1) {
        return;
    }
    if (controlledNpc.isDead() != 0U) {
        return;
    }
    if (isSoundPlaying(npcs[id].hurtSound) == 0U) {
        sound = -1;
        switch (itemShape[id]) {
        case SHAPE_LMC_GUARD:
        case SHAPE_LMC_GUARD_KNEELING_B:
        case SHAPE_LMC_GUARD_KNEELING_A:
            sound = hurtSoundsLmcGuard[static_cast<i16>(borlandRandom(hurtSoundsLmcGuard[0]) + 1)];
            break;
        case SHAPE_STORM_TROOPER:
        case SHAPE_STORM_TROOPER_KNEELING_A:
        case SHAPE_STORM_TROOPER_KNEELING_B:
            sound = hurtSoundsStormTrooper[static_cast<i16>(borlandRandom(hurtSoundsStormTrooper[0]) + 1)];
            break;
        case SHAPE_ELITE_GUARD:
        case SHAPE_ELITE_GRENADIER:
            sound = hurtSoundsElite[static_cast<i16>(borlandRandom(hurtSoundsElite[0]) + 1)];
            break;
        case SHAPE_ENFORCER:
        case SHAPE_ENFORCER_KNEELING_B:
        case SHAPE_ENFORCER_KNEELING_A:
            sound = hurtSoundsEnforcer[static_cast<i16>(borlandRandom(hurtSoundsEnforcer[0]) + 1)];
            break;
        case SHAPE_HQ_GUARD:
        case SHAPE_HQ_GUARD_KNEELING_B:
        case SHAPE_HQ_GUARD_KNEELING_A:
            sound = hurtSoundsHqGuard[static_cast<i16>(borlandRandom(hurtSoundsHqGuard[0]) + 1)];
            break;
        case SHAPE_CONGRESS:
            sound = hurtSoundsCongress[static_cast<i16>(borlandRandom(hurtSoundsCongress[0]) + 1)];
            break;
        case SHAPE_FREIGHTER:
            sound = hurtSoundsFreighter[static_cast<i16>(borlandRandom(hurtSoundsFreighter[0]) + 1)];
            break;
        case SHAPE_MINER:
            sound = hurtSoundsMiner[static_cast<i16>(borlandRandom(hurtSoundsMiner[0]) + 1)];
            break;
        case SHAPE_MAINTENANCE_MAN:
            sound = hurtSoundsMaintenanceMan[static_cast<i16>(borlandRandom(hurtSoundsMaintenanceMan[0]) + 1)];
            break;
        case SHAPE_SEATED_LADY:
            sound = hurtSoundsSeatedLady[static_cast<i16>(borlandRandom(hurtSoundsSeatedLady[0]) + 1)];
            break;
        case SHAPE_OFFICE_WORKER:
        case SHAPE_SEATED_WORKER:
            sound = hurtSoundsWorkers[static_cast<i16>(borlandRandom(hurtSoundsWorkers[0]) + 1)];
            break;
        case SHAPE_CHEMSUIT_WORKER:
            sound = hurtSoundsChemsuitWorker[static_cast<i16>(borlandRandom(hurtSoundsChemsuitWorker[0]) + 1)];
            break;
        case SHAPE_SCIENTIST:
            sound = hurtSoundsScientist[static_cast<i16>(borlandRandom(hurtSoundsScientist[0]) + 1)];
            break;
        case SHAPE_ANDROID:
            sound = hurtSoundsAndroid[static_cast<i16>(borlandRandom(hurtSoundsAndroid[0]) + 1)];
            break;
        case SHAPE_BOSS_ROBOT:
            sound = hurtSoundsBossRobot[static_cast<i16>(borlandRandom(hurtSoundsBossRobot[0]) + 1)];
            break;
        }
        if (sound != -1 && npcs[id].hurtSound != sound && sound != lastHurtSounds[0] && sound != lastHurtSounds[1]) {
            lastHurtSounds[1] = lastHurtSounds[0];
            lastHurtSounds[0] = sound;
            setHurtSound(sound);
            playSound(sound);
        }
    }
}
