/* The usecode's trace: with REGRET_USECODE_TRACE set to a file, what the
 * usecode does and what it starts, a line each, so that a run can be
 * followed where a replay goes wrong, and the classes, functions and
 * intrinsics a recording runs can be counted (tools/usecode.py coverage).
 * The system layer gives it (port/system/trace.cpp); without the variable
 * each call returns at once.
 *
 * A line starts with the pass it comes in (the state log's line of that
 * pass, REGRET_STATE, comes at its end) and a letter; numbers are in
 * decimal but a class, a function and an intrinsic's number and bytes, in
 * hexadecimal as tools/usecode.py shows them. A line comes when what it
 * tells has happened: an intrinsic's after it returned, so after the lines
 * of what it did (the processes it made, the events it sent):
 *
 *   pass E item class:event          an event the item's class has a
 *                                    function for, sent (callEvent)
 *   pass + pid creator               a process made, by the process
 *                                    running (0: none)
 *   pass U pid class:function        a usecode process starts there
 *   pass C pid class:function        a usecode function called (OP_CALL)
 *   pass I pid class:function number bytes... = result
 *                                    an intrinsic called from that function
 *                                    with the bytes on the usecode's stack,
 *                                    and what it returned; "= wait" when it
 *                                    opened a wait, its result an R line
 *                                    after it
 *   pass R pid result                an intrinsic's result after its wait
 *   pass - pid type item name        a process killed: its type, its
 *                                    item and its name (a usecode
 *                                    process's is its class's)
 *   pass T npc x y z map             an NPC teleported (Npc::teleport)
 *   pass M map                       a map loaded
 *   pass F item                      the item the camera follows set
 *   pass G offset bytes...           at the pass's end, a run of the
 *                                    usecode's globals that the pass
 *                                    changed (unlike the last pass's end,
 *                                    zeros before the first): its offset
 *                                    and its bytes */
#ifndef PORT_PLATFORM_TRACE_H
#define PORT_PLATFORM_TRACE_H
#include <cstdint>

void traceEvent(std::uint16_t item, std::uint16_t classId, std::uint16_t event);
void traceProcessMade(std::uint16_t pid, std::uint16_t creator);
void traceUsecodeStart(std::uint16_t pid, std::uint16_t classId, std::uint16_t function);
void traceCall(std::uint16_t pid, std::uint16_t classId, std::uint16_t function);
/* waited: the intrinsic opened a wait; result is not yet its result. */
void traceIntrinsic(std::uint16_t pid, std::uint16_t classId, std::uint16_t function, std::uint16_t number,
                    const std::uint8_t *bytes, std::uint16_t size, std::int32_t result, bool waited);
void traceIntrinsicResult(std::uint16_t pid, std::int32_t result);
void traceProcessKilled(std::uint16_t pid, std::int16_t type, std::uint16_t item, const char *name);
void traceTeleport(std::uint16_t npc, std::uint16_t x, std::uint16_t y, std::uint8_t z, std::uint8_t map);
void traceMapLoaded(std::uint16_t map);
void traceFollowed(std::uint16_t item);
/* At a pass's end (statelog.cpp): the usecode's globals, size bytes. */
void traceGlobals(const std::uint8_t *globals, std::uint16_t size);
#endif
