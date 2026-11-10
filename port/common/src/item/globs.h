/* ..\\ITEM\\GLOB.C (Code22, 10a8): globs, the groups of items kept in
 * static\glob.flx. A glob egg (FAMILY_GLOB_EGG) expands its glob, whose
 * items are placed around the egg's map chunk, and the globs' data is kept
 * in the cache. */
#ifndef PORT_GLOB_H
#define PORT_GLOB_H
#include "common.h"
#include "flex.h"
#include "cachehnd.h"
#include "item.h"

/* The handler number of the globs' cache blocks. */
constexpr int GLOB_HANDLER = 2;
/* The cache slots kept for globs. */
constexpr int GLOB_HANDLES = 0x200;
/* The objects of glob.flx. */
constexpr int GLOBS = 0xc00;
/* The most items a glob may hold (markFrames). */
constexpr int GLOB_ITEMS = 1000;

/* An item of a glob: its place in half steps from the chunk's corner. */
struct __attribute__((packed)) GlobEntry {
    u8 x;
    u8 y;
    u8 z;
    u16 shape;
    u8 frame;
};
ASSERT_FILE_SIZE(GlobEntry, 6);

struct __attribute__((packed)) Glob {
    u16 count;
    GlobEntry entries[1]; /* count entries */
};
ASSERT_FILE_SIZE(Glob, 8);

/* Its destructor is the compiler's (10a8:0845): ~File closes the file. */
class GlobFlex : public Flex {
public:
    GlobFlex(); /* 10a8:0000 */
};

class GlobCacheHandler : public CacheHandler {
public:
    /* Takes the cache slots and opens glob.flx (10a8:005a). */
    GlobCacheHandler();
    ~GlobCacheHandler() override; /* 10a8:00e8 */
    /* The size of glob's object in glob.flx (10a8:0151). */
    s32 globSize(u16 glob);
    /* The glob's data, read into the cache when it is not there
     * (10a8:018d). */
    Glob *globData(u16 glob);
    /* The glob's slot is free again (10a8:0325). */
    void evicted(u32 data, u16 handle) override;
    /* Expands the glob of a glob egg at the egg's place (10a8:0339). */
    i16 expandEgg(Item egg, u8 force);
    /* Makes records of the glob's items around the chunk of x,y, at height
     * z. It creates the items at once when force is set or the chunk is on
     * the screen, else it hands the records to ITEMCACH.C. The number of
     * items (10a8:039c). */
    i16 expandGlob(u16 glob, u16 x, u16 y, u8 z, u8 force);
    /* Marks the frames of glob's items to be preloaded (10a8:06f1). */
    void markFrames(u16 glob);
    /* Frees glob's block when it is in the cache (10a8:077d). */
    /* UNCALLED: nothing calls it. */
    void discard(u16 glob);
    /* The number of items in glob (10a8:07db). */
    /* UNCALLED: nothing calls it. */
    u16 itemCount(u16 glob);
};

extern GlobCacheHandler *&globHandler; /* 1480:1384 */

void createGlobHandler(); /* 10a8:07fd */
void deleteGlobHandler(); /* 10a8:082a */
#endif
