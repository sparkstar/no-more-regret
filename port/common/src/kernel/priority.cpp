/* PRIORITY.C (11d0:066f-070c): priority conditions. See priority.h. */
#include "priority.h"

bool Priority::isSet() const
{
    if ((value & PRIORITY_TESTS) == PRIORITY_TESTS) {
        return true;
    }
    if (((value & PRIORITY_LEVEL) != 0) && ((value & PRIORITY_TESTS) != 0)) {
        return true;
    }
    return false;
}

bool Priority::holds(u8 level) const
{
    u8 wanted;
    u8 tests;

    wanted = value & PRIORITY_LEVEL;
    tests = value & PRIORITY_TESTS;
    if (level == 0 || ((level & PRIORITY_TESTS) != 0)) {
        halt();
    }
    if (((tests & PRIORITY_ABOVE) != 0) && level < wanted) {
        return true;
    }
    if (((tests & PRIORITY_EQUAL) != 0) && level == wanted) {
        return true;
    }
    if (((tests & PRIORITY_BELOW) != 0) && level > wanted) {
        return true;
    }
    return false;
}
