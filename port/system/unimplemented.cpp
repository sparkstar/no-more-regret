/* See unimplemented.h. */
#include "unimplemented.h"
#include "gamememory.h"
#include "common.h"
#include "report.h"
#include "host.h"

#include <cstdio>
#include <cstdlib>

void portUnimplemented(const char *name)
{
    portEnd(3, "%s: the port cannot go on", name);
}

void hostMissingIntrinsic(u16 number)
{
    const SystemSide systemSide;
    static char name[64];
    std::snprintf(name, sizeof(name), "intrinsic %03X, which the port does not have", number);
    portUnimplemented(name);
}

void hostUnimplemented(const char *name)
{
    const SystemSide systemSide;
    portUnimplemented(name);
}
