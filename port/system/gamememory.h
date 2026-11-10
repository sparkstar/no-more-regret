/* The game's memory: what the game's code allocates comes from one region
 * of the port's own, the arena, and what the system layer allocates from
 * the host's heap, so that the game's state is in places the port knows and
 * can copy and put back (memory snapshots: savestate, loadstate, rewind).
 * The global operator new and delete are the port's: new takes from the
 * arena on the game's side of the boundary and from the host's heap on the
 * system's; delete knows the arena by the address.
 *
 * The side is the thread's: the game's from playGame (end.h) and from each
 * callback the system layer makes into the game (the clock's ticks, the
 * keys, the mouse, the devices, the window's close, the script's actions),
 * the system's in each platform function (the headers of port/platform). Each GameSide or
 * SystemSide object sets the side for its scope and gives back the side
 * before it at its end, an exception's unwinding too.
 *
 * The arena's allocator is deterministic (the same calls give the same
 * places) and keeps its state in the arena's first bytes, so that a copy of
 * the arena up to its top is the allocator's state too. A block remembers
 * the bytes asked, whether new or new[] made it, which delete or delete[]
 * must give it back, and where it was asked; at the run's end, with
 * PORT_RELEASE_CHECK set, a block not given back is reported and the run
 * ends with 23, as LeakSanitizer's (test/setup.cmake's release_checked).
 * PORT_MEMORY_REPORT set reports the arena's use at the end.
 * With the address sanitizer the arena's free bytes and the blocks' headers
 * and slack are poisoned. */
#ifndef PORT_GAMEMEMORY_H
#define PORT_GAMEMEMORY_H
#include <cstddef>
#include <new>
#include <vector>

/* The game's side of the boundary for the object's scope. */
class GameSide {
public:
    GameSide();
    ~GameSide();
    GameSide(const GameSide &) = delete;
    GameSide &operator=(const GameSide &) = delete;
    GameSide(GameSide &&) = delete;
    GameSide &operator=(GameSide &&) = delete;

private:
    bool before;
};

/* The system's side of the boundary for the object's scope: the first
 * statement of each platform function. */
class SystemSide {
public:
    SystemSide();
    ~SystemSide();
    SystemSide(const SystemSide &) = delete;
    SystemSide &operator=(const SystemSide &) = delete;
    SystemSide(SystemSide &&) = delete;
    SystemSide &operator=(SystemSide &&) = delete;

private:
    bool before;
};

/* The arena's bytes in use: from its start to its top, the allocator's
 * state and every block, used or free; none before the game's first
 * allocation. */
struct GameMemory {
    std::byte *start = nullptr;
    std::size_t used = 0;
};
[[nodiscard]] GameMemory gameMemory();
/* The settings' part's bytes in use (platform/gamestate.h's gameSetting):
 * outside the arena, which a snapshot does not hold, but a rewind and a
 * state do while the run follows a recording. */
[[nodiscard]] GameMemory gameSettings();
/* The most bytes the blocks held at once. */
[[nodiscard]] std::size_t gameMemoryPeak();

/* The bytes of the game's global gameGlobalPlace gave place, or of the
 * setting gameSettingPlace gave it (platform/gamestate.h), 0 for none: for
 * a test that finds a global by its symbol, the reference's. */
[[nodiscard]] std::size_t gameGlobalBytes(const void *place);

/* Copies bytes the address sanitizer may have poisoned (the arena's free
 * bytes and headers, the globals' and the stack frames' redzones), for the
 * snapshots (snapshot.h). */
void copyUnchecked(std::byte *to, const std::byte *from, std::size_t bytes);
/* Whether bytes at two places are the same, read as copyUnchecked reads
 * them. */
[[nodiscard]] bool sameUnchecked(const std::byte *one, const std::byte *other, std::size_t bytes);
/* A copy of the arena's bytes in use (gameMemory's), and the arena put back
 * to such a copy: the allocator's state is in it, and with the address
 * sanitizer the poison is laid again by its chunks. */
[[nodiscard]] std::vector<std::byte> gameMemoryCopy();
void gameMemoryPut(const std::vector<std::byte> &bytes);
/* The arena written in place (a rewind's, rewind.h): with the address
 * sanitizer the poison laid again by its chunks. */
void gameMemoryWritten();

/* newdelete.cpp's: bytes from the arena on the game's side, else from the
 * host's heap; array for new[]; caller where it was asked. And back to where
 * pointer came from. */
[[nodiscard]] void *portAllocate(std::size_t bytes, bool array, const void *caller);
void portRelease(void *pointer, bool array);
[[nodiscard]] void *portAllocateAligned(std::size_t bytes, std::align_val_t alignment, bool array, const void *caller);
void portReleaseAligned(void *pointer, std::align_val_t alignment, bool array);
#endif
