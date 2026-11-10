/* GUMPSHAP.C (1338:0000-02ec). See gumpshap.h. */
#include "gumpshap.h"
#include "gamestate.h"
#include "gamepal.h"
#include "hittest.h"

GuiColors (&guiColorSets)[2] =
    gameGlobal<GuiColors[2]>({{0x00, 0x95, 0x18, {0x16, 0x73}, 0x14, 0x42, 0xff, 0x0f, 0x33, {0x58}},
                              {0x00, 0xd7, 0x0b, {0x08, 0x45}, 0x06, 0x97, 0xff, 0x0e, 0x51, {0xfe}}});
GuiColors *&guiColors = gameGlobal<GuiColors *>(&guiColorSets[1]);
GumpShapes *&gumpShapes = gameGlobal<GumpShapes *>(nullptr);
FlexCacheHandler *&gumpShapeCache = gameGlobal<FlexCacheHandler *>(nullptr);

void initGumpShapes(char *path)
{
    if (gumpShapes == nullptr) {
        gumpShapes = new GumpShapes(path);
    }
}

void closeGumpShapes()
{
    if (gumpShapes != nullptr) {
        delete gumpShapes;
        gumpShapes = nullptr;
    }
}

GumpShapes::GumpShapes(char *path)
{
    gumpShapeCache = new FlexCacheHandler(0x100, 3, path);
}

GumpShapes::~GumpShapes()
{
    if (gumpShapeCache != nullptr) {
        delete gumpShapeCache;
        gumpShapeCache = nullptr;
    }
}

void GumpShapes::draw(s16 x, s16 y, u16 shape, u16 frame, u8 mode)
{
    if (mode == 0U) {
        drawShape(x, y, shapeFrame(shape, frame));
    } else {
        drawShapeRemapped(x, y, shapeFrame(shape, frame), paletteTables->remap);
    }
}

bool GumpShapes::contains(s16 x, s16 y, u16 shape, u16 frame)
{
    return frameHit(shapeFrame(shape, frame), Point(0, 0), Point(x, y));
}

Rect GumpShapes::dimensions(u16 shape, u16 frame)
{
    Rect area;
    ShapeFrame *image;

    area.left = 1;
    area.top = 1;
    image = shapeFrame(shape, frame);
    area.right = image->width;
    area.bottom = image->height;
    area.moveTo(0, 0);
    return area;
}

/* Not called. */
/* UNCALLED: nothing calls it. */
ShapeFrame *GumpShapes::frameAt(s16 /*unused*/, s16 /*unused*/, u16 shape, u16 frame)
{
    return shapeFrame(shape, frame);
}
