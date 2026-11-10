/* ANIMFILE.C (Code49, 1180): reading and writing anim.dat. */
#include <cstdio>
#include "gamestate.h"
#include <cstring>
#include "animfile.h"
#include "filespec.h"
#include "files.h"

/* Nothing reads it (1480:34c0). */
char (&animDatName)[9] = gameGlobal<char[9]>("anim.dat");

/* UNCALLED: nothing calls it. */
AnimFile::AnimFile(const char *filePath) : File(filePath, FILE_READWRITE | FILE_SHARED, 0)
{
    if (strlen(filePath) > 0x50) {
        halt();
    }
    strcpy(this->path, filePath);
}

/* UNCALLED: nothing calls it. */
void AnimFile::create(const char *filePath)
{
    i16 i;
    s32 empty = 0;
    File::create(filePath);
    setPath(filePath);
    for (i = 0; i < ANIM_SHAPES; i++) {
        write(&empty, 4, -1);
    }
    close();
}

/* UNCALLED: nothing calls it. */
bool AnimFile::readAction(u16 shape, u16 anim, AnimAction *action)
{
    ActorAnim table;
    i16 directions;
    if (!readTable(shape, &table)) {
        return false;
    }
    if ((static_cast<u8>(table.offsets[anim] != 0)) == 0U) {
        return false;
    }
    read(&action->info, 4, table.offsets[anim]);
    action->allocate();
    if (action->info.sixteenDirections) {
        directions = 16;
    } else {
        directions = 8;
    }
    read(action->frameList, static_cast<ui16>(static_cast<u8>(action->info.frames) * sizeof(AnimFrame) * directions),
         -1);
    return true;
}

/* UNCALLED: nothing calls it. */
void AnimFile::writeAction(u16 shape, u16 anim, AnimAction *action)
{
    ActorAnim table;
    s32 end;
    i16 directions;
    if (!readTable(shape, &table)) {
        addTable(shape);
    }
    if (action != nullptr) {
        if (action->frameList == nullptr) {
            halt();
        }
        end = size();
        if (action->info.sixteenDirections) {
            directions = 16;
        } else {
            directions = 8;
        }
        table.setOffset(anim, end);
        writeTable(shape, &table);
        write(&action->info, 4, end);
        write(action->frameList,
              static_cast<ui16>(static_cast<u8>(action->info.frames) * sizeof(AnimFrame) * directions), -1);
    } else {
        table.setOffset(anim, 0);
        writeTable(shape, &table);
    }
}

/* UNCALLED: nothing calls it. */
s32 AnimFile::compact()
{
    i16 shape;
    i16 anim;
    AnimFile temp;
    ActorAnim table;
    AnimAction action;
    s32 offset;
    s32 result;
    FileSpec spec(path, 0);
    strcpy(spec.name, "anim_opt");
    open(path, FILE_READWRITE | FILE_SHARED, 0);
    temp.create(makePath(spec.drive, spec.dir, spec.name, spec.ext));
    temp.open(makePath(spec.drive, spec.dir, spec.name, spec.ext), FILE_READWRITE, 0);
    for (shape = 0; shape < ANIM_SHAPES; shape++) {
        if (readTable(shape, &table) && table.any()) {
            temp.writeTable(shape, &table);
            for (anim = 0; anim < ANIM_ACTIONS; anim++) {
                if ((static_cast<u8>(table.offsets[anim] != 0)) != 0U) {
                    offset = table.offsets[anim];
                    read(&action.info, 4, offset);
                    if ((static_cast<u8>(action.info.frames)) != 0U) {
                        action.allocate();
                        read(action.frameList, action.size(), -1);
                        temp.writeAction(shape, anim, &action);
                    } else {
                        temp.writeAction(shape, anim, nullptr);
                    }
                }
            }
        }
    }
    result = temp.size();
    copyAll(&temp);
    commitSize();
    temp.close();
    fileRemove(temp.path);
    close();
    return result;
}

bool AnimFile::readTable(u16 shape, ActorAnim *table)
{
    s32 offset;
    read(&offset, 4, static_cast<ui16>(shape * sizeof(s32)));
    if (offset != 0) {
        read(table->offsets, sizeof(table->offsets), offset);
        return true;
    }
    return false;
}

/* UNCALLED: nothing calls it. */
bool AnimFile::writeTable(u16 shape, ActorAnim *table)
{
    s32 offset;
    read(&offset, 4, static_cast<ui16>(shape * sizeof(s32)));
    if (offset != 0) {
        write(table->offsets, sizeof(table->offsets), offset);
        return true;
    }
    return false;
}

/* UNCALLED: nothing calls it. */
void AnimFile::addTable(u16 shape)
{
    s32 end = size();
    ActorAnim table;
    const s32 position = static_cast<ui16>(shape * sizeof(s32));
    write(&end, 4, position);
    write(table.offsets, sizeof(table.offsets), end);
}

/* UNCALLED: nothing calls it. */
void AnimFile::removeTable(u16 shape)
{
    s32 empty = 0;
    const s32 position = static_cast<ui16>(shape * sizeof(s32));
    write(&empty, 4, position);
}

/* UNCALLED: nothing calls it. */
void AnimFile::setPath(const char *newPath)
{
    if (strlen(newPath) > 0x50) {
        halt();
    }
    strcpy(this->path, newPath);
    setName(this->path);
}
