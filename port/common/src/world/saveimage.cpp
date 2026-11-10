/* The saved game's translation layer (savefields.h): a field of a
 * process or a global between the port and the original's bytes. The
 * pointers go by their kinds (savefields.h's SavePointer): the
 * bytes hold what the original's would, and reading them the port keeps
 * only what its load uses; the loads make the rest again, as the
 * original's do. */
#include "savefields.h"
#include "gamestate.h"
#include "attack.h"
#include "fade.h"
#include "sublist.h"
#include "inter.h"
#include "uprocess.h"
#include "usemem.h"
#include "host.h"

#include <cstdio>
#include <cstring>

namespace {

/* The far pointers the original's saves hold for what its load sets again
 * (BiosTimer's handler, a tactic, the unit file's and the stack's
 * selectors) and SubscriberList's vtable are the game's (savefields.h). */
/* The selectors the port writes where the original's load finds the
 * memory again by other fields: a usecode process's stack by the process,
 * its code by class, function and offset. The original's are its heap's,
 * one an allocation, other each run; the port's are local descriptors
 * numbered by the pid (the stack's number in the usecode's memory,
 * usemem.h) and by the class (below 0C00h), the same each run. */
const u16 FIRST_CODE = 0x8007;

u32 heapPointer(const void *value)
{
    return (value != nullptr) ? static_cast<u32>(SAVE_HEAP_SELECTOR) << 16 : 0;
}

u32 stackPointer(const UsecodeProcess *usecode, u16 offset)
{
    return static_cast<u32>(UsecodeMemory::stackOf(usecode->pid)) << 16 | offset;
}

}

u16 &savePending = gameGlobal<u16>(0);

SaveImage::SaveImage(Direction direction, u8 *bytes, u16 size, Process *process)
    : direction_(direction), bytes_(bytes), size_(size), process_(process)
{
}

void SaveImage::move(u16 offset, u16 size, u32 &word)
{
    if (offset + size > size_) {
        char text[80];
        std::snprintf(text, sizeof(text), "a field at %Xh of %u bytes past a saved object of %Xh", offset, size, size_);
        hostUnimplemented(text);
    }
    for (u16 i = 0; i < size; i++) {
        if (direction_ == WRITE) {
            bytes_[offset + i] = static_cast<u8>(word >> 8 * i);
        } else if (i == 0) {
            word = bytes_[offset];
        } else {
            word |= static_cast<u32>(bytes_[offset + i]) << 8 * i;
        }
    }
}

void SaveImage::bits(u16 offset, u8 shift, u8 width, u32 &value)
{
    const u16 size = static_cast<u16>((shift + width + 7) / 8);
    const u32 mask = (width == 32 ? 0xffffffffU : (1U << width) - 1) << shift;
    u32 word = 0;
    const Direction direction = direction_;
    direction_ = READ;
    move(offset, size, word);
    direction_ = direction;
    if (direction_ == READ) {
        value = (word & mask) >> shift;
    } else {
        word = (word & ~mask) | (value << shift & mask);
        move(offset, size, word);
    }
}

void SaveImage::vtable(u16 offset, u16 value)
{
    u16 word = value;
    integer(offset, word);
    if (direction_ == READ && word != value) {
        wrongVtable_ = true;
    }
}

u32 SaveImage::farPointer(u16 offset)
{
    u32 value = 0;
    const Direction direction = direction_;
    direction_ = READ;
    move(offset, 4, value);
    direction_ = direction;
    return value;
}

void SaveImage::setFarPointer(u16 offset, u32 value)
{
    const Direction direction = direction_;
    direction_ = WRITE;
    move(offset, 4, value);
    direction_ = direction;
}

void SaveImage::notePointer(u16 offset, SavePointer kind)
{
    if ((pointers_ != nullptr) && pointers_->count < SavePointers::MOST) {
        pointers_->offsets[pointers_->count] = offset;
        pointers_->kinds[pointers_->count] = kind;
        pointers_->count++;
    }
}

void SaveImage::pointer(u16 offset, SavePointer kind, void *&value)
{
    notePointer(offset, kind);
    UsecodeProcess *usecode = static_cast<UsecodeProcess *>(process_);
    if (direction_ == WRITE) {
        u32 far = 0;
        switch (kind) {
        case SAVE_LIST:
        case SAVE_FADE:
        case SAVE_FLIC:
        case SAVE_USECODE_CALLS:
            far = heapPointer(value);
            break;
        case SAVE_TRANSIENT:
            break;
        case SAVE_TACTIC:
            if (value != nullptr) {
                far = static_cast<u32>(ORIGINAL_FIRST_TACTIC + (8 * (static_cast<Attack *>(process_))->tacticNumber))
                      << 16;
            }
            break;
        case SAVE_BIOS_VECTOR:
            far = ORIGINAL_BIOS_VECTOR;
            break;
        case SAVE_USECODE_TOP:
        case SAVE_USECODE_END:
            /* Its place in the stack, as the usecode's pointers into it have it
             * (usemem.h). */
            far = (value != nullptr)
                      ? stackPointer(usecode,
                                     static_cast<u16>(USECODE_DATA + (static_cast<u8 *>(value) - usecode->thread.data)))
                      : 0;
            break;
        case SAVE_USECODE_IP:
            far = (value != nullptr) ? static_cast<u32>(static_cast<u16>(FIRST_CODE + (8 * usecode->classId))) << 16 |
                                           static_cast<u16>(usecodeMemory.codePointer(static_cast<const u8 *>(value)))
                                     : 0;
            break;
        case SAVE_USECODE_RESULT:
            /* The interpreter's local, on the original's stack. */
            far = (value != nullptr) ? static_cast<u32>(ORIGINAL_STACK) << 16 : 0;
            break;
        case SAVE_USECODE_PROCESS:
            far = stackPointer(usecode, 0);
            break;
        case SAVE_USECODE_CLASS:
            far = (value != nullptr) ? static_cast<u32>(ORIGINAL_UNIT_FILE) << 16 : 0;
            break;
        }
        setFarPointer(offset, far);
        return;
    }
    const u32 far = farPointer(offset);
    switch (kind) {
    case SAVE_LIST:
        value = (far != 0U) ? &savePending : nullptr;
        break;
    case SAVE_FLIC:
        if (far != 0U) {
            hostUnimplemented("a saved game with a flic playing (the original cannot go on with it either)");
        }
        value = nullptr;
        break;
    case SAVE_USECODE_TOP: {
        /* The original's load keeps the depth, the offset from the
         * stack's start. */
        const u16 at = static_cast<u16>(far);
        value = usecode->thread.data +
                (at >= USECODE_DATA && at - USECODE_DATA <= static_cast<u16>(sizeof(usecode->thread.data))
                     ? at - USECODE_DATA
                     : 0);
        break;
    }
    case SAVE_USECODE_END:
        value = usecode->thread.data + sizeof(usecode->thread.data);
        break;
    case SAVE_USECODE_PROCESS:
        value = usecode;
        break;
    case SAVE_TACTIC:
    case SAVE_FADE:
        value = (far != 0U) ? &savePending : nullptr;
        break;
    default:
        /* The load makes it again: BiosTimer's vectors (loadKernel), the
         * usecode's call stack, code, class name and result (its load). */
        value = nullptr;
        break;
    }
}

void saveLoaded(Process *process)
{
    if (Attack *attack = dynamic_cast<Attack *>(process)) {
        if (static_cast<void *>(attack->tactic) == &savePending) {
            attack->tacticLoaded();
        }
    }
    if (FadeProcess *fade = dynamic_cast<FadeProcess *>(process)) {
        if (static_cast<void *>(fade->current) == &savePending || static_cast<void *>(fade->target) == &savePending) {
            fade->current = fade->target = nullptr;
            fade->eventMask &= ~EVENT_TIMER;
            fade->flags &= ~PROCESS_ACTIVE;
        }
    }
}

const char *saveNameAt(u16 offset)
{
    for (u16 i = 0; i < SAVE_NAME_COUNT; i++) {
        if (SAVE_NAMES[i].offset == offset) {
            return SAVE_NAMES[i].name;
        }
    }
    return "";
}

u16 saveNameOffset(const char *name)
{
    for (u16 i = 0; (name != nullptr) && i < SAVE_NAME_COUNT; i++) {
        if (std::strcmp(SAVE_NAMES[i].name, name) == 0) {
            return SAVE_NAMES[i].offset;
        }
    }
    return 0;
}

const SaveClass *saveClassOf(Process *process)
{
    for (u16 i = 0; i < SAVE_CLASS_COUNT; i++) {
        if (*SAVE_CLASSES[i].type == typeid(*process)) {
            return &SAVE_CLASSES[i];
        }
    }
    return nullptr;
}

const SaveClass *saveClassOfVtable(u16 vtable)
{
    for (u16 i = 0; i < SAVE_CLASS_COUNT; i++) {
        if (SAVE_CLASSES[i].vtable == vtable) {
            return &SAVE_CLASSES[i];
        }
    }
    return nullptr;
}

void saveGlobals(SaveImage &image, const SaveGlobal *globals, u16 count)
{
    u16 at = 0;
    for (u16 i = 0; i < count; i++) {
        const SaveGlobal &global = globals[i];
        void *const place = global.place();
        switch (global.kind) {
        case GLOBAL_AS_IS:
            if (global.size == 1 && global.portSize == 2) {
                /* A byte of the original's that is a word of the port's (No
                 * Remorse's frameSkip, both games' i16 in the port): its
                 * low byte, the high one 0 after a load. */
                u8 low = static_cast<u8>(*static_cast<u16 *>(place));
                image.integer(at, low);
                if (image.direction() == SaveImage::READ) {
                    *static_cast<u16 *>(place) = low;
                }
                break;
            }
            if (global.size != global.portSize) {
                hostUnimplemented("a saved global of another size in the port");
            }
            if (global.size == 1) {
                image.integer(at, *static_cast<u8 *>(place));
            } else if (global.size == 2) {
                image.integer(at, *static_cast<u16 *>(place));
            } else {
                image.integer(at, *static_cast<u32 *>(place));
            }
            break;
        case GLOBAL_TABLE: {
            /* The load allocates the tables again (allocateTables). */
            void *table = *static_cast<void **>(place);
            if (image.direction() == SaveImage::WRITE) {
                image.setFarPointer(at, heapPointer(table));
            }
            break;
        }
        case GLOBAL_LIST: {
            SubscriberList &list = *static_cast<SubscriberList *>(place);
            image.vtable(at, SUBSCRIBER_LIST_VTABLE);
            image.integer(static_cast<u16>(at + 2), list.count);
            image.integer(static_cast<u16>(at + 4), list.tag);
            image.pointer(static_cast<u16>(at + 6), SAVE_LIST, list.data);
            image.integer(static_cast<u16>(at + 10), list.stop);
            image.integer(static_cast<u16>(at + 11), list.index);
            image.integer(static_cast<u16>(at + 13), list.total);
            break;
        }
        }
        at = static_cast<u16>(at + global.size);
    }
}
