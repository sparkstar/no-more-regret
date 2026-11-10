/* Borland C++ 3.1's library for the port's modern builds: the build
 * includes it before each file of the games' sources (-include); the rest of
 * Borland's library headers are the headers of the same names beside this
 * one. The memory-model and calling keywords (far, pascal, interrupt ...)
 * were empty macros here until they were taken out of the sources. */
#ifndef PORT_BORLAND_H
#define PORT_BORLAND_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>

/* Borland C++ 3.1's rand, which the game's logic draws on through
 * random(n) (NPC.C, the activities, the explosions and more): the port must
 * give the same numbers (seed * 22695477 + 1, bits 16-30), so these are the
 * port library's own rather than the C library's. */
int borlandRand();
void borlandSrand(unsigned value);
/* Borland's random(n), a macro of stdlib.h: (int)(((long)rand()*(n))/
 * (RAND_MAX+1)), with its int of 16 bits. */
inline std::int16_t borlandRandom(std::int32_t num)
{
    return static_cast<std::int16_t>(static_cast<std::int32_t>(borlandRand()) * num / 0x8000);
}

/* Borland's additions to string.h and stdlib.h. Windows's C library has
 * them too, declared for C: the port's are Borland's (borland_test), which
 * take their place, declared the same way. */
#ifdef _WIN32
#define PORT_BORLAND_LIBRARY extern "C"
#else
#define PORT_BORLAND_LIBRARY
#endif
PORT_BORLAND_LIBRARY int stricmp(const char *s1, const char *s2);
PORT_BORLAND_LIBRARY int strnicmp(const char *s1, const char *s2, std::size_t count);
PORT_BORLAND_LIBRARY char *strlwr(char *s);
#ifndef _WIN32
/* Windows's stdlib.h declares it already, the same way. */
PORT_BORLAND_LIBRARY char *itoa(int value, char *string, int radix);
#endif
/* Borland's modes of fcntl.h, which the port's files do not need (Windows's
 * C library has O_BINARY, of the same value). */
#ifndef _WIN32
constexpr int O_BINARY = 0x8000;
#endif
constexpr int O_DEVICE = 0x2000;

#endif
