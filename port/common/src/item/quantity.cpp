/* Code19 (1090:0000-01a6, file name unknown, among the ..\ITEM modules):
 * stacking quantity items. */
#include "item.h"
#include "type.h"
#include "itemfind.h"
#include "itemutil.h"

bool Item::absorb(u16 item)
{
    ItemRef other(item);
    i16 family = typeFlags.infos[shape()].family;

    if (shape() == other.shape() && (family == FAMILY_QUANTITY || family == FAMILY_REAGENT)) {
        if (family == FAMILY_REAGENT) {
            const u16 otherFrame = other.frame();
            const u16 thisFrame = frame();
            if (otherFrame != thisFrame) {
                return false;
            }
        }
        family = quality() + other.quality();
        if (family <= MAX_QUANTITY) {
            setQuality(family);
            other.destroy();
        } else {
            setQuality(MAX_QUANTITY);
            other.setQuality(family - MAX_QUANTITY);
        }
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool Item::putInContainer(u16 container)
{
    if (Container(container).canAdd(id)) {
        push();
        popToContainer(container);
        RecursiveContainerSearch search(container, "$", 1);
        while (search.isValid()) {
            if (absorb(search.id)) {
                break;
            }
            search.next();
        }
        return true;
    }
    return false;
}
