/* See portversion.h. CMake gives this file alone the version (PORT_VERSION), so that a new one builds it again
 * and nothing else. */
#include "portversion.h"

const char *portVersion()
{
    return PORT_VERSION;
}
