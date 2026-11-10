/* The saved game's translation layer. The original's kernel writes a
 * process as its memory, BCC's layout of its class, and some blocks of
 * globals as they lie in its data segment; the port's objects and globals
 * are laid out otherwise, so they move field by field between the port and
 * the original's bytes, which the save holds as the original has them.
 *
 * savefields.cpp (generated) has, for each class under
 * Process, the function that moves its fields at BCC's offsets, its base's
 * first; SAVE_CLASSES lists the classes a process can be, with the
 * original's vtable of each. A SaveImage is the original's bytes of one
 * process (or one block) and the way the fields go: into the bytes (WRITE)
 * or out of them (READ), the same calls both ways. */
#ifndef PORT_SAVEFIELDS_H
#define PORT_SAVEFIELDS_H
#include "kprocess.h"

#include <cstddef>
#include <typeinfo>

/* The kinds of the pointer fields: what the original's bytes hold there, and
 * what the port makes of them. */
enum SavePointer {
    /* An MList's data: the original's load asks only whether it is 0, and
     * reads the words from the process's second object. */
    SAVE_LIST,
    /* 0 whenever a save can happen. */
    SAVE_TRANSIENT,
    /* Attack's tactic and script: tactics[tacticNumber]. */
    SAVE_TACTIC,
    /* BiosTimer's handler, which the original's loadKernel sets again. */
    SAVE_BIOS_VECTOR,
    /* A fade's palettes and a flic's buffers on the heap, which the save
     * does not hold: the original cannot go on with them either. */
    SAVE_FADE,
    SAVE_FLIC,
    /* The usecode thread's. */
    SAVE_USECODE_CALLS,
    SAVE_USECODE_TOP,
    SAVE_USECODE_END,
    SAVE_USECODE_IP,
    SAVE_USECODE_PROCESS,
    SAVE_USECODE_CLASS,
    SAVE_USECODE_RESULT
};

/* The selector the port writes for a far pointer to the heap whose value
 * the original sets again or asks only whether it is 0: one of the
 * original's local descriptors, as its heap's are (TI and RPL 3). */
const u16 SAVE_HEAP_SELECTOR = 0x2f07;

/* The pointer fields of an image, with their kinds (SaveImage's
 * listPointers): the places where the original's bytes hold its own
 * addresses, which the pass oracle compares by their kinds. */
struct SavePointers {
    static const u16 MOST = 64;
    u16 count = 0;
    u16 offsets[MOST];
    SavePointer kinds[MOST];
};

class SaveImage {
public:
    enum Direction { READ, WRITE };

    /* The bytes of process (of size, BCC's) or of a block of globals
     * (process 0). */
    SaveImage(Direction direction, u8 *bytes, u16 size, Process *process);

    [[nodiscard]] Direction direction() const { return direction_; }
    [[nodiscard]] Process *process() const { return process_; }
    /* READ: a vtable other than the class's was in the bytes. */
    [[nodiscard]] bool wrongVtable() const { return wrongVtable_; }

    /* An integer of the original's size, little-endian. */
    template <class T> void integer(u16 offset, T &value)
    {
        u32 word = 0;
        if (direction_ == WRITE) {
            word = static_cast<u32>(value);
        }
        move(offset, sizeof(T), word);
        if (direction_ == READ) {
            value = static_cast<T>(word);
        }
    }
    template <class T, std::size_t N> void integers(u16 offset, T (&values)[N])
    {
        for (std::size_t i = 0; i < N; i++) {
            integer(static_cast<u16>((offset + (i * sizeof(T)))), values[i]);
        }
    }
    /* A bit field: width bits from bit shift of the bytes at offset. */
    void bits(u16 offset, u8 shift, u8 width, u32 &value);
    /* A vtable pointer: the original's DS offset of the vtable. */
    void vtable(u16 offset, u16 value);
    template <class T> void pointer(u16 offset, SavePointer kind, T *&value)
    {
        void *any = static_cast<void *>(value);
        pointer(offset, kind, any);
        value = static_cast<T *>(any);
    }
    void pointer(u16 offset, SavePointer kind, void *&value);
    /* The far pointer at offset as the bytes have it (READ, WRITE). */
    u32 farPointer(u16 offset);
    void setFarPointer(u16 offset, u32 value);
    /* The pointer fields the moves go through added to list while it is
     * set. */
    void listPointers(SavePointers *list) { pointers_ = list; }
    void notePointer(u16 offset, SavePointer kind);

private:
    Direction direction_;
    u8 *bytes_;
    u16 size_;
    Process *process_;
    bool wrongVtable_ = false;
    SavePointers *pointers_ = nullptr;

    void move(u16 offset, u16 size, u32 &word);
};

/* What a pointer read holds until the load makes it again: not 0, as the
 * original's was, and never read through. A list's data until the
 * process's load reads the list, Attack's tactic and a fade's palettes
 * until saveLoaded. */
extern u16 &savePending;
/* A process just read, before its load: Attack's tactic from its number,
 * and a fade whose palettes the save does not hold ended, as its event
 * ends it (the original's load keeps their pointers as they were). */
void saveLoaded(Process *process);

/* The names the processes give themselves (savefields.cpp), at the
 * original's DS offsets of their literals, which the kernel's table of
 * names holds as far pointers: the name at an offset ("" for none: a
 * usecode class's name, in the unit file), and the offset of a name (0 for
 * none). */
struct SaveName {
    u16 offset;
    const char *name;
};
const char *saveNameAt(u16 offset);
u16 saveNameOffset(const char *name);

/* A class a saved process can be (savefields.cpp): its name, its type, the
 * original's vtable and size, the function that moves its fields and the
 * one that makes a process of it to load into (a Loading constructor:
 * nothing of the kernel's). */
struct SaveClass {
    const char *name;
    const std::type_info *type;
    u16 vtable;
    u16 size;
    void (*fields)(SaveImage &image, Process &process);
    Process *(*make)();
};
/* The class of a process of the port, or of the original's vtable; 0 for
 * none. */
const SaveClass *saveClassOf(Process *process);
const SaveClass *saveClassOfVtable(u16 vtable);

/* A block of globals the original writes whole: each global with its size
 * in the original and in the port, and how it goes. The save's tables are
 * checked against the original's offsets. */
enum SaveGlobalKind {
    GLOBAL_AS_IS, /* an integer */
    GLOBAL_TABLE, /* a table's far pointer, which the load allocates again */
    GLOBAL_LIST   /* a SubscriberList (15 bytes: its vtable, count, tag, data, stop, index, total) */
};
/* place gives the global's place when it is saved or loaded: the game's
 * globals are references bound as the program starts, so a table of them
 * made before that holds no address but a function, and stays a
 * constant. */
struct SaveGlobal {
    void *(*place)();
    u8 size;
    u8 portSize;
    u8 kind;
    /* Its name, for the pass oracle's reports. */
    const char *name;
};
#define SAVE_GLOBAL(variable, size, kind)                                                                              \
    {[]() -> void * { return &(variable); }, size, sizeof(variable), kind, #variable}
/* The globals' bytes, one after another from offset 0. */
void saveGlobals(SaveImage &image, const SaveGlobal *globals, u16 count);
/* The kernel's block (kernel.cpp, 114 bytes) and Controller's (the game's
 * control_game.cpp, 79 bytes in No Regret, 75 in No Remorse). */
extern const SaveGlobal KERNEL_GLOBALS[];
extern const u16 KERNEL_GLOBAL_COUNT;
/* The game's tables (savefields_game.h). */
#include "savefields_game.h"
#endif
