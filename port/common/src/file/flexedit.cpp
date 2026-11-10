/* 1260:0218-0336, file name unknown: rewriting FLEX objects. See flexedit.h.
 * The segment starts with the file handle table's module, which the
 * platform's files stand in for (files.h). */
#include "common.h"
#include "flex.h"
#include "flexedit.h"

void EditableFlex::replaceObject(i16 index, void *buffer, s32 length, s32 offset)
{
    FlexEntry entry;
    s32 end;

    readEntry(index, &entry);
    if (entry.offset == 0) {
        entry.offset = size();
        entry.size = length + offset;
        writeEntry(index, &entry);
    } else if (length + offset > entry.size) {
        end = size();
        copy(entry.offset, end, entry.size, nullptr);
        entry.offset = end;
        entry.size = length + offset;
        writeEntry(index, &entry);
    }
    if (length != 0) {
        write(buffer, length, entry.offset + offset);
    }
    commit();
}
