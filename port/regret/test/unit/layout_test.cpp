/* The layout of the types the files and the saved games hold against BCC's.
 * layout_cases.h, generated from a program built with Borland C++ 3.1,
 * has each type's size and,
 * per member, the bytes the member's bits set in the type's cleared bytes;
 * the port sets the same members here. */
#include <cstdio>
#include <cstring>

namespace {

int checks = 0, failures = 0;
unsigned char buffer[0x1000];

void compare(const char *type, const char *member, unsigned size, unsigned offset, const char *bytes, unsigned length)
{
    checks++;
    unsigned first = size, last = 0;
    for (unsigned k = 0; k < size; k++) {
        if (buffer[k]) {
            if (first == size) {
                first = k;
            }
            last = k;
        }
    }
    if (first == offset && last + 1 - first == length && !std::memcmp(buffer + first, bytes, length)) {
        return;
    }
    failures++;
    std::printf("MISMATCH layout: %s::%s at %#x:", type, member, first);
    for (unsigned k = first; k <= last && k < size; k++) {
        std::printf(" %02x", buffer[k]);
    }
    std::printf(", BCC's at %#x:", offset);
    for (unsigned k = 0; k < length; k++) {
        std::printf(" %02x", (unsigned char)bytes[k]);
    }
    std::printf("\n");
}

}

#define SIZE(type, size) static_assert(sizeof(type) == (size), #type "'s size in BCC")
#define LEAF(type, member, offset, bytes)                                                                              \
    do {                                                                                                               \
        std::memset(buffer, 0, sizeof(type));                                                                          \
        type *p = reinterpret_cast<type *>(buffer);                                                                    \
        p->member = ~p->member;                                                                                        \
        compare(#type, #member, sizeof(type), offset, bytes, sizeof(bytes) - 1);                                       \
    } while (0)
#include "layout_cases.h"

int main()
{
    layoutCases();
    std::printf("layout: %d members, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}
