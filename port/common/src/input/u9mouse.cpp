/* U9MOUSE.C (Code122, 13c8:0000-03dc). See u9mouse.h. */
#include "u9mouse.h"
#include "filespec.h"
#include "gamefile.h"
#include "gamestate.h"

MouseCtl *&gameMouse = gameGlobal<MouseCtl *>(nullptr);

namespace {

/* checkRange's box about the centre, which it moves there (the original's
 * static in it, in the arena: platform/gamestate.h), and its half sizes,
 * which the original counted at the first call from the box as it was
 * made. */
constexpr Rect RANGE_BOX(-30, -25, 30, 25);
constexpr i16 HALF_WIDTH = static_cast<i16>(RANGE_BOX.right - RANGE_BOX.left + 1) >> 1;
constexpr i16 HALF_HEIGHT = static_cast<i16>(RANGE_BOX.bottom - RANGE_BOX.top + 1) >> 1;
Rect &rangeBox = gameGlobal<Rect>(RANGE_BOX);

}

U9MouseControlProcess::U9MouseControlProcess() : MouseCtl(makePath(nullptr, staticDirectory, "mouse.shp", nullptr))
{
    if (gameMouse != nullptr) {
        halt();
    }
    gameMouse = this;
    centreX = 320;
    centreY = 200;
}

i16 U9MouseControlProcess::checkRange(s16 x, s16 y)
{
    /* The original's static too, never changed. */
    const Rect activeArea(20, 20, 620, 460);

    rangeBox.moveTo(centreX - HALF_WIDTH, centreY - HALF_HEIGHT);
    if (!activeArea.contains(x, y)) {
        range = 2;
    } else if (!rangeBox.contains(x, y)) {
        range = 2;
    } else {
        range = 1;
    }
    return range;
}

i16 U9MouseControlProcess::frameAt(s16 x, s16 y)
{
    i16 dx;
    i16 dy;

    switch (shape()) {
    case 0:
    case 0x19:
        dx = x - centreX;
        dy = y - centreY;
        checkRange(x, y);
        if (dx == 0) {
            direction = dy < 0 ? 0 : 4;
        } else {
            const i16 slope = static_cast<i16>(dy * 8) / dx;

            if (slope < -2 && slope > -12) {
                direction = dx < 0 ? 5 : 1;
            } else if (slope <= -12 || slope >= 12) {
                direction = dy <= 0 ? 0 : 4;
            } else if (slope < 12 && slope > 2) {
                direction = dy <= 0 ? 7 : 3;
            } else if (slope <= 2 && slope >= -2) {
                direction = dx < 0 ? 6 : 2;
            } else {
                direction = 0;
            }
        }
        direction += static_cast<char>(shape());
        if (shape() == 0) {
            direction += (range - 1) << 3;
        }
        break;
    default:
        direction = shape();
    }
    if (shapeCount() > direction) {
        return direction;
    }
    return shapeCount() - 1;
}

/* UNCALLED: nothing calls it. */
i16 U9MouseControlProcess::arrowDirection()
{
    frameAt(pointerX, pointerY);
    if (shape() == 0) {
        return direction & 7;
    }
    if (shape() == 0x19) {
        return (direction - 0x19) & 7;
    }
    return 0;
}
