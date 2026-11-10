/* MISC\PFLOAT.C (12c0:0000-09c5): 16.16 fixed point numbers, their
 * multiplication and division, and sine, cosine and tangent from tables.
 * Built through TASM (-B) for the 32-bit asm of common.h. */
#ifndef PORT_PFLOAT_H
#define PORT_PFLOAT_H
#include "common.h"

class PFloat {
public:
    /* The whole part in the high word, the fraction in the low word. */
    s32 value;
    PFloat() = default;
    explicit PFloat(s32 raw) { value = raw; }
    PFloat(const PFloat &other) { value = other.value; }
    /* The assignment the compiler made (C++ no longer makes it by itself next
     * to a copy constructor of the class's own). */
    PFloat &operator=(const PFloat &other) = default;
    PFloat(i16 whole, u16 fraction) { value = (static_cast<s32>(whole) << 16) | fraction; }
    PFloat operator*(PFloat factor) const;
    PFloat operator/(PFloat divisor) const;
    PFloat operator*(i16 factor) const;
    PFloat operator/(i16 divisor) const;
    PFloat operator*=(PFloat factor);
    /* Divides by the whole part of divisor only. */
    PFloat operator/=(PFloat divisor);
    PFloat operator*=(i16 factor);
    PFloat operator/=(i16 divisor);
    u8 operator>(PFloat other) const { return static_cast<u8>(value > other.value); }
};
ASSERT_SIZE(PFloat, 0x4);

/* Reads the 256-entry tables from path (static\trig.dat); halts when there is
 * no memory for them, and leaves them empty when the file is missing. */
void loadTrigTables(const char *path);
void freeTrigTables();
/* The functions of angle in degrees, from the table entry of its whole part
 * (256 entries for 360 degrees). */
PFloat sine(PFloat &angle);
PFloat cosine(PFloat &angle);

/* Nothing calls these, in the original either. */
PFloat tangent(PFloat &angle);
void unusedPFloat();
#endif
