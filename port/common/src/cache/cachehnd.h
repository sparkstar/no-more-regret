/* 1078:02d1-0318, file name unknown: CacheHandler, the owner of some of the
 * cache's blocks, registered under a number (the handler in their headers).
 * Its members are out of line in a module of their own between FLEXHAND.C
 * and HANDLER.C: the derived destructors call ~CacheHandler far. */
#ifndef PORT_CACHEHND_H
#define PORT_CACHEHND_H
#include "common.h"

class CacheHandler {
public:
    u16 id;

    explicit CacheHandler(u16 number) { id = number; }
    /* Unregisters the handler (1078:02e2). Virtual in the port, not in the original. */
    virtual ~CacheHandler();
    /* The block of data moved there (1078:02d1). */
    virtual void relocated(u32 data);
    /* The block of data, in slot handle, was freed (1078:02d6). */
    virtual void evicted(u32 data, u16 handle);
    /* The block's contents are right: checkBlocks asks (1078:02db). */
    virtual bool check(u32 block);
};
ASSERT_SIZE(CacheHandler, 4);
#endif
