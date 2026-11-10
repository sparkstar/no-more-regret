/* PRIORITY.C (11d0:066f-070c): priority conditions. A condition holds a level
 * (0-31) in its low five bits and the comparisons that pass in its top three:
 * equal (20h), above (40h), below (80h). KERNEL.C passes them by value. */
#ifndef PORT_PRIORITY_H
#define PORT_PRIORITY_H
#include "common.h"

constexpr int PRIORITY_LEVEL = 0x1f;
constexpr int PRIORITY_EQUAL = 0x20;
constexpr int PRIORITY_ABOVE = 0x40;
constexpr int PRIORITY_BELOW = 0x80;
constexpr int PRIORITY_TESTS = 0xe0;

class Priority {
public:
    u8 value;

    explicit Priority(u8 condition) { value = condition; }
    /* Whether the condition tests anything. */
    [[nodiscard]] bool isSet() const;
    /* Whether level passes the condition. */
    [[nodiscard]] bool holds(u8 level) const;
};
ASSERT_SIZE(Priority, 1);
#endif
