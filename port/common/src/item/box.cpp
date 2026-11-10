/* Code44 (1158:0000-01c7, file name unknown): the members of a box between
 * two map points. */
#include "box.h"

/* UNCALLED: nothing calls it. */
void Box::set(i16 newX1, i16 newY1, u8 newZ1, i16 newX2, i16 newY2, u8 newZ2)
{
    this->x1 = newX1;
    this->y1 = newY1;
    this->z1 = newZ1;
    this->x2 = newX2;
    this->y2 = newY2;
    this->z2 = newZ2;
}

/* The second test only passes when the first does: a box inside this one
 * has its corner inside it too. */
/* UNCALLED: nothing calls it. */
bool Box::overlaps(Box *box) const
{
    if (((x1 >= box->x1 && box->x2 >= x1) || (box->x1 >= x1 && x2 >= box->x1)) &&
        ((y1 >= box->y1 && y1 <= box->y2) || (box->y1 >= y1 && box->y1 <= y2)) &&
        ((z1 >= box->z1 && z1 <= box->z2) || (box->z1 >= z1 && box->z1 <= z2))) {
        return true;
    }
    if (((x1 >= box->x1 && box->x2 >= x1) || (box->x1 >= x1 && box->x2 <= x2)) &&
        ((y1 >= box->y1 && y1 <= box->y2) || (box->y1 >= y1 && box->y2 <= y2)) &&
        ((z1 >= box->z1 && z1 <= box->z2) || (box->z1 >= z1 && box->z2 <= z2))) {
        return true;
    }
    return false;
}

/* UNCALLED: nothing calls it. */
void Box::moveBy(i16 dx, i16 dy, u8 dz)
{
    x1 += dx;
    y1 += dy;
    z1 = z1 + dz;
    x2 += dx;
    y2 += dy;
    z2 = z2 + dz;
}
