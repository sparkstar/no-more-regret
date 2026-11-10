/* The program's new and delete (gamememory.h): a file of its own, so that
 * a test with new and delete of its own links the rest of
 * the system layer without it. */
#include "gamememory.h"

#include <new>

void *operator new(std::size_t bytes)
{
    return portAllocate(bytes, false, __builtin_return_address(0));
}

void *operator new[](std::size_t bytes)
{
    return portAllocate(bytes, true, __builtin_return_address(0));
}

void *operator new(std::size_t bytes, const std::nothrow_t & /*unused*/) noexcept
{
    try {
        return portAllocate(bytes, false, __builtin_return_address(0));
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}

void *operator new[](std::size_t bytes, const std::nothrow_t & /*unused*/) noexcept
{
    try {
        return portAllocate(bytes, true, __builtin_return_address(0));
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}

void *operator new(std::size_t bytes, std::align_val_t alignment)
{
    return portAllocateAligned(bytes, alignment, false, __builtin_return_address(0));
}

void *operator new[](std::size_t bytes, std::align_val_t alignment)
{
    return portAllocateAligned(bytes, alignment, true, __builtin_return_address(0));
}

void *operator new(std::size_t bytes, std::align_val_t alignment, const std::nothrow_t & /*unused*/) noexcept
{
    try {
        return portAllocateAligned(bytes, alignment, false, __builtin_return_address(0));
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}

void *operator new[](std::size_t bytes, std::align_val_t alignment, const std::nothrow_t & /*unused*/) noexcept
{
    try {
        return portAllocateAligned(bytes, alignment, true, __builtin_return_address(0));
    } catch (const std::bad_alloc &) {
        return nullptr;
    }
}

void operator delete(void *pointer) noexcept
{
    portRelease(pointer, false);
}

void operator delete[](void *pointer) noexcept
{
    portRelease(pointer, true);
}

void operator delete(void *pointer, std::size_t /*unused*/) noexcept
{
    portRelease(pointer, false);
}

void operator delete[](void *pointer, std::size_t /*unused*/) noexcept
{
    portRelease(pointer, true);
}

void operator delete(void *pointer, const std::nothrow_t & /*unused*/) noexcept
{
    portRelease(pointer, false);
}

void operator delete[](void *pointer, const std::nothrow_t & /*unused*/) noexcept
{
    portRelease(pointer, true);
}

void operator delete(void *pointer, std::align_val_t alignment) noexcept
{
    portReleaseAligned(pointer, alignment, false);
}

void operator delete[](void *pointer, std::align_val_t alignment) noexcept
{
    portReleaseAligned(pointer, alignment, true);
}

void operator delete(void *pointer, std::size_t /*unused*/, std::align_val_t alignment) noexcept
{
    portReleaseAligned(pointer, alignment, false);
}

void operator delete[](void *pointer, std::size_t /*unused*/, std::align_val_t alignment) noexcept
{
    portReleaseAligned(pointer, alignment, true);
}

void operator delete(void *pointer, std::align_val_t alignment, const std::nothrow_t & /*unused*/) noexcept
{
    portReleaseAligned(pointer, alignment, false);
}

void operator delete[](void *pointer, std::align_val_t alignment, const std::nothrow_t & /*unused*/) noexcept
{
    portReleaseAligned(pointer, alignment, true);
}
