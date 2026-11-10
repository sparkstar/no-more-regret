/* ..\CACHE\FLEXHAND.C (Code16, 1078:0000-02d0): a cache handler for the
 * entries of a FLEX archive. Entry i of the archive lives in the handle table
 * at firstHandle+i and is read when it is not in the cache. The GUI fonts
 * (handler 1) and the gump shapes (handler 3) keep their archives in one
 * each. */
#ifndef PORT_FLEXHAND_H
#define PORT_FLEXHAND_H
#include "common.h"
#include "cachehnd.h"
#include "fastflex.h"
#include "shape.h"

class FlexCacheHandler : public CacheHandler {
public:
    u16 firstHandle;
    FastFlex *archive;

    /* Takes entries handles and opens the archive at path. */
    FlexCacheHandler(u16 entries, u16 handlerId, char *path); /* 1078:0000 */
    ~FlexCacheHandler() override;                             /* 1078:00ed */
    /* Entry handle of the archive (its slot is firstHandle+handle), read into
     * the cache when it is not there. */
    [[nodiscard]] ShapeHeader *loadEntry(u16 handle) const;         /* 1078:0159 */
    [[nodiscard]] ShapeFrame *getFrame(u16 index, u16 frame) const; /* 1078:026a */
    [[nodiscard]] u16 frameCount(u16 index) const { return loadEntry(index)->frameCount; }
};
ASSERT_SIZE(FlexCacheHandler, 0xa);
#endif
