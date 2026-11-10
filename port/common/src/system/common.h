/* Basic types and runtime services shared by every module of the port. The
 * original is Borland C++ 3.1's 16-bit large model; the types keep its
 * widths. */
#ifndef PORT_COMMON_H
#define PORT_COMMON_H
/* The original's integer types: its int and unsigned are i16 and ui16, its
 * long s32 (a long has 64 bits on 64-bit Linux). */
#include <cstdint>
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using u32 = std::uint32_t;
using i16 = std::int16_t;
using ui16 = std::uint16_t;
/* The size of a type in the original, where BCC checked it. The port's
 * sizes differ (pointers, alignment, enums), so the macro only records it. */
#define ASSERT_SIZE(type, size) typedef char type##SizeCheck[sizeof(type) > 0]
/* A type the files and the saved games hold as bytes keeps the original's
 * layout: __attribute__((packed)) lays it out byte by byte as BCC does, bit
 * fields too, which both pack across the words. ASSERT_FILE_SIZE checks its
 * size; port/test's layout_test the place of each member against BCC's. */
#define ASSERT_FILE_SIZE(type, size) static_assert(sizeof(type) == (size), #type " as the files hold it")

/* The machine-level services of the original (port/compat). */
#include "port_machine.h"

#include <source_location>
#include <span>

/* The place of a call in the port's sources: the halts take the place they
 * are called from, and their messages name its file under port/ (CMake maps
 * the source directory away), its line and its function. The original passed
 * its own __FILE__ and __LINE__, which the two games' sources had at other
 * lines. A build with PORT_PLACES=OFF (a comparison of objects', which
 * would otherwise change with every line that moves) gives the halts no
 * place. */
#ifndef PORT_PLACES
#define PORT_PLACES 1
#endif
#if PORT_PLACES
#define CALL_PLACE std::source_location::current()
#else
#define CALL_PLACE std::source_location()
#endif

/* 12d0:00fd-049e: fatal errors and debug output. A module of
 * its own: TLINK packed it into the segment between the joystick module and
 * the random module, before SYSTEM\SYSTEM.C (12d0:0513). Its file name is not
 * in the binary. Each halt shuts the system down once, prints its message
 * between two rules and exits with status 1. */
/* The debugPrintf functions print when their level is at least this. */
extern i16 &debugLevel;
/* Each ends the program (exit(1)), which the port says with [[noreturn]].
 * "Halted file %s, line %d." (12d0:00fd) */
[[noreturn]] void halt(std::source_location place = CALL_PLACE);
/* The same, followed by a message (12d0:014d); the original's took a
 * printf-style format, which haltFormat formats. */
[[noreturn]] void haltMessage(const char *message, std::source_location place = CALL_PLACE);
/* A halt's message from a printf-style format, in a buffer of its own that
 * the next call overwrites. */
[[gnu::format(printf, 1, 2)]] const char *haltFormat(const char *format, ...);
/* "Out of memory in file %s, line %d." (12d0:01fe) */
[[noreturn]] void allocationFailure(std::source_location place = CALL_PLACE);
/* "Unable to open file %s, in file %s, line %d.", then a breakpoint (12d0:02eb). */
[[noreturn]] void openFailure(const char *name, std::source_location place = CALL_PLACE);
/* Formats into formatBuffer and prints it when level is at least debugLevel (12d0:0342). */
[[gnu::format(printf, 2, 3)]] void debugPrintf(i16 level, const char *format, ...);
/* debugPrintf, then waits for a key (12d0:03ee). */
[[gnu::format(printf, 2, 3)]] void debugPrintfWait(i16 level, const char *format, ...);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void allocationFailureMessage(const char *message, std::source_location place = CALL_PLACE);
/* UNCALLED: nothing calls it. */
[[gnu::format(printf, 4, 5)]] void debugPrintfAt(i16 level, i16 x, i16 y, const char *format, ...);
/* UNCALLED: nothing calls it. */
[[gnu::format(printf, 4, 5)]] void debugPrintfAtWait(i16 level, i16 x, i16 y, const char *format, ...);

/* The first entry of a game's table (its value header's): a C array when
 * the two games' are of a size, a span when they differ
 * (tools/make_unified_values.py). */
template <typename T> const T *tableStart(const T *table)
{
    return table;
}
template <typename T> const T *tableStart(std::span<const T> table)
{
    return table.data();
}
#endif
