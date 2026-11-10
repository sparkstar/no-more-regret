/* Code20 (1098:0000-07e8, file name unknown, among the ..\ITEM modules):
 * containers and the screen place of a map point. */
#include "itemutil.h"
#include "gamestate.h"
#include "item.h"
#include "npc.h"
#include "type.h"
#include "camera.h"

/* A container nothing may go in and whose contents weigh nothing
 * (1480:12dc). Nothing sets it, so it stays 0. */
u16 &sealedContainer = gameGlobal<u16>(0);

bool Container::contains(u16 item)
{
    TableItem current(item);

    if ((current.flags() & ITEM_CONTAINED) != 0) {
        while (((current.flags() & ITEM_CONTAINED) != 0) && current.container() != id) {
            current = TableItem(current.container());
        }
        return current.container() == id;
    }
    return false;
}

bool Container::canAdd(u16 item)
{
    if (Item(item).isNpc()) {
        return false;
    }
    if (contains(item)) {
        return true;
    }
    if (id == item || (Container(item).contains(id))) {
        return false;
    }
    if ((sealedContainer != 0U) && (id == sealedContainer || (Container(sealedContainer).contains(id)))) {
        return false;
    }
    if (isNpc()) {
        [[maybe_unused]] i16 weight = Container(item).contentsWeight();

        weight += Item(item).weight();
        const Npc npc(id);
        const ShapeInfo info = typeFlags.infos[ItemRef(item).shape()];
        if ((info.equipType != 0U) && (!Item(controlledNpc.equipment(info.equipType - 1)).isValid())) {
            return true;
        }
        Container pack(controlledNpc.equipment(6));
        if (pack.isValid()) {
            return pack.canAdd(item);
        }
        return false;
    }
    i16 weight = Container(item).contentsWeight();

    weight += Item(item).weight();
    if (tooHeavy(weight)) {
        return false;
    }
    const i16 volume = Container(item).totalVolume();
    const i16 room = capacity() - contentsVolume();
    if (room >= volume) {
        if (typeFlags.infos[shape()].volume == 0 && ((flags() & ITEM_CONTAINED) != 0)) {
            return Container(container()).canAdd(item);
        }
        return true;
    }
    return false;
}

bool Container::canHold(u16 shape, i16 count)
{
    const ShapeInfo info = typeFlags.infos[shape];
    i16 volume;

    if (info.family == FAMILY_QUANTITY) {
        if (tooHeavy(static_cast<i16>((info.weight * count) + 9) / 10)) {
            return false;
        }
        volume = static_cast<i16>((info.volume * count) + 99) / 100;
    } else if (info.family == FAMILY_REAGENT) {
        if (tooHeavy(info.weight * count)) {
            return false;
        }
        volume = static_cast<i16>((info.volume * count) + 9) / 10;
    } else {
        if (tooHeavy(info.weight * 10)) {
            return false;
        }
        if (info.family == FAMILY_CONTAINER && info.volume == 0) {
            volume = 1;
        } else {
            volume = info.volume;
        }
    }
    if (static_cast<i16>(capacity() - contentsVolume()) >= volume) {
        if (typeFlags.infos[this->shape()].volume == 0 && ((flags() & ITEM_CONTAINED) != 0)) {
            return Container(container()).canHold(shape, count);
        }
        return true;
    }
    return false;
}

bool Container::tooHeavy(i16 /*unused*/)
{
    Item outer(id);

    while (outer.isValid() && ((outer.flags() & ITEM_CONTAINED) != 0)) {
        outer = Container(outer.container());
    }
    if (outer.isValid() && outer.isNpc()) {
        /* The original makes the NPC who carries the container and checks
         * nothing. */
        const Npc npc(outer.id);
    }
    return false;
}

u16 Container::unknown04fe()
{
    return 0;
}

void Container::removeContents()
{
    if ((flags() & ITEM_CONTAINED) != 0) {
        while (contents() != 0U) {
            Item item = contents();
            item.moveToContainer(container());
            item.setGumpPosition(gumpX(), gumpY());
        }
        return;
    }
    while (contents() != 0U) {
        Item(contents()).moveTo(point());
    }
}

void Container::destroyContents()
{
    while (contents() != 0U) {
        Item(contents()).destroy();
    }
}

i16 Container::contentsWeight()
{
    i16 weight = 0;

    if (id == sealedContainer) {
        return 0;
    }
    for (u16 item = contents(); item != 0U; item = ItemRef(item).next()) {
        weight += Container(item).contentsWeight();
        weight += Item(item).weight();
    }
    return weight;
}

i16 Container::totalVolume()
{
    i16 volume = 0;

    for (u16 item = contents(); item != 0U; item = ItemRef(item).next()) {
        volume += Container(item).totalVolume();
    }
    if (this->volume() > volume) {
        volume = this->volume();
    }
    return volume;
}

i16 Container::contentsVolume()
{
    i16 volume = 0;

    for (u16 item = contents(); item != 0U; item = ItemRef(item).next()) {
        volume += Container(item).totalVolume();
    }
    return volume;
}

i16 Container::contentsCount()
{
    i16 count = 0;

    for (u16 item = contents(); item != 0U; item = ItemRef(item).next()) {
        count += Container(item).contentsCount() + 1;
    }
    return count;
}

bool ItemPoint::operator==(const ItemPoint &other) const
{
    return x == other.x && y == other.y && z == other.z;
}

void mapToScreen(u16 x, u16 y, i16 *screenX, i16 *screenY)
{
    *screenX = (static_cast<i16>(x - y) >> 1) - scrollX;
    *screenY = (static_cast<ui16>(x + y) >> 2) - scrollY;
}
