/* GUMPSHAP.C (Code104): the gump shape set. Its initializer creates the object
 * and, in that constructor, the FLEX cache that holds the shapes
 * (flexhand.h). */
#ifndef PORT_GUMPSHAP_H
#define PORT_GUMPSHAP_H
#include "flexhand.h"

extern FlexCacheHandler *&gumpShapeCache;
/* Palette indices of the GUI. GUMPSHAP.C keeps two sets; guiColors starts at
 * the second and DISPATCH.C switches to the first. Offsets 6 and 9 are read by
 * other modules. */
struct GuiColors {
    u8 frame; /* outlines, lines and arrows */
    u8 unknown1;
    u8 track; /* the scrollbar track */
    u8 unknown3[2];
    u8 face;      /* panels, lists and buttons */
    u8 highlight; /* the debugger's marked line and chosen watch */
    u8 unknown7;
    u8 selection; /* the selected item or text */
    u8 unknown9;
    u8 unknown0a[22];
};
ASSERT_SIZE(GuiColors, 0x20);
/* The shapes of the gumps. The members never read the object. */
class GumpShapes {
public:
    explicit GumpShapes(char *path);
    ~GumpShapes();
    ShapeFrame *shapeFrame(u16 shape, u16 frame) { return gumpShapeCache->getFrame(shape, frame); }
    /* Draws the frame at (x,y); a mode other than 0 draws it through the
     * palette's remap table. */
    void draw(s16 x, s16 y, u16 shape, u16 frame, u8 mode);
    /* 1 when the frame drawn at (0,0) covers (x,y). */
    bool contains(s16 x, s16 y, u16 shape, u16 frame);
    /* The frame's size as a rectangle at (0,0). */
    Rect dimensions(u16 shape, u16 frame);
    /* UNCALLED: nothing calls it. */
    ShapeFrame *frameAt(s16 x, s16 y, u16 shape, u16 frame);
};
extern GumpShapes *&gumpShapes;
/* A frame of a gump shape and how to draw it. The shape buttons derive from
 * it (a non-polymorphic second base initializes the five bytes before the
 * derived vptr is installed); the computer gump keeps one on the stack. The
 * original name is unknown. */
struct GumpShape {
    u16 frame, shape;
    u8 paintMode;
    explicit GumpShape(u16 id)
    {
        frame = 0;
        shape = id;
        paintMode = 0;
    }
    void setFrame(u16 number) { frame = number; }
    void draw(u16 x, u16 y) const { gumpShapes->draw(x, y, shape, frame, paintMode); }
    [[nodiscard]] Rect dimensions() const { return gumpShapes->dimensions(shape, frame); }
    [[nodiscard]] s16 height() const
    {
        const Rect area = dimensions();
        return RECT_HEIGHT(area);
    }
    /* 1 when the frame drawn at (0,0) covers (x,y) (No Remorse's shape
     * buttons). */
    [[nodiscard]] bool contains(s16 x, s16 y) const { return gumpShapes->contains(x, y, shape, frame); }
};
ASSERT_SIZE(GumpShape, 0x5);
extern GuiColors (&guiColorSets)[2];
/* No Remorse's PLAYER.C reads it far. */
extern GuiColors *&guiColors;
/* Creates and deletes gumpShapes, which opens the archive at path. */
void initGumpShapes(char *path);
void closeGumpShapes();
#endif
