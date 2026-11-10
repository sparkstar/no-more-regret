/* INTER.C (13f0:0000-3eb7): the usecode interpreter. A thread runs one
 * function's code with a data stack of 200 bytes that grows down from its end
 * and a stack of the calls it is in. */
#ifndef PORT_INTER_H
#define PORT_INTER_H
#include <mem.h>
#include "common.h"
#include "kprocess.h"
#include "usemem.h"

class UsecodeProcess;

/* What UsecodeThread::run returns. */
constexpr int THREAD_FINISHED = 1; /* the function returned, or its item has another such process */
constexpr int THREAD_SUSPENDED = 2;
constexpr int THREAD_DEAD = 4; /* an intrinsic killed the process */
/* The port's: a wait was opened in a call of the thread's, which goes on
 * after it; the state it stops in comes after (waits.h's restResult). */
constexpr int THREAD_WAITING = 8;

/* The calls of a thread, (function << 16 | class) each. */
class __attribute__((packed)) CallStack {
public:
    u32 values[32];
    i16 count;

    CallStack()
    {
        count = 0;
        memset(values, 0, sizeof(values));
    }
    ~CallStack() = default;
    void push(u32 value); /* 13f0:0000 */
    u32 pop();            /* 13f0:003c */
};
ASSERT_FILE_SIZE(CallStack, 0x82);

class DataStack {
public:
    CallStack *calls;
    u8 data[200];
    u8 *top;
    /* The end of data. */
    u8 *end;
    /* The frame of the running function: where its caller's base is, as an
     * offset in data. */
    u16 base;

    /* Puts length bytes of source on the stack (13f0:00e8). Its thread gives
     * the data its region of the usecode's memory (usemem.h). */
    DataStack(void *source, u16 length);
    /* A stack to load a saved one into (kprocess.h's Loading): the rest
     * comes from the save, the region from UsecodeProcess::load. */
    explicit DataStack(Loading /*unused*/) : calls(nullptr) {}
    ~DataStack() { release(); }
    /* Gives up the data's region and the calls (the original's destructor,
     * which UsecodeProcess's kill runs; again, nothing). */
    void release()
    {
        usecodeMemory.remove(data);
        if (calls != nullptr) {
            delete calls;
            calls = nullptr;
        }
    }
    /* Makes room for size bytes on the stack (drops them for -size). The
     * original's top is a far pointer whose 16-bit offset wraps: -n, as
     * 10000h-n, moves it up n bytes; the port's takes size as that signed
     * word. */
    u8 *reserve(u16 size)
    {
        top -= static_cast<i16>(size);
        return top;
    }
    void pushWord(u16 value) { *reinterpret_cast<u16 *>(top -= 2) = value; }
    void pushByte(u8 value) { *--top = value; }
    void pushLong(s32 value) { *reinterpret_cast<s32 *>(top -= 4) = value; }
    i16 popWord()
    {
        const i16 value = *reinterpret_cast<i16 *>(top);
        top += 2;
        return value;
    }
    s32 popLong()
    {
        const s32 value = *reinterpret_cast<s32 *>(top);
        top += 4;
        return value;
    }
    /* The memory of the usecode's pointer on top, size bytes (usemem.h). */
    void *popPointer(u32 size) { return usecodeMemory.at(static_cast<u32>(popLong()), size); }
    /* offset bytes into the stack. */
    [[nodiscard]] u8 *stackAt(i16 offset) const { return top + offset; }
    /* The locals of the running function, at offset from its frame. */
    u8 *local(i16 offset) { return data + base + offset; }
    char getByte(i16 offset) { return *reinterpret_cast<char *>(data + base + offset); }
    i16 getWord(i16 offset) { return *reinterpret_cast<i16 *>(data + base + offset); }
    s32 getLong(i16 offset) { return *reinterpret_cast<s32 *>(data + base + offset); }
    void setByte(i16 offset, u8 value) { *(data + base + offset) = value; }
    void setWord(i16 offset, u16 value) { *reinterpret_cast<u16 *>(data + base + offset) = value; }
    void setLong(i16 offset, s32 value) { *reinterpret_cast<s32 *>(data + base + offset) = value; }
    /* Takes the data and position of other's stack (13f0:0172). */
    /* UNCALLED: nothing calls it. */
    void copy(DataStack *other);
    /* Starts a frame with size bytes of cleared locals (13f0:01c7). */
    void enterFrame(u16 size);
};
ASSERT_SIZE(DataStack, 0xd6);

class UsecodeThread : public DataStack {
public:
    /* The next byte of code; operands are read as bytes, words and longs. */
    union {
        u8 *ip;
        i16 *ipWord;
        s32 *ipLong;
    };
    UsecodeProcess *process;
    u16 unknownDE;
    /* The return value's place is on the stack. */
    u8 started;
    /* The name of the class, in the unit file's header. */
    char *className;

    /* Starts function of classId at code, with arguments on the stack
     * (13f0:0244). */
    UsecodeThread(void *arguments, u16 size, u8 *code, UsecodeProcess *ownerProcess, char *usecodeClassName,
                  u16 classId, u16 function);
    /* A thread to load a saved one into (kprocess.h's Loading): nothing set. */
    explicit UsecodeThread(Loading loading) : DataStack(loading) {}
    /* Runs until the thread stops and returns why (THREAD_FINISHED: the
     * function's value is in result) (13f0:038b). */
    i16 run(u32 *result);

    /* Where the run goes on after a wait (the port's): from the start,
     * after an intrinsic, after a spawned process's constructor; the
     * yamm's owner the run found, the intrinsic's result and the spawned
     * process. */
    enum class ThreadResume : u8 { START, INTRINSIC, SPAWN, SPAWN_INLINE };
    struct ThreadAt {
        ThreadResume resume = ThreadResume::START;
        u16 owner = 0;
        s32 returned = 0;
        UsecodeProcess *spawned = nullptr;
    };
    i16 runFrom(u32 *result, ThreadAt at);
    /* While a wait was opened in a call of the run's: the run from at, as
     * the rest. */
    void giveThreadRest(u32 *result, ThreadAt at);
    void jump(s32 offset); /* 13f0:034f */
    /* Moves to the start of a function (13f0:035f). */
    void call(u16 classId, u16 function);
    /* The usecode's pointer to at in the stack (usemem.h). */
    u32 pointerTo(const u8 *at) const;
};
ASSERT_SIZE(UsecodeThread, 0xe5);

u16 numberString(u16 value);
/* UNCALLED: nothing calls it. */
u32 cSymbol(u16 index);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
s32 stringNumber(char *text);
#endif
