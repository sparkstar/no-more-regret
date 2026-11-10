/* The pass dump's records to the file of REGRET_PASS_DUMP (passdump.h); the
 * game's state at a pass's end comes from passDumpStateHook (passstate.cpp). */
#include "passdump.h"
#include "pc.h"
#include "report.h"
#include "window.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

bool (*passDumpStateHook)() = nullptr;

namespace {

std::FILE *file = nullptr;

/* The records go in large pieces, a pass's at once. */
const std::size_t BUFFER_SIZE = 1U << 20U;

[[maybe_unused]] const bool STARTED = [] {
    if (const char *path = std::getenv("REGRET_PASS_DUMP")) {
        file = std::fopen(path, "wb");
        if (file == nullptr) {
            portRefuse("REGRET_PASS_DUMP %s cannot be written", path);
        }
        std::setvbuf(file, nullptr, _IOFBF, BUFFER_SIZE);
#ifdef SIGPIPE
        /* A FIFO whose reader has gone fails the write (readerGone). */
        std::signal(SIGPIPE, SIG_IGN);
#endif
    }
    return true;
}();

/* The pass oracle stops reading when it has compared its passes: the run
 * ends there as a run's end does (report.h's portEnd), its other files (the
 * usecode's trace) written out and its replay's sandbox gone, the dump
 * written to no more. */
[[noreturn]] void readerGone()
{
    file = nullptr;
    std::fflush(nullptr);
    windowExit(0);
}

void put(const void *bytes, std::size_t size)
{
    if (std::fwrite(bytes, 1, size, file) != size) {
        readerGone();
    }
}

void add(std::vector<std::uint8_t> &payload, std::uint64_t value, int bytes)
{
    for (int k = 0; k < bytes; k++) {
        payload.push_back(static_cast<std::uint8_t>(value >> (8 * k)));
    }
}

}

bool passDumping()
{
    return file != nullptr;
}

void passDumpRecord(char tag, const std::vector<std::uint8_t> &payload)
{
    if (file == nullptr) {
        return;
    }
    const std::uint8_t head[5] = {static_cast<std::uint8_t>(tag), static_cast<std::uint8_t>(payload.size()),
                                  static_cast<std::uint8_t>(payload.size() >> 8),
                                  static_cast<std::uint8_t>(payload.size() >> 16),
                                  static_cast<std::uint8_t>(payload.size() >> 24)};
    put(head, sizeof head);
    put(payload.data(), payload.size());
}

void passDumpTime(char kind, std::uint64_t until)
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload{static_cast<std::uint8_t>(kind)};
    add(payload, until, 8);
    passDumpRecord('T', payload);
}

void passDumpTick()
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload{0};
    add(payload, pcTime(), 8);
    passDumpRecord('I', payload);
}

void passDumpScan(std::uint8_t scan)
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload{1};
    add(payload, pcTime(), 8);
    payload.push_back(scan);
    passDumpRecord('I', payload);
}

void passDumpDevice(std::uint8_t kind)
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload{1};
    add(payload, pcTime(), 8);
    payload.push_back(0xff);
    payload.push_back(kind);
    passDumpRecord('I', payload);
}

void passDumpMouse(std::uint16_t events, std::uint16_t buttons, std::int16_t x, std::int16_t y, bool handled)
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload{12};
    add(payload, pcTime(), 8);
    add(payload, events, 2);
    add(payload, buttons, 2);
    add(payload, static_cast<std::uint16_t>(x), 2);
    add(payload, static_cast<std::uint16_t>(y), 2);
    payload.push_back(handled ? 1 : 0);
    passDumpRecord('I', payload);
}

void passDumpSound(std::uint8_t number, std::initializer_list<std::uint16_t> arguments, std::uint16_t result,
                   const std::uint8_t *bytes, std::uint16_t size)
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload{number, static_cast<std::uint8_t>(arguments.size())};
    for (const std::uint16_t argument : arguments) {
        add(payload, argument, 2);
    }
    add(payload, result, 2);
    if (bytes != nullptr) {
        payload.insert(payload.end(), bytes, bytes + size);
    }
    passDumpRecord('A', payload);
}

void passDumpSoundCallback(std::int16_t channel)
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload;
    add(payload, static_cast<std::uint16_t>(channel), 2);
    passDumpRecord('B', payload);
}

void passDumpAction(char tag, std::int16_t argument)
{
    if (file == nullptr) {
        return;
    }
    std::vector<std::uint8_t> payload;
    add(payload, static_cast<std::uint16_t>(argument), 2);
    passDumpRecord(tag, payload);
}

void passDumpEnd()
{
    if (file == nullptr) {
        return;
    }
    passDumpRecord('Q', {});
    if (std::fflush(file) != 0) {
        readerGone();
    }
}

void passDumpPass(std::uint64_t pass, const char *line)
{
    if (file == nullptr) {
        return;
    }
    passDumpRecord('S', std::vector<std::uint8_t>(line, line + std::strlen(line)));
    const bool made = (passDumpStateHook != nullptr) && passDumpStateHook();
    std::vector<std::uint8_t> payload;
    add(payload, pass, 8);
    payload.push_back(made ? 1 : 0);
    passDumpRecord('E', payload);
    if (std::fflush(file) != 0) {
        readerGone();
    }
}
