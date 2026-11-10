/* Borland's mem.h for the port: the far forms of the memory functions, which are the plain ones in one
 * address space. */
#ifndef PORT_MEM_H
#define PORT_MEM_H
#include <cstring>

/* UNCALLED: nothing calls it. */
inline void *_fmemcpy(void *to, const void *from, std::size_t count)
{
    return std::memcpy(to, from, count);
}
inline void *_fmemset(void *to, int value, std::size_t count)
{
    return std::memset(to, value, count);
}
/* memmove with the source first. */
/* UNCALLED: nothing calls it. */
inline void movmem(const void *from, void *to, unsigned count)
{
    std::memmove(to, from, count);
}

#endif
