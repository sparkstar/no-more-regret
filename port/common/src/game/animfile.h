/* ANIMFILE.C (Code49, 1180): anim.dat, the animations of the shapes. The file
 * starts with a table of 800h offsets, one per shape, each to the shape's table
 * of 100h offsets, one per animation, each to the animation's header (AnimInfo)
 * and frames. */
#ifndef PORT_ANIMFILE_H
#define PORT_ANIMFILE_H
#include "common.h"
#include "file.h"
#include "actanim.h"

constexpr int ANIM_SHAPES = 0x800;
constexpr int ANIM_ACTIONS = 0x100;

class AnimFile : public File {
public:
    char path[0x51];

    AnimFile() = default;
    /* Opens path to read and write (1180:0000). */
    /* UNCALLED: nothing calls it. */
    explicit AnimFile(const char *filePath);
    /* Makes path an empty anim.dat and closes it (1180:0091). */
    /* UNCALLED: nothing calls it. */
    void create(const char *filePath);
    /* Reads anim of shape into action; 0 when there is none (1180:0103). */
    /* UNCALLED: nothing calls it. */
    bool readAction(u16 shape, u16 anim, AnimAction *action);
    /* Writes action as anim of shape at the end of the file, or takes anim
     * away for 0 (1180:01eb). */
    /* UNCALLED: nothing calls it. */
    void writeAction(u16 shape, u16 anim, AnimAction *action);
    /* Copies the file without its unused bytes through a temporary
     * anim_opt and returns its new size (1180:0348). */
    /* UNCALLED: nothing calls it. */
    s32 compact();
    /* Reads and writes the table of shape; 0 when the shape has none
     * (1180:0615, 066a). */
    bool readTable(u16 shape, ActorAnim *table);
    /* UNCALLED: nothing calls it. */
    bool writeTable(u16 shape, ActorAnim *table);
    /* Gives shape an empty table at the end of the file (1180:06bf) and
     * takes it away (0732). */
    /* UNCALLED: nothing calls it. */
    void addTable(u16 shape);
    /* UNCALLED: nothing calls it. */
    void removeTable(u16 shape);
    /* UNCALLED: nothing calls it. */
    void setPath(const char *newPath); /* 1180:076d */
};
ASSERT_SIZE(AnimFile, 0x59);
#endif
