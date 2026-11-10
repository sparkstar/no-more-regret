/* setenv and unsetenv for the tests, which Windows's C library has as
 * _putenv_s (an empty value takes the variable away). */
#ifndef PORT_TESTENV_H
#define PORT_TESTENV_H
#include <cstdlib>

#ifdef _WIN32
inline int setenv(const char *name, const char *value, int overwrite)
{
    if (!overwrite && std::getenv(name)) {
        return 0;
    }
    return _putenv_s(name, value);
}

inline int unsetenv(const char *name)
{
    return _putenv_s(name, "");
}
#endif

#endif
