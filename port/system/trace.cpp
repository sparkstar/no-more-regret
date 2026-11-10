/* The usecode's trace (platform/trace.h): the lines to the file of
 * REGRET_USECODE_TRACE, the pass each comes in counted by the state log
 * (statelog.cpp, whose count is of the passes ended). */
#include "trace.h"
#include "pc.h"
#include "report.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

std::FILE *file = nullptr;

/* The usecode's globals at the last pass's end (traceGlobals), zeros
 * before the first. */
std::vector<std::uint8_t> seenGlobals;

/* The traces of a long recording are large: written in large pieces. */
const std::size_t BUFFER_SIZE = 1U << 20U;

[[maybe_unused]] const bool STARTED = [] {
    if (const char *path = std::getenv("REGRET_USECODE_TRACE")) {
        file = std::fopen(path, "w");
        if (!file) {
            portRefuse("REGRET_USECODE_TRACE %s cannot be written", path);
        }
        std::setvbuf(file, nullptr, _IOFBF, BUFFER_SIZE);
    }
    return true;
}();

unsigned long long pass()
{
    return static_cast<unsigned long long>(stateLogPasses()) + 1;
}

}

void traceEvent(std::uint16_t item, std::uint16_t classId, std::uint16_t event)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu E %u %04X:%04X\n", pass(), item, classId, event);
    }
}

void traceProcessMade(std::uint16_t pid, std::uint16_t creator)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu + %u %u\n", pass(), pid, creator);
    }
}

void traceUsecodeStart(std::uint16_t pid, std::uint16_t classId, std::uint16_t function)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu U %u %04X:%04X\n", pass(), pid, classId, function);
    }
}

void traceCall(std::uint16_t pid, std::uint16_t classId, std::uint16_t function)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu C %u %04X:%04X\n", pass(), pid, classId, function);
    }
}

void traceIntrinsic(std::uint16_t pid, std::uint16_t classId, std::uint16_t function, std::uint16_t number,
                    const std::uint8_t *bytes, std::uint16_t size, std::int32_t result, bool waited)
{
    if (file == nullptr) {
        return;
    }
    std::fprintf(file, "%llu I %u %04X:%04X %03X", pass(), pid, classId, function, number);
    for (std::uint16_t at = 0; at < size; at++) {
        std::fprintf(file, " %02X", bytes[at]);
    }
    if (waited) {
        std::fputs(" = wait\n", file);
    } else {
        std::fprintf(file, " = %X\n", static_cast<unsigned>(result));
    }
}

void traceIntrinsicResult(std::uint16_t pid, std::int32_t result)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu R %u %X\n", pass(), pid, static_cast<unsigned>(result));
    }
}

void traceProcessKilled(std::uint16_t pid, std::int16_t type, std::uint16_t item, const char *name)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu - %u %d %u %s\n", pass(), pid, type, item, (name != nullptr) ? name : "-");
    }
}

void traceTeleport(std::uint16_t npc, std::uint16_t x, std::uint16_t y, std::uint8_t z, std::uint8_t map)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu T %u %u %u %u %u\n", pass(), npc, x, y, z, map);
    }
}

void traceMapLoaded(std::uint16_t map)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu M %u\n", pass(), map);
    }
}

void traceFollowed(std::uint16_t item)
{
    if (file != nullptr) {
        std::fprintf(file, "%llu F %u\n", pass(), item);
    }
}

void traceGlobals(const std::uint8_t *globals, std::uint16_t size)
{
    if ((file == nullptr) || (globals == nullptr)) {
        return;
    }
    if (seenGlobals.size() < size) {
        seenGlobals.resize(size, 0);
    }
    std::uint16_t at = 0;
    while (at < size) {
        if (globals[at] == seenGlobals[at]) {
            at++;
            continue;
        }
        std::fprintf(file, "%llu G %04X", pass(), static_cast<unsigned>(at));
        while ((at < size) && (globals[at] != seenGlobals[at])) {
            std::fprintf(file, " %02X", static_cast<unsigned>(globals[at]));
            seenGlobals[at] = globals[at];
            at++;
        }
        std::fputc('\n', file);
    }
}
