/* The check of a saved game's file before its load (kernel.h's
 * kernelSaveLoadable), over a game's values, one program a game
 * (savecheck, remorse_savecheck): a FLEX made here with the kernel's
 * objects at 100 as saveKernel writes them, one process of a class the
 * game saves; it is loadable, and each thing loadKernel cannot take makes
 * it not: a vtable no class has, a size the class has not, the guard, the
 * block's size, the tables' sizes, a process limit of 0, a process's
 * objects past the count, a file cut short, an object table past the
 * file, a file shorter than a FLEX's head. */
#include "kernel.h"
#include "kprocess.h"
#include "savefields.h"
#include "savefields_game.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH savecheck: %s\n", what.c_str());
    }
}

constexpr i16 KERNEL = 100;
constexpr u32 PROCESSES = 15;
/* The block's offsets of processLimit and saveGuard (kernel.cpp's
 * KERNEL_GLOBALS: processes, processSizes and timerMasked before the one,
 * the other the last byte of 114). */
constexpr std::size_t LIMIT_AT = 9;
constexpr std::size_t GUARD_AT = 113;
constexpr u8 GUARD_SAVING = 0xed;

/* A saved game's objects (only the kernel's have bytes) as a FLEX. */
struct Save {
    std::vector<std::vector<u8>> objects;
    std::size_t extraCount = 0;

    std::vector<u8> &object(u32 number)
    {
        if (objects.size() <= number) {
            objects.resize(number + 1);
        }
        return objects[number];
    }

    [[nodiscard]] std::vector<u8> bytes() const
    {
        std::vector<u8> file(0x80 + (8 * objects.size()));
        auto put32 = [&file](std::size_t at, u32 value) {
            for (std::size_t k = 0; k < 4; k++) {
                file[at + k] = static_cast<u8>(value >> (8 * k));
            }
        };
        put32(0x54, static_cast<u32>(objects.size() + extraCount));
        for (std::size_t i = 0; i < objects.size(); i++) {
            put32(0x80 + (8 * i), static_cast<u32>(file.size()));
            put32(0x80 + (8 * i) + 4, static_cast<u32>(objects[i].size()));
            file.insert(file.end(), objects[i].begin(), objects[i].end());
        }
        return file;
    }
};

/* The kernel's objects of a game with two process slots, the first the
 * class's, the second empty. */
Save goodSave(const SaveClass &saved)
{
    Save save;
    const char text[] = "Defines=81f7";
    save.object(KERNEL).assign(text, text + sizeof(text));
    std::vector<u8> &block = save.object(KERNEL + 1);
    block.assign(114, 0);
    block[LIMIT_AT] = 2;
    block[GUARD_AT] = GUARD_SAVING;
    save.object(KERNEL + 5) = {static_cast<u8>(saved.size), static_cast<u8>(saved.size >> 8), 0, 0};
    save.object(KERNEL + 6).assign(KERNEL_ITEMS / 8, 0);
    save.object(KERNEL + 7).assign(KERNEL_ITEMS, 0);
    save.object(KERNEL + 8).assign(LEVELS * 2, 0);
    save.object(KERNEL + 9).assign(2 * 4, 0);
    std::vector<u8> &process = save.object(KERNEL + PROCESSES);
    process.assign(saved.size, 0);
    process[0] = static_cast<u8>(saved.vtable);
    process[1] = static_cast<u8>(saved.vtable >> 8);
    save.object(KERNEL + PROCESSES + 1) = {0};
    save.object(KERNEL + PROCESSES + 3);
    return save;
}

/* A vtable no class of the game's has. */
u16 unknownVtable()
{
    u16 vtable = 0x10;
    while (saveClassOfVtable(vtable) != nullptr) {
        vtable = static_cast<u16>(vtable + 2);
    }
    return vtable;
}

}

int main()
{
    const SaveClass &saved = SAVE_CLASSES[0];
    check(kernelSaveLoadable(goodSave(saved).bytes(), KERNEL), "the kernel's objects as saveKernel writes them");

    Save save = goodSave(saved);
    const u16 vtable = unknownVtable();
    save.object(KERNEL + PROCESSES)[0] = static_cast<u8>(vtable);
    save.object(KERNEL + PROCESSES)[1] = static_cast<u8>(vtable >> 8);
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "a vtable no class has");

    save = goodSave(saved);
    save.object(KERNEL + 5)[0] = static_cast<u8>(saved.size + 2);
    save.object(KERNEL + PROCESSES).resize(saved.size + 2);
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "a size the class has not");

    save = goodSave(saved);
    save.object(KERNEL + 1)[GUARD_AT] = 0xec;
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "the guard of no save");

    save = goodSave(saved);
    save.object(KERNEL + 1).resize(113);
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "a block of another size");

    save = goodSave(saved);
    save.object(KERNEL + 7).resize(KERNEL_ITEMS + 1);
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "the item types' table of another size");

    save = goodSave(saved);
    save.object(KERNEL + 1)[LIMIT_AT] = 0;
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "a process limit of 0");

    save = goodSave(saved);
    save.objects.resize(KERNEL + PROCESSES + 1);
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "the process's load past the objects");

    std::vector<u8> cut = goodSave(saved).bytes();
    cut.resize(cut.size() - 4);
    check(!kernelSaveLoadable(cut, KERNEL), "a file cut short");

    save = goodSave(saved);
    save.extraCount = 100000;
    check(!kernelSaveLoadable(save.bytes(), KERNEL), "an object table past the file");

    check(!kernelSaveLoadable(std::vector<u8>(0x40, 0), KERNEL), "shorter than a FLEX's head");

    std::printf("savecheck: %d checks, %d mismatches\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
