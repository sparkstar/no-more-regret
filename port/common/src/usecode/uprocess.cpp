/* UPROCESS.C (13f8:0000-2159): usecode processes and their classes. See
 * uprocess.h. */
#include "gamestate.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "uprocess.h"
#include "trace.h"
#include "waits.h"
#include "filespec.h"
#include "gamefile.h"
#include "kernel.h"
#include "system.h"
#include "utility.h"
#include "yamm.h"
#include "files.h"
#include "host.h"

#include <tuple>

/* The game's main function (1050:0079): its segment in the map file gives
 * the difference between the map's segments and the program's selectors. */

u16 (&classCounts)[4] = gameGlobal<u16[4]>({0x800, 0x100, 0x100, 0x200});
u8 &usecodeBroken = gameGlobal<u8>(0);
ClassTable *&classes = gameGlobal<ClassTable *>(nullptr);
u8 &usecodeLoaded = gameGlobal<u8>(0);
/* The first class number of shapes, NPCs, eggs and the rest. */
u16 (&classBases)[4] = gameGlobal<u16[4]>();
u32 *&cSymbols = gameGlobal<u32 *>();
u16 &cSymbolCount = gameGlobal<u16>();

CodeBlock::CodeBlock(u16 newClassId, u16 newFunction, u16 size, u32 newOffset)
{
    this->classId = newClassId;
    this->function = newFunction;
    code = std::make_unique_for_overwrite<u8[]>(size);
    if (code == nullptr) {
        allocationFailure();
    }
    usecodeMemory.addCode(code.get(), size);
    memset(code.get(), 0xff, size);
    if ((newOffset & 0xffff) == 0U) {
        halt();
    }
    this->offset = newOffset;
}

/* code frees itself after the body. */
CodeBlock::~CodeBlock()
{
    usecodeMemory.remove(code.get());
}

void CodeList::add(CodeBlock *block)
{
    prepend(block);
}

void CodeList::touch(CodeBlock *block)
{
    GumpLink *link = nullptr;

    while (forward(&link)) {
        if (link == block) {
            lower(link);
            return;
        }
    }
}

/* A separator mangleName did not find: its offset is past every other. */
#define NO_SEPARATOR ((char *)-1L)

namespace {
/* The place of a separator in text, which the original compares as the
 * offsets of far pointers: NO_SEPARATOR's is FFFFh. */
u16 placeOf(const char *separator, const char *text)
{
    return separator == NO_SEPARATOR ? 0xffff : static_cast<u16>(separator - text);
}

/* Turns a C++ prototype from the map file into its mangled name; 0 when the
 * line is not a function. */
bool mangleName(char *name, char *prototype)
{
    char *open;
    char *space;
    char *comma;
    char *close;
    char *end;
    u8 done;

    open = strstr(prototype, "(");
    if (open == nullptr) {
        char *line = strstr(prototype, "\r");
        if (line != nullptr) {
            *line = 0;
            strcpy(name, prototype);
            return true;
        }
        return false;
    }
    *open = 0;
    strcpy(name, prototype);
    prototype = open + 1;
    if (*prototype == ')') {
        return true;
    }
    prototype = open + 1;
    done = 0;
    while (done == 0U) {
        while ((*prototype == 0) || *prototype == ' ') {
            prototype++;
        }
        space = strstr(prototype, " ");
        comma = strstr(prototype, ",");
        close = strstr(prototype, ")");
        if (space == nullptr) {
            space = NO_SEPARATOR;
        }
        if (comma == nullptr) {
            comma = NO_SEPARATOR;
        }
        if (close == nullptr) {
            close = NO_SEPARATOR;
        }
        if (placeOf(space, prototype) < placeOf(comma, prototype) &&
            placeOf(space, prototype) < placeOf(close, prototype)) {
            end = space;
        } else if (placeOf(comma, prototype) < placeOf(space, prototype) &&
                   placeOf(comma, prototype) < placeOf(close, prototype)) {
            end = comma;
        } else {
            if (close == NO_SEPARATOR) {
                return true;
            }
            end = close;
            done++;
        }
        if (((comma != nullptr) && comma != NO_SEPARATOR) || ((close != nullptr) && close != NO_SEPARATOR)) {
            if ((comma == nullptr) || comma == NO_SEPARATOR) {
                comma = close;
            }
            *comma = 0;
            if (strcmp(prototype, "char far*") == 0) {
                prototype += 10;
                strcat(name, "@s");
                continue;
            }
            *comma = ',';
        }
        if (end != nullptr) {
            *end = 0;
        }
        if (strcmp(prototype, "unsigned") == 0) {
            prototype += 8;
        } else if (strcmp(prototype, "int") == 0) {
            prototype += 3;
            strcat(name, "@i");
        } else if (strcmp(prototype, "short") == 0) {
            prototype += 5;
            strcat(name, "@i");
        } else if (strcmp(prototype, "char") == 0) {
            prototype += 4;
            strcat(name, "@i");
        } else if (strcmp(prototype, "long") == 0) {
            prototype += 4;
            strcat(name, "@d");
        } else {
            if ((strstr(prototype, "*") == nullptr) && (strstr(prototype, "&") == nullptr)) {
                strcat(name, "@$");
                strcat(name, prototype);
                strcat(name, "$");
            }
            prototype += strlen(prototype) + 1;
        }
    }
    return true;
}

/* The name with each $...$ group replaced by i. */
void shortName(char *name, const char *mangled)
{
    u16 from = 0;
    u16 to = 0;

    while (mangled[from] != 0) {
        if (mangled[from] != '$') {
            name[to] = mangled[from];
            from++;
            to++;
        } else {
            name[to] = 'i';
            to++;
            from++;
            while (mangled[from++] != '$') {
                ;
            }
        }
    }
    name[to] = 0;
}

void loadCSymbols()
{
    /* The map's segment of main, which the port does not move by. */
    [[maybe_unused]] u16 mainSegment;
    bool useSaved;
    FileSpec offsets(nullptr, usecodeDirectory, "unkcoff", ".dat");
    FileSpec names(nullptr, usecodeDirectory, "unkcsym", ".dat");
    FileSpec map(nullptr, nullptr, gameFiles[GAME_PROGRAM], ".map");

    useSaved = false;
    if ((map.exists() != 0) && (names.exists() != 0)) {
        if ((map.isNewer(&names) != 0) || (names.isNewer(&offsets) != 0)) {
            FILE *mapFile = fileStream(map.path(), "rb");
            if (mapFile == nullptr) {
                openFailure(map.path());
            }
            const File input(names.path(), FILE_READ, 0);
            File output;
            output.open(offsets.path(), FILE_READWRITE, static_cast<u8>(fileExists(offsets.path()) == 0));
            const u32 size = input.size();
            cSymbolCount = size / 74;
            cSymbols = new u32[cSymbolCount];
            memset(cSymbols, 0, cSymbolCount * 4);
            char *records = new char[size];
            input.read(records, size);
            char line[200];
            char full[256];
            /* Nothing looks at what fgets reads (the original's). */
            do {
                std::ignore = fileStreamLine(mapFile, line, sizeof(line));
            } while (strstr(line, "Publics by Name") == nullptr);
            do {
                std::ignore = fileStreamLine(mapFile, line, sizeof(line));
                char *prototype = line + 17;
                const bool parsed = mangleName(formatBuffer, prototype);
                shortName(full, formatBuffer);
                if (strcmp(prototype, "_main") == 0) {
                    line[5] = 0;
                    line[22] = 0;
                    mainSegment = strtol(line + 1, nullptr, 16);
                }
                if (parsed) {
                    u16 i = 0;
                    char *record = records;
                    for (; i < cSymbolCount; i++, record += 74) {
                        if ((strcmp(record, formatBuffer) == 0) || (strcmp(record, full) == 0)) {
                            line[5] = 0;
                            line[10] = 0;
                            const u16 segment = strtol(line + 1, nullptr, 16);
                            const u16 start = strtol(line + 6, nullptr, 16);
                            cSymbols[i] = static_cast<u32>(segment) << 16 | start;
                        }
                    }
                }
            } while (strstr(line, "Publics by Value") == nullptr);
            for (u16 i = 0; i < cSymbolCount; i++) {
                if (cSymbols[i] == 0U) {
                    debugPrintf(0xff, "\r\nUnresolved c symbol %s", records + static_cast<ui16>(i * 74));
                    usecodeBroken = 1;
                }
            }
            delete[] records;
            if (usecodeBroken != 0U) {
                debugPrintf(0xff, "\r\nNo Usecode! (press any key)\r\n\r\n");
                return;
            }
            output.write(cSymbols, static_cast<ui16>(cSymbolCount * 4));
        } else {
            useSaved = true;
        }
    } else {
        useSaved = true;
    }
    if (useSaved) {
        const File saved(offsets.path(), FILE_READ, 0);
        const u16 size = saved.size();
        cSymbolCount = size / 4;
        if (cSymbols != nullptr) {
            delete[] cSymbols;
            cSymbols = nullptr;
        }
        cSymbols = new u32[cSymbolCount];
    }
    /* The functions' far addresses, which the original reads from the map or
     * unkcoff.dat and moves to the program's selectors by main's segment: the
     * port calls them through intrinsics.cpp's switch, and keeps only their
     * number. */
    memset(cSymbols, 0, cSymbolCount * sizeof(u32));
}
}

void resetUsecode()
{
    classes->unloadAll();
    classes->reset();
    freeYamm();
    initYamm();
}

/* UNCALLED: nothing calls it. */
void unusedUsecode() {}

void initUsecode()
{
    initYamm();
    if (usecodeDirectory[0] == 0) {
        return;
    }
    /* The one entry of gameFiles that is not a literal: usecodeFile. */
    const_cast<char *>(gameFiles[GAME_USECODE])[0] = 'e';
    if (fileExists(makePath(nullptr, usecodeDirectory, gameFiles[GAME_USECODE], nullptr)) != 0) {
        classes = new ClassTable(makePath(nullptr, usecodeDirectory, gameFiles[GAME_USECODE], nullptr));
        loadCSymbols();
        usecodeLoaded = 1;
        u16 base = 0;
        for (i16 i = 0; i < 4; i++) {
            classBases[i] = base;
            base += classCounts[i];
        }
    }
}

void closeUsecode()
{
    if (cSymbols != nullptr) {
        delete[] cSymbols;
        cSymbols = nullptr;
    }
    if (classes != nullptr) {
        delete classes;
        classes = nullptr;
    }
    freeYamm();
}

/* UNCALLED: nothing calls it. */
void unusedUsecode2() {}

u16 classOf(Item *item)
{
    u16 classId;

    if (item->isNpc() && ((item->flags() & ITEM_DISPOSABLE) == 0)) {
        classId = item->id + classBases[1];
    } else if (item->family() == FAMILY_EGG) {
        classId = item->eggType() + classBases[2];
    } else {
        classId = item->shape() + classBases[0];
    }
    return classId;
}

bool callEvent(Item *item, u32 events, u16 event, u16 *result, u32 argument)
{
    if (usecodeBroken != 0U) {
        return false;
    }
    const u16 classId = classOf(item);
    if ((classes->unitFile != nullptr) && ((classes->info(classId).events & events) != 0U)) {
        traceEvent(item->id, classId, event);
        UsecodeProcess *process = new UsecodeProcess(nullptr, item, 2, classId, event, 0, &argument, 4);
        if (waitOpened()) {
            /* The event's thread waits: the caller's result after it, as
             * a rest's (its caller's result pointer goes with this
             * call). */
            *result = 0;
            giveRest([process]() {
                setRestResult((process->hasFlags(PROCESS_DEAD) != 0U) ? 0 : process->pid);
                return false;
            });
            return true;
        }
        if (process->hasFlags(PROCESS_DEAD) != 0U) {
            *result = 0;
        } else {
            *result = process->pid;
        }
        return true;
    }
    return false;
}

UsecodeProcess::UsecodeProcess(u32 *resultAddress, void *arguments, u16 size, u16 usecodeClassId, u16 usecodeFunction,
                               u16 offset, void *extra, u16 extraSize)
    : thread(arguments, size, classes->address(usecodeClassId, usecodeFunction, offset), this,
             classes->info(usecodeClassId).name, usecodeClassId, usecodeFunction)
{
    nameProcess(pid, classes->info(usecodeClassId).name);
    traceUsecodeStart(pid, usecodeClassId, usecodeFunction);
    finished = 0;
    argumentSize = size;
    localsOffset = 0;
    this->classId = usecodeClassId;
    this->function = usecodeFunction;
    resultPointer = resultAddress;
    if (extra != nullptr) {
        u8 *base = thread.top;

        memcpy(thread.reserve(extraSize), extra, extraSize);
        if (size != 0U) {
            thread.pushLong(static_cast<s32>(thread.pointerTo(base)));
            localsOffset = thread.top - thread.data;
        }
    }
    run();
}

void UsecodeProcess::releaseCalls()
{
    delete thread.calls;
}

void UsecodeProcess::run()
{
    running = 1;
    const i16 state = thread.run(&result);
    if (waitOpened()) {
        /* Its thread waits in a call: the state it stops in after the wait. */
        giveRest([this]() {
            runEnd(static_cast<i16>(restResult()));
            return false;
        });
        return;
    }
    runEnd(state);
}

void UsecodeProcess::runEnd(i16 state)
{
    if (state == THREAD_FINISHED) {
        if (resultPointer != nullptr) {
            *resultPointer = result;
        }
        finished = 1;
        running = 0;
        terminate(result);
        return;
    }
    if (state != THREAD_DEAD) {
        running = 0;
    }
}

void UsecodeProcess::onKill()
{
    thread.release();
}

void UsecodeProcess::cleanup()
{
    /* The kernel frees some processes without their destructor. */
    usecodeMemory.remove(thread.data);
    Process::cleanup();
    if (finished == 0U) {
        yammFreeOwner(pid);
    }
}

void UsecodeProcess::save(File *file)
{
    Process::save(file);
    file->write(&localsOffset, 2);
    file->write(&argumentSize, 1);
    /* The offset in the code block: the usecode's pointer's. */
    u16 ip = static_cast<u16>(usecodeMemory.codePointer(thread.ip));
    file->write(&ip, 2);
    file->write(&function, 2);
    file->write(thread.calls, sizeof(thread.calls->values));
    file->write(&thread.calls->count, 2);
}

void UsecodeProcess::load(File *file)
{
    Process::load(file);
    const u16 depth = thread.top - thread.data;
    file->read(&localsOffset, 2);
    file->read(&argumentSize, 1);
    resultPointer = nullptr;
    /* The stack's region, whose number comes from the pid the save has
     * given, and the arguments' pointer into it. */
    usecodeMemory.addStack(pid, thread.data, sizeof(thread.data));
    if (localsOffset != 0U) {
        *reinterpret_cast<u32 *>(thread.data + localsOffset) =
            usecodeMemory.stackPointer(pid, thread.data + static_cast<ui16>(sizeof(thread.data) - argumentSize));
    }
    thread.top = thread.data + depth;
    thread.end = thread.data + sizeof(thread.data);
    u16 ip;
    file->read(&ip, 2);
    thread.calls = new CallStack;
    file->read(&function, 2);
    file->read(thread.calls, sizeof(thread.calls->values));
    file->read(&thread.calls->count, 2);
    thread.ip = classes->address(classId, function, ip);
    thread.process = this;
    thread.className = classes->info(classId).name;
}

ClassTable::ClassTable(const char *path)
{
    reset();
    unitFile = new UnitFile(path);
}

void ClassTable::reset()
{
    memset(static_cast<void *>(classes), 0, sizeof(classes));
    loaded = 0;
    limit = CODE_LIMIT;
    free = -1;
    verbose = 0;
}

void ClassTable::unloadAll()
{
    for (i16 i = 0; i < CLASSES; i++) {
        if (classes[i].functions != nullptr) {
            if (classes[i].functions != nullptr) {
                delete[] classes[i].functions;
                classes[i].functions = nullptr;
            }
            if (classes[i].uses != nullptr) {
                delete[] classes[i].uses;
                classes[i].uses = nullptr;
            }
            delete[] classes[i].loadedBlocks;
            classes[i].loadedBlocks = nullptr;
        }
    }
    blocks.clear();
}

ClassTable::~ClassTable()
{
    unloadAll();
    delete unitFile;
}

ClassEntry *ClassTable::find(u16 unit, ClassEntry **slot)
{
    ClassEntry *entry = classes;
    u16 i = 0;
    /* Its 0 makes no code, but it keeps BCC from seeing that the first test
     * passes: the original jumps to the test first. */
    bool used = false;

    for (; i < CLASSES; i++, entry++) {
        if (static_cast<ui16>(entry->unit) == unit) {
            return entry;
        }
        if (entry->unit == 0) {
            if (*slot == nullptr) {
                *slot = entry;
            }
        } else {
            used = false;
            if (free == -1) {
                for (u16 j = 0; (!used) && j < entry->header.functions + 32; j++) {
                    if (entry->uses[j] > 0) {
                        used = true;
                    } else if (entry->uses[j] < -1) {
                        halt();
                    }
                }
            }
            if (!used && free == -1) {
                free = i;
            }
        }
    }
    if (*slot == nullptr) {
        if (free == -1) {
            halt();
        }
        if (unloadCode(classes[free].unit, -1) == 0U) {
            halt();
        }
        *slot = &classes[free];
        if ((*slot)->functions != nullptr) {
            delete[] (*slot)->functions;
            (*slot)->functions = nullptr;
        }
        if ((*slot)->uses != nullptr) {
            delete[] (*slot)->uses;
            (*slot)->uses = nullptr;
        }
        delete[] (*slot)->loadedBlocks;
        (*slot)->loadedBlocks = nullptr;
        free = -1;
    }
    return nullptr;
}

void ClassTable::load(u16 unit, ClassEntry *entry) const
{
    FlexEntry place;

    unitFile->readUnitEntry(unit, &place);
    unitFile->readUnit(unit, &entry->header, sizeof(ClassHeader), 0L);
    entry->unit = unit;
    entry->place = place;
    if (entry->functions != nullptr) {
        halt();
    }
    if (entry->uses != nullptr) {
        halt();
    }
    entry->functions = new FunctionEntry[entry->header.functions + 32];
    entry->uses = new i16[entry->header.functions + 32];
    entry->loadedBlocks = new CodeBlock *[entry->header.functions + 32]();
    for (u16 i = 0; i < entry->header.functions + 32; i++) {
        entry->uses[i] = -1;
    }
    unitFile->readUnit(unit, entry->functions,
                       (entry->header.functions + 32) * static_cast<ui16>(sizeof(FunctionEntry)), sizeof(ClassHeader));
}

u8 *ClassTable::code(u16 classId, u16 function)
{
    ClassEntry *slot = nullptr;
    ClassEntry *entry = find(classId, &slot);
    CodeBlock *block;

    if (entry == nullptr) {
        load(classId, slot);
        entry = slot;
    }
    if (entry->uses[function] == -1) {
        const u32 offset = entry->functions[function].offset;
        if ((offset & 0xffff) == 0U) {
            halt();
        }
        const u32 size = entry->functions[function].size;
        u16 room = 0;
        while (loaded + size > limit + room) {
            if (unloadCode(-1, -1) == 0U) {
                room += size;
            }
        }
        block = new CodeBlock(classId, function, size, offset);
        loaded += size;
        entry->loadedBlocks[function] = block;
        entry->uses[function] = 1;
        unitFile->readUnit(classId, block->code.get(), size, offset + entry->header.codeBase - 1);
        blocks.add(block);
    } else if (entry->uses[function] >= 0) {
        block = entry->loadedBlocks[function];
        blocks.touch(block);
        entry->uses[function]++;
        if (entry->uses[function] >= 30000) {
            debugPrintf(0xff, "InUseCounter:%d\r", entry->uses[function]);
            halt();
        }
    } else {
        halt();
    }
    const i16 index = entry - classes;
    if (free == index) {
        free = -1;
    }
    printStats();
    return block->code.get();
}

void ClassTable::printStats()
{
    if (verbose == 0U) {
        return;
    }
    CodeBlock *block = nullptr;
    hostTextAt(1, 1);
    /* %ld: BCC's long, 32 bits. */
    debugPrintf(50, "Mem: %-8ld\r\n", static_cast<long>(static_cast<s32>(loaded)));
    while (blocks.forward(reinterpret_cast<GumpLink **>(&block))) {
        debugPrintf(50, "%9s(%-4d)  r%-2d\r\n", info(block->classId).name, block->classId, block->function);
    }
}

u8 *ClassTable::address(u16 classId, u16 function, u32 offset)
{
    return code(classId, function) + offset;
}

void ClassTable::release(u16 classId, u16 function)
{
    ClassEntry *slot = nullptr;
    ClassEntry *entry = find(classId, &slot);

    if (entry == nullptr) {
        hostDebugBreak();
        entry = find(classId, &slot);
    }
    if (entry->uses[function] > 0) {
        entry->uses[function]--;
    } else {
        halt();
    }
}

bool ClassTable::unload(CodeBlock *block)
{
    ClassEntry *entry;
    u16 classId;
    ClassEntry *slot;

    if (block == nullptr) {
        halt();
    }
    classId = block->classId;
    slot = nullptr;
    entry = find(classId, &slot);
    if (entry->uses[block->function] == 0) {
        loaded -= entry->functions[block->function].size;
        if ((block->offset & 0xffff) == 0U) {
            halt();
        }
        entry->functions[block->function].offset = block->offset;
        entry->loadedBlocks[block->function] = nullptr;
        entry->uses[block->function] = -1;
        blocks.destroy(block);
        return true;
    }
    return false;
}

u8 ClassTable::unloadCode(i16 classId, i16 function)
{
    CodeBlock *block;
    u8 unloaded;
    CodeBlock *previous;

    if (classId == -1) {
        block = nullptr;
        while (blocks.backward(reinterpret_cast<GumpLink **>(&block))) {
            if (unload(block)) {
                return 1;
            }
        }
        return 0;
    }
    block = nullptr;
    unloaded = 1;
    while (blocks.forward(reinterpret_cast<GumpLink **>(&block))) {
        if (block->classId == static_cast<ui16>(classId) &&
            (function == -1 || block->function == static_cast<ui16>(function))) {
            previous = static_cast<CodeBlock *>(block->previous);
            unloaded &= static_cast<int>(unload(block));
            block = previous;
            if (function != -1) {
                break;
            }
        }
    }
    return unloaded;
}

/* UNCALLED: nothing calls it. */
void killUsecode(u16 item, i16 type)
{
    killItem(item, type);
}

/* UNCALLED: nothing calls it. */
void moveUsecode(u16 item, i16 type, u16 target)
{
    Process *process = firstProcess(item, type);
    if (process != nullptr) {
        process->setItem(target);
    }
}
