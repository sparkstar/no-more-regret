/* GAMEPAL.C (Code8, 1038): the game's palette (gamepal.pal) and the colour
 * tables derived from it (xformpal.dat), and fades to grey. */
#ifndef PORT_GAMEPAL_H
#define PORT_GAMEPAL_H
#include "common.h"
#include "kprocess.h"
#include "palette.h"

#include <memory>

class XformFlex;

/* The colour tables of the shape variants of x32code.h, 256 bytes each, read
 * from object 0 of xformpal.dat (1038:01db makes one). */
class PaletteTables {
public:
    /* The number of tables after the first two; at least 7. */
    i16 count;
    /* The file and the tables are its own; remap points into translucent. */
    std::unique_ptr<XformFlex> file;
    std::unique_ptr<u8[]> translucent; /* drawShapeTranslucent */
    u8 *remap;                         /* drawShapeRemapped */

    PaletteTables();
    ~PaletteTables(); /* 1038:0216 */
    /* Opens xformpal.dat and reads the tables (1038:0250). */
    void open();
    void close(); /* 1038:02dc */
    void load();  /* 1038:0348 */
    /* Writes the tables back (1038:040a). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    void save() const;
    /* Empty (1038:0440, 0445). Nothing calls them. */
    /* UNCALLED: nothing calls it. */
    void unknown0440();
    /* UNCALLED: nothing calls it. */
    void unknown0445();
};
ASSERT_SIZE(PaletteTables, 0xe);

/* The game's palette (1480:07ba). */
extern Palette *&gamePalette;
extern PaletteTables *&paletteTables; /* 1480:07be */

/* Reads the palette and the tables (1038:0000) and frees them (00f4). */
void loadPalettes();
/* Fade to the grey of the game palette and back; the pid to wait on
 * (1038:044a, 0547). */
/* UNCALLED: nothing calls it. */
u16 fadeToGreyScale(u16 rate);
/* UNCALLED: nothing calls it. */
u16 fadeFromGreyScale(u16 rate);
/* The grey at once (1038:0605, intrinsic 09D). */
void jumpToGreyScale();
/* Declared in gamepal.cpp in the original; here so that the saved
 * games' schema reaches it. */
/* Puts the resumer's cycling back when the grey fade ends. */
class CycleResumer : public Process {
public:
    explicit CycleResumer(Loading loading) : Process(loading) {}
    /* UNCALLED: nothing calls it. */
    CycleResumer() = default;
    void run() override;
};

/* Nothing calls these, in the original either. */
void freePalettes();
/* UNCALLED: nothing calls it. */
void reloadGamePalette();
/* UNCALLED: nothing calls it. */
void applyGamePalette();
#endif
