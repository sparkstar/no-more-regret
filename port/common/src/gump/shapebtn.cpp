/* 1370:0000-043d, file name unknown: the shape buttons, with the three vtables
 * and four virtual function copies the compiler emits. See shapebtn.h. */
#include "shapebtn.h"

ShapeButtonGump::ShapeButtonGump(u16 mode, NewGump *parentGump, u16 shapeValue)
    : ButtonGump(static_cast<u8>(mode), parentGump), GumpShape(shapeValue)
{
    bounds = gumpShapes->dimensions(shape, frame);
}

void ShapeButtonGump::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

void ShapeButtonGump::showPressed()
{
    frame = 1;
    paint(0);
}

void ShapeButtonGump::showReleased()
{
    frame = 0;
    paint(0);
}

bool ShapeButtonGump::acceptsEvent(NewGumpEvent *event)
{
    if (!bounds.contains(event->x, event->y)) {
        return false;
    }
    /* Only the frame's drawn pixels, in both games (No Remorse's 1370:0132). */
    const i16 x = event->x - bounds.left;
    const i16 y = event->y - bounds.top;
    return GumpShape::contains(x, y);
}

/* UNCALLED: nothing calls it. */
CyclingShapeButtonGump::CyclingShapeButtonGump(u8 code, NewGump *parentGump, u16 shapeValue)
    : CyclingButtonGump(code, parentGump), GumpShape(shapeValue)
{
    count = gumpShapeCache->frameCount(shape);
    bounds = gumpShapes->dimensions(shape, frame);
}

void CyclingShapeButtonGump::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

void CyclingShapeButtonGump::showPressed()
{
    const u16 nextFrame = static_cast<i16>(current + 3) % static_cast<i16>(count * 2);
    frame = nextFrame;
    paint(0);
    current = nextFrame;
}

/* UNCALLED: nothing calls it. */
AutoRepeatShapeButtonGump::AutoRepeatShapeButtonGump(u8 mode, NewGump *parentGump, u16 shapeValue)
    : AutoRepeatButtonGump(mode, parentGump), GumpShape(shapeValue)
{
    bounds = gumpShapes->dimensions(shape, frame);
}

void AutoRepeatShapeButtonGump::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

void AutoRepeatShapeButtonGump::showPressed()
{
    frame = 1;
    paint(0);
}

void AutoRepeatShapeButtonGump::showReleased()
{
    frame = 0;
    paint(0);
}
