/* ..\\ITEM\\ITEMDAM.C (Code26, 10c8:0000-0615): how the items take damage
 * and what becomes of them when they break. */
#include "gamestate.h"
#include "itemdam.h"
#include "crusys.h"
#include "item.h"
#include "file.h"
#include "explode.h"
#include "random.h"

DamageTable *&damageTable = gameGlobal<DamageTable *>(nullptr);

DamageTable::DamageTable()
{
    flex = std::make_unique<FastFlex>("static\\damage.flx", FILE_READ, -1);
    if (flex == nullptr) {
        allocationFailure();
    }
    entries = std::make_unique_for_overwrite<DamageEntry[]>(DAMAGE_SHAPES);
    if (entries == nullptr) {
        allocationFailure();
    }
    damageTable = this;
    load();
}

DamageTable::~DamageTable()
{
    damageTable = nullptr;
    flex.reset();
    entries.reset();
}

void DamageTable::load() const
{
    FlexEntry entry;

    flex->readEntry(0, &entry);
    if (entry.size == static_cast<ui16>(DAMAGE_SHAPES * sizeof(DamageEntry))) {
        flex->readObject(0, entries.get(), -1, 0);
        return;
    }
    memset(entries.get(), 0, DAMAGE_SHAPES * sizeof(DamageEntry));
}

u8 DamageTable::isDamageable(u16 item) const
{
    const Item damaged(item);
    DamageEntry *entry = &entries[shapeOf(damaged.id)];

    if (entry->takesDamage() != 0U) {
        return entry->durable;
    }
    return 1;
}

u8 DamageTable::takesDamage(u16 shape) const
{
    DamageEntry *entry = &entries[shape];

    return entry->takesDamage();
}

bool DamageTable::wouldBreak(u16 item, i16 points) const
{
    const Item damaged(item);
    DamageEntry *entry = &entries[shapeOf(damaged.id)];

    if ((entry->takesDamage() == 0U) || (worldActive == 0U)) {
        return false;
    }
    if (damaged.hasFlags(ITEM_GUMP_OPEN) != 0U) {
        return false;
    }
    if (itemDamagePoints[damaged.id] != 0U) {
        if (points != 0) {
            if (itemDamagePoints[damaged.id] > points) {
                return false;
            }
        } else if (itemDamagePoints[damaged.id] != 0U) {
            return false;
        }
    }
    return true;
}

bool DamageTable::breakItem(u16 item, i16 points) const
{
    Item damaged(item);
    DamageEntry *entry = &entries[shapeOf(damaged.id)];
    bool destroyed;

    if ((entry->takesDamage() == 0U) || (worldActive == 0U)) {
        return false;
    }
    /* The original marks a broken item with ITEM_GUMP_OPEN. */
    if (damaged.hasFlags(ITEM_GUMP_OPEN) != 0U) {
        return false;
    }
    destroyed = false;
    if (itemDamagePoints[damaged.id] != 0U) {
        if (points != 0) {
            if (itemDamagePoints[damaged.id] <= points) {
                destroyed = true;
                damaged.setDamagePoints(0);
            } else {
                damaged.setDamagePoints(itemDamagePoints[damaged.id] - points);
                return false;
            }
        } else if (itemDamagePoints[damaged.id] != 0U) {
            return false;
        }
    }
    damaged.setFlags(damaged.flags() | ITEM_GUMP_OPEN);
    damaged.setBroken();
    if (entry->explosionType() != 0U) {
        explode(&damaged, entry->explosionType() - 1, entry->explodeDestroysItem(), entry->explodeWithDamage());
    }
    if (entry->sound != 0U) {
        damaged.playSound(entry->sound);
    }
    if (entry->replaceItem() != 0U) {
        ItemPoint point;
        const u16 qualityLow = damaged.qualityLow();
        const u16 qualityHigh = damaged.qualityHigh();

        point.x = itemX[damaged.id];
        point.y = itemY[damaged.id];
        point.z = itemZ[damaged.id];
        if ((damaged.id != 0U) && itemZ[damaged.id] <= 0xfb) {
            damaged.destroy();
        }
        damaged.legalCreateAtPoint(entry->replacement.shape, entry->replacement.frame, &point);
        if (damaged.isValid()) {
            damaged.setQualityLow(qualityLow);
            damaged.setQualityHigh(qualityHigh);
        }
    } else if (entry->frameDataIsAbsolute() != 0U) {
        i16 count = 1;

        if (entry->frames[1] != 0U) {
            count++;
        }
        if (entry->frames[2] != 0U) {
            count++;
        }
        const i16 choice = randomNumber(count);
        damaged.setFrame(entry->frames[choice]);
    } else {
        i16 count = 0;

        for (i16 i = 0; i < 3; i++) {
            if (entry->frames[i] != 0U) {
                count++;
            }
        }
        if (count != 0) {
            const i16 choice = randomNumber(count);
            damaged.setFrame(damaged.frame() + entry->frames[choice]);
        } else if (destroyed && (damaged.id != 0U) && itemZ[damaged.id] <= 0xfb) {
            damaged.destroy();
        }
    }
    return true;
}
