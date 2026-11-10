/* MEMORY\HUGE.C (No Remorse only, 1298:0000-1477). See huge.h. */
#include "huge.h"

#include <cstring>

/* UNCALLED: nothing calls it. */
void hugeMemset(void *pointer, char value, u32 count)
{
    u8 *bytes = static_cast<u8 *>(pointer);
    u32 left = count;

    while (left > 0) {
        const u16 piece = left > 0xffff ? 0xffff : static_cast<u16>(left);

        /* BCC's memset takes a 16-bit count: left's low word, which past
         * 64K is not the piece (the original's). */
        std::memset(bytes, value, static_cast<u16>(left));
        left -= piece;
        bytes += piece;
    }
}

/* UNCALLED: nothing calls it. */
void hugeSetmem(void *pointer, u32 count, char value)
{
    hugeMemset(pointer, value, count);
}
