/* Written by port/tools/make_unified_values.py from regret/src/world/savefields.cpp: regret's data in its namespace in
 * the one program; do not edit. */

#include "values.h"
#include "savefields.h"
#include "activity.h"
#include "anchor.h"
#include "animprim.h"
#include "assproc.h"
#include "attack.h"
#include "basecam.h"
#include "biostimr.h"
#include "boboboom.h"
#include "cache.h"
#include "camera.h"
#include "control.h"
#include "crusader.h"
#include "crusys.h"
#include "cycle.h"
#include "dispatch.h"
#include "display.h"
#include "dumbtimr.h"
#include "egg.h"
#include "fade.h"
#include "flicplay.h"
#include "gamepal.h"
#include "gametime.h"
#include "gravity.h"
#include "item.h"
#include "joydrv.h"
#include "kdaemon.h"
#include "keyio.h"
#include "kprocess.h"
#include "mapjump.h"
#include "mousectl.h"
#include "music.h"
#include "pathfind.h"
#include "refstore.h"
#include "snap.h"
#include "sprite.h"
#include "stdinter.h"
#include "systimer.h"
#include "u9mouse.h"
#include "uprocess.h"
#include "world.h"
#include <typeinfo>
#include "values_regret.h"

namespace regret_values {
#include "../../../regret/src/world/savefields.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_world_savefields();

void linkRegretData_world_savefields() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
        ::SAVE_NAMES = regret_values::SAVE_NAMES;
        ::SAVE_NAME_COUNT = regret_values::SAVE_NAME_COUNT;
        ::SAVE_CLASSES = regret_values::SAVE_CLASSES;
        ::SAVE_CLASS_COUNT = regret_values::SAVE_CLASS_COUNT;
        ::ORIGINAL_DS = regret_values::ORIGINAL_DS;
        ::ORIGINAL_UNIT_FILE = regret_values::ORIGINAL_UNIT_FILE;
        ::ORIGINAL_STACK = regret_values::ORIGINAL_STACK;
        ::ORIGINAL_FIRST_TACTIC = regret_values::ORIGINAL_FIRST_TACTIC;
        ::ORIGINAL_BIOS_VECTOR = regret_values::ORIGINAL_BIOS_VECTOR;
        ::SUBSCRIBER_LIST_VTABLE = regret_values::SUBSCRIBER_LIST_VTABLE;
    }
    return true;
}();

}
