/* 12d0:00fd-049e, file name unknown: fatal errors and debug output. See
 * common.h. */
#include "gamestate.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <crash.h>
#include "common.h"
#include "system.h"
#include "input.h"
#include "host.h"

namespace {
/* Set by the first halt, so that a failure while halting cannot recurse. */
u8 &halting = gameGlobal<u8>(0);
/* A double rule (code page 437 box drawing) around every halt message. */
char (&separator)[39] = gameGlobal<char[39]>("\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
                                             "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD"
                                             "\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\r\n");
}
i16 &debugLevel = gameGlobal<i16>();
namespace {
/* What haltFormat made. */
char (&haltText)[1024] = gameGlobal<char[1024]>();

/* What a halt printed, also where a run with a window shows it (the port's
 * host.h's hostEndMessage). */
[[gnu::format(printf, 1, 2)]] void ended(const char *format, ...)
{
    char text[256];
    va_list args;

    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    hostEndMessage(text);
}

/* Where a halt was called, as its messages say it: lead, then "file
 * common/src/cache/cache.cpp, line 310 (void f())"; nothing in a build
 * without places (PORT_PLACES=OFF), whose places have line 0. */
void placeText(char *text, size_t size, const char *lead, std::source_location place)
{
    if (place.line() == 0) {
        text[0] = 0;
        return;
    }
    snprintf(text, size, "%sfile %s, line %u (%s)", lead, place.file_name(), static_cast<unsigned>(place.line()),
             place.function_name());
}
}

const char *haltFormat(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(haltText, sizeof(haltText), format, args);
    va_end(args);
    return haltText;
}

void halt(std::source_location place)
{
    if (halting == 0U) {
        halting = 1;
        char where[512];
        placeText(where, sizeof(where), " ", place);
        char reason[640];
        snprintf(reason, sizeof(reason), "Halted%s.", where);
        crashReport(reason);
        systemShutdown(0);
        hostPrint("%s", separator);
        hostPrint("%s\r\n", reason);
        hostPrint("%s", separator);
        ended("%s", reason);
    }
    hostExit(1);
}

void haltMessage(const char *message, std::source_location place)
{
    if (halting == 0U) {
        halting = 1;
        char where[512];
        placeText(where, sizeof(where), " ", place);
        char reason[1664];
        snprintf(reason, sizeof(reason), "Halted%s. %s", where, (message != nullptr) ? message : "");
        crashReport(reason);
        systemShutdown(0);
        hostPrint("%s", separator);
        hostPrint("Halted%s.\r\n%s\r\n", where, (message != nullptr) ? message : "");
        hostPrint("%s", separator);
        ended("Halted%s.\n%s", where, (message != nullptr) ? message : "");
    }
    hostExit(1);
}

void allocationFailure(std::source_location place)
{
    if (halting == 0U) {
        halting = 1;
        char where[512];
        placeText(where, sizeof(where), " in ", place);
        char reason[640];
        snprintf(reason, sizeof(reason), "Out of memory%s.", where);
        crashReport(reason);
        systemShutdown(0);
        hostPrint("%s", separator);
        hostPrint("%s\r\n", reason);
        hostPrint("%s", separator);
        ended("%s", reason);
    }
    hostExit(1);
}

/* The same, followed by a message (12d0:024e). */
/* UNCALLED: nothing calls it. */
void allocationFailureMessage(const char *message, std::source_location place)
{
    if (halting == 0U) {
        halting = 1;
        char where[512];
        placeText(where, sizeof(where), " in ", place);
        char reason[1664];
        snprintf(reason, sizeof(reason), "Out of memory%s. %s", where, (message != nullptr) ? message : "");
        crashReport(reason);
        systemShutdown(0);
        hostPrint("%s", separator);
        hostPrint("Out of memory%s.\r\n%s\r\n", where, (message != nullptr) ? message : "");
        hostPrint("%s", separator);
        ended("Out of memory%s.\n%s", where, (message != nullptr) ? message : "");
    }
    hostExit(1);
}

void openFailure(const char *name, std::source_location place)
{
    if (halting == 0U) {
        halting = 1;
        char where[512];
        placeText(where, sizeof(where), ", in ", place);
        char reason[768];
        snprintf(reason, sizeof(reason), "Unable to open file %s%s.", name, where);
        crashReport(reason);
        hostPrint("%s", separator);
        hostPrint("%s\r\n", reason);
        hostPrint("%s", separator);
        hostDebugBreak();
        systemShutdown(0);
        ended("%s", reason);
    }
    hostExit(1);
}

void debugPrintf(i16 level, const char *format, ...)
{
    va_list args;

    if (format != formatBuffer) {
        va_start(args, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, args);
    }
    if (level >= debugLevel) {
        /* The original gives the text to printf as its format; Borland's
         * writes a lone % as it is, as "%s" does. */
        hostPrint("%s", formatBuffer);
    }
}

/* The same at a text position (12d0:0391). */
/* UNCALLED: nothing calls it. */
void debugPrintfAt(i16 level, i16 x, i16 y, const char *format, ...)
{
    va_list args;

    if (format != formatBuffer) {
        va_start(args, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, args);
    }
    if (level >= debugLevel) {
        hostTextAt(x, y);
        hostPrint("%s", formatBuffer);
    }
}

void debugPrintfWait(i16 level, const char *format, ...)
{
    va_list args;

    if (format != formatBuffer) {
        va_start(args, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, args);
    }
    if (level >= debugLevel) {
        hostPrint("%s", formatBuffer);
        inputKey();
    }
}

/* Meant as debugPrintfAt with a wait, but the same code as debugPrintfAt:
 * it does not wait (12d0:0442). */
/* UNCALLED: nothing calls it. */
void debugPrintfAtWait(i16 level, i16 x, i16 y, const char *format, ...)
{
    va_list args;

    if (format != formatBuffer) {
        va_start(args, format);
        vsnprintf(formatBuffer, FORMAT_BUFFER_SIZE, format, args);
    }
    if (level >= debugLevel) {
        hostTextAt(x, y);
        hostPrint("%s", formatBuffer);
    }
}
