/* DEBUGGER.C (13e0): the usecode debugger's state: breakpoints by function
 * name and line, the stack of usecode calls, and stepping. INTER.C reports
 * each line and call to it; the debug gump (DEBUGUMP.C) derives from it and
 * shows the source (the class's.unk) when it stops. Nothing in the original
 * makes one (initDebugger has no caller, the shipped usecode no lines), so
 * debugger stays 0; the port leaves DEBUGUMP.C and the gumps only it uses
 * out. */
#ifndef PORT_DEBUGGER_H
#define PORT_DEBUGGER_H
#include "common.h"

constexpr int MAX_BREAKPOINTS = 10;
constexpr int MAX_DEBUG_FRAMES = 30;

struct Breakpoint {
    char function[9];
    i16 line;
};
ASSERT_SIZE(Breakpoint, 11);

/* A name the debugger shows the value of: a local of function, or a global
 * flag when function is empty (DEBUGUMP.C). */
struct Watch {
    char function[9];
    char name[32];
};
ASSERT_SIZE(Watch, 0x29);

/* A usecode call on the debugger's stack. */
struct DebugFrame {
    char function[9];
    /* The function's table of locals, which follows its code: a count, then
     * a direction, kind, offset and size byte and a name each (formatLocal).
     * The usecode of No Remorse carries such tables. */
    u8 *code;
    /* Where the call's locals start in the process. */
    u8 *locals;
    u32 unknown11;
};
ASSERT_SIZE(DebugFrame, 21);

class Debugger {
public:
    /* Sorted by function name and line. */
    i16 breakpointCount;
    Breakpoint breakpoints[MAX_BREAKPOINTS];
    /* The line being run, from 0. */
    i16 line;
    /* Stop at the next line. */
    u8 stepping;
    /* Stop at the next line that is not in a call made after stepping
     * over began; nesting counts those calls (INTER.C). */
    u8 steppingOver;
    s32 nesting;
    i16 depth;
    DebugFrame frames[MAX_DEBUG_FRAMES];

    /* UNCALLED: nothing calls it. */
    Debugger(); /* 13e0:0000 */
    /* Virtual in the port, not in the original. */
    virtual ~Debugger() = default;
    /* Called by the interpreter before each line (line counts from 1). */
    void atLine(u16 newLine); /* 13e0:0053 */
    /* UNCALLED: nothing calls it. */
    void addBreakpoint(const char *function, i16 breakLine); /* 13e0:00dd */
    /* UNCALLED: nothing calls it. */
    void removeBreakpoint(const char *function, i16 breakLine); /* 13e0:01a5 */
    /* The first breakpoint in function at line or after it, or -1. */
    /* UNCALLED: nothing calls it. */
    i16 nextBreakpoint(const char *function, i16 breakLine); /* 13e0:0230 */
    bool isBreakpoint(const char *function, i16 breakLine);  /* 13e0:029e */
    /* A call begins. */
    void enter(const char *function, u8 *code, u8 *locals, u32 unknown); /* 13e0:02f5 */
    void enter(DebugFrame *frame);                                       /* 13e0:03b0 */
    /* The call returns. */
    void leave(); /* 13e0:03f7 */
    /* UNCALLED: nothing calls it. */
    void stepOver(); /* 13e0:0419 */
    /* Runs on until a breakpoint. */
    void run(); /* 13e0:0432 */
    /* The innermost call, or 0. */
    DebugFrame *currentFrame(); /* 13e0:0444 */
    /* Stopped at a breakpoint or a step. */
    virtual void stop(); /* 13e0:046f */
    /* The value of a watch as text, or 0. */
    virtual char *inspect(Watch *watch); /* 13e0:0474 */
};
ASSERT_SIZE(Debugger, 0x2f2);

extern Debugger *&debugger;
#endif
