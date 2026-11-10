/* Written by port/tools/make_unified_values.py from regret/src/usecode/intrinsics.cpp: regret's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "intrinsics.h"
#include "host.h"
#include "alert.h"
#include "camera.h"
#include "compgump.h"
#include "control.h"
#include "crusader.h"
#include "crusys.h"
#include "distance.h"
#include "dsfxman.h"
#include "egg.h"
#include "explode.h"
#include "fade.h"
#include "gamepal.h"
#include "gametime.h"
#include "gravity.h"
#include "inter.h"
#include "item.h"
#include "itemcach.h"
#include "kernel.h"
#include "keypad.h"
#include "mapjump.h"
#include "movie.h"
#include "music.h"
#include "npc.h"
#include "sprite.h"
#include "status.h"
#include "type.h"
#include "utility.h"
#include "world.h"
#include "values_regret.h"

namespace regret_values {
s32 callIntrinsic(u16 number, const UsecodeArguments &args);
#include "../../../regret/src/usecode/intrinsics.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_usecode_intrinsics();

void linkRegretData_usecode_intrinsics() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
    }
    return true;
}();

}
