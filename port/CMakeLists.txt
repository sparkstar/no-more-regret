# The modern C++ port of Crusader: No Regret and Crusader: No Remorse: the
# games' sources (common/src and <game>/src), the C++ versions of the
# assembly modules, the system layer under them and the tests.
#   cmake -S port -B build/port -G Ninja && cmake --build build/port && ctest --test-dir build/port
cmake_minimum_required(VERSION 3.24)
project(crusader VERSION 0.9.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# The compile commands, for clang-tidy (tools/tidy.py).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
# macOS's linker warns of a static library on the line more than once,
# which CMake writes for the libraries that link the same one (game_memory).
if(APPLE)
  add_link_options(-Wl,-no_warn_duplicate_libraries)
endif()
# The libraries that call each other, linked as a group ($<LINK_GROUP:RESCAN>):
# macOS's linker searches every static library again for each symbol still
# undefined, whatever their order, and needs nothing around the group, which
# CMake gives it no way of its own.
if(APPLE)
  set(CMAKE_CXX_LINK_GROUP_USING_RESCAN_SUPPORTED TRUE)
  set(CMAKE_CXX_LINK_GROUP_USING_RESCAN "" "")
endif()

# What the original leaned on: arithmetic that wraps and memory read as
# another type. The compilers' warnings, none of which the port has: the
# basic ones, those that point at bugs, the modern ones, errors on the port's
# targets (port_options below).
set(PORT_WARNINGS
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Wformat=2 -Wnon-virtual-dtor
    -Woverloaded-virtual -Wimplicit-fallthrough -Wundef -Wcast-qual -Wzero-as-null-pointer-constant
    -Wsuggest-override -Wmissing-declarations -Wshadow -Wextra-semi -Wredundant-decls>
  $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond -Wduplicated-branches -Wlogical-op>
  $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wconditional-uninitialized -Wunreachable-code -Wcomma
    -Wmissing-prototypes>)
set(PORT_OPTIONS
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fwrapv -fno-strict-aliasing>
  ${PORT_WARNINGS}
  $<$<CXX_COMPILER_ID:MSVC>:/W4>)
# MinGW-w64 (Windows, cmake/mingw-w64-x86_64.cmake): the structures laid out
# as GCC lays them out elsewhere, not Microsoft's way (the files' and the
# saves' layouts), and the programs without the compiler's DLLs.
if(MINGW)
  list(APPEND PORT_OPTIONS -mno-ms-bitfields)
  add_link_options(-static)
endif()
# The options on a target of the port's, its warnings as errors (not on
# SDL's). -DPORT_WERROR=OFF leaves them warnings: for a
# compiler the port has not been built with.
option(PORT_WERROR "Treat the warnings on the port's targets as errors" ON)
# The halts name the place in the port's sources they are called from
# (common/src/system/common.h): its path under port/, its line and its
# function. On in every build, release ones too: a player's crash report has
# little else to go by. A comparison of objects builds without them, since
# every line that moves would change the objects. On the port's targets
# only, like the rest of port_options: SDL's commands stay the same, so it
# is not built again.
option(PORT_PLACES "Name the port's file, line and function in the halts' messages" ON)
function(port_options target)
  target_compile_options(${target} PRIVATE ${PORT_OPTIONS})
  target_compile_definitions(${target} PRIVATE PORT_PLACES=$<BOOL:${PORT_PLACES}>)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -fmacro-prefix-map=${PROJECT_SOURCE_DIR}/=)
  endif()
  set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ${PORT_WERROR})
endfunction()

# The game's sources, in subsystem groups (tools/overlay.py): common/src,
# what both games have the same, and unified/src, the one program's own:
# the run-time value headers tools/make_unified_values.py writes from each
# game's <game>/src (its value headers, <name>_game.h) and the wrappers that
# compile the game's data files there in a namespace of the game's. A file
# is in one of the two trees, so a header is found by its name in either.
# Borland's library headers from compat first, borland.h before each file,
# char signed as in the original.
set(GAME_GROUPS kernel input usecode item game world gump display graph sound movie text file cache system)
# The TASM modules in C++, a library of their own: the tests link it
# without the rest of the game.
set(ASM_MODULES cacheblk fastfind random framehit shp32 shpm32 shpmr32 shpmt32 shpr32 shpt32 svgaprim view32 bitfield
  xanthunk)
# The program's libraries of their own sources: the sound module
# (common/asylum) and the Xan movie decoder (common/xan).
set(GAME_LIBRARIES_unified asylum xan)
set(LIBRARY_DIRECTORY_asylum ${PROJECT_SOURCE_DIR}/common/asylum)
set(LIBRARY_DIRECTORY_xan ${PROJECT_SOURCE_DIR}/common/xan)

# <game>_INCLUDES, <game>_SOURCES (without the TASM modules) and
# <game>_ASM_SOURCES of the game's trees (unified, the one program's).
function(game_trees game)
  set(includes)
  set(sources)
  set(asm)
  foreach(tree ${game} common)
    foreach(group ${GAME_GROUPS})
      set(directory ${PROJECT_SOURCE_DIR}/${tree}/src/${group})
      if(IS_DIRECTORY ${directory})
        list(APPEND includes ${directory})
        file(GLOB files CONFIGURE_DEPENDS ${directory}/*.cpp)
        foreach(file ${files})
          get_filename_component(name ${file} NAME_WE)
          if(name IN_LIST ASM_MODULES)
            list(APPEND asm ${file})
          else()
            list(APPEND sources ${file})
          endif()
        endforeach()
      endif()
    endforeach()
  endforeach()
  # The libraries' headers, and the platform's interfaces, which the
  # game calls and the system layer gives.
  foreach(library ${GAME_LIBRARIES_${game}})
    list(APPEND includes ${LIBRARY_DIRECTORY_${library}})
  endforeach()
  set(${game}_INCLUDES ${includes} ${PROJECT_SOURCE_DIR}/platform PARENT_SCOPE)
  set(${game}_SOURCES ${sources} PARENT_SCOPE)
  set(${game}_ASM_SOURCES ${asm} PARENT_SCOPE)
endfunction()

# The paths of a game's source files by their names.
function(game_files variable game)
  set(paths)
  foreach(name ${ARGN})
    file(GLOB found ${PROJECT_SOURCE_DIR}/${game}/src/*/${name} ${PROJECT_SOURCE_DIR}/common/src/*/${name})
    list(LENGTH found count)
    if(NOT count EQUAL 1)
      message(FATAL_ERROR "${name}: ${count} files of that name in the sources of ${game}")
    endif()
    list(APPEND paths ${found})
  endforeach()
  set(${variable} ${paths} PARENT_SCOPE)
endfunction()

# The arena and the places of the game's globals in it (platform/gamestate.h),
# a library of their own, which each target compiled as the game's sources
# links: a test with system/gamememory.cpp among its own sources takes
# nothing of the library.
add_library(game_memory STATIC ${PROJECT_SOURCE_DIR}/system/gamememory.cpp)
target_include_directories(game_memory PUBLIC ${PROJECT_SOURCE_DIR}/platform PRIVATE ${PROJECT_SOURCE_DIR}/system)
port_options(game_memory)

# A target compiled as a game's sources.
function(game_target target game)
  target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/compat ${${game}_INCLUDES})
  target_link_libraries(${target} PUBLIC game_memory)
  target_compile_options(${target} PUBLIC $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsigned-char>)
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-include ${PROJECT_SOURCE_DIR}/compat/borland.h>)
  port_options(${target})
endfunction()
# Borland's and Phar Lap's library for the port (compat's headers), the crash
# reports and the port's version (the project's VERSION above, portversion.h).
add_library(compat STATIC compat/borland.cpp compat/dos.cpp compat/crash.cpp compat/portversion.cpp)
set_source_files_properties(compat/portversion.cpp PROPERTIES COMPILE_DEFINITIONS
  "PORT_VERSION=\"${PROJECT_VERSION}\"")
target_include_directories(compat PUBLIC compat PRIVATE ${PROJECT_SOURCE_DIR}/platform)
# Borland's rand's seed is the game's, in the arena.
target_link_libraries(compat PUBLIC game_memory)
port_options(compat)
if(WIN32)
  target_link_libraries(compat PUBLIC dbghelp)
endif()

# ASYLUM.DLL as the port's sound module, a library of its own for the
# tests as the TASM modules: modern C++, not the game's dialect, over the
# platform's output (audio.h). Both games play through it.
file(GLOB ASYLUM_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/common/asylum/*.cpp)
add_library(asylum STATIC ${ASYLUM_SOURCES})
target_include_directories(asylum PUBLIC ${LIBRARY_DIRECTORY_asylum} PRIVATE ${PROJECT_SOURCE_DIR}/platform)
target_link_libraries(asylum PUBLIC game_memory)
port_options(asylum)
# XANDRV32.DLL's decoder in C++, the same way: No Regret's movies.
file(GLOB XAN_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/common/xan/*.cpp)
add_library(xan STATIC ${XAN_SOURCES})
target_include_directories(xan PUBLIC ${LIBRARY_DIRECTORY_xan} PRIVATE ${PROJECT_SOURCE_DIR}/platform)
target_link_libraries(xan PUBLIC game_memory)
port_options(xan)

# The game's libraries, the one program's: the common code with the
# run-time value headers (unified/src, tools/make_unified_values.py), whose
# values the game the program plays fills, and each game's data files and
# generated tables in a namespace of their own (unified/src's wrappers);
# asm_unified holds the TASM modules, game_unified the rest.
game_trees(unified)
# Its choice of the game's values (unified/src/system/values.cpp) runs before
# the other static objects start: by init_priority on ELF and PE; Mach-O's
# start runs the initializers in the link's order and takes no priority, so
# the choice is the program's first object.
set(UNIFIED_VALUES ${PROJECT_SOURCE_DIR}/unified/src/system/values.cpp)
# The links of both games' data files (values_data.cpp), an object of the
# program's and of the tests' over the whole game: the data files, whose
# starts point the program's tables at the game's, are linked through it. A
# test of a part alone takes the values without them.
set(UNIFIED_DATA ${PROJECT_SOURCE_DIR}/unified/src/system/values_data.cpp)
list(REMOVE_ITEM unified_SOURCES ${UNIFIED_VALUES} ${UNIFIED_DATA})
add_library(unified_values OBJECT ${UNIFIED_VALUES})
game_target(unified_values unified)
add_library(unified_data OBJECT ${UNIFIED_DATA})
game_target(unified_data unified)
add_library(asm_unified STATIC ${unified_ASM_SOURCES})
game_target(asm_unified unified)
add_library(game_unified STATIC ${unified_SOURCES})
game_target(game_unified unified)
target_link_libraries(game_unified PUBLIC asm_unified ${GAME_LIBRARIES_unified} compat)

# The window: SDL3, the system's when it has 3.4 or later, or built here
# from its sources as a static library. Without it the port runs only
# without a window, on scripts (nowindow.cpp for window.cpp).
option(PORT_WINDOW "Open a window with SDL3 (without it the port runs only on scripts)" ON)
if(PORT_WINDOW)
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE
    FIND_PACKAGE_ARGS 3.4 CONFIG)
  FetchContent_MakeAvailable(SDL3)
endif()

# The system layer (system/): the PC the port stands in for (system/pc.h)
# and the window over it, compiled once (port_system_objects) as the
# program compiles it: all of it but the program's main (main.cpp), the
# window (window.cpp, or nowindow.cpp for a run without one), the program's
# new and delete (newdelete.cpp, which a test may go without) and the arena
# (gamememory.cpp, game_memory's). A target names its own of those. The
# program links every object, as some only register themselves at the start
# (passstate.cpp, the pass dump's state), which nothing calls; the tests
# take what they call of the same objects as a library (port_system). The
# game's libraries and the system layer call each other: a target links
# them as one group (PORT_LIBRARIES).
file(GLOB SYSTEM_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/system/*.cpp)
foreach(name main window nowindow newdelete gamememory)
  list(REMOVE_ITEM SYSTEM_SOURCES ${PROJECT_SOURCE_DIR}/system/${name}.cpp)
endforeach()
add_library(port_system_objects OBJECT ${SYSTEM_SOURCES})
target_include_directories(port_system_objects PUBLIC system)
port_options(port_system_objects)
target_link_libraries(port_system_objects PUBLIC game_unified)
add_library(port_system STATIC $<TARGET_OBJECTS:port_system_objects>)
target_include_directories(port_system PUBLIC system)
target_link_libraries(port_system PUBLIC game_unified)
set(PORT_LIBRARIES "$<LINK_GROUP:RESCAN,game_unified,asm_unified,port_system,asylum,xan,game_memory,compat>")
if(PORT_WINDOW)
  set(WINDOW_SOURCE ${PROJECT_SOURCE_DIR}/system/window.cpp)
else()
  set(WINDOW_SOURCE ${PROJECT_SOURCE_DIR}/system/nowindow.cpp)
endif()
# The stack of the program's first thread, which runs the game when the
# run has no window (with one, the game thread's, window.cpp's GAME_STACK):
# 8 MiB, Linux's, on Windows too, whose linker gives 2 MiB.
function(game_stack target)
  if(WIN32)
    target_link_options(${target} PRIVATE -Wl,--stack,8388608)
  endif()
endfunction()

# The program: both games, the system layer over the game's libraries. Its
# file is crusader-port: a player copies it into a GOG install, where No
# Remorse's own CRUSADER.EXE is, which Windows does not tell from
# crusader.exe.
add_executable(crusader $<TARGET_OBJECTS:unified_values> $<TARGET_OBJECTS:unified_data>
  $<TARGET_OBJECTS:port_system_objects> ${PROJECT_SOURCE_DIR}/system/main.cpp ${WINDOW_SOURCE}
  ${PROJECT_SOURCE_DIR}/system/newdelete.cpp)
set_target_properties(crusader PROPERTIES OUTPUT_NAME crusader-port)
port_options(crusader)
target_link_libraries(crusader PRIVATE ${PORT_LIBRARIES})
if(UNIX AND NOT APPLE)
  target_link_options(crusader PRIVATE -rdynamic)
endif()
game_stack(crusader)
if(PORT_WINDOW)
  target_link_libraries(crusader PRIVATE SDL3::SDL3)
endif()

enable_testing()
# The shared code's tests (label common) and each game's (label <game>),
# whose build directory is <game>-test.
include(test/setup.cmake)

# The game's boundary (tools/boundary.py): what the game's libraries take
# from outside them, held to its rules (the platform's interfaces, the
# library port/compat keeps, the runtime's functions without side effects
# on the host) before the program links, so that a build that breaks them
# stops. On every build; -DPORT_BOUNDARY_CHECK=OFF leaves it out, for a
# while of debugging that prints from the game.
option(PORT_BOUNDARY_CHECK "Check the game's boundary (tools/boundary.py) before the program links" ON)
if(PORT_BOUNDARY_CHECK)
  find_program(PORT_CXXFILT NAMES c++filt llvm-cxxfilt)
  if(NOT CMAKE_NM OR NOT PORT_CXXFILT)
    message(FATAL_ERROR "The game's boundary's check needs nm and c++filt (-DPORT_BOUNDARY_CHECK=OFF builds without it)")
  endif()
  set(BOUNDARY_ARGUMENTS --build ${CMAKE_BINARY_DIR} --nm ${CMAKE_NM} --cxxfilt ${PORT_CXXFILT})
  # Mach-O's names have an underscore before C's.
  if(APPLE)
    list(APPEND BOUNDARY_ARGUMENTS --leading-underscore)
  endif()
  file(GLOB PLATFORM_HEADERS CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/platform/*.h)
  add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/boundary.checked
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/boundary.checked
    COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/boundary.py ${BOUNDARY_ARGUMENTS}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/boundary.checked
    DEPENDS game_unified asm_unified asylum xan compat game_memory $<TARGET_OBJECTS:unified_values>
      $<TARGET_OBJECTS:unified_data> $<TARGET_OBJECTS:port_system_objects> ${PROJECT_SOURCE_DIR}/tools/boundary.py
      ${PROJECT_SOURCE_DIR}/tools/overlay.py ${PLATFORM_HEADERS}
    COMMENT "Checking the game's boundary (tools/boundary.py)"
    VERBATIM)
  add_custom_target(boundary DEPENDS ${CMAKE_BINARY_DIR}/boundary.checked)
  add_dependencies(crusader boundary)
endif()

add_subdirectory(test)
add_subdirectory(regret/test ${CMAKE_BINARY_DIR}/regret-test)
add_subdirectory(remorse/test ${CMAKE_BINARY_DIR}/remorse-test)

# More tests from outside this repository: a CMake file included here, after
# the port's own, in this directory's scope (its functions, sources and
# targets at hand).
set(PORT_EXTRA_TESTS "" CACHE FILEPATH "A CMake file of more tests, included after the port's own")
if(PORT_EXTRA_TESTS)
  include(${PORT_EXTRA_TESTS})
endif()
