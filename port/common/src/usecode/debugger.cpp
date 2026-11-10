/* DEBUGGER.C (13e0:0000-047c): the usecode debugger's state. See
 * debugger.h. */
#include <mem.h>
#include "gamestate.h"
#include <cstring>
#include "debugger.h"

Debugger *&debugger = gameGlobal<Debugger *>(nullptr);

/* UNCALLED: nothing calls it. */
Debugger::Debugger()
{
    memset(breakpoints, -1, sizeof(breakpoints));
    breakpointCount = 0;
    depth = 0;
    steppingOver = 0;
}

void Debugger::atLine(u16 newLine)
{
    this->line = newLine - 1;
    if (isBreakpoint(currentFrame()->function, this->line)) {
        stop();
    } else if (steppingOver != 0U) {
        if (nesting <= 0) {
            stop();
        }
    } else if (stepping != 0U) {
        stop();
    }
}

/* UNCALLED: nothing calls it. */
void Debugger::addBreakpoint(const char *function, i16 breakLine)
{
    if (breakpointCount < MAX_BREAKPOINTS) {
        Breakpoint *entry = breakpoints;

        for (i16 i = 0; i < breakpointCount; i++, entry++) {
            const i16 order = stricmp(function, entry->function);
            if (order < 0 || (entry->line > breakLine && order == 0)) {
                movmem(entry, entry + 1, (breakpointCount - i) * sizeof(Breakpoint));
                break;
            }
        }
        strcpy(entry->function, function);
        entry->line = breakLine;
        breakpointCount++;
    }
}

/* UNCALLED: nothing calls it. */
void Debugger::removeBreakpoint(const char *function, i16 breakLine)
{
    if (breakpointCount > 0) {
        Breakpoint *entry = breakpoints;

        for (i16 i = 0; i < breakpointCount; i++, entry++) {
            if (entry->line == breakLine && stricmp(function, entry->function) == 0) {
                movmem(entry + 1, entry, (breakpointCount - i) * sizeof(Breakpoint));
                breakpointCount--;
                return;
            }
        }
    }
}

/* UNCALLED: nothing calls it. */
i16 Debugger::nextBreakpoint(const char *function, i16 breakLine)
{
    Breakpoint *entry = breakpoints;

    for (i16 i = 0; i < breakpointCount; i++, entry++) {
        if (stricmp(entry->function, function) == 0) {
            for (; i < breakpointCount; i++, entry++) {
                if (entry->line >= breakLine) {
                    return i;
                }
            }
        }
    }
    return -1;
}

bool Debugger::isBreakpoint(const char *function, i16 breakLine)
{
    Breakpoint *entry = breakpoints;

    for (i16 i = 0; i < breakpointCount; i++, entry++) {
        if (entry->line == breakLine && stricmp(entry->function, function) == 0) {
            return true;
        }
    }
    return false;
}

void Debugger::enter(const char *function, u8 *code, u8 *locals, u32 unknown)
{
    DebugFrame *frame = &frames[depth];

    strcpy(frame->function, function);
    if (strlen(frame->function) > 8) {
        halt();
    }
    frame->code = code;
    frame->locals = locals;
    frame->unknown11 = unknown;
    depth++;
    if (depth >= MAX_DEBUG_FRAMES) {
        halt();
    }
}

void Debugger::enter(DebugFrame *frame)
{
    memcpy(&frames[depth], frame, sizeof(DebugFrame));
    depth++;
    if (depth >= MAX_DEBUG_FRAMES) {
        halt();
    }
}

void Debugger::leave()
{
    depth--;
    if (depth < 0) {
        halt();
    }
}

/* UNCALLED: nothing calls it. */
void Debugger::stepOver()
{
    nesting = 0;
    steppingOver = 1;
}

void Debugger::run()
{
    stepping = 0;
    steppingOver = 0;
}

DebugFrame *Debugger::currentFrame()
{
    if (depth > 0) {
        return &frames[static_cast<i16>(depth - 1)];
    }
    return nullptr;
}

void Debugger::stop() {}

char *Debugger::inspect(Watch * /*unused*/)
{
    return nullptr;
}
