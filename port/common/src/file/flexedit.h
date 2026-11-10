/* 1260:0218-0336, file name unknown: a FastFlex whose
 * objects can be rewritten in place. SHAPHAND.C uses it (vtable 1480:1295)
 * and derives from it (1480:1255). */
#ifndef PORT_FLEXEDIT_H
#define PORT_FLEXEDIT_H
#include "common.h"
#include "fastflex.h"

class EditableFlex : public FastFlex {
public:
    EditableFlex(const char *path, u16 openMode, i16 entryCount) : FastFlex(path, openMode, entryCount) {}
    /* Writes length bytes at offset in the object, which becomes
     * offset+length long; an object that grows moves to the end of the
     * file. */
    virtual void replaceObject(i16 index, void *buffer, s32 length, s32 offset);
};
ASSERT_SIZE(EditableFlex, 0x10);
#endif
