/* See gamememory.h. */
#include "gamememory.h"
#include "gamestate.h"

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <new>

#if defined(__SANITIZE_ADDRESS__)
#define PORT_ARENA_POISON 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define PORT_ARENA_POISON 1
#endif
#endif
#if defined(PORT_ARENA_POISON)
#include <sanitizer/asan_interface.h>
#endif

namespace {

/* The arena's bytes: the host gives pages as they are touched. */
constexpr std::size_t ARENA_BYTES = std::size_t{128} << 20;
/* The blocks' alignment, the host's new's. */
constexpr std::size_t ALIGN = 16;

/* Which new made a block. */
enum Kind : std::uint8_t { SINGLE = 1, ARRAY = 2 };

/* A chunk of the arena: its header, then the bytes it gives. Its size is
 * the header's and the bytes', a multiple of ALIGN; previous is the size of
 * the chunk below it, 0 for the first. */
struct Chunk {
    std::size_t size;
    std::size_t previous;
    std::uint32_t asked;
    std::uint8_t used;
    Kind kind;
    std::uint8_t unused[2];
    const void *caller;
};
static_assert(sizeof(Chunk) % ALIGN == 0);
constexpr std::size_t HEADER = sizeof(Chunk);

/* A free chunk's links in its bin, in the bytes it would give. */
struct Links {
    Chunk *next;
    Chunk *previous;
};
constexpr std::size_t SMALLEST = HEADER + ((sizeof(Links) + ALIGN - 1) / ALIGN * ALIGN);

/* The bins of the free chunks: below 1024 bytes one size each (by ALIGN),
 * then one power of two each. */
constexpr int SMALL_BINS = 1024 / ALIGN;
constexpr int BINS = SMALL_BINS + 24;

/* The allocator's state, the arena's first bytes. */
struct Control {
    Chunk *bins[BINS];
    /* The first byte above the chunks, and the size of the chunk below it
     * (0 for none). */
    std::byte *top;
    std::size_t topPrevious;
    std::byte *end;
    std::size_t liveBlocks;
    std::size_t liveBytes;
    std::size_t peakBytes;
};
constexpr std::size_t CONTROL = (sizeof(Control) + ALIGN - 1) / ALIGN * ALIGN;
/* The part of the game's globals (platform/gamestate.h) after the
 * allocator's state; the chunks begin after it. */
constexpr std::size_t GLOBALS = std::size_t{256} << 10;
constexpr std::size_t HEAP = CONTROL + GLOBALS;

thread_local bool gameSide = false;

/* What is wrong with the game's memory, and the run's end (as report.h's
 * portEnd, which the tests that link this file alone do not have). */
[[noreturn, gnu::format(printf, 1, 2)]] void memoryFailure(const char *format, ...)
{
    std::fflush(stdout);
    std::fputs("game memory: ", stderr);
    va_list arguments;
    va_start(arguments, format);
    std::vfprintf(stderr, format, arguments);
    va_end(arguments);
    std::fputc('\n', stderr);
    std::fflush(stderr);
    std::_Exit(3);
}
std::byte *arena = nullptr;
Control *control = nullptr;
/* The globals' part given so far, each global's place and bytes (in the
 * order of their places), and their destructors, which the program's end
 * runs in the reverse order (lists on the host's heap: the globals are
 * given at the program's start only). */
std::size_t globalsUsed = 0;
struct GlobalPlace {
    const void *place;
    std::size_t bytes;
};
std::vector<GlobalPlace> &globalPlaces()
{
    static std::vector<GlobalPlace> places;
    return places;
}
struct GlobalEnd {
    void *place;
    void (*destroy)(void *);
};
std::vector<GlobalEnd> &globalEnds()
{
    static std::vector<GlobalEnd> ends;
    return ends;
}

/* The settings' part (platform/gamestate.h's gameSetting), outside the
 * arena: static storage, zero before the program's start binds the first;
 * the part given so far and each setting's place and bytes, as the
 * globals'. */
constexpr std::size_t SETTINGS = std::size_t{64} << 10;
alignas(ALIGN) std::byte settingsPart[SETTINGS];
std::size_t settingsUsed = 0;
std::vector<GlobalPlace> &settingPlaces()
{
    static std::vector<GlobalPlace> places;
    return places;
}

/* A place's bytes in a list of places in the order of their places, 0 for
 * none. */
std::size_t bytesOf(const std::vector<GlobalPlace> &places, const void *place)
{
    const auto found = std::lower_bound(places.begin(), places.end(), place, [](const GlobalPlace &p, const void *at) {
        return std::less<const void *>()(p.place, at);
    });
    return found != places.end() && found->place == place ? found->bytes : 0;
}

bool inArena(const void *pointer)
{
    const auto *byte = static_cast<const std::byte *>(pointer);
    return arena != nullptr && byte >= arena && byte < arena + ARENA_BYTES;
}

int binOf(std::size_t size)
{
    if (size < 1024) {
        return static_cast<int>(size / ALIGN);
    }
    int bin = SMALL_BINS;
    std::size_t limit = 2048;
    while (size >= limit && bin < BINS - 1) {
        limit <<= 1;
        bin++;
    }
    return bin;
}

[[gnu::no_sanitize_address]] Links *linksOf(Chunk *chunk)
{
    return reinterpret_cast<Links *>(reinterpret_cast<std::byte *>(chunk) + HEADER);
}

[[gnu::no_sanitize_address]] Chunk *chunkAt(std::byte *place)
{
    return reinterpret_cast<Chunk *>(place);
}

[[gnu::no_sanitize_address]] std::byte *placeOf(Chunk *chunk)
{
    return reinterpret_cast<std::byte *>(chunk);
}

[[gnu::no_sanitize_address]] void insertFree(Chunk *chunk)
{
    const int bin = binOf(chunk->size);
    Links *links = linksOf(chunk);
    links->previous = nullptr;
    links->next = control->bins[bin];
    if (links->next != nullptr) {
        linksOf(links->next)->previous = chunk;
    }
    control->bins[bin] = chunk;
}

[[gnu::no_sanitize_address]] void removeFree(Chunk *chunk)
{
    Links *links = linksOf(chunk);
    if (links->previous != nullptr) {
        linksOf(links->previous)->next = links->next;
    } else {
        control->bins[binOf(chunk->size)] = links->next;
    }
    if (links->next != nullptr) {
        linksOf(links->next)->previous = links->previous;
    }
}

/* The chunk above chunk now has a new size below it. */
[[gnu::no_sanitize_address]] void setAbove(Chunk *chunk)
{
    std::byte *above = placeOf(chunk) + chunk->size;
    if (above == control->top) {
        control->topPrevious = chunk->size;
    } else {
        chunkAt(above)->previous = chunk->size;
    }
}

/* Keeps size bytes of chunk; the rest, if a chunk can be made of it, free. */
[[gnu::no_sanitize_address]] void split(Chunk *chunk, std::size_t size)
{
    if (chunk->size - size < SMALLEST) {
        return;
    }
    Chunk *rest = chunkAt(placeOf(chunk) + size);
    rest->size = chunk->size - size;
    rest->previous = size;
    rest->used = 0;
    chunk->size = size;
    setAbove(rest);
    insertFree(rest);
}

void poison(void *start, std::size_t bytes)
{
#if defined(PORT_ARENA_POISON)
    ASAN_POISON_MEMORY_REGION(start, bytes);
#else
    (void)start;
    (void)bytes;
#endif
}

void unpoison(void *start, std::size_t bytes)
{
#if defined(PORT_ARENA_POISON)
    ASAN_UNPOISON_MEMORY_REGION(start, bytes);
#else
    (void)start;
    (void)bytes;
#endif
}

void closeArena();

void openArena()
{
    arena = static_cast<std::byte *>(std::malloc(ARENA_BYTES));
    if (arena == nullptr) {
        memoryFailure("no %llu MB for the arena", static_cast<unsigned long long>(ARENA_BYTES >> 20));
    }
    control = new (arena) Control{};
    control->top = arena + HEAP;
    control->end = arena + ARENA_BYTES;
    /* The arena outlives what is made in it, whose ends register after
     * this (the globals' destructors, destroyGlobals): an exit runs them in
     * the reverse order. Not by an initializer's priority, which macOS's
     * start does not take. */
    std::atexit(closeArena);
    /* Zero as static storage is. */
    std::memset(arena + CONTROL, 0, GLOBALS);
    poison(arena + HEAP, ARENA_BYTES - HEAP);
}

[[gnu::no_sanitize_address]] void *arenaAllocate(std::size_t bytes, Kind kind, const void *caller)
{
    if (arena == nullptr) {
        openArena();
    }
    if (bytes > ARENA_BYTES) {
        memoryFailure("%llu bytes asked, more than the arena's", static_cast<unsigned long long>(bytes));
    }
    std::size_t size = (bytes + HEADER + ALIGN - 1) / ALIGN * ALIGN;
    size = std::max(size, SMALLEST);
    Chunk *chunk = nullptr;
    for (int bin = binOf(size); bin < BINS && chunk == nullptr; bin++) {
        for (Chunk *candidate = control->bins[bin]; candidate != nullptr; candidate = linksOf(candidate)->next) {
            if (candidate->size >= size) {
                chunk = candidate;
                break;
            }
        }
    }
    if (chunk != nullptr) {
        removeFree(chunk);
        split(chunk, size);
    } else {
        if (static_cast<std::size_t>(control->end - control->top) < size) {
            memoryFailure("the arena's %llu MB are full (%llu bytes asked)",
                          static_cast<unsigned long long>(ARENA_BYTES >> 20), static_cast<unsigned long long>(bytes));
        }
        chunk = chunkAt(control->top);
        chunk->size = size;
        chunk->previous = control->topPrevious;
        control->top += size;
        control->topPrevious = size;
    }
    chunk->used = 1;
    chunk->kind = kind;
    chunk->asked = static_cast<std::uint32_t>(bytes);
    chunk->caller = caller;
    control->liveBlocks++;
    control->liveBytes += chunk->size;
    control->peakBytes = std::max(control->peakBytes, control->liveBytes);
    std::byte *given = placeOf(chunk) + HEADER;
    unpoison(given, bytes);
    return given;
}

[[gnu::no_sanitize_address]] void arenaRelease(void *pointer, Kind kind)
{
    Chunk *chunk = chunkAt(static_cast<std::byte *>(pointer) - HEADER);
    if (chunk->used == 0) {
        memoryFailure("%p given back twice", pointer);
    }
    if (chunk->kind != kind) {
        memoryFailure("%p made by %s (at %p) given back by %s", pointer, chunk->kind == ARRAY ? "new[]" : "new",
                      chunk->caller, kind == ARRAY ? "delete[]" : "delete");
    }
    poison(pointer, chunk->size - HEADER);
    chunk->used = 0;
    control->liveBlocks--;
    control->liveBytes -= chunk->size;
    std::byte *above = placeOf(chunk) + chunk->size;
    if (above != control->top && chunkAt(above)->used == 0) {
        Chunk *next = chunkAt(above);
        removeFree(next);
        chunk->size += next->size;
    }
    if (chunk->previous != 0) {
        Chunk *below = chunkAt(placeOf(chunk) - chunk->previous);
        if (below->used == 0) {
            removeFree(below);
            below->size += chunk->size;
            chunk = below;
        }
    }
    if (placeOf(chunk) + chunk->size == control->top) {
        control->top = placeOf(chunk);
        control->topPrevious = chunk->previous;
        return;
    }
    setAbove(chunk);
    insertFree(chunk);
}

/* The blocks still used at the run's end, reported (PORT_RELEASE_CHECK).
 * Their callers are given from gameMemory's address in the program, which
 * a host that loads the program anywhere keeps: gameMemory's address in nm
 * plus that, through addr2line, names the caller. */
[[gnu::no_sanitize_address]] void checkReleased()
{
    if (control == nullptr || control->liveBlocks == 0 || std::getenv("PORT_RELEASE_CHECK") == nullptr) {
        return;
    }
    std::fflush(stdout);
    std::fprintf(stderr, "game memory: %llu blocks (%llu bytes) not given back at the end\n",
                 static_cast<unsigned long long>(control->liveBlocks),
                 static_cast<unsigned long long>(control->liveBytes));
    int shown = 0;
    for (std::byte *place = arena + HEAP; place < control->top && shown < 40; place += chunkAt(place)->size) {
        const Chunk *chunk = chunkAt(place);
        if (chunk->used != 0) {
            std::fprintf(stderr, "  %u bytes by %s at gameMemory%+lld\n", chunk->asked,
                         chunk->kind == ARRAY ? "new[]" : "new",
                         static_cast<long long>(static_cast<const char *>(chunk->caller) -
                                                reinterpret_cast<const char *>(&gameMemory)));
            shown++;
        }
    }
    std::fflush(stderr);
    std::_Exit(23);
}

/* The arena's end at the program's exit (openArena registers it). */
void closeArena()
{
    if (control != nullptr && std::getenv("PORT_MEMORY_REPORT") != nullptr) {
        std::fflush(stdout);
        std::fprintf(stderr,
                     "game memory: %llu bytes at most in blocks, %llu up to the top, %llu blocks left, %llu bytes of "
                     "globals, %llu of settings\n",
                     static_cast<unsigned long long>(control->peakBytes),
                     static_cast<unsigned long long>(control->top - arena),
                     static_cast<unsigned long long>(control->liveBlocks), static_cast<unsigned long long>(globalsUsed),
                     static_cast<unsigned long long>(settingsUsed));
    }
    checkReleased();
    if (arena != nullptr) {
        unpoison(arena, ARENA_BYTES);
        std::free(arena);
        arena = nullptr;
        control = nullptr;
    }
}

/* The globals' destructors, the last given the first destroyed. */
void destroyGlobals()
{
    std::vector<GlobalEnd> &ends = globalEnds();
    while (!ends.empty()) {
        const GlobalEnd end = ends.back();
        ends.pop_back();
        end.destroy(end.place);
    }
}

}

GameSide::GameSide() : before(gameSide)
{
    gameSide = true;
}

GameSide::~GameSide()
{
    gameSide = before;
}

SystemSide::SystemSide() : before(gameSide)
{
    gameSide = false;
}

SystemSide::~SystemSide()
{
    gameSide = before;
}

void *gameGlobalPlace(std::size_t bytes, std::size_t alignment, void (*destroy)(void *))
{
    const SystemSide systemSide;
    if (arena == nullptr) {
        openArena();
    }
    std::byte *const globals = arena + CONTROL;
    const auto at = reinterpret_cast<std::uintptr_t>(globals + globalsUsed);
    const std::uintptr_t aligned = (at + alignment - 1) / alignment * alignment;
    const std::size_t offset = globalsUsed + static_cast<std::size_t>(aligned - at);
    if (offset + bytes > GLOBALS) {
        memoryFailure("the game's globals past their %llu KB of the arena",
                      static_cast<unsigned long long>(GLOBALS >> 10));
    }
    globalsUsed = offset + bytes;
    globalPlaces().push_back({globals + offset, bytes});
    if (destroy != nullptr) {
        std::vector<GlobalEnd> &ends = globalEnds();
        if (ends.empty()) {
            /* After the list's own end, before the arena's. */
            std::atexit(destroyGlobals);
        }
        ends.push_back({globals + offset, destroy});
    }
    return globals + offset;
}

void *gameSettingPlace(std::size_t bytes, std::size_t alignment)
{
    const SystemSide systemSide;
    const auto at = reinterpret_cast<std::uintptr_t>(settingsPart + settingsUsed);
    const std::uintptr_t aligned = (at + alignment - 1) / alignment * alignment;
    const std::size_t offset = settingsUsed + static_cast<std::size_t>(aligned - at);
    if (offset + bytes > SETTINGS) {
        memoryFailure("the game's settings past their %llu KB", static_cast<unsigned long long>(SETTINGS >> 10));
    }
    settingsUsed = offset + bytes;
    settingPlaces().push_back({settingsPart + offset, bytes});
    return settingsPart + offset;
}

std::size_t gameGlobalBytes(const void *place)
{
    const std::size_t global = bytesOf(globalPlaces(), place);
    return global != 0 ? global : bytesOf(settingPlaces(), place);
}

GameMemory gameMemory()
{
    if (arena == nullptr) {
        return {};
    }
    return {arena, static_cast<std::size_t>(control->top - arena)};
}

GameMemory gameSettings()
{
    return {settingsPart, settingsUsed};
}

std::size_t gameMemoryPeak()
{
    return control != nullptr ? control->peakBytes : 0;
}

void *portAllocate(std::size_t bytes, bool array, const void *caller)
{
    if (gameSide) {
        return arenaAllocate(bytes, array ? ARRAY : SINGLE, caller);
    }
    for (;;) {
        if (void *given = std::malloc(bytes != 0 ? bytes : 1)) {
            return given;
        }
        std::new_handler handler = std::get_new_handler();
        if (handler == nullptr) {
            throw std::bad_alloc();
        }
        handler();
    }
}

void portRelease(void *pointer, bool array)
{
    if (pointer == nullptr) {
        return;
    }
    if (inArena(pointer)) {
        arenaRelease(pointer, array ? ARRAY : SINGLE);
    } else {
        std::free(pointer);
    }
}

void *portAllocateAligned(std::size_t bytes, std::align_val_t alignment, bool array, const void *caller)
{
    const auto align = static_cast<std::size_t>(alignment);
    if (align <= ALIGN) {
        return portAllocate(bytes, array, caller);
    }
    if (gameSide) {
        memoryFailure("an alignment of %llu, more than the arena's", static_cast<unsigned long long>(align));
    }
    const std::size_t size = (bytes + align - 1) / align * align;
#if defined(_WIN32)
    void *given = _aligned_malloc(size != 0 ? size : align, align);
#else
    void *given = std::aligned_alloc(align, size != 0 ? size : align);
#endif
    if (given == nullptr) {
        throw std::bad_alloc();
    }
    return given;
}

void portReleaseAligned(void *pointer, std::align_val_t alignment, bool array)
{
    if (static_cast<std::size_t>(alignment) <= ALIGN || inArena(pointer)) {
        portRelease(pointer, array);
        return;
    }
#if defined(_WIN32)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

[[gnu::no_sanitize_address]] void copyUnchecked(std::byte *to, const std::byte *from, std::size_t bytes)
{
#if defined(PORT_ARENA_POISON)
    /* Byte by byte, as the sanitizer's memcpy would check the poison. */
    volatile std::byte *target = to;
    for (std::size_t i = 0; i < bytes; i++) {
        target[i] = from[i];
    }
#else
    std::memcpy(to, from, bytes);
#endif
}

[[gnu::no_sanitize_address]] bool sameUnchecked(const std::byte *one, const std::byte *other, std::size_t bytes)
{
#if defined(PORT_ARENA_POISON)
    /* Byte by byte, as the sanitizer's memcmp would check the poison. */
    const volatile std::byte *left = one;
    for (std::size_t i = 0; i < bytes; i++) {
        if (left[i] != other[i]) {
            return false;
        }
    }
    return true;
#else
    return std::memcmp(one, other, bytes) == 0;
#endif
}

std::vector<std::byte> gameMemoryCopy()
{
    const GameMemory memory = gameMemory();
    std::vector<std::byte> bytes(memory.used);
    copyUnchecked(bytes.data(), memory.start, memory.used);
    return bytes;
}

namespace {
/* The poison of the arena as its chunks have it (put back by a copy). */
[[gnu::no_sanitize_address]] void layPoison()
{
    unpoison(arena + CONTROL, ARENA_BYTES - CONTROL);
    poison(control->top, static_cast<std::size_t>(control->end - control->top));
    for (std::byte *place = arena + HEAP; place < control->top; place += chunkAt(place)->size) {
        const Chunk *chunk = chunkAt(place);
        poison(place, chunk->size);
        if (chunk->used != 0) {
            unpoison(place + HEADER, chunk->asked);
        }
    }
}
}

void gameMemoryPut(const std::vector<std::byte> &bytes)
{
    if (arena == nullptr || bytes.size() < HEAP || bytes.size() > ARENA_BYTES) {
        memoryFailure("a copy of the arena that is not one");
    }
    unpoison(arena, ARENA_BYTES);
    copyUnchecked(arena, bytes.data(), bytes.size());
    layPoison();
}

void gameMemoryWritten()
{
    if (arena != nullptr) {
        layPoison();
    }
}
