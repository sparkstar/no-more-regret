# The port for 64-bit Windows, cross-compiled with MinGW-w64:
#   cmake -S port -B build/port-win -G Ninja --toolchain $PWD/port/cmake/mingw-w64-x86_64.cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# The tests run the Windows programs with wine (wine.sh).
set(CMAKE_CROSSCOMPILING_EMULATOR ${CMAKE_CURRENT_LIST_DIR}/wine.sh)
