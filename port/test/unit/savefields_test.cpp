/* The saved game's schema (savefields.h): the translation layer between the
 * original's bytes of a process and the port's objects.
 *
 * BCC's layout: for each class a process can be, and each of its members
 * (its bases' too), the port's object read from the original's bytes of a
 * cleared object (its vtables in place), the member set all ones, and the
 * object written again must give the bytes BCC gives the member all ones
 * in a cleared object (savefields_cases.h), and nothing
 * else but the vtables and the pointers' bytes, whose kinds say what they
 * hold.
 *
 * Both games' (CRUSADER_GAME's savefields_cases.h, beside the game's
 * tests). The original's saves (savefields_test SAVES: a directory with a
 * new game's original_newgame.sav and the missions' in saves/, written by
 * the original in DOSBox): every process of each read into an object of its
 * class (the class of its
 * vtable, made with its Loading constructor) and written again gives the
 * original's bytes, but for the pointers the loads make again: a list's
 * data and a tactic's the same whether 0 or not, BiosTimer's handler as it
 * is (011B:0008), the usecode stack's top and
 * end at the same offsets, and none of the others. The kernel's block and
 * Controller's, read into the globals and written again, give their
 * bytes too, but for the tables' pointers (the load allocates the tables
 * again) and the lists' data (0 or not). */
#include "savefields.h"
#include "attack.h"
#include "item.h"
#include "itemdata.h"
#include "kernel.h"
#include "spmgr.h"
#include "random.h"
#include "sublist.h"
#include "uprocess.h"
#define SAVEFIELDS_INCLUDES
#include "savefields_cases.h"
#undef SAVEFIELDS_INCLUDES

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/* kernel.cpp's: whether the timer ran when the kernel was saved. */
extern u8 &timerMasked;

namespace {

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        if (failures <= 40) {
            std::printf("MISMATCH savefields: %s\n", what.c_str());
        }
    }
}

std::string hex(const u8 *bytes, u16 size)
{
    std::string out;
    char text[4];
    for (u16 i = 0; i < size; i++) {
        std::snprintf(text, sizeof(text), "%02x", bytes[i]);
        out += text;
    }
    return out;
}

/* A class's case: its vtables, its members' bytes, its pointers. */
struct Pointer {
    u16 offset;
    SavePointer kind;
};
struct Leaf {
    std::string member;
    u16 offset;
    std::string bytes;
    void (*set)(Process &process);
};
struct Case {
    u16 vtable, size;
    std::vector<std::pair<u16, u16>> vtables;
    std::vector<Leaf> leaves;
    std::vector<Pointer> pointers;
};
std::map<std::string, Case> cases;

const SaveClass *classNamed(const std::string &name)
{
    for (u16 i = 0; i < SAVE_CLASS_COUNT; i++) {
        if (name == SAVE_CLASSES[i].name) {
            return &SAVE_CLASSES[i];
        }
    }
    return nullptr;
}

/* A function a class (savefields_cases.h): the classes' members set one
 * by one are thousands of lambdas, which one function would make too
 * large for an optimizer with the sanitizers. */
#define CLASS(c, vtable, size) void case##c(){Case &cs = cases[#c]; cs = Case{vtable, size, {}, {}, {}};
#define VPTR(c, member, offset, vtable) cs.vtables.push_back({offset, vtable});
#define LEAF(c, member, offset, bytes)                                                                                 \
    cs.leaves.push_back({#member, offset, std::string(bytes, sizeof(bytes) - 1), [](Process &process) {                \
                             c &p = (c &)process;                                                                      \
                             p.member = (decltype(p.member))~p.member;                                                 \
                         }});
#define ARRAY(c, member, offset, bytes)                                                                                \
    cs.leaves.push_back({#member, offset, std::string(bytes, sizeof(bytes) - 1), [](Process &process) {                \
                             c &p = (c &)process;                                                                      \
                             std::memset((void *)&p.member, 0xff, sizeof(p.member));                                   \
                         }});
#define POINTER(c, member, offset, kind) cs.pointers.push_back({offset, kind});
#define END(c) }
#include "savefields_cases.h"
#undef CLASS
#undef VPTR
#undef LEAF
#undef ARRAY
#undef POINTER
#undef END

void readCases()
{
#define SAVEFIELDS_LIST
#define CALL(c) case##c();
#include "savefields_cases.h"
#undef CALL
#undef SAVEFIELDS_LIST
}

/* The bytes of a cleared object of the class, its vtables in place. */
std::vector<u8> cleared(const Case &c)
{
    std::vector<u8> bytes(c.size);
    bytes[0] = (u8)c.vtable;
    bytes[1] = (u8)(c.vtable >> 8);
    for (auto [offset, vtable] : c.vtables) {
        bytes[offset] = (u8)vtable;
        bytes[offset + 1] = (u8)(vtable >> 8);
    }
    return bytes;
}

/* Whether offset is in one of the pointers. */
bool inPointer(const Case &c, u16 offset)
{
    for (const Pointer &p : c.pointers) {
        if (offset >= p.offset && offset < p.offset + 4) {
            return true;
        }
    }
    return false;
}

void layoutCases()
{
    for (auto &[name, c] : cases) {
        const SaveClass *sc = classNamed(name);
        check(sc && sc->vtable == c.vtable && sc->size == c.size, name + ": the class table");
        if (!sc) {
            continue;
        }
        std::vector<u8> zero = cleared(c);
        for (const Leaf &leaf : c.leaves) {
            Process *process = sc->make();
            std::vector<u8> in = zero;
            SaveImage read(SaveImage::READ, in.data(), c.size, process);
            sc->fields(read, *process);
            check(!read.wrongVtable(), name + ": its vtables");
            leaf.set(*process);
            std::vector<u8> out(c.size, 0xcc);
            SaveImage write(SaveImage::WRITE, out.data(), c.size, process);
            sc->fields(write, *process);
            std::vector<u8> expected = zero;
            std::memcpy(&expected[leaf.offset], leaf.bytes.data(), leaf.bytes.size());
            bool same = out[0] == 0xcc && out[1] == 0xcc;
            for (u16 i = 2; i < c.size; i++) {
                if (!inPointer(c, i) && out[i] != expected[i]) {
                    same = false;
                }
            }
            check(same, name + "::" + leaf.member + " at " + std::to_string(leaf.offset) + ": " +
                            hex(out.data(), c.size) + ", " + hex(expected.data(), c.size));
        }
    }
}

/* The objects of a FLEX file (the saved game): its count at 54h, its
 * entries at 80h, an offset and a size each. */
std::map<u16, std::vector<u8>> flexObjects(const std::vector<u8> &data)
{
    std::map<u16, std::vector<u8>> out;
    auto u32At = [&](std::size_t at) {
        return (u32)data[at] | (u32)data[at + 1] << 8 | (u32)data[at + 2] << 16 | (u32)data[at + 3] << 24;
    };
    u32 count = u32At(0x54);
    for (u32 i = 0; i < count; i++) {
        u32 offset = u32At(0x80 + 8 * i), size = u32At(0x80 + 8 * i + 4);
        if (size) {
            out[(u16)i] = std::vector<u8>(data.begin() + offset, data.begin() + offset + size);
        }
    }
    return out;
}

u32 farAt(const std::vector<u8> &bytes, u16 at)
{
    return (u32)bytes[at] | (u32)bytes[at + 1] << 8 | (u32)bytes[at + 2] << 16 | (u32)bytes[at + 3] << 24;
}

/* A pointer's bytes, as the loads make them: equal when they must be. The
 * code's offset (ip) the process's load finds again: only a process loaded
 * whole has it. */
bool samePointer(SavePointer kind, u32 original, u32 port, bool loaded = false)
{
    switch (kind) {
    case SAVE_LIST:
    case SAVE_TACTIC:
    case SAVE_FADE:
    case SAVE_FLIC:
        return (original != 0) == (port != 0);
    case SAVE_TRANSIENT:
        return original == 0 && port == 0;
    case SAVE_BIOS_VECTOR:
        return original == port;
    case SAVE_USECODE_IP:
        if (!loaded) {
            return true;
        }
        [[fallthrough]];
    case SAVE_USECODE_TOP:
    case SAVE_USECODE_END:
    case SAVE_USECODE_PROCESS:
        return (u16)original == (u16)port && (original != 0) == (port != 0);
    default:
        return true;
    }
}

const u16 KERNEL = 100, PROCESS_OBJECTS = 15;

/* The bytes a block of globals takes in a save: the kernel's 114,
 * Controller's 79 in No Regret and 75 in No Remorse. */
u16 blockSize(const SaveGlobal *globals, u16 count)
{
    u16 size = 0;
    for (u16 g = 0; g < count; g++) {
        size = (u16)(size + globals[g].size);
    }
    return size;
}

void originalSaves(const fs::path &folder)
{
    if (!fs::exists(folder / "original_newgame.sav")) {
        std::printf("savefields: no original saves in %s\n", folder.string().c_str());
        return;
    }
    std::vector<fs::path> saves;
    for (const fs::path &path : {folder / "original_newgame.sav"}) {
        saves.push_back(path);
    }
    for (const auto &entry : fs::directory_iterator(folder / "saves")) {
        if (entry.path().extension() == ".sav") {
            saves.push_back(entry.path());
        }
    }
    std::map<std::string, int> seen;
    int processes = 0;
    for (const fs::path &path : saves) {
        std::ifstream in(path, std::ios::binary);
        std::vector<u8> data((std::istreambuf_iterator<char>(in)), {});
        auto objects = flexObjects(data);
        std::string file = path.filename().string();
        check(objects.count(KERNEL + 1) && objects[KERNEL + 1].size() == blockSize(KERNEL_GLOBALS, KERNEL_GLOBAL_COUNT),
              file + ": the kernel's block");
        u16 limit = (u16)(objects[KERNEL + 5].size() / 2);
        for (u16 pid = 0; pid < limit; pid++) {
            auto found = objects.find((u16)(KERNEL + PROCESS_OBJECTS + 2 * pid));
            if (found == objects.end()) {
                continue;
            }
            std::vector<u8> &original = found->second;
            u16 vtable = (u16)(original[0] | original[1] << 8);
            const SaveClass *sc = saveClassOfVtable(vtable);
            check(sc != nullptr, file + ": pid " + std::to_string(pid) + "'s vtable " + std::to_string(vtable));
            if (!sc) {
                continue;
            }
            processes++;
            seen[sc->name]++;
            check(original.size() == sc->size, file + ": pid " + std::to_string(pid) + " " + sc->name + "'s size");
            Process *process = sc->make();
            std::vector<u8> bytes = original;
            SaveImage read(SaveImage::READ, bytes.data(), (u16)bytes.size(), process);
            sc->fields(read, *process);
            check(!read.wrongVtable(), file + ": pid " + std::to_string(pid) + " " + sc->name + "'s vtables");
            std::vector<u8> out(original.size(), 0xcc);
            out[0] = original[0];
            out[1] = original[1];
            SaveImage write(SaveImage::WRITE, out.data(), (u16)out.size(), process);
            sc->fields(write, *process);
            const Case &c = cases[sc->name];
            std::string wrong;
            for (u16 i = 0; i < out.size(); i++) {
                if (!inPointer(c, i) && out[i] != original[i] && wrong.empty()) {
                    wrong = " byte " + std::to_string(i);
                }
            }
            for (const Pointer &p : c.pointers) {
                if (!samePointer(p.kind, farAt(original, p.offset), farAt(out, p.offset)) && wrong.empty()) {
                    wrong = " the pointer at " + std::to_string(p.offset);
                }
            }
            check(wrong.empty(), file + ": pid " + std::to_string(pid) + " " + sc->name + wrong + ": " +
                                     hex(out.data(), (u16)out.size()) + ", " +
                                     hex(original.data(), (u16)original.size()));
        }
        /* The blocks of globals. */
        {
            const SaveGlobal *globals = KERNEL_GLOBALS;
            u16 count = KERNEL_GLOBAL_COUNT;
            std::vector<u8> &original = objects[(u16)(KERNEL + 1)];
            std::vector<u8> bytes = original;
            SaveImage read(SaveImage::READ, bytes.data(), (u16)bytes.size(), nullptr);
            saveGlobals(read, globals, count);
            std::vector<u8> out(original.size(), 0xcc);
            SaveImage write(SaveImage::WRITE, out.data(), (u16)out.size(), nullptr);
            saveGlobals(write, globals, count);
            std::string wrong;
            u16 at = 0;
            for (u16 g = 0; g < count; g++) {
                const SaveGlobal &global = globals[g];
                for (u16 i = 0; i < global.size; i++) {
                    bool pointer = global.kind == GLOBAL_TABLE || (global.kind == GLOBAL_LIST && i >= 6 && i < 10);
                    if (!pointer && out[at + i] != original[at + i] && wrong.empty()) {
                        wrong = " byte " + std::to_string(at + i);
                    }
                }
                if (global.kind == GLOBAL_LIST &&
                    (farAt(original, (u16)(at + 6)) != 0) != (farAt(out, (u16)(at + 6)) != 0) && wrong.empty()) {
                    wrong = " the list data at " + std::to_string(at + 6);
                }
                at = (u16)(at + global.size);
            }
            check(wrong.empty(), file + ": the kernel's block" + wrong);
            /* The lists' data read is the pending mark, which no list's
             * load has replaced here. */
            for (u16 g = 0; g < count; g++) {
                if (globals[g].kind == GLOBAL_LIST) {
                    ((SubscriberList *)globals[g].place())->data = nullptr;
                }
            }
        }
        /* Controller's block: the extra object of its process. */
        for (u16 pid = 0; pid < limit; pid++) {
            auto found = objects.find((u16)(KERNEL + PROCESS_OBJECTS + 2 * pid));
            if (found == objects.end() ||
                saveClassOfVtable((u16)(found->second[0] | found->second[1] << 8)) != classNamed("Controller")) {
                continue;
            }
            /* Process::save's lists first, then the block. */
            std::vector<u8> extra = objects[(u16)(KERNEL + PROCESS_OBJECTS + 2 * pid + 1)];
            u16 lists =
                (u16)(2 * ((found->second[0x20] | found->second[0x21] << 8) * (farAt(found->second, 0x24) != 0) +
                           (found->second[0x2a] | found->second[0x2b] << 8) * (farAt(found->second, 0x2e) != 0)));
            u16 size = blockSize(tableStart(CONTROLLER_GLOBALS), CONTROLLER_GLOBAL_COUNT);
            check(extra.size() == lists + size, file + ": Controller's block of " + std::to_string(size) + " bytes");
            if (extra.size() != lists + size) {
                continue;
            }
            std::vector<u8> block(extra.begin() + lists, extra.end());
            std::vector<u8> bytes = block;
            SaveImage read(SaveImage::READ, bytes.data(), size, nullptr);
            saveGlobals(read, tableStart(CONTROLLER_GLOBALS), CONTROLLER_GLOBAL_COUNT);
            std::vector<u8> out(size, 0xcc);
            SaveImage write(SaveImage::WRITE, out.data(), size, nullptr);
            saveGlobals(write, tableStart(CONTROLLER_GLOBALS), CONTROLLER_GLOBAL_COUNT);
            check(out == block, file + ": Controller's block");
        }
    }
    std::string classes;
    for (auto &[name, count] : seen) {
        classes += " " + name;
    }
    std::printf("savefields: %zu saves, %d processes of %zu classes:%s\n", saves.size(), processes, seen.size(),
                classes.c_str());
}

std::vector<u8> fileBytes(const fs::path &path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<u8>((std::istreambuf_iterator<char>(in)), {});
}

u16 wordAt(const std::vector<u8> &bytes, std::size_t at)
{
    return (u16)(bytes[at] | bytes[at + 1] << 8);
}

/* Where the items' flags are in object 0 (itemcach.cpp's saveItems), a
 * sprite's flags in a SpriteEntry. */
const u32 SAVED_ITEM_FLAGS = 0x14000;
const u16 SAVED_SPRITE_FLAGS = 4;
const u16 DESCRIPTION = 9;

typedef std::map<u16, std::vector<u8>> Objects;

bool isProcessObject(u16 object)
{
    return object >= KERNEL + PROCESS_OBJECTS;
}

/* A process's objects: its image, then what its save() adds. */
bool isImage(u16 object)
{
    return (object - KERNEL - PROCESS_OBJECTS) % 2 == 0;
}

/* The vtable of the process whose object that is, 0 for none. */
u16 vtableOf(const Objects &objects, u16 object)
{
    auto at = objects.find(isImage(object) ? object : (u16)(object - 1));
    return at != objects.end() && at->second.size() >= 2 ? wordAt(at->second, 0) : 0;
}

/* The places the time between a load and a save changes (--time): where
 * the port's own saves, made some passes after it loaded the first save,
 * differ from that save, by the kind of field, since the original's time
 * (DOSBox's real time) moves other NPCs and processes than the port's
 * passes do. The objects before the processes by their bytes, but the
 * NPCs' items in all of object 0's arrays and its array of found items,
 * the fields of the NPCs' and the sprites' records, and the kernel's lists
 * and tables whole; a process's by its class, the same field of each
 * process of it (an animation's all when it runs: its step's state),
 * and Process's own fields by any class's; every process's lists (what
 * save() adds first too); the classes whose processes begin or end, and
 * the objects whose size changes. */
struct TimeMask {
    std::map<u16, std::vector<bool>> bytes;
    std::set<u16> resized, whole;
    std::map<u16, std::vector<bool>> images, extras;
    std::vector<bool> process;
    std::set<u16> transient, resizedExtras;
};

/* Object 0's arrays (itemcach.cpp's saveItems): where each is and the size
 * of an item's element; foundItems, the searches' own. */
struct ItemArray {
    u32 offset;
    u8 size;
};
const ItemArray ITEM_ARRAYS[] = {{0x00000, 2}, {0x05000, 2}, {0x0a000, 1}, {0x0c800, 2}, {0x11800, 1}, {0x14000, 2},
                                 {0x19000, 2}, {0x1e000, 1}, {0x20800, 1}, {0x23000, 2}, {0x28000, 2}, {0x2d000, 1}};
const u32 FOUND_ITEMS = 0x28000;
/* The NPCs' records (object 10), Process's size under BCC. */
const u16 NPC_RECORD = 0x97, PROCESS_SIZE = 0x32;
/* The lists' words in what Process::save adds: its lists', when they have
 * data (MList::save). */
std::size_t listWords(const std::vector<u8> &image)
{
    return 2 * (wordAt(image, 0x20) * (farAt(image, 0x24) != 0) + wordAt(image, 0x2a) * (farAt(image, 0x2e) != 0));
}

void mark(std::vector<bool> &mask, const std::vector<u8> &x, const std::vector<u8> &y)
{
    if (mask.size() < x.size()) {
        mask.resize(x.size());
    }
    for (std::size_t i = 0; i < x.size(); i++) {
        if (x[i] != y[i]) {
            mask[i] = true;
        }
    }
}

/* A field the time changed changes whole (a clock's high byte too, which
 * a longer time moves): the members of a process's class, the kernel's
 * globals. */
void widen(std::vector<bool> &mask, const std::vector<std::pair<u16, u16>> &fields)
{
    for (auto [at, size] : fields) {
        bool any = false;
        for (u16 i = at; i < at + size && i < mask.size(); i++) {
            any |= mask[i];
        }
        for (u16 i = at; any && i < at + size && i < mask.size(); i++) {
            mask[i] = true;
        }
    }
}

/* Records of size bytes from first on: a field that changed in one record
 * may in each. */
void byRecord(std::vector<bool> &mask, std::size_t first, std::size_t size, std::size_t count)
{
    std::vector<bool> fields(size);
    for (std::size_t r = 0; r < count; r++) {
        for (std::size_t i = 0; i < size && first + r * size + i < mask.size(); i++) {
            if (mask[first + r * size + i]) {
                fields[i] = true;
            }
        }
    }
    for (std::size_t r = 0; r < count; r++) {
        for (std::size_t i = 0; i < size && first + r * size + i < mask.size(); i++) {
            if (fields[i]) {
                mask[first + r * size + i] = true;
            }
        }
    }
}

void widen(TimeMask &time)
{
    time.process.resize(PROCESS_SIZE);
    for (auto &[vtable, mask] : time.images) {
        for (u16 i = 0; i < PROCESS_SIZE && i < mask.size(); i++) {
            if (mask[i]) {
                time.process[i] = true;
            }
        }
    }
    /* The lists (waitList, relations, and their words in what save() adds):
     * the processes wait on one another and are let go as they act,
     * whichever they are; their vtables stay. */
    for (u16 i = 0x20; i < PROCESS_SIZE; i++) {
        if (i != 0x28 && i != 0x29) {
            time.process[i] = true;
        }
    }
    for (u16 vtable : time.transient) {
        if (vtable) {
            time.images[vtable];
        }
    }
    for (auto &[vtable, mask] : time.images) {
        mask.resize(std::max<std::size_t>(mask.size(), PROCESS_SIZE));
        for (u16 i = 0; i < PROCESS_SIZE; i++) {
            mask[i] = mask[i] || time.process[i];
        }
        if (const SaveClass *sc = saveClassOfVtable(vtable)) {
            /* An animation that runs in the port's passes (its fields
             * change, or one begins or ends): its fields are its step's
             * state (a direction, a target, the item it touched), each may
             * change. */
            bool acts = false;
            for (const Leaf &leaf : cases[sc->name].leaves) {
                if (leaf.member.rfind("AnimPrimitive::", 0) == 0) {
                    acts |= time.transient.count(vtable) != 0;
                    for (std::size_t i = PROCESS_SIZE; i < mask.size(); i++) {
                        acts |= mask[i];
                    }
                }
            }
            std::vector<std::pair<u16, u16>> fields;
            for (const Leaf &leaf : cases[sc->name].leaves) {
                fields.push_back({leaf.offset, (u16)leaf.bytes.size()});
            }
            for (const Pointer &pointer : cases[sc->name].pointers) {
                fields.push_back({pointer.offset, 4});
            }
            mask.resize(std::max<std::size_t>(mask.size(), sc->size));
            for (auto [at, size] : fields) {
                for (u16 i = at; acts && at >= PROCESS_SIZE && i < at + size && i < mask.size(); i++) {
                    mask[i] = true;
                }
            }
            widen(mask, fields);
        }
    }
    if (time.bytes.count(0)) {
        std::vector<bool> &mask = time.bytes[0];
        auto any = [&](std::size_t from, std::size_t to) {
            for (std::size_t i = from; i < to && i < mask.size(); i++) {
                if (mask[i]) {
                    return true;
                }
            }
            return false;
        };
        auto all = [&](std::size_t from, std::size_t to) {
            for (std::size_t i = from; i < to && i < mask.size(); i++) {
                mask[i] = true;
            }
        };
        /* The NPCs' items act: when one changed, each may, in each array
         * (a place, a frame, flags). */
        bool npcs = false;
        for (const ItemArray &array : ITEM_ARRAYS) {
            npcs |= any(array.offset, array.offset + (std::size_t)array.size * NPCS);
        }
        for (const ItemArray &array : ITEM_ARRAYS) {
            std::size_t end = array.offset + (std::size_t)array.size * ITEMS;
            if (array.offset == FOUND_ITEMS && any(array.offset, end)) {
                all(array.offset, end);
            } else if (npcs) {
                all(array.offset, array.offset + (std::size_t)array.size * NPCS);
            }
        }
    }
    if (time.bytes.count(10)) {
        byRecord(time.bytes[10], 0, NPC_RECORD, NPCS);
    }
    if (time.bytes.count(32)) {
        byRecord(time.bytes[32], 0, sizeof(SpriteEntry), time.bytes[32].size() / sizeof(SpriteEntry));
    }
    /* The blocks of globals: the kernel's object, the end of what
     * Controller::save adds. */
    auto blockFields = [](const SaveGlobal *globals, u16 count, u16 at) {
        std::vector<std::pair<u16, u16>> fields;
        for (u16 g = 0; g < count; g++) {
            fields.push_back({at, globals[g].size});
            at = (u16)(at + globals[g].size);
        }
        return fields;
    };
    if (time.bytes.count(KERNEL + 1)) {
        widen(time.bytes[KERNEL + 1], blockFields(KERNEL_GLOBALS, KERNEL_GLOBAL_COUNT, 0));
    }
    for (auto &[vtable, mask] : time.extras) {
        const SaveClass *sc = saveClassOfVtable(vtable);
        u16 controllerBlock = blockSize(tableStart(CONTROLLER_GLOBALS), CONTROLLER_GLOBAL_COUNT);
        if (sc && std::string(sc->name) == "Controller" && mask.size() >= controllerBlock) {
            widen(mask, blockFields(tableStart(CONTROLLER_GLOBALS), CONTROLLER_GLOBAL_COUNT,
                                    (u16)(mask.size() - controllerBlock)));
        }
    }
}

void addTime(TimeMask &time, const Objects &a, const Objects &c)
{
    std::set<u16> all;
    for (auto &[object, bytes] : a) {
        all.insert(object);
    }
    for (auto &[object, bytes] : c) {
        all.insert(object);
    }
    for (u16 object : all) {
        auto x = a.find(object), y = c.find(object);
        bool both = x != a.end() && y != c.end();
        if (both && x->second == y->second) {
            continue;
        }
        if (isProcessObject(object)) {
            if (!both || vtableOf(a, object) != vtableOf(c, object)) {
                time.transient.insert(vtableOf(a, object));
                time.transient.insert(vtableOf(c, object));
            } else if (x->second.size() != y->second.size()) {
                if (isImage(object)) {
                    time.transient.insert(vtableOf(a, object));
                } else {
                    time.resizedExtras.insert(vtableOf(a, object));
                }
            } else {
                mark((isImage(object) ? time.images : time.extras)[vtableOf(a, object)], x->second, y->second);
            }
        } else if ((object >= KERNEL + 2 && object <= KERNEL + 4) || (object >= KERNEL + 6 && object <= KERNEL + 8)) {
            time.whole.insert(object);
        } else if (!both || x->second.size() != y->second.size()) {
            time.resized.insert(object);
        } else {
            mark(time.bytes[object], x->second, y->second);
        }
    }
}

/* --compare A B: save b is save a loaded and saved again at once. They are
 * the same bytes, but where the original's load makes them again or
 * changes them: the pointers by their kinds; the usecode stack's argument
 * pointer, whose selector is the process's (the offset the same); a
 * usecode class's name, in the unit file (the selector the same); the
 * items' ITEM_DRAWN (loadItems clears it), the sprites' SPRITE_MOVED
 * (loadSprites sets it), the random seed (setRandomSeed sets its low
 * bit) and timerMasked (saveKernel writes whether the timer runs, which
 * loadKernel leaves off until the kernel turns it on again). The
 * description (object 9) is the slot's the game saves in (savegame.cpp),
 * not the game's; a process's reserved byte and a usecode thread's
 * unknownDE, which nothing sets, the original's heap's.
 *
 * --time C...: b was saved some passes after the load (the original's in
 * DOSBox, where the passes are the real time's); the places the time
 * changes (TimeMask, from the port's saves C made from a) are left out too:
 * a process that begins or ends of a class that does so in C, the fields
 * that change in C by their kinds, and in the kernel's tables of sizes and
 * names the entries of the processes that began or ended. */
int compareSaves(const fs::path &first, const fs::path &second, const std::vector<fs::path> &passes)
{
    Objects a = flexObjects(fileBytes(first)), b = flexObjects(fileBytes(second));
    const TimeMask *time = nullptr;
    TimeMask mask;
    for (const fs::path &path : passes) {
        addTime(mask, a, flexObjects(fileBytes(path)));
        time = &mask;
    }
    widen(mask);
    std::vector<std::string> wrong;
    std::size_t changed = 0;
    auto differ = [&](u16 object, std::size_t at, const std::string &why) {
        if (wrong.size() < 40) {
            wrong.push_back("object " + std::to_string(object) + " byte " + std::to_string(at) + ": " + why);
        }
    };
    /* A process that is in one only, or of another class: it began or
     * ended meanwhile. */
    auto begunOrEnded = [&](u16 object) {
        return time && isProcessObject(object) &&
               (time->transient.count(vtableOf(a, object)) || time->transient.count(vtableOf(b, object)));
    };
    for (auto &[object, bytes] : a) {
        if (!b.count(object) && !begunOrEnded(object)) {
            differ(object, 0, "not in the second");
        }
    }
    for (auto &[object, bytes] : b) {
        if (!a.count(object) && !begunOrEnded(object)) {
            differ(object, 0, "not in the first");
        }
    }
    for (auto &[object, x] : a) {
        if (!b.count(object) || object == DESCRIPTION) {
            continue;
        }
        std::vector<u8> &y = b[object];
        if (isProcessObject(object) && vtableOf(a, object) != vtableOf(b, object)) {
            if (!begunOrEnded(object)) {
                differ(object, 0, "another class");
            }
            continue;
        }
        if (x.size() != y.size()) {
            bool changes = false;
            if (time && isProcessObject(object) && !isImage(object)) {
                /* Other lists' words, the rest the same size and bytes but
                 * where the class's change. */
                std::size_t p = listWords(a.at((u16)(object - 1))), q = listWords(b.at((u16)(object - 1)));
                auto at = time->extras.find(vtableOf(a, object));
                changes = time->resizedExtras.count(vtableOf(a, object)) != 0;
                if (!changes && x.size() - p == y.size() - q) {
                    changes = true;
                    for (std::size_t i = p; i < x.size(); i++) {
                        if (x[i] != y[i - p + q] &&
                            !(at != time->extras.end() && i < at->second.size() && at->second[i])) {
                            changes = false;
                        }
                    }
                }
            } else if (time) {
                changes = isProcessObject(object) ? begunOrEnded(object)
                                                  : time->resized.count(object) || time->whole.count(object);
            }
            if (!changes) {
                differ(object, 0, "sizes " + std::to_string(x.size()) + ", " + std::to_string(y.size()));
            }
            continue;
        }
        std::vector<bool> checked(x.size());
        auto skip = [&](std::size_t at, std::size_t size) {
            for (std::size_t i = at; i < at + size && i < checked.size(); i++) {
                checked[i] = true;
            }
        };
        /* The places the time changes in this object, and whether a field
         * is among them. */
        std::vector<bool> m;
        auto timed = [&](std::size_t at, std::size_t size) {
            for (std::size_t i = at; i < at + size && i < m.size(); i++) {
                if (m[i]) {
                    return true;
                }
            }
            return false;
        };
        if (time) {
            if (time->whole.count(object)) {
                m.assign(x.size(), true);
            } else if (!isProcessObject(object)) {
                auto at = time->bytes.find(object);
                if (at != time->bytes.end()) {
                    m = at->second;
                }
            } else {
                auto &classes = isImage(object) ? time->images : time->extras;
                auto at = classes.find(vtableOf(a, object));
                if (at != classes.end()) {
                    m = at->second;
                }
                if (isImage(object)) {
                    for (u16 i = 0; i < time->process.size(); i++) {
                        if (time->process[i]) {
                            m.resize(std::max<std::size_t>(m.size(), i + 1));
                            m[i] = true;
                        }
                    }
                } else {
                    std::size_t words = listWords(a.at((u16)(object - 1)));
                    m.resize(std::max(m.size(), words));
                    for (std::size_t i = 0; i < words; i++) {
                        m[i] = true;
                    }
                }
            }
            for (std::size_t i = 0; i < x.size() && i < m.size(); i++) {
                if (m[i] && x[i] != y[i]) {
                    checked[i] = true;
                    changed++;
                }
            }
            if (object == KERNEL + 5 || object == KERNEL + 9) {
                /* The sizes and the names of the processes that began
                 * or ended. */
                std::size_t entry = object == KERNEL + 5 ? 2 : 4;
                for (u16 pid = 0; (pid + 1) * entry <= x.size(); pid++) {
                    u16 image = (u16)(KERNEL + PROCESS_OBJECTS + 2 * pid);
                    if ((a.count(image) != b.count(image) || vtableOf(a, image) != vtableOf(b, image)) &&
                        begunOrEnded(image)) {
                        skip(pid * entry, entry);
                    }
                }
            }
        }
        if (object == 0) {
            for (u16 item = 0; item < ITEMS; item++) {
                std::size_t at = SAVED_ITEM_FLAGS + 2 * item;
                if (!timed(at, 2) && (wordAt(x, at) & ~ITEM_DRAWN) != (wordAt(y, at) & ~ITEM_DRAWN)) {
                    differ(object, at, "an item's flags");
                }
                skip(at, 2);
            }
        } else if (object == 20) {
            setRandomSeed(farAt(x, 4));
            if (!timed(4, 4) && farAt(y, 4) != getRandomSeed()) {
                differ(object, 4, "the random seed is not the one set");
            }
            skip(4, 4);
        } else if (object == 32) {
            for (std::size_t at = SAVED_SPRITE_FLAGS; at + 2 <= x.size(); at += sizeof(SpriteEntry)) {
                if (!timed(at, 2) && (wordAt(x, at) & ~SPRITE_MOVED) != (wordAt(y, at) & ~SPRITE_MOVED)) {
                    differ(object, at, "a sprite's flags");
                }
                skip(at, 2);
            }
        } else if (object == KERNEL + 1) {
            u16 at = 0;
            for (u16 g = 0; g < KERNEL_GLOBAL_COUNT; g++) {
                const SaveGlobal &global = KERNEL_GLOBALS[g];
                if (global.place() == &timerMasked) {
                    skip(at, 1);
                } else if (global.kind == GLOBAL_TABLE) {
                    skip(at, 4);
                } else if (global.kind == GLOBAL_LIST) {
                    if (!timed(at + 6, 4) && (farAt(x, (u16)(at + 6)) != 0) != (farAt(y, (u16)(at + 6)) != 0)) {
                        differ(object, at + 6, "a list's data");
                    }
                    skip(at + 6, 4);
                }
                at = (u16)(at + global.size);
            }
        } else if (object == KERNEL + 9) {
            for (std::size_t at = 0; at + 4 <= x.size(); at += 4) {
                u16 selector = wordAt(x, at + 2);
                if (selector != ORIGINAL_DS && selector == wordAt(y, at + 2)) {
                    skip(at, 2);
                }
            }
        } else if (isProcessObject(object) && isImage(object)) {
            const SaveClass *sc = saveClassOfVtable(wordAt(x, 0));
            if (!sc) {
                differ(object, 0, "a class the port does not know");
                continue;
            }
            /* Process::reserved and a usecode thread's unknownDE: nothing
             * sets them, the original's are what its heap held. */
            skip(9, 1);
            if (std::string(sc->name) == "UsecodeProcess") {
                skip(0x114, 2);
            }
            const Case &c = cases[sc->name];
            for (const Pointer &p : c.pointers) {
                /* A pointer the time moves (a usecode stack's top) is
                 * left out whole. */
                if (!timed(p.offset, 4) && !samePointer(p.kind, farAt(x, p.offset), farAt(y, p.offset), true)) {
                    differ(object, p.offset, std::string(sc->name) + "'s pointer");
                }
                skip(p.offset, 4);
            }
            if (std::string(sc->name) == "UsecodeProcess") {
                /* The argument pointer on the stack (UsecodeProcess::load), after
                 * Process::save's lists in the second object: localsOffset. */
                for (const Objects *side : {&a, &b}) {
                    auto extra = side->find((u16)(object + 1));
                    const std::vector<u8> &image = side->at(object);
                    if (extra == side->end()) {
                        continue;
                    }
                    std::size_t lists = listWords(image);
                    u16 locals = extra->second.size() >= lists + 2 ? wordAt(extra->second, lists) : 0;
                    if (locals) {
                        skip(USECODE_DATA + locals + 2, 2);
                    }
                }
            }
        }
        for (std::size_t i = 0; i < x.size(); i++) {
            if (!checked[i] && x[i] != y[i]) {
                differ(object, i, "differs");
                break;
            }
        }
    }
    for (const std::string &w : wrong) {
        std::printf("MISMATCH savefields: %s\n", w.c_str());
    }
    std::printf("savefields: %s and %s, %zu objects, ", first.filename().string().c_str(),
                second.filename().string().c_str(), a.size());
    if (time) {
        std::printf("%zu bytes the time changes (%zu saves), ", changed, passes.size());
    }
    std::printf("%zu mismatches\n", wrong.size());
    return wrong.empty() ? 0 : 1;
}

}

int main(int argc, char *argv[])
{
    readCases();
    if (argc >= 4 && std::string(argv[1]) == "--compare" && (argc == 4 || std::string(argv[4]) == "--time")) {
        std::vector<fs::path> passes(argv + std::min(argc, 5), argv + argc);
        return compareSaves(argv[2], argv[3], passes);
    }
    check(cases.size() == SAVE_CLASS_COUNT, "a case for each class");
    layoutCases();
    if (argc == 2) {
        originalSaves(argv[1]);
    }
    std::printf("savefields: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
