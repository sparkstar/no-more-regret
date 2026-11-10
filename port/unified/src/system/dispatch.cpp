/* Written by port/tools/make_unified_values.py from DISPATCHED: the functions each game's generated file defines,
 * called through the game the one program plays; do not edit. */

#include "values.h"
#include "intrinsics.h"

namespace regret_values {
s32 callIntrinsic(u16 number, const UsecodeArguments &args);
}
namespace remorse_values {
s32 callIntrinsic(u16 number, const UsecodeArguments &args);
}

s32 callIntrinsic(u16 number, const UsecodeArguments &args)
{
    if (playingRemorse) {
        return remorse_values::callIntrinsic(number, args);
    }
    return regret_values::callIntrinsic(number, args);
}
