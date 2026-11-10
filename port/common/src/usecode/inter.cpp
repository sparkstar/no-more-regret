/* INTER.C (13f0:0000-3eb7): the usecode interpreter. See inter.h. */
#include "gamestate.h"
#include <cstdlib>
#include <cstring>
#include "inter.h"
#include "waits.h"
#include "debugger.h"
#include "flag.h"
#include "item.h"
#include "itemfind.h"
#include "kernel.h"
#include "mlist.h"
#include "system.h"
#include "uprocess.h"
#include "yamm.h"
#include "intrinsics.h"
#include "usesearch.h"
#include "translate.h"
#include "trace.h"

/* The tokens of the usecode (the original's word, in its "Undefined token"
 * message). The operands follow the token: xx is a byte, xxxx a word and
 * xxxxxxxx a long, and a local is at the signed offset xx from the frame.
 * Arithmetic and comparisons work on words, the _LONG ones on s32; a HUGE
 * value has n bytes. A binary operator's left operand was pushed first, its
 * right one is on top. The game's usecode never uses the tokens marked
 * unused. */
enum {
    OP_POP_LOCAL_BYTE = 0x00,    /* xx: pops a word into the byte of local xx */
    OP_POP_LOCAL_WORD = 0x01,    /* xx */
    OP_POP_LOCAL_LONG = 0x02,    /* xx */
    OP_ASSIGN_LOCAL_HUGE = 0x03, /* xx yy: copies the yy bytes on top to local xx and leaves them (unused) */
    OP_POP_RESULT = 0x08,        /* pops the result of the process */
    OP_ASSIGN_ELEMENT = 0x09,    /* xx yy zz: pops an index and copies the yy bytes on top to that element of
                                    local list xx (a string list if zz); they stay (unused) */
    OP_PUSH_BYTE = 0x0a,         /* xx: a constant, widened with its sign */
    OP_PUSH_WORD = 0x0b,         /* xxxx */
    OP_PUSH_LONG = 0x0c,         /* xxxxxxxx */
    OP_PUSH_STRING = 0x0d,       /* xxxx, then xxxx characters and a 0: a new string */
    OP_CREATE_LIST = 0x0e,       /* xx yy: pops yy values of xx bytes into a new list, the first pushed first */
    OP_CALL_INTRINSIC = 0x0f,    /* xx yyyy: C function yyyy with the xx bytes on top as its arguments; they stay */
    OP_CALL_NEAR = 0x10,         /* xxxxxxxx: calls the code xxxxxxxx bytes on in this function (unused) */
    OP_CALL = 0x11,              /* xxxx yyyy: calls function yyyy of class xxxx */
    OP_POP_RETURNED_WORD = 0x12, /* pops into what the intrinsic returned */
    OP_POP_RETURNED_LONG = 0x13, /* (unused) */
    OP_ADD = 0x14,
    OP_ADD_LONG = 0x15,
    OP_CONCAT = 0x16,             /* pops two strings, frees them and pushes them joined */
    OP_APPEND = 0x17,             /* pops two lists and pushes the first with the second appended */
    OP_UNION_LIST = 0x18,         /* xx: pops two lists of xx-byte elements, frees the second and pushes the first
                                     with the elements it lacks (unused) */
    OP_UNION_STRING_LIST = 0x19,  /* xx (unused) */
    OP_REMOVE_STRING_LIST = 0x1a, /* xx: as OP_UNION_LIST, but the first without the elements of the second */
    OP_REMOVE_LIST = 0x1b,        /* xx */
    OP_SUB = 0x1c,
    OP_SUB_LONG = 0x1d,
    OP_MUL = 0x1e,
    OP_MUL_LONG = 0x1f,
    OP_DIV = 0x20, /* 0 when dividing by 0 */
    OP_DIV_LONG = 0x21,
    OP_MOD = 0x22,
    OP_MOD_LONG = 0x23, /* (unused) */
    OP_EQ = 0x24,
    OP_EQ_LONG = 0x25,   /* (unused) */
    OP_EQ_STRING = 0x26, /* pops two strings, frees them and pushes whether they are the same */
    OP_EQ_HUGE = 0x27,   /* xx: whether the two values of xx bytes on top are the same; they stay (unused) */
    OP_LT = 0x28,
    OP_LT_LONG = 0x29, /* (unused) */
    OP_LE = 0x2a,
    OP_LE_LONG = 0x2b, /* (unused) */
    OP_GT = 0x2c,
    OP_GT_LONG = 0x2d,
    OP_GE = 0x2e,
    OP_GE_LONG = 0x2f, /* (unused) */
    OP_NOT = 0x30,
    OP_NOT_LONG = 0x31, /* (unused) */
    OP_AND = 0x32,
    OP_AND_LONG = 0x33, /* (unused) */
    OP_OR = 0x34,
    OP_OR_LONG = 0x35, /* (unused) */
    OP_NE = 0x36,
    OP_NE_LONG = 0x37,
    OP_IN_LIST = 0x38, /* xx yy: pops a list and a value of xx bytes, frees the list (and the value if
                          yy: a string list) and pushes whether it was in it */
    OP_BIT_AND = 0x39,
    OP_BIT_OR = 0x3a,
    OP_BIT_NOT = 0x3b,
    OP_SHIFT_LEFT = 0x3c,
    OP_SHIFT_RIGHT = 0x3d,            /* with the sign */
    OP_PUSH_LOCAL_BYTE = 0x3e,        /* xx: widened with its sign */
    OP_PUSH_LOCAL_WORD = 0x3f,        /* xx */
    OP_PUSH_LOCAL_LONG = 0x40,        /* xx */
    OP_PUSH_LOCAL_STRING = 0x41,      /* xx: a copy */
    OP_PUSH_LOCAL_LIST = 0x42,        /* xx yy: a copy; the elements have yy bytes */
    OP_PUSH_LOCAL_STRING_LIST = 0x43, /* xx: a copy with copies of the strings */
    OP_PUSH_ELEMENT = 0x44,           /* xx yy: pops an index (from 1) and a list and pushes that element of xx bytes,
                                         0 if none; bit 0 of yy: a string list (a copy of the string), bit 1: frees the
                                         list (unused) */
    OP_PUSH_LOCAL_HUGE = 0x45,        /* xx yy: yy bytes (unused) */
    OP_PUSH_LOCAL_ADDRESS = 0x4b,     /* xx */
    OP_PUSH_INDIRECT = 0x4c,          /* xx: pops a pointer and pushes the xx bytes there */
    OP_POP_INDIRECT = 0x4d,           /* xx: pops a pointer and pops xx bytes to there (unused) */
    OP_PUSH_GLOBAL = 0x4e,            /* xxxx yy: yy bytes of flags at xxxx, one byte as a word */
    OP_POP_GLOBAL = 0x4f,             /* xxxx yy */
    OP_RETURN = 0x50,
    OP_JUMP_IF_FALSE = 0x51, /* xxxx: pops a word and jumps by xxxx when it is 0 */
    OP_JUMP = 0x52,          /* xxxx */
    OP_SUSPEND = 0x53,       /* the thread goes on from here the next time it runs */
    OP_IMPLIES = 0x54,       /* xx yy: pops xx processes; the yy processes under them wait for them (the game
                                has 1 1) */
    OP_AND_IMPLIES = 0x55,   /* pushes LIST_AND (unused) */
    OP_OR_IMPLIES = 0x56,    /* pushes LIST_OR (unused) */
    OP_SPAWN = 0x57,         /* aa tt xxxx yyyy: pops a pointer to tt bytes and runs function yyyy of class
                                xxxx on a copy of them in a new process, with the aa bytes on top as its
                                arguments; its pid is what the intrinsic returned (0 if it ended at once) */
    OP_SPAWN_INLINE = 0x58,  /* xxxx yyyy zzzz tt uu: a new process of class xxxx with function yyyy+zzzz at
                                offset 1 and a copy of this
                                frame; pushes its pid (unused) */
    OP_PUSH_PID = 0x59,
    OP_ENTER = 0x5a,              /* xx: the frame of the function, with xx bytes of cleared locals */
    OP_DEBUG_LINE = 0x5b,         /* xxxx: the line, for the debugger (unused) */
    OP_DEBUG_FUNCTION = 0x5c,     /* xxxx, then the function's name in 9 bytes: the debugger's frame of the
                                     function, whose table of locals is xxxx bytes from the name (unused) */
    OP_PUSH_RETURNED_BYTE = 0x5d, /* what the intrinsic returned */
    OP_PUSH_RETURNED_WORD = 0x5e,
    OP_PUSH_RETURNED_LONG = 0x5f,
    OP_WORD_TO_LONG = 0x60, /* with the sign */
    OP_LONG_TO_WORD = 0x61,
    OP_FREE_LOCAL_STRING = 0x62,       /* xx */
    OP_FREE_LOCAL_STRING_LIST = 0x63,  /* xx */
    OP_FREE_LOCAL_LIST = 0x64,         /* xx */
    OP_FREE_STRING = 0x65,             /* xx: the one xx bytes into the stack */
    OP_FREE_LIST = 0x66,               /* xx (unused) */
    OP_FREE_STRING_LIST = 0x67,        /* xx (unused) */
    OP_COPY_STRING = 0x68,             /* pops a string and pushes a copy (unused) */
    OP_LOCAL_STRING_TO_POINTER = 0x69, /* xx */
    OP_POINTER_TO_STRING = 0x6a,       /* pops a pointer and pushes a new string of the text there (unused) */
    OP_STRING_TO_POINTER = 0x6b,       /* pops a string */
    OP_GIVE_LOCAL = 0x6c,              /* xx yy: gives the process local xx, a GIVE_* yy */
    OP_PUSH_RESULT = 0x6d,             /* the result of the process */
    OP_MOVE_TOP = 0x6e,                /* xx: reserve(xx); the usecode gives -n to drop the n bytes of a call's
                                          arguments */
    OP_PUSH_STACK_ADDRESS = 0x6f,      /* xx: top-xx (unused) */
    OP_LOOP = 0x70,                    /* xx yy zz: pops an item and a range or container and starts a search of kind zz
                                          (LOOP_*) with the loop script of yy bytes on the stack; the search stays
                                          there, and its item goes to local xx and on top (0 at the end) */
    OP_LOOP_NEXT = 0x73,               /* the next item of the search */
    OP_LOOP_SCRIPT = 0x74,             /* xx: pushes one byte of a loop script (scrit.h) */
    OP_FOREACH_LIST = 0x75,            /* xx yy zzzz: the next element (yy bytes) of the list under the top goes to
                                          local xx; at the end the list is freed and popped and the thread jumps by zzzz */
    OP_FOREACH_STRING_LIST = 0x76,     /* xx yy zzzz: as OP_FOREACH_LIST, a copy of the next string */
    OP_SET_INFO = 0x77,                /* pops the item and the type of the process */
    OP_EXCLUDE = 0x78,                 /* the thread finishes when its item has another process of its type */
    OP_PUSH_GLOBAL_ADDRESS = 0x79      /* xxxx: of flags at xxxx */
};
/* OP_LOOP: the searches (itemfind.h). */
enum {
    LOOP_AREA = 2, /* around an item */
    LOOP_RECURSIVE_AREA = 3,
    LOOP_CONTAINER = 4,
    LOOP_RECURSIVE_CONTAINER = 5,
    LOOP_SURFACE = 6
};
/* OP_GIVE_LOCAL: what the local is. For a string list the original passes 1
 * as the element size, not as the strings flag, so its strings keep their
 * owner (the game gives only strings). */
enum { GIVE_STRING = 1, GIVE_STRING_LIST = 2, GIVE_LIST = 3 };

i16 &unusedInter = gameGlobal<i16>(0);

void CallStack::push(u32 value)
{
    if (count < 31) {
        values[count++] = value;
        return;
    }
    halt();
}

u32 CallStack::pop()
{
    if (count == 0) {
        halt();
    }
    return values[--count];
}

namespace {
/* Where a process that OP_SPAWN started puts its result when it finishes
 * after the call (the original's pointer to the interpreter's stack). */
u32 &outlivedResult = gameGlobal<u32>();
}

/* A usecode string of value in decimal (13f0:0073). */
u16 numberString(u16 value)
{
    char text[10];

    itoa(value, text, 10);
    return yammString(text);
}

/* 13f0:009a */
/* UNCALLED: nothing calls it. */
s32 stringNumber(char *text)
{
    char *digits = text;

    return atol(digits);
}

/* The C function of a usecode intrinsic (13f0:00b8): the port calls them
 * through intrinsics.cpp's switch, whose OP_CALL_INTRINSIC keeps this
 * check. */
/* UNCALLED: nothing calls it. */
u32 cSymbol(u16 index)
{
    if (index >= cSymbolCount) {
        halt();
    }
    return cSymbols[index];
}

DataStack::DataStack(void *source, u16 length)
{
    top = end = data + sizeof(data);
    base = sizeof(data) - 1;
    if ((source != nullptr) && (length != 0U)) {
        memcpy(reserve(length), source, length);
    }
}

/* UNCALLED: nothing calls it. */
void DataStack::copy(DataStack *other)
{
    memcpy(data, other->data, sizeof(data));
    top = other->top;
    end = other->end;
}

void DataStack::enterFrame(u16 size)
{
    pushWord(base);
    base = top - data;
    top -= size;
    memset(data + base - size, 0, size);
}

UsecodeThread::UsecodeThread(void *arguments, u16 size, u8 *code, UsecodeProcess *ownerProcess, char *usecodeClassName,
                             u16 classId, u16 function)
    : DataStack(arguments, size)
{
    ip = code;
    started = 0;
    this->process = ownerProcess;
    this->className = usecodeClassName;
    usecodeMemory.addStack(ownerProcess->pid, data, sizeof(data));
    calls = new CallStack;
    u32 call = function;
    call <<= 16;
    call |= classId;
    calls->push(call);
}

void UsecodeThread::jump(s32 offset)
{
    ip += offset;
}

void UsecodeThread::call(u16 classId, u16 function)
{
    ip = classes->address(classId, function, 0);
}

u32 UsecodeThread::pointerTo(const u8 *at) const
{
    return usecodeMemory.stackPointer(process->pid, at);
}

/* The interpreter: one switch over the token byte. The operands are read
 * into the scratch variables a-e, first and second, which every token uses
 * in its own way; the stack words are ints, so a word widened to s32 keeps
 * its sign. A token that stops the thread sets done. */
i16 UsecodeThread::run(u32 *result)
{
    return runFrom(result, ThreadAt{});
}

void UsecodeThread::giveThreadRest(u32 *result, ThreadAt at)
{
    giveRest([this, result, at]() mutable {
        /* An intrinsic's result, which the wait's rests left; after a spawn
         * the last one's, as it was. */
        if (at.resume == ThreadResume::INTRINSIC) {
            at.returned = restResult();
        }
        const i16 state = runFrom(result, at);
        if (!waitOpened()) {
            setRestResult(state);
        }
        return false;
    });
}

/* The thread's run from at: from its start, or after an intrinsic or a
 * spawned process whose thread opened a wait. The thread then gives its
 * rest and returns (the yamm's owner left its process's, as while the
 * original waited in the call), and goes on from the call when the wait is
 * over. */
i16 UsecodeThread::runFrom(u32 *result, ThreadAt at)
{
    s32 first;
    s32 second;
    /* What the last intrinsic returned. The usecode sets it before each
     * PUSH_RETURNED (EUSECODE.FLX). */
    s32 returned = at.returned;
    i16 a;
    i16 b;
    i16 c;
    i16 d;
    i16 e;
    u8 *p;
    u8 *q;
    u8 *s;
    YammList from;
    YammList to;
    u16 owner;
    i16 op;
    i16 done;

    MListEntry entries[10];
    DebugFrame saved;
    UsecodeProcess *spawned;
    if (at.resume == ThreadResume::START) {
        if (yammOwner != 0U) {
            owner = yammOwner;
        } else {
            owner = 0;
        }
        yammOwner = process->pid;
        if (debugger != nullptr) {
            debugger->run();
        }
        if (started == 0U) {
            pushLong(0);
        }
    } else {
        owner = at.owner;
        yammOwner = process->pid;
    }
    done = 0;
    /* The rest of the call the wait came in (the port never makes a
     * debugger: nothing of it is kept over a wait). */
    switch (at.resume) {
    case ThreadResume::START:
        break;
    case ThreadResume::INTRINSIC:
        traceIntrinsicResult(process->pid, returned);
        if (process->hasFlags(PROCESS_DEAD) != 0U) {
            done = THREAD_DEAD;
            goto stopped;
        }
        break;
    case ThreadResume::SPAWN:
        spawned = at.spawned;
        if (spawned->hasFlags(PROCESS_DEAD) == 0U) {
            spawned->resultPointer = &outlivedResult;
        }
        if (process->hasFlags(PROCESS_DEAD) != 0U) {
            done = THREAD_DEAD;
            goto stopped;
        }
        if (spawned->hasFlags(PROCESS_DEAD) != 0U) {
            returned = 0;
            /* What it put through its pointer when it finished, else the
             * original's 0. */
            *result = (spawned->finished != 0U) ? spawned->result : 0;
        } else {
            returned = spawned->pid;
        }
        break;
    case ThreadResume::SPAWN_INLINE:
        spawned = at.spawned;
        if (process->hasFlags(PROCESS_DEAD) != 0U) {
            done = THREAD_DEAD;
            goto stopped;
        }
        a = data + base - top;
        memcpy(spawned->thread.reserve(a), top, a);
        a = (spawned->hasFlags(PROCESS_DEAD) != 0U) ? 0 : spawned->pid;
        pushWord(a);
        break;
    }
    while (done == 0) {
        op = static_cast<char>(*ip++);
        switch (op) {
        case OP_ENTER:
            enterFrame(static_cast<char>(*ip++));
            break;
        case OP_DEBUG_LINE:
            u16 line;
            line = *ipWord++;
            if (debugger != nullptr) {
                debugger->atLine(line);
            }
            break;
        case OP_DEBUG_FUNCTION:
            a = *ipWord++;
            if (debugger != nullptr) {
                debugger->enter(reinterpret_cast<char *>(ip), ip + a, data + base, 0);
            }
            ip += 9;
            break;
        case OP_POP_LOCAL_BYTE:
            i16 offset;
            offset = static_cast<char>(*ip++);
            setByte(offset, popWord());
            break;
        case OP_POP_LOCAL_WORD:
            offset = static_cast<char>(*ip++);
            setWord(offset, popWord());
            break;
        case OP_POP_LOCAL_LONG:
            offset = static_cast<char>(*ip++);
            setLong(offset, popLong());
            break;
        case OP_ASSIGN_LOCAL_HUGE:
            a = static_cast<char>(*ip++);
            b = static_cast<char>(*ip++);
            memcpy(data + base + a, top, b);
            break;
        case OP_POP_RESULT:
            *result = popLong();
            break;
        case OP_POP_INDIRECT:
            a = static_cast<char>(*ip++);
            p = static_cast<u8 *>(popPointer(a));
            memcpy(p, top, a);
            reserve(-a);
            break;
        case OP_LOOP_SCRIPT:
            pushByte(*ip++);
            break;
        case OP_PUSH_BYTE:
            pushWord(static_cast<char>(*ip++));
            break;
        case OP_PUSH_WORD:
            pushWord(*ipWord++);
            break;
        case OP_PUSH_LONG:
            pushLong(*ipLong++);
            break;
        case OP_PUSH_LOCAL_BYTE:
            pushWord(getByte(static_cast<char>(*ip++)));
            break;
        case OP_PUSH_LOCAL_WORD:
            pushWord(getWord(static_cast<char>(*ip++)));
            break;
        case OP_PUSH_LOCAL_LONG:
            pushLong(getLong(static_cast<char>(*ip++)));
            break;
        case OP_PUSH_LOCAL_HUGE:
            a = static_cast<char>(*ip++);
            b = static_cast<char>(*ip++);
            p = top;
            reserve(b);
            memcpy(p - b, data + base + a, b);
            break;
        case OP_PUSH_RESULT:
            pushLong(*result);
            break;
        case OP_PUSH_INDIRECT:
            a = static_cast<char>(*ip++);
            p = static_cast<u8 *>(popPointer(a));
            q = top;
            reserve(a);
            memcpy(q - a, p, a);
            break;
        case OP_PUSH_GLOBAL_ADDRESS:
            a = *ipWord++;
            pushLong(static_cast<s32>(usecodeMemory.flagsPointer(static_cast<u16>(a))));
            break;
        case OP_PUSH_GLOBAL:
            a = *ipWord++;
            b = static_cast<char>(*ip++);
            if (b == 1) {
                reserve(2);
                top[0] = flags[a];
                top[1] = 0;
            } else {
                memcpy(reserve(b), flags + a, b);
            }
            break;
        case OP_POP_GLOBAL:
            a = *ipWord++;
            b = static_cast<char>(*ip++);
            memcpy(flags + a, top, b);
            reserve(b == 1 ? -2 : -b);
            break;
        case OP_CREATE_LIST:
            a = static_cast<char>(*ip++);
            b = static_cast<char>(*ip++);
            d = 0;
            e = 0;
            for (c = 0; c < b; c++) {
                d = yammAlloc(a + 2);
                p = yammHeap + d;
                memcpy(p + 2, top + static_cast<i16>(c * a), a);
                *reinterpret_cast<u16 *>(p) = e;
                e = d;
            }
            reserve(-a * b);
            pushWord(d);
            break;
        case OP_PUSH_LOCAL_LIST:
            a = static_cast<char>(*ip++);
            b = static_cast<char>(*ip++);
            c = *reinterpret_cast<i16 *>(data + base + a);
            from.attach(c, b, 0);
            to.copy(&from);
            pushWord(to.head);
            to.attach(0, 0, 0);
            break;
        case OP_FREE_LOCAL_LIST:
            YammList(getWord(static_cast<char>(*ip++)), 0, 0).clear();
            break;
        case OP_PUSH_ELEMENT: {
            a = popWord();
            b = popWord();
            c = static_cast<char>(*ip++);
            d = static_cast<char>(*ip++);
            if (c == 1) {
                *reinterpret_cast<u16 *>(reserve(2)) = 0;
            } else {
                reserve(c);
            }
            const YammList elements(b, c, d & 1);
            q = static_cast<u8 *>(elements.at(a - 1));
            if (q != nullptr) {
                if ((d & 1) != 0) {
                    *reinterpret_cast<u16 *>(top) = yammString(reinterpret_cast<char *>(q));
                } else {
                    memcpy(top, q, c);
                }
            } else {
                memset(top, 0, c);
            }
            if ((d & 2) != 0) {
                elements.clear();
            }
            break;
        }
        case OP_APPEND: {
            b = popWord();
            a = popWord();
            YammList appended(a, 0, 0);
            const YammList tail(b, 0, 0);
            appended.append(b);
            pushWord(appended.head);
            break;
        }
        case OP_FREE_LIST: {
            a = static_cast<char>(*ip++);
            const YammList dropped(*reinterpret_cast<u16 *>(top + a), 0, 0);
            dropped.clear();
            break;
        }
        case OP_IN_LIST: {
            a = static_cast<char>(*ip++);
            d = static_cast<char>(*ip++);
            b = popWord();
            p = top;
            YammList members(b, a, d);
            c = members.contains(p);
            members.clear();
            if (d != 0) {
                yammFree(*reinterpret_cast<u16 *>(top));
            }
            reserve(-a);
            pushWord(c);
            break;
        }
        case OP_ASSIGN_ELEMENT: {
            a = popWord();
            b = static_cast<char>(*ip++);
            c = static_cast<char>(*ip++);
            d = static_cast<char>(*ip++);
            const YammList assigned(*reinterpret_cast<u16 *>(data + base + b), c, d);
            assigned.set(a, top);
            break;
        }
        case OP_PUSH_LOCAL_STRING_LIST:
            a = static_cast<char>(*ip++);
            b = *reinterpret_cast<i16 *>(data + base + a);
            from.attach(b, 2, 1);
            to.copy(&from);
            pushWord(to.head);
            to.attach(0, 0, 0);
            break;
        case OP_FREE_LOCAL_STRING_LIST:
            YammList(getWord(static_cast<char>(*ip++)), 0, 1).clear();
            break;
        case OP_UNION_LIST:
        case OP_UNION_STRING_LIST:
        case OP_REMOVE_STRING_LIST:
        case OP_REMOVE_LIST: {
            a = static_cast<char>(*ip++);
            c = popWord();
            b = popWord();
            YammList left(b, a, static_cast<i16>(op == OP_UNION_STRING_LIST || op == OP_REMOVE_STRING_LIST));
            const YammList right(c, a, static_cast<i16>(op == OP_UNION_STRING_LIST || op == OP_REMOVE_STRING_LIST));
            if (op == OP_REMOVE_STRING_LIST || op == OP_REMOVE_LIST) {
                left.remove(right.head);
            } else {
                left.merge(right.head);
            }
            pushWord(left.head);
            right.clear();
            break;
        }
        case OP_FREE_STRING_LIST: {
            const YammList strings(*reinterpret_cast<u16 *>(stackAt(static_cast<char>(*ip++))), 0, 1);
            strings.clear();
            break;
        }
        case OP_PUSH_STRING:
            i16 length;
            length = *ipWord++;
            /* The literal's translation in the game's language
             * (translate.h); none in English. */
            const char *translated;
            translated = usecodeTranslation(reinterpret_cast<const char *>(ip), static_cast<std::size_t>(length));
            if (translated != nullptr) {
                u16 size;
                size = static_cast<u16>(strlen(translated));
                a = yammAlloc(size + 1);
                if (a == 0) {
                    halt();
                }
                memcpy(yammHeap + a, translated, size + 1);
                pushWord(a);
                jump(static_cast<i16>(length + 1));
                break;
            }
            a = yammAlloc(length + 1);
            if (a == 0) {
                halt();
            }
            p = yammHeap + a;
            strncpy(reinterpret_cast<char *>(p), reinterpret_cast<char *>(ip), length);
            p[length] = 0;
            pushWord(a);
            jump(static_cast<i16>(length + 1));
            break;
        case OP_PUSH_LOCAL_STRING:
            a = static_cast<char>(*ip++);
            b = *reinterpret_cast<i16 *>(data + base + a);
            p = yammHeap + b;
            c = yammAlloc(strlen(reinterpret_cast<char *>(p)) + 1);
            q = yammHeap + c;
            strcpy(reinterpret_cast<char *>(q), reinterpret_cast<char *>(p));
            pushWord(c);
            break;
        case OP_COPY_STRING:
            a = popWord();
            p = yammHeap + a;
            b = strlen(reinterpret_cast<char *>(p));
            c = yammAlloc(b + 1);
            q = yammHeap + c;
            strncpy(reinterpret_cast<char *>(q), reinterpret_cast<char *>(p), b + 1);
            q[b] = 0;
            pushWord(c);
            break;
        case OP_CONCAT:
        case OP_EQ_STRING:
            b = popWord();
            a = popWord();
            p = yammHeap + a;
            q = yammHeap + b;
            if (op == OP_EQ_STRING) {
                c = static_cast<i16>(strcmp(reinterpret_cast<char *>(p), reinterpret_cast<char *>(q)) == 0);
                goto freeStrings;
            }
            d = strlen(reinterpret_cast<char *>(p));
            e = strlen(reinterpret_cast<char *>(q));
            c = yammAlloc(d + e + 1);
            s = yammHeap + c;
            strcpy(reinterpret_cast<char *>(s), reinterpret_cast<char *>(p));
            strcat(reinterpret_cast<char *>(s), reinterpret_cast<char *>(q));
            /* 26 comes here from its compare: a goto, as the original's dead
             * second JMP shows. */
        freeStrings:
            yammFree(b);
            yammFree(a);
            pushWord(c);
            break;
        case OP_FREE_STRING:
            a = static_cast<char>(*ip++);
            yammFree(*reinterpret_cast<u16 *>(top + a));
            break;
        case OP_FREE_LOCAL_STRING:
            a = static_cast<char>(*ip++);
            yammFree(*reinterpret_cast<u16 *>(data + base + a));
            break;
        case OP_POINTER_TO_STRING:
            p = static_cast<u8 *>(popPointer(1));
            b = yammString(reinterpret_cast<char *>(p));
            pushWord(b);
            break;
        case OP_STRING_TO_POINTER:
            a = popWord();
            pushLong(static_cast<s32>(usecodeMemory.yammPointer(static_cast<u16>(a))));
            break;
        case OP_LOCAL_STRING_TO_POINTER:
            a = static_cast<char>(*ip++);
            pushLong(static_cast<s32>(usecodeMemory.yammPointer(*reinterpret_cast<u16 *>(data + base + a))));
            break;
        case OP_ADD:
            pushWord(popWord() + popWord());
            break;
        case OP_ADD_LONG:
            pushLong(popLong() + popLong());
            break;
        case OP_SUB:
            b = popWord();
            a = popWord();
            pushWord(a - b);
            break;
        case OP_SUB_LONG:
            second = popLong();
            first = popLong();
            pushLong(first - second);
            break;
        case OP_MUL:
            pushWord(popWord() * popWord());
            break;
        case OP_MUL_LONG:
            pushLong(popLong() * popLong());
            break;
        case OP_DIV:
            b = popWord();
            a = popWord();
            if (b == 0) {
                pushWord(0);
            } else {
                pushWord(a / b);
            }
            break;
        case OP_DIV_LONG:
            second = popLong();
            first = popLong();
            /* BCC's LDIV@ gives -2^31 for -2^31 by -1, where the port's
             * division would trap. */
            if (second == 0) {
                pushLong(0);
            } else if (second == -1) {
                pushLong(static_cast<s32>(0U - static_cast<u32>(first)));
            } else {
                pushLong(first / second);
            }
            break;
        case OP_MOD:
            b = popWord();
            a = popWord();
            if (b == 0) {
                pushWord(0);
            } else {
                pushWord(a % b);
            }
            break;
        case OP_MOD_LONG:
            second = popLong();
            first = popLong();
            if (second == 0 || second == -1) {
                pushLong(0);
            } else {
                pushLong(first % second);
            }
            break;
        case OP_EQ:
            a = popWord();
            b = popWord();
            pushWord(static_cast<u16>(a == b));
            break;
        case OP_EQ_LONG:
            pushWord(static_cast<u16>(popLong() == popLong()));
            break;
        case OP_EQ_HUGE:
            a = static_cast<char>(*ip++);
            pushWord(static_cast<u16>(memcmp(top, top + a, a) == 0));
            break;
        case OP_NE:
            pushWord(static_cast<u16>(popWord() != popWord()));
            break;
        case OP_NE_LONG:
            pushWord(static_cast<u16>(popLong() != popLong()));
            break;
        case OP_LT:
            /* The right operand is on top. BCC took it first as the left
             * side of top > under; C++ leaves the operands' order to the
             * compiler, so the pops are written out (and so for the rest). */
            b = popWord();
            a = popWord();
            pushWord(static_cast<u16>(a < b));
            break;
        case OP_LT_LONG:
            second = popLong();
            first = popLong();
            pushWord(static_cast<u16>(first < second));
            break;
        case OP_LE:
            b = popWord();
            a = popWord();
            pushWord(static_cast<u16>(a <= b));
            break;
        case OP_LE_LONG:
            second = popLong();
            first = popLong();
            pushWord(static_cast<u16>(first <= second));
            break;
        case OP_GT:
            b = popWord();
            a = popWord();
            pushWord(static_cast<u16>(a > b));
            break;
        case OP_GT_LONG:
            second = popLong();
            first = popLong();
            pushWord(static_cast<u16>(first > second));
            break;
        case OP_GE:
            b = popWord();
            a = popWord();
            pushWord(static_cast<u16>(a >= b));
            break;
        case OP_GE_LONG:
            second = popLong();
            first = popLong();
            pushWord(static_cast<u16>(first >= second));
            break;
        case OP_AND:
            a = popWord();
            b = popWord();
            pushWord(static_cast<u16>((a != 0) && (b != 0)));
            break;
        case OP_AND_LONG:
            first = popLong();
            second = popLong();
            pushWord(static_cast<u16>((first != 0) && (second != 0)));
            break;
        case OP_OR:
            a = popWord();
            b = popWord();
            pushWord(static_cast<u16>((a != 0) || (b != 0)));
            break;
        case OP_OR_LONG:
            first = popLong();
            second = popLong();
            pushWord(static_cast<u16>((first != 0) || (second != 0)));
            break;
        case OP_NOT:
            pushWord(static_cast<u16>(popWord() == 0));
            break;
        case OP_NOT_LONG:
            pushWord(static_cast<u16>(popLong() == 0));
            break;
        case OP_BIT_AND:
            pushWord(popWord() & popWord());
            break;
        case OP_BIT_OR:
            pushWord(popWord() | popWord());
            break;
        case OP_SHIFT_LEFT:
            a = popWord();
            b = popWord();
            pushWord(b << a);
            break;
        case OP_SHIFT_RIGHT:
            a = popWord();
            b = popWord();
            pushWord(b >> a);
            break;
        case OP_BIT_NOT:
            pushWord(~popWord());
            break;
        case OP_SPAWN:
            if (debugger != nullptr) {
                debugger->nesting++;
            }
            first = static_cast<char>(*ip++);
            a = static_cast<char>(*ip++);
            p = static_cast<u8 *>(popPointer(a));
            second = *ipLong++;
            i16 entered;
            entered = 0;
            if (debugger != nullptr) {
                DebugFrame *frame =
                    (debugger->depth != 0) ? &debugger->frames[static_cast<i16>(debugger->depth - 1)] : nullptr;
                if (frame != nullptr) {
                    entered++;
                    saved = *frame;
                    debugger->leave();
                }
            }
            /* The original's value is what its stack held when the process is
             * killed before it finishes. */
            u32 value;
            value = 0;
            spawned = new UsecodeProcess(&value, p, a, second, second >> 16, 0, top, first);
            if (waitOpened()) {
                /* Its thread waits: value goes with this call, so it puts
                 * its result where nothing reads it (as the original's
                 * process keeps its pointer to the interpreter's stack all
                 * the while), and the rest reads it from the process. */
                spawned->resultPointer = &outlivedResult;
                giveThreadRest(
                    result,
                    ThreadAt{.resume = ThreadResume::SPAWN, .owner = owner, .returned = returned, .spawned = spawned});
                return THREAD_WAITING;
            }
            /* The original's process keeps the address of value, on the
             * interpreter's stack, and puts its result there when it finishes,
             * after this call has returned. */
            if (spawned->hasFlags(PROCESS_DEAD) == 0U) {
                spawned->resultPointer = &outlivedResult;
            }
            if ((debugger != nullptr) && (entered != 0)) {
                debugger->enter(&saved);
            }
            if (process->hasFlags(PROCESS_DEAD) != 0U) {
                done = THREAD_DEAD;
                goto stopped;
            }
            if (spawned->hasFlags(PROCESS_DEAD) != 0U) {
                returned = 0;
                *result = value;
            } else {
                returned = spawned->pid;
            }
            break;
        case OP_SPAWN_INLINE:
            a = *ipWord++;
            b = *ipWord++;
            c = *ipWord++;
            d = static_cast<char>(*ip++);
            e = static_cast<char>(*ip++);
            spawned = new UsecodeProcess(nullptr, usecodeMemory.at(*reinterpret_cast<u32 *>(data + base + 6), d), d, a,
                                         b + c, 1, data + base + 10, e);
            if (waitOpened()) {
                giveThreadRest(result, ThreadAt{.resume = ThreadResume::SPAWN_INLINE,
                                                .owner = owner,
                                                .returned = returned,
                                                .spawned = spawned});
                return THREAD_WAITING;
            }
            if (process->hasFlags(PROCESS_DEAD) != 0U) {
                done = THREAD_DEAD;
                goto stopped;
            }
            a = data + base - top;
            memcpy(spawned->thread.reserve(a), top, a);
            a = (spawned->hasFlags(PROCESS_DEAD) != 0U) ? 0 : spawned->pid;
            pushWord(a);
            break;
        case OP_IMPLIES:
            i16 count;
            count = static_cast<char>(*ip++);
            i16 waiting;
            waiting = static_cast<char>(*ip++);
            i16 i;
            for (i = 0; i < count; i++) {
                const u16 pid = popWord();
                entries[i] = MListEntry(pid);
                for (i16 j = count + waiting - 1 - i; j > static_cast<i16>(count - 1 - i); j--) {
                    const u16 waiter = (reinterpret_cast<u16 *>(top))[static_cast<i16>(j - 1)];
                    addWait(waiter, pid);
                }
            }
            popWord();
            for (i = 0; i < count; i++) {
                if (entries[i].isPid()) {
                    pushWord(entries[i].value);
                }
            }
            break;
        case OP_AND_IMPLIES:
            pushWord(LIST_AND);
            break;
        case OP_OR_IMPLIES:
            pushWord(LIST_OR);
            break;
        case OP_SUSPEND:
            done = THREAD_SUSPENDED;
            started = 1;
            break;
        case OP_SET_INFO:
            b = popWord();
            a = popWord();
            process->setType(a);
            process->setItem(b);
            break;
        case OP_EXCLUDE:
            a = countItemProcesses(process->item, process->type);
            if (a > 1) {
                done = THREAD_FINISHED;
            }
            break;
        case OP_PUSH_PID:
            pushWord(process->pid);
            break;
        case OP_GIVE_LOCAL:
            a = static_cast<char>(*ip++);
            b = static_cast<char>(*ip++);
            c = *reinterpret_cast<i16 *>(data + base + a);
            d = process->pid;
            if (b == GIVE_STRING) {
                setChunkOwner(c, d);
            } else if (b == GIVE_STRING_LIST) {
                const YammList stringList(c, 1, 0);
                stringList.setOwner(d);
            } else if (b == GIVE_LIST) {
                const YammList list(c, 0, 0);
                list.setOwner(d);
            }
            break;
        case OP_CALL_INTRINSIC:
            /* The original makes size bytes of room on the CPU's stack, copies
             * the arguments there and calls the far address of cSymbol(number)
             * without arguments; the port's switch takes them from the usecode's
             * stack (intrinsics.h). */
            i16 size;
            size = static_cast<char>(*ip++);
            u16 number;
            number = *ipWord++;
            if (number >= cSymbolCount) {
                halt(); /* cSymbol's check */
            }
            returned = callIntrinsic(number, UsecodeArguments(top));
            traceIntrinsic(process->pid, process->classId, process->function, number, top, static_cast<u16>(size),
                           returned, waitOpened());
            if (waitOpened()) {
                giveThreadRest(result, ThreadAt{.resume = ThreadResume::INTRINSIC, .owner = owner});
                return THREAD_WAITING;
            }
            if (process->hasFlags(PROCESS_DEAD) != 0U) {
                done = THREAD_DEAD;
                goto stopped;
            }
            break;
        case OP_CALL_NEAR:
        case OP_CALL:
            if (debugger != nullptr) {
                debugger->nesting++;
            }
            first = *ipLong++;
            second = static_cast<u32>(process->function) << 16 | process->classId;
            u16 classId;
            classId = first;
            u16 function;
            function = first >> 16;
            pushLong(static_cast<s32>(usecodeMemory.codePointer(ip)));
            calls->push(second);
            if (op == OP_CALL) {
                traceCall(process->pid, classId, function);
                call(classId, function);
                process->classId = classId;
                process->function = function;
            } else {
                jump(first);
            }
            break;
        case OP_PUSH_RETURNED_BYTE:
            pushWord(returned & 0xff);
            break;
        case OP_PUSH_RETURNED_WORD:
            pushWord(returned);
            break;
        case OP_PUSH_RETURNED_LONG:
            pushLong(returned);
            break;
        case OP_MOVE_TOP:
            a = static_cast<char>(*ip++);
            reserve(a);
            break;
        case OP_RETURN:
            top = data + base;
            base = popWord();
            first = popLong();
            second = calls->pop();
            classes->release(process->classId, process->function);
            process->classId = second;
            process->function = second >> 16;
            if (first == 0) {
                done = THREAD_FINISHED;
            } else {
                if (debugger != nullptr) {
                    if (debugger->depth > 0) {
                        debugger->leave();
                    }
                    debugger->nesting--;
                }
                ip = usecodeMemory.at(first, 1);
            }
            break;
        case OP_LOOP:
            a = static_cast<char>(*ip++);
            b = static_cast<char>(*ip++);
            e = static_cast<char>(*ip++);
            c = popWord();
            d = popWord();
            memcpy(formatBuffer, top, b);
            u16 searchSize;
            ItemSearch *search;
            /* The usecode's loops are of kinds 2, 4 and 6 (EUSECODE.FLX). */
            search = nullptr;
            {
                const Item item(c);
                switch (e) {
                case LOOP_AREA:
                    search = new AreaSearch(itemX[item.id], itemY[item.id], d, formatBuffer, b);
                    break;
                case LOOP_RECURSIVE_AREA:
                    search = new RecursiveAreaSearch(itemX[item.id], itemY[item.id], d, formatBuffer, b);
                    break;
                case LOOP_CONTAINER:
                    search = new ContainerSearch(d, formatBuffer, b);
                    break;
                case LOOP_RECURSIVE_CONTAINER:
                    search = new RecursiveContainerSearch(d, formatBuffer, b);
                    break;
                case LOOP_SURFACE: {
                    /* Each surface loop of the usecode gives an item on one
                     * side and -1 on the other (EUSECODE.FLX). */
                    i16 mode = 0;
                    u16 target = 0;
                    if (d != -1 && c != -1) {
                        mode = 3;
                        target = d;
                    } else {
                        if (d != -1) {
                            mode = 2;
                            target = d;
                        }
                        if (c != -1) {
                            mode = 1;
                            target = c;
                        }
                    }
                    search = new SurfaceSearch(target, formatBuffer, b, mode);
                    break;
                }
                }
            }
            /* The original copies its search over the script's b bytes on the
             * stack, sizeof as BCC laid it out; the port puts it there so
             * (usesearch.h). */
            searchSize = searchImageSize(e);
            putSearchImage(reserve(searchSize - b), search, e);
            pushWord(a);
            d = search->id;
            *reinterpret_cast<i16 *>(data + base + a) = d;
            if (d != 0) {
                nextSearchImage(top + 2);
            }
            pushWord(d);
            delete search;
            break;
        case OP_LOOP_NEXT:
            a = *reinterpret_cast<i16 *>(top + 2);
            b = *reinterpret_cast<i16 *>(top);
            *reinterpret_cast<i16 *>(data + base + b) = a;
            pushWord(a);
            if (a != 0) {
                nextSearchImage(top + 4);
            }
            break;
        case OP_FOREACH_LIST:
        case OP_FOREACH_STRING_LIST:
            b = static_cast<char>(*ip++);
            c = static_cast<char>(*ip++);
            d = *ipWord++;
            a = *reinterpret_cast<i16 *>(top);
            if (a == -1) {
                a = *reinterpret_cast<i16 *>(top + 2);
            }
            if (a == 0) {
                jump(d);
                YammList(*reinterpret_cast<u16 *>(top + 2), 0, static_cast<i16>(op == OP_FOREACH_STRING_LIST)).clear();
                popLong();
                break;
            }
            p = yammHeap + a;
            *reinterpret_cast<u16 *>(top) = *reinterpret_cast<u16 *>(p);
            q = data + base + b;
            if (op == OP_FOREACH_STRING_LIST) {
                yammFree(*reinterpret_cast<u16 *>(q));
                *reinterpret_cast<u16 *>(q) =
                    yammString(reinterpret_cast<char *>(yammHeap + *reinterpret_cast<u16 *>(p + 2)));
            } else {
                memcpy(data + base + b, p + 2, c);
            }
            break;
        case OP_JUMP_IF_FALSE:
            first = popWord();
            a = *ipWord++;
            if (first == 0) {
                jump(a);
            }
            break;
        case OP_JUMP:
            jump(*ipWord++);
            break;
        case OP_WORD_TO_LONG:
            pushLong(popWord());
            break;
        case OP_LONG_TO_WORD:
            pushWord(popLong());
            break;
        case OP_PUSH_STACK_ADDRESS:
            pushLong(static_cast<s32>(pointerTo(stackAt(-static_cast<char>(*ip++)))));
            break;
        case OP_PUSH_LOCAL_ADDRESS:
            pushLong(static_cast<s32>(pointerTo(local(static_cast<char>(*ip++)))));
            break;
        case OP_POP_RETURNED_WORD:
            returned = popWord();
            break;
        case OP_POP_RETURNED_LONG:
            returned = popLong();
            break;
        default:
            haltMessage(haltFormat("Undefined token %d\n", op));
        }
    }
    /* A process that died during a call leaves the switch and the loop here. */
stopped:
    yammOwner = owner;
    if (debugger != nullptr) {
        if (debugger->depth > 0) {
            debugger->leave();
        }
        debugger->nesting--;
    }
    return done;
}
