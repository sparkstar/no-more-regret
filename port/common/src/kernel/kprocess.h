/* PROCESS.C (11f8:0000-1b6b): the kernel's processes. Every process has a pid
 * (its slot in the kernel's table), may belong to an item, and keeps two
 * lists: the ids it waits on, and its relations to other processes (waiting,
 * waited on, listening, listened to). EventProcess adds events and
 * PeriodicProcess a period on the kernel's timer. See KERNEL.C (kernel.h). */
#ifndef PORT_PROCESS_H
#define PORT_PROCESS_H
#include "common.h"
#include "mlist.h"
#include "notlist.h"

#include <new>

class File;

/* Process::flags */
constexpr int PROCESS_ACTIVE = 0x1;
constexpr int PROCESS_SYSTEM = 0x2;     /* cannot be suspended */
constexpr int PROCESS_SUBSCRIBED = 0x4; /* on the interrupt handler's list (STDINT.C) */
constexpr int PROCESS_DEAD = 0x8;
constexpr int PROCESS_FAIL_WITH_CHILD = 0x10; /* fails when a process it waits on fails */
constexpr int PROCESS_FAILED = 0x20;
constexpr int PROCESS_BLOCKED = 0x40;
constexpr int PROCESS_RECEIVES = 0x80;  /* takes messages (Process::send) */
constexpr int PROCESS_EVENTS = 0x100;   /* an EventProcess */
constexpr int PROCESS_PERIODIC = 0x200; /* a PeriodicProcess */
constexpr int PROCESS_TIMED = 0x400;    /* on the kernel's timer */
constexpr int PROCESS_SUSPENDED = 0x800;
constexpr int PROCESS_QUEUED = 0x1000;
constexpr int PROCESS_PAUSED = 0x2000;
constexpr int PROCESS_KILLED = 0x4000;
constexpr int PROCESS_CHILD_FAILED = 0x8000;
/* Process::status */
constexpr int STATUS_STARTED = 0x1;
constexpr int STATUS_WATCHED = 0x2;
constexpr int STATUS_CREATED_CHILD = 0x4;
constexpr int STATUS_MOUSE = 0x40;    /* on the kernel's mouse list */
constexpr int STATUS_KEYBOARD = 0x80; /* on the kernel's keyboard list */
constexpr int STATUS_HOLD = 0x100;
constexpr int STATUS_NEW = 0x200; /* registered while the kernel kills processes */
constexpr int STATUS_TICKING = 0x400;
constexpr int STATUS_TIMEOUT = 0x1000;
/* EventProcess events */
constexpr s32 EVENT_RELATED_DIED = (static_cast<s32>(0x1L));
constexpr s32 EVENT_TIMER = (static_cast<s32>(0x2L));
constexpr s32 EVENT_WAIT_OVER = (static_cast<s32>(0x4L));
constexpr s32 EVENT_KILL_ALL = (static_cast<s32>(0x8L)); /* before killAll kills the process */
constexpr s32 EVENT_KEYBOARD = (static_cast<s32>(0x10L));
constexpr s32 EVENT_MOUSE = (static_cast<s32>(0x40L));
constexpr s32 EVENT_TIMEOUT = (static_cast<s32>(0x100L));
/* Process::type: the kernel's own processes. A process may be suspended only
 * when it has another type (Process::suspend). */
constexpr int DEFAULT_TYPE = 1;      /* Process() */
constexpr int STDINT_TYPE = 2;       /* StdIntHandler */
constexpr int BIOS_TIMER_TYPE = 3;   /* BiosTimer */
constexpr int SYSTEM_TIMER_TYPE = 4; /* SystemTimer */
constexpr int KEYBOARD_TYPE = 7;     /* KeyboardProcess */
constexpr int KEY_DAEMON_TYPE = 8;   /* KeyDaemon */
constexpr int WAIT_TYPE = 9;         /* WaitProcess */
constexpr int ACC_WAIT_TYPE = 10;    /* AccWait */
constexpr int TIMER_TYPE = 11;       /* TimerProcess */
/* Searches: any type, and the end of a list of types. */
constexpr int ANY_TYPE = 12;
constexpr int TYPE_END = 13;
/* Types the kernel counts per item. */
constexpr int FIRST_ITEM_TYPE = 0xf0;
#define IS_ITEM_TYPE(type) ((type) >= FIRST_ITEM_TYPE && (type) <= 0xf7)
/* Priority levels (Process::level): the kernel runs the processes of its
 * current level and of the top two levels, which it never makes current. */
constexpr int LEVELS = 32;
constexpr int BASE_LEVEL = 1; /* the level the kernel starts at: the game's processes */
constexpr int ALWAYS_LEVEL = 30;
constexpr int SYSTEM_LEVEL = 31; /* the kernel's own processes */

/* A process made to load a saved one into: the original's loadKernel poured
 * the saved bytes into new memory and ran no constructor, so a Loading
 * constructor does nothing of the kernel's and the saved game's fields come
 * after it (savefields.h). */
struct Loading {};

class Process {
public:
    u16 pid;
    u16 item;
    i16 type;
    u8 priority;
    u8 reserved;
    u32 timeoutStart;
    u32 timeout;
    u32 result;
    u16 flags;
    u16 status;
    u16 allocations;
    u16 messageMask;
    MList waitList;
    NotifyList relations;

    Process();
    explicit Process(Loading /*unused*/) {}
    /* The memory of a process, 0 (process.cpp). */
    void *operator new(size_t size);
    /* A delete of a process, which only a killed one may have (the kernel
     * frees its own without one: kernel.cpp's freeProcess). It reads the
     * flags before the destructor runs, as a destroying delete; the
     * original's read them after, from an object that was no more. */
    void operator delete(Process *process, std::destroying_delete_t /*unused*/);
    /* Only when a constructor throws, which none does: the memory back. */
    void operator delete(void *process);
    /* The memory of a process to load: no size for the kernel to take. */
    void *operator new(size_t size, Loading /*unused*/);
    /* UNCALLED: nothing calls it. */
    void operator delete(void *process, Loading /*unused*/);
    /* The new flags go in with one store, which an interrupt cannot split. */
    void addFlags(u16 bits)
    {
        const u16 value = flags | bits;
        flags = value;
    }
    /* UNCALLED: nothing calls it. */
    void removeFlags(u16 bits)
    {
        const u16 value = flags & ~bits;
        flags = value;
    }
    [[nodiscard]] u8 hasFlags(u16 bits) const { return ((flags & bits) != 0) ? 1 : 0; }
    [[nodiscard]] u8 hasStatus(u16 bits) const { return ((status & bits) != 0) ? 1 : 0; }
    [[nodiscard]] u8 level() const { return priority & 0x1f; }
    void setResult(u32 value) { result = value; }
    [[nodiscard]] u16 getPid() const { return pid; }
    /* UNCALLED: nothing calls it. */
    void setPid(u16 id) { pid = id; }
    void setItem(u16 newItem);
    void setType(i16 newType);
    void setPriority(u8 level);
    void setTimeout(u32 delay);
    void clearTimeout();
    void checkTimeout();
    /* UNCALLED: nothing calls it. */
    void raisePriority();
    void detach();
    void unlink();
    void shutdown();
    void clearEvents();
    void kill();
    void checkWait(u16 finished);
    /* build with the arguments, to LIST_END, that PROCESS.C's assembly pushes
     * again. */
    template <class... Words> void waitOn(Words... words) { waitList.build(pid, words...); }
    void removeListener(u16 other);
    void removeListening(u16 other);
    void removeWaitingFor(u16 other);
    void removeWaiter(u16 other);
    void addListener(u16 other);
    void addListening(u16 other);
    void addWaitingFor(u16 other);
    void addWaiter(u16 other);
    /* UNCALLED: nothing calls it. */
    Process *waitFor(Process *process);
    /* UNCALLED: nothing calls it. */
    Process *waitForChild(Process *child);
    Process *block(Process *waiter);
    /* UNCALLED: nothing calls it. */
    Process *blockChild(Process *waiter);
    /* UNCALLED: nothing calls it. */
    u16 waitFor(u16 other);
    /* UNCALLED: nothing calls it. */
    void broadcast(u16 message, u16 filter, u16 mask, void *data) const;
    /* UNCALLED: nothing calls it. */
    void send(u16 message, u16 other, void *data) const;
    /* UNCALLED: nothing calls it. */
    void suspend();
    /* UNCALLED: nothing calls it. */
    void resume();
    void makeSystem();
    void queue();
    void unqueue();
    void markDead();
    /* UNCALLED: nothing calls it. */
    void *allocate(u32 size);
    /* UNCALLED: nothing calls it. */
    void release(void *memory);
    /* UNCALLED: nothing calls it. */
    void stopListening(u16 other);
    void listenTo(u16 other);
    /* UNCALLED: nothing calls it. */
    void subscribe();
    void unsubscribe();
    /* UNCALLED: nothing calls it. */
    void listenMouse();
    void ignoreMouse();
    void listenKeyboard();
    void ignoreKeyboard();
    void hold();
    /* UNCALLED: nothing calls it. */
    void unhold();
    void startTimer(u8 run) const;
    void stopTimer() const;
    void pause();
    /* UNCALLED: nothing calls it. */
    void unpause();

    virtual void run() = 0;
    /* The C++ destructor runs when the kernel frees the process's storage
     * (freeProcess): the members'. */
    virtual ~Process() = default;
    /* What the original's destructor does when the kernel kills the process
     * (destroyProcess): each class's own work, as its destructor's body.
     * The object lives on, flags and all, until the kernel frees it. */
    virtual void onKill() {}
    virtual void terminate(u32 value);
    virtual void fail(u32 value);
    virtual void receive([[maybe_unused]] u16 message, [[maybe_unused]] u16 from, [[maybe_unused]] void *data) {}
    virtual void cleanup();
    virtual void load(File *file);
    virtual void save(File *file);
    virtual void afterLoad() {}
};

/* What an event carries (EventProcess::event, poll): a number, or a pointer
 * to what the sender keeps for the call (a key, a mouse event). The
 * original has a u32 for both, a far pointer being 32 bits; the port's
 * pointers are wider. Each comes out as it went in. */
class EventData {
    std::uintptr_t value_;

public:
    // NOLINTNEXTLINE(google-explicit-constructor): an event's data is a number or a pointer, as the original's
    EventData(u32 number = 0) : value_(number) {}
    // NOLINTNEXTLINE(google-explicit-constructor): the same
    template <class T> EventData(T *pointer) : value_(reinterpret_cast<std::uintptr_t>(pointer)) {}
    [[nodiscard]] u32 number() const { return static_cast<u32>(value_); }
    template <class T> [[nodiscard]] T *pointer() const { return reinterpret_cast<T *>(value_); }
};

class EventProcess : public Process {
public:
    explicit EventProcess(Loading loading) : Process(loading) {}
    u32 eventMask;

    EventProcess();
    /* As Process::addFlags. */
    void addEvents(u32 bits)
    {
        const u32 value = eventMask | bits;
        eventMask = value;
    }
    void run() override {}
    virtual bool poll([[maybe_unused]] u32 event, [[maybe_unused]] EventData *data) { return false; }
    virtual void event([[maybe_unused]] u32 event, [[maybe_unused]] EventData data) {}
};

class PeriodicProcess : public EventProcess {
public:
    explicit PeriodicProcess(Loading loading) : EventProcess(loading) {}
    u16 rate;
    u32 period;
    u32 elapsed;

    PeriodicProcess();
    void setRate(u16 hertz);
    void setPeriod(u32 microseconds);
    void tick();
};
ASSERT_SIZE(Process, 0x32);
ASSERT_SIZE(EventProcess, 0x36);
ASSERT_SIZE(PeriodicProcess, 0x40);

/* The TASM module after STDINT.C (1208:0506): destroys the process of the
 * calling function's frame through its virtual destructor; the port calls
 * its onKill. */
extern "C" void destroyProcess(Process *process);
#endif
