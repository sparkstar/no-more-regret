/* SYSTEM\SYSTEM.C (12d0:0513-072c): the system object, the video mode and
 * the objects every program of Origin's library sets up at start-up. */
#ifndef PORT_SYSTEM_H
#define PORT_SYSTEM_H
#include "common.h"
#include "supervga.h"

/* Set by System::startup, cleared by the constructor. */
extern u8 &systemStarted;

/* The base of the program's system object. The game derives its own
 * (CRUSYS.C, vtable 1480:0f77); this class's vtable is 1480:0f87. */
class System {
public:
    /* CRUSYS.C inlines it (1058:033f). */
    System()
    {
        systemStarted = 0;
        startup();
    }
    /* Virtual in the port, not in the original. */
    virtual ~System() = default;
    /* Resets the mouse, starts SPANKER.C, notes the video mode and makes this
     * the current system (12d0:0513). Runs once. */
    virtual void startup();
    /* Called by systemShutdown after the video mode is restored (12d0:0684). */
    virtual void shutdown();
    /* Called twice by MEMORY\CPROTMEM.C with the pass number before it
     * compacts memory (1058:0f21). */
    virtual void purgeMemory(u16 /*unused*/) {}
    /* Sets a video mode: VESA 101h, 103h, 105h or 111h through superVga, else
     * a BIOS mode in the low byte. 0 when the VESA mode is not there (12d0:0577). */
    virtual bool setMode(u32 mode);
};

extern System *&currentSystem;
/* The buffer the printf-style functions format into (debugPrintf, GumpFont);
 * System allocates it, of FORMAT_BUFFER_SIZE bytes. */
extern char *&formatBuffer;
const u32 FORMAT_BUFFER_SIZE = 1024;
/* The SuperVGA modes (1480:933c). */
extern SuperVga &superVga;
/* The BIOS's 80x25 colour text mode. */
constexpr int MODE_TEXT = 3;

/* The current video mode as described above (12d0:0652). */
u32 getVideoMode();
/* Restores the original video mode, shuts the current system down and stops
 * SPANKER.C, once; ends the run with status 1 when exitProgram is set, the
 * original's exit, the port's hostEnd (12d0:0689). */
void systemShutdown(u8 exitProgram);
#endif
