/* DESTROY.ASM (the TASM module after STDINT.C, 1208:0506): a process's
 * destruction without its freeing. See kprocess.h. */
#include "kprocess.h"

extern "C" void destroyProcess(Process *process)
{
    /* The original calls the second entry of the vtable, the destructor,
     * with the flag 2: destroy without freeing (the kernel frees the
     * process), and goes on reading the destroyed process's flags until
     * then. The port does the destructors' work and keeps the object
     * whole; freeProcess (kernel.cpp) ends it. */
    process->onKill();
}
