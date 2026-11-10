/* MISC\PFLOAT.C (12c0:0000-09c5): 16.16 fixed point numbers. See pfloat.h. */
#include "pfloat.h"
#include "gamestate.h"
#include "file.h"
#include "utility.h"

/* sine, cosine and tangent of 360*i/256 degrees. */
s32 *&sineTable = gameGlobal<s32 *>(nullptr);
s32 *&cosineTable = gameGlobal<s32 *>(nullptr);
s32 *&tangentTable = gameGlobal<s32 *>(nullptr);

void loadTrigTables(const char *path)
{
    sineTable = new s32[256];
    cosineTable = new s32[256];
    tangentTable = new s32[256];
    if (fileExists(path) == 0) {
        return;
    }
    const File file(path, FILE_READ, 0);
    file.read(sineTable, static_cast<ui16>(256 * sizeof(s32)));
    file.read(cosineTable, static_cast<ui16>(256 * sizeof(s32)));
    file.read(tangentTable, static_cast<ui16>(256 * sizeof(s32)));
}

void freeTrigTables()
{
    if (sineTable != nullptr) {
        delete[] sineTable;
        sineTable = nullptr;
    }
    if (cosineTable != nullptr) {
        delete[] cosineTable;
        cosineTable = nullptr;
    }
    if (tangentTable != nullptr) {
        delete[] tangentTable;
        tangentTable = nullptr;
    }
}

PFloat PFloat::operator*(PFloat factor) const
{
    s32 high = value;
    s32 low = 0;

    LONG_MULTIPLY(high, factor, high, low);
    return PFloat(static_cast<i16>(high), static_cast<u16>(low >> 16));
}

PFloat PFloat::operator/(PFloat divisor) const
{
    const s32 dividend = value;
    const s32 by = divisor.value;

    if (divisor.value == 0) {
        halt();
    }
    i16 whole = 0;
    u16 fraction = 0;
    FIXED_DIVIDE(dividend, by, whole, fraction);
    return PFloat(whole, fraction);
}

PFloat PFloat::operator*(i16 factor) const
{
    s32 high = value;
    s32 low = static_cast<s32>(factor) << 16;

    LONG_MULTIPLY(high, low, high, low);
    return PFloat(static_cast<i16>(high), static_cast<u16>(low >> 16));
}

PFloat PFloat::operator/(i16 divisor) const
{
    const s32 dividend = value;
    const s32 by = static_cast<s32>(divisor) << 16;

    if (divisor == 0) {
        halt();
    }
    i16 whole = 0;
    u16 fraction = 0;
    FIXED_DIVIDE(dividend, by, whole, fraction);
    return PFloat(whole, fraction);
}

PFloat PFloat::operator*=(PFloat factor)
{
    s32 high = value;
    s32 low = factor.value;

    LONG_MULTIPLY(high, low, high, low);
    value = high << 16;
    value |= low >> 16;
    return *this;
}

PFloat PFloat::operator/=(PFloat divisor)
{
    const s32 dividend = value;
    const s32 by = divisor.value >> 16;

    if (divisor.value == 0) {
        halt();
    }
    i16 whole = 0;
    u16 fraction = 0;
    FIXED_DIVIDE(dividend, by, whole, fraction);
    value = 0;
    value |= static_cast<s32>(whole) << 16;
    value |= fraction;
    return *this;
}

PFloat PFloat::operator*=(i16 factor)
{
    s32 high = value;
    s32 low = static_cast<s32>(factor) << 16;

    LONG_MULTIPLY(high, low, high, low);
    value = high << 16;
    value |= low >> 16;
    return *this;
}

PFloat PFloat::operator/=(i16 divisor)
{
    const s32 dividend = value;
    const s32 by = static_cast<s32>(divisor) << 16;

    if (divisor == 0) {
        halt();
    }
    i16 whole = 0;
    u16 fraction = 0;
    FIXED_DIVIDE(dividend, by, whole, fraction);
    value = 0;
    value |= static_cast<s32>(whole) << 16;
    value |= fraction;
    return *this;
}

PFloat sine(PFloat &angle)
{
    s32 index;

    if (sineTable == nullptr) {
        halt();
    }
    index = angle.value;
    index = index >> 16;
    index = index % 360;
    if (index < 0) {
        index = index + 360;
    }
    index = index * 1000;
    index /= 1406; /* /=: the bytes load 1406 before index */
    if (index > 255) {
        index = 255;
    }
    return PFloat(sineTable[index]);
}

PFloat cosine(PFloat &angle)
{
    s32 index;

    if (cosineTable == nullptr) {
        halt();
    }
    index = angle.value;
    index = index >> 16;
    index = index % 360;
    if (index < 0) {
        index = index + 360;
    }
    index = index * 1000;
    index /= 1406;
    if (index > 255) {
        index = 255;
    }
    return PFloat(cosineTable[index]);
}

PFloat tangent(PFloat &angle)
{
    s32 index;

    if (tangentTable == nullptr) {
        halt();
    }
    index = angle.value;
    index = index >> 16;
    index = index % 360;
    if (index < 0) {
        index = index + 360;
    }
    index = index * 1000;
    index /= 1406;
    if (index > 255) {
        index = 255;
    }
    return PFloat(tangentTable[index]);
}

/* The last function of the module: empty, and nothing calls it. */
void unusedPFloat() {}
