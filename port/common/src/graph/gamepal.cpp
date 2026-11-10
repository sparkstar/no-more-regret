/* GAMEPAL.C (Code8, 1038): the game's palette and colour tables, and fades to
 * grey. */
#include "gamepal.h"
#include "gamestate.h"
#include "cycle.h"
#include "fade.h"
#include "flex.h"
#include "filespec.h"
#include "gamefile.h"
#include "kernel.h"

/* Nothing reads it. */
u16 &unknown07b8 = gameGlobal<u16>(0);
Palette *&gamePalette = gameGlobal<Palette *>(nullptr);
PaletteTables *&paletteTables = gameGlobal<PaletteTables *>(nullptr);
namespace {
/* A copy of the palette as loaded. */
Palette *&loadedPalette = gameGlobal<Palette *>(nullptr);
}

/* xformpal.dat. It changes nothing of Flex: the constructor makes the path
 * after new, as the original calls makePath inside the allocation's test. */
class XformFlex : public Flex {
public:
    XformFlex() : Flex(makePath(nullptr, staticDirectory, gameFiles[GAME_XFORMPAL], nullptr), FILE_READWRITE, 1) {}
};

void CycleResumer::run()
{
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 0;
    }
}

void loadPalettes()
{
    char *path = makePath(nullptr, staticDirectory, gameFiles[GAME_PALETTE], nullptr);

    gamePalette = new Palette(path);
    loadedPalette = new Palette(*gamePalette);
    paletteTables = new PaletteTables;
    paletteTables->open();
}

void freePalettes()
{
    if (gamePalette != nullptr) {
        delete gamePalette;
        gamePalette = nullptr;
    }
    if (loadedPalette != nullptr) {
        delete loadedPalette;
        loadedPalette = nullptr;
    }
    if (paletteTables != nullptr) {
        paletteTables->close();
        delete paletteTables;
        paletteTables = nullptr;
    }
}

/* Reads gamepal.pal again and applies it (1038:0190). Nothing calls it. */
/* UNCALLED: nothing calls it. */
void reloadGamePalette()
{
    gamePalette->load(makePath(nullptr, staticDirectory, gameFiles[GAME_PALETTE], nullptr), 0);
    gamePalette->apply();
}

/* 1038:01ca */
/* UNCALLED: nothing calls it. */
void applyGamePalette()
{
    gamePalette->apply();
}

PaletteTables::PaletteTables()
{
    remap = nullptr;
    count = 0;
}

PaletteTables::~PaletteTables()
{
    if (translucent != nullptr) {
        close();
    }
}

void PaletteTables::open()
{
    file = std::make_unique<XformFlex>();
    if (file == nullptr) {
        halt();
    }
    load();
}

void PaletteTables::close()
{
    translucent.reset();
    remap = nullptr;
    file.reset();
}

void PaletteTables::load()
{
    FlexEntry entry;

    file->readEntry(0, &entry);
    translucent.reset();
    count = (entry.size >> 8) - 2;
    if (count < 7) {
        count = 7;
    }
    translucent = std::make_unique_for_overwrite<u8[]>(0x900);
    file->readObject(0, translucent.get(), -1, 0);
    remap = translucent.get() + 0x100;
}

/* UNCALLED: nothing calls it. */
void PaletteTables::save() const
{
    file->writeObject(0, translucent.get(), static_cast<i16>((count + 2) << 8), 0, 1);
}

/* UNCALLED: nothing calls it. */
void PaletteTables::unknown0440() {}

/* UNCALLED: nothing calls it. */
void PaletteTables::unknown0445() {}

/* UNCALLED: nothing calls it. */
u16 fadeToGreyScale(u16 rate)
{
    Palette grey(*gamePalette);
    FadeProcess *fade;
    i16 i;

    grey.toYiq();
    for (i = 0; i < 256; i++) {
        grey.data[i].green = grey.data[i].blue = grey.data[i].red;
    }
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    fade = new FadeProcess(&grey, rate, 0, 0, 1);
    return fade->pid;
}

/* UNCALLED: nothing calls it. */
u16 fadeFromGreyScale(u16 rate)
{
    FadeProcess *fade = new FadeProcess(gamePalette, rate, 1, 0, 1);
    CycleResumer *resumer = new CycleResumer;

    fade->block(resumer);
    return resumer->pid;
}

void jumpToGreyScale()
{
    Palette grey(*gamePalette);
    i16 i;

    grey.toYiq();
    for (i = 0; i < 256; i++) {
        grey.data[i].green = grey.data[i].blue = grey.data[i].red;
    }
    if (cycleProcess != nullptr) {
        cycleProcess->paused = 1;
    }
    grey.apply();
}
