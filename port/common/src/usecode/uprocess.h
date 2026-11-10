/* UPROCESS.C (13f8): usecode processes and the classes they run. A usecode
 * process runs one function of a class in an interpreter thread (INTER.C).
 * The classes are units of the usecode file (UNITFILE.C); a ClassTable keeps
 * up to 128 of them with the code of their functions loaded on demand, and
 * frees the least recently used code above a limit. The usecode reaches C
 * functions by index through a table built from the linker's map file. */
#ifndef PORT_UPROCESS_H
#define PORT_UPROCESS_H
#include "common.h"
#include "gumplist.h"
#include "inter.h"
#include "item.h"
#include "kprocess.h"
#include "unitfile.h"

#include <memory>

constexpr int CLASSES = 128;
/* A class is free code to evict until its functions are released. */
constexpr s32 CODE_LIMIT = (static_cast<s32>(0x19000L));
/* Object 1 of the usecode file: per class, the events it has functions for
 * and its name. */
struct __attribute__((packed)) ClassInfo {
    u32 events;
    char name[9];
};
ASSERT_FILE_SIZE(ClassInfo, 13);

/* The code of one loaded function. */
class CodeBlock : public GumpLink {
public:
    /* Its own. */
    std::unique_ptr<u8[]> code;
    /* Where the code lies in the class's unit. */
    u32 offset;
    u16 classId;
    u16 function;

    CodeBlock(u16 newClassId, u16 newFunction, u16 size, u32 newOffset); /* 13f8:0000 */
    ~CodeBlock() override;                                               /* 13f8:00cd */
};
ASSERT_SIZE(CodeBlock, 0x16);

/* The loaded code, the most recently used first. */
class CodeList : public GumpList {
public:
    void add(CodeBlock *block);   /* 13f8:011b */
    void touch(CodeBlock *block); /* 13f8:0134 */
};
ASSERT_SIZE(CodeList, 0xa);

/* A function of a class: its code's size and place in the unit, or the
 * CodeBlock holding it while it is loaded. */
struct __attribute__((packed)) FunctionEntry {
    u16 size;
    u32 offset;
};
ASSERT_FILE_SIZE(FunctionEntry, 6);
/* The first 20 bytes of a class's unit; the function table follows. */
struct __attribute__((packed)) ClassHeader {
    u32 functions;
    u32 unknown4;
    /* Where the functions' code starts in the unit, counted from 1. */
    u32 codeBase;
    u8 unknownC[8];
};
ASSERT_FILE_SIZE(ClassHeader, 20);

class ClassEntry {
public:
    /* The unit of the class; -1 (0 after a reset) when the slot is free. */
    i16 unit;
    FlexEntry place;
    ClassHeader header;
    FunctionEntry *functions;
    /* How many processes run each function; -1 when it is not loaded. */
    i16 *uses;
    /* The code of each loaded function, which the original keeps in the
     * function's offset, a far pointer in its 32 bits: the port's pointers
     * are wider. */
    CodeBlock **loadedBlocks;

    ClassEntry()
    {
        unit = -1;
        functions = nullptr;
        uses = nullptr;
        loadedBlocks = nullptr;
    }
};
ASSERT_SIZE(ClassEntry, 0x26);

class ClassTable {
public:
    ClassEntry classes[CLASSES];
    /* Bytes of code loaded, and the most to keep. */
    u32 loaded;
    u32 limit;
    /* A class slot whose functions are all unused, or -1. */
    i16 free;
    CodeList blocks;
    /* Prints the loaded code after each load. */
    u8 verbose;
    UnitFile *unitFile;

    explicit ClassTable(const char *path); /* 13f8:1499 */
    /* The class's entry in the unit file's header. */
    [[nodiscard]] ClassInfo &info(u16 classId) const
    {
        return (reinterpret_cast<ClassInfo *>(unitFile->header.get()))[classId];
    }
    void reset();     /* 13f8:1536 */
    void unloadAll(); /* 13f8:1575 */
    ~ClassTable();    /* 13f8:1601 */
    /* The slot of the class in unit, or 0 with a free slot in *slot. */
    ClassEntry *find(u16 unit, ClassEntry **slot); /* 13f8:167c */
    void load(u16 unit, ClassEntry *entry) const;  /* 13f8:1866 */
    /* The code of a function, loaded if need be; each call counts a use. */
    u8 *code(u16 classId, u16 function);                /* 13f8:19fd */
    void printStats();                                  /* 13f8:1cca */
    u8 *address(u16 classId, u16 function, u32 offset); /* 13f8:1d72 */
    void release(u16 classId, u16 function);            /* 13f8:1d8d */
    /* Frees the code of an unused block; 0 when it is in use. */
    bool unload(CodeBlock *block); /* 13f8:1e17 */
    /* Frees the code of a function (all of a class for -1, the least
     * recently used unused code for classId -1). */
    u8 unloadCode(i16 classId, i16 function); /* 13f8:1f24 */
};
ASSERT_SIZE(ClassTable, 0x1319);

class UsecodeProcess : public Process {
public:
    explicit UsecodeProcess(Loading loading) : Process(loading), thread(loading) {}
    u16 classId;
    u16 function;
    UsecodeThread thread;
    u32 *resultPointer;
    /* Where the pointer to the arguments is on the stack. */
    u16 localsOffset;
    u8 argumentSize;
    u8 finished;
    u8 running;

    /* 13f8:0eec */
    UsecodeProcess(u32 *resultAddress, void *arguments, u16 size, u16 usecodeClassId, u16 usecodeFunction, u16 offset,
                   void *extra, u16 extraSize);
    virtual void releaseCalls(); /* 13f8:10b6 */
    void run() override;         /* 13f8:10da */
    /* run's after its thread's run, on the state it stopped in (the port's:
     * after a wait in it). */
    void runEnd(i16 state);
    void cleanup() override; /* 13f8:1162 */
    /* The original's destructor, which BCC made: the thread's. */
    void onKill() override;
    void save(File *file) override; /* 13f8:118f */
    void load(File *file) override; /* 13f8:1278 */
};
ASSERT_SIZE(UsecodeProcess, 0x124);

extern ClassTable *&classes;
/* The far addresses of the C functions the usecode calls, from the map
 * file (segment << 16 | offset), which unkcoff.dat holds. */
extern u32 *&cSymbols;
extern u16 &cSymbolCount;

void resetUsecode(); /* 13f8:0cb3 */
void initUsecode();  /* 13f8:0cdf */
/* The usecode class of an item. */
u16 classOf(Item *item); /* 13f8:0dc5 */
/* Starts the item's usecode for event when its class handles one of events;
 * the process's result goes to *result. */
bool callEvent(Item *item, u32 events, u16 event, u16 *result, u32 argument); /* 13f8:0e3a */

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void unusedUsecode();
void closeUsecode();
/* UNCALLED: nothing calls it. */
void unusedUsecode2();
/* UNCALLED: nothing calls it. */
void killUsecode(u16 item, i16 type);
/* UNCALLED: nothing calls it. */
void moveUsecode(u16 item, i16 type, u16 target);
#endif
