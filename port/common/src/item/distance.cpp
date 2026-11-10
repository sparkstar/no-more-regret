/* Code29 (10e0:0000-057a, file name unknown): directions and distances on
 * the map. */
#include <cstdlib>
#include "gamestate.h"
#include "distance.h"
#include "itemcach.h"
#include "pfloat.h"

/* The direction of a step of -1 to 1 in x and y: [dx+1][dy+1] (0-7, the 8
 * directions counted by one). */
char (&stepDirections)[3][3] = gameGlobal<char[3][3]>({{7, 6, 5}, {0, NO_DIRECTION, 4}, {1, 2, 3}});
/* The step of each of the 8 directions (1480:1677-1698). */
char (&directionX)[17] = gameGlobal<char[17]>({0, 1, 1, 1, 0, -1, -1, -1});
char (&directionY)[17] = gameGlobal<char[17]>({-1, -1, 0, 1, 1, 1, 0, -1});
char (&directionX16)[17] = gameGlobal<char[17]>({0, 1, 1, 2, 1, 2, 1, 1, 0, -1, -1, -2, -1, -2, -1, -1, 0});
char (&directionY16)[17] = gameGlobal<char[17]>({-1, -2, -1, -1, 0, 1, 1, 2, 1, 2, 1, 1, 0, -1, -1, -2, 0});

i16 closestDirection(i16 x1, i16 y1, i16 x2, i16 y2, i16 directions, i16 low, i16 high)
{
    i16 result;

    if (directions == 8) {
        result = direction(x1, y1, x2, y2);
    } else {
        result = fineDirection(x1, y1, x2, y2);
    }
    if (result < low || result > high) {
        i16 up = result - low;
        i16 down = low - result;
        if (up < 0) {
            up += directions;
        }
        if (down < 0) {
            down += directions;
        }
        i16 toLow;
        if (up < down) {
            toLow = up;
        } else {
            toLow = down;
        }
        up = result - high;
        down = high - result;
        if (up < 0) {
            up += directions;
        }
        if (down < 0) {
            down += directions;
        }
        i16 toHigh;
        if (up < down) {
            toHigh = up;
        } else {
            toHigh = down;
        }
        if (toLow < toHigh) {
            result = low;
        } else {
            result = high;
        }
    }
    return result;
}

i16 mapDistance(i16 x1, i16 y1, i16 x2, i16 y2)
{
    const i16 dx = abs(static_cast<i16>(x1 - x2));
    const i16 dy = abs(static_cast<i16>(y1 - y2));

    return dx > dy ? dx : dy;
}

i16 mapDistance(i16 x1, i16 y1, u8 z1, i16 x2, i16 y2, u8 z2)
{
    const i16 dx = abs(static_cast<i16>(x1 - x2));
    const i16 dy = abs(static_cast<i16>(y1 - y2));
    const i16 dz = abs(z1 - z2);

    i16 larger = dx > dy ? dx : dy;

    larger = larger > dz ? larger : dz;
    return larger;
}

char direction(i16 x1, i16 y1, i16 x2, i16 y2)
{
    const i16 dx = x1 - x2;
    const i16 dy = y1 - y2;
    const i16 absX = abs(dx);
    const i16 absY = abs(dy);

    if (dx == 0 && dy == 0) {
        return NO_DIRECTION;
    }
    if (absX / 2 <= absY && static_cast<i16>(absX * 2) >= absY) {
        if (dx > 0 && dy > 0) {
            return 14;
        }
        if (dx < 0 && dy < 0) {
            return 6;
        }
        if (dx > 0 && dy < 0) {
            return 10;
        }
        return 2;
    }
    if (static_cast<i16>(absX * 2) <= absY) {
        if (dy >= 0) {
            return 0;
        }
        return 8;
    }
    if (dx >= 0) {
        return 12;
    }
    return 4;
}

char fineDirection(i16 x1, i16 y1, i16 x2, i16 y2)
{
    const i16 dx = x2 - x1;
    const i16 dy = y1 - y2;

    if (dx == 0 && dy == 0) {
        return NO_DIRECTION;
    }
    if (dy == 0) {
        if (dx > 0) {
            return 4;
        }
        return 12;
    }
    if (dx == 0) {
        if (dy > 0) {
            return 0;
        }
        return 8;
    }
    const i16 absX = abs(dx);
    const i16 absY = abs(dy);
    const PFloat slope = PFloat(absY, 0) / PFloat(absX, 0);
    const PFloat quarter = PFloat(1, 0) / PFloat(4, 0);
    const PFloat threeQuarters = PFloat(3, 0) / PFloat(4, 0);
    const PFloat fourThirds = PFloat(4, 0) / PFloat(3, 0);
    const PFloat four(4, 0);
    if (dy > 0 && dx > 0) {
        if ((slope > four) != 0U) {
            return 0;
        }
        if ((slope > fourThirds) != 0U) {
            return 1;
        }
        if ((slope > threeQuarters) != 0U) {
            return 2;
        }
        if ((slope > quarter) != 0U) {
            return 3;
        }
        return 4;
    }
    if (dy > 0 && dx < 0) {
        if ((slope > four) != 0U) {
            return 0;
        }
        if ((slope > fourThirds) != 0U) {
            return 15;
        }
        if ((slope > threeQuarters) != 0U) {
            return 14;
        }
        if ((slope > quarter) != 0U) {
            return 13;
        }
        return 12;
    }
    if (dy < 0 && dx < 0) {
        if ((slope > four) != 0U) {
            return 8;
        }
        if ((slope > fourThirds) != 0U) {
            return 9;
        }
        if ((slope > threeQuarters) != 0U) {
            return 10;
        }
        if ((slope > quarter) != 0U) {
            return 11;
        }
        return 12;
    }
    if ((slope > four) != 0U) {
        return 8;
    }
    if ((slope > fourThirds) != 0U) {
        return 7;
    }
    if ((slope > threeQuarters) != 0U) {
        return 6;
    }
    if ((slope > quarter) != 0U) {
        return 5;
    }
    return 4;
}

bool inFastArea(ItemPoint point)
{
    const i16 column = point.x >> CHUNK_SHIFT;
    const i16 row = point.y >> CHUNK_SHIFT;

    return fastArea.contains(column, row);
}
