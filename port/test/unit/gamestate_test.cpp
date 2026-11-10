/* The game's globals in the arena (platform/gamestate.h): globals of each
 * kind the game has, bound as the program starts. Each in the arena's part
 * for them, in the order of their definitions, aligned, with its initial
 * value (zero, a constructor's arguments, an aggregate's, an array's first
 * elements, a string, rows); a copy of the arena put back puts them back;
 * at the program's end they are destroyed in the reverse order (the last
 * lines, which ctest looks for). The settings have the same forms outside
 * the arena, which a copy put back leaves. */
#include "gamememory.h"
#include "gamestate.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH game_globals: %s\n", what.c_str());
    }
}

struct Rect {
    std::int16_t x, y, width, height;
};

struct alignas(64) Wide {
    std::uint8_t bytes[64];
};

/* A class whose end is seen. */
class Noted {
public:
    explicit Noted(char name) : name_(name) {}
    Noted(const Noted &) = delete;
    Noted &operator=(const Noted &) = delete;
    Noted(Noted &&) = delete;
    Noted &operator=(Noted &&) = delete;
    ~Noted() { std::printf("destroyed %c\n", name_); }

private:
    char name_;
};

std::uint16_t &zero = gameGlobal<std::uint16_t>();
int &five = gameGlobal<int>(5);
Rect &box = gameGlobal<Rect>({1, 2, 30, 40});
std::uint8_t (&table)[4] = gameGlobal<std::uint8_t[4]>({1, 2, 3});
char (&name)[16] = gameGlobal<char[16]>("usecode");
std::int16_t (&rows)[2][3] = gameGlobal<std::int16_t[2][3]>({{1, 2, 3}, {4}});
Rect (&boxes)[2] = gameGlobal<Rect[2]>({{5, 6, 7, 8}});
Wide &wide = gameGlobal<Wide>();
std::vector<int> &sevens = gameGlobal<std::vector<int>>(3, 7);
std::string (&words)[2] = gameGlobal<std::string[2]>({"one"});
Noted &first = gameGlobal<Noted>('a');
Noted &second = gameGlobal<Noted>('b');
/* A global's address as another's initial value, of the same file. */
int *const fivePlace = &five;
std::int16_t &skip = gameSetting<std::int16_t>(3);
Rect &window = gameSetting<Rect>({9, 8, 7, 6});
std::uint8_t (&steps)[4] = gameSetting<std::uint8_t[4]>({1, 2});
Rect (&corners)[2] = gameSetting<Rect[2]>();

/* Whether bytes at place lie in the arena's bytes in use. */
bool inArena(const void *place, std::size_t bytes)
{
    const GameMemory memory = gameMemory();
    const auto *byte = static_cast<const std::byte *>(place);
    return memory.start != nullptr && byte >= memory.start && byte + bytes <= memory.start + memory.used;
}

}

int main()
{
    check(zero == 0 && five == 5, "a scalar zero, one from its constructor's argument");
    check(box.x == 1 && box.y == 2 && box.width == 30 && box.height == 40, "an aggregate");
    check(table[0] == 1 && table[2] == 3 && table[3] == 0, "an array's first elements, the rest zero");
    check(std::strcmp(name, "usecode") == 0 && name[15] == 0, "a string");
    check(rows[0][2] == 3 && rows[1][0] == 4 && rows[1][2] == 0, "rows");
    check(boxes[0].height == 8 && boxes[1].x == 0 && boxes[1].height == 0, "an array of aggregates");
    check(reinterpret_cast<std::uintptr_t>(&wide) % 64 == 0, "an alignment of 64");
    check(sevens.size() == 3 && sevens[2] == 7, "a class from its constructor's arguments");
    check(words[0] == "one" && words[1].empty(), "an array of a class, the rest default");
    check(fivePlace == &five, "a global's address taken by another's initial value");

    check(inArena(&zero, sizeof zero) && inArena(&words, sizeof words) && inArena(&second, sizeof second),
          "in the arena");
    const auto *one = reinterpret_cast<const std::byte *>(&zero);
    const auto *two = reinterpret_cast<const std::byte *>(&five);
    const auto *last = reinterpret_cast<const std::byte *>(&second);
    check(one + sizeof zero <= two && two < last, "in the order of their definitions, apart");

    check(skip == 3 && window.x == 9 && window.height == 6, "a setting's initial value");
    check(steps[1] == 2 && steps[2] == 0 && corners[1].width == 0, "a setting's array, the rest zero");
    check(!inArena(&skip, sizeof skip) && !inArena(&corners, sizeof corners), "the settings outside the arena");
    check(gameGlobalBytes(&five) == sizeof five && gameGlobalBytes(&window) == sizeof window &&
              gameGlobalBytes(&steps) == sizeof steps && gameGlobalBytes(&steps[1]) == 0,
          "a global's and a setting's bytes by their places");

    /* A snapshot's way: a copy of the arena put back; the settings stay. */
    const std::vector<std::byte> copy = gameMemoryCopy();
    zero = 9;
    table[3] = 4;
    box.x = -1;
    rows[1][2] = 6;
    skip = 0;
    window.x = 1;
    steps[3] = 4;
    corners[1].width = 5;
    gameMemoryPut(copy);
    check(zero == 0 && table[3] == 0 && box.x == 1 && rows[1][2] == 0, "put back with a copy of the arena");
    check(skip == 0 && window.x == 1 && steps[3] == 4 && corners[1].width == 5, "the settings kept");

    std::printf("game_globals: %d checks, %d mismatches\n", checks, failures);
    std::fflush(stdout);
    return failures != 0 ? 1 : 0;
}
