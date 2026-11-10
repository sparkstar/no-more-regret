/* File name unknown: starting and closing the animation cache. See
 * actanim.h. No Remorse links them as a module of their own before ASTREAM.C,
 * which calls its own functions far (it is not its segment's first module);
 * No Regret's are in actanim's module. */
#include "actanim.h"
#include "animcach.h"

void initAnims()
{
    initAnimCache();
}

void closeAnims()
{
    closeAnimCache();
}
