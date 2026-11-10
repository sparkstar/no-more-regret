# What the tests of port/test (the games' shared code, the platform and
# compat) and of each game (port/<game>/test) share, included by
# port/CMakeLists.txt before them: the repository, Python, the games' CD
# files, the program as the scripts run it, and the sources of the tests
# over the system layer.
get_filename_component(REPOSITORY ${PROJECT_SOURCE_DIR}/.. ABSOLUTE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# The scripts' Python by its path: CMake puts a cross build's emulator in
# front of any program target, the imported Python's too.
set(PYTHON ${Python3_EXECUTABLE})
# Each game's CD: the player's own files in cd/<game> of the repository,
# the game's executable there, and the game as the tools name it
# (CRUSADER_GAME, tools/overlay.py).
set(GAME_ORIGIN_regret cd/noregret)
set(GAME_EXECUTABLE_regret REGRET.EXE)
set(CRUSADER_GAME_regret regret)
set(GAME_ORIGIN_remorse cd/noremorse)
set(GAME_EXECUTABLE_remorse CRUSADER.EXE)
set(CRUSADER_GAME_remorse noremorse)
# GAME_DATA_<game>: whether the game's CD files are there, by its executable
# in whatever case. Without them the tests that read the game's files are
# not made, and the others compare with the results the repository keeps.
# The files put there later run CMake again (CONFIGURE_DEPENDS).
foreach(game regret remorse)
  file(GLOB found LIST_DIRECTORIES false CONFIGURE_DEPENDS "${REPOSITORY}/${GAME_ORIGIN_${game}}/*")
  set(GAME_DATA_${game} FALSE)
  foreach(path ${found})
    get_filename_component(name ${path} NAME)
    string(TOUPPER ${name} name)
    if(name STREQUAL GAME_EXECUTABLE_${game})
      set(GAME_DATA_${game} TRUE)
    endif()
  endforeach()
  if(NOT GAME_DATA_${game})
    message(STATUS "No ${GAME_EXECUTABLE_${game}} in ${GAME_ORIGIN_${game}}: the tests of the game's files left out")
  endif()
endforeach()

# A program as the scripts run it: through a cross build's emulator (wine,
# which sees the host's files on drive Z:) by a script <name>-run, else
# itself.
function(program_run variable target name)
  if(CMAKE_CROSSCOMPILING_EMULATOR)
    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/${name}-run
      CONTENT "#!/bin/sh\nexec ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:${target}> \"$@\"\n"
      FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set(${variable} ${CMAKE_BINARY_DIR}/${name}-run PARENT_SCOPE)
  else()
    set(${variable} $<TARGET_FILE:${target}> PARENT_SCOPE)
  endif()
endfunction()
# The program, which the games' tests that run a game run, each with its
# game's CRUSADER_GAME: what is shipped is what is tested.
program_run(CRUSADER crusader crusader)
# A scenario of the game's baseline (tools/baseline.py's SCENARIOS) as its
# test: run once through baseline.py scenario, which writes its files'
# digests for the baseline's check (the tests' times); the check needs
# them (the fixture <game>_scenarios).
function(scenario_test test game scenario program savefields)
  add_test(NAME ${test} COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/baseline.py scenario ${scenario} ${program}
    ${savefields} --digests ${CMAKE_BINARY_DIR}/scenarios/${game})
  set_tests_properties(${test} PROPERTIES FIXTURES_SETUP ${game}_scenarios)
endfunction()
# Each run checks that it releases all it holds at its end: the game's arena
# has no block left (PORT_RELEASE_CHECK, system/gamememory.h), and with the
# address sanitizer what only a global holds of the host's heap is reported
# too (use_globals=0), but the C and C++ libraries' own (test/lsan.supp).
# Each directory of tests calls it after its tests.
function(release_checked)
  get_property(tests DIRECTORY PROPERTY TESTS)
  if(tests)
    set_property(TEST ${tests} APPEND PROPERTY ENVIRONMENT PORT_RELEASE_CHECK=1)
    if(CMAKE_CXX_FLAGS MATCHES "sanitize=[a-z,]*address")
      set_property(TEST ${tests} APPEND PROPERTY ENVIRONMENT ASAN_OPTIONS=detect_leaks=1
        LSAN_OPTIONS=use_globals=0:suppressions=${PROJECT_SOURCE_DIR}/test/lsan.supp)
    endif()
  endif()
endfunction()
# The tests' game speed (platform/clock.h): slow by REGRET_GAME_SPEED, the
# passes of a sixtieth of a second at which their results came; a test of
# another speed names it in its enhanced.cfg (game_speed=), which comes
# after. Each directory of tests calls it after its tests.
function(game_speed_slow)
  get_property(tests DIRECTORY PROPERTY TESTS)
  if(tests)
    set_property(TEST ${tests} APPEND PROPERTY ENVIRONMENT REGRET_GAME_SPEED=slow)
  endif()
endfunction()
# A path of the repository as a test's program sees it: a Windows build
# under wine on drive Z:.
function(test_path variable path)
  if(WIN32 AND CMAKE_CROSSCOMPILING)
    set(${variable} "Z:${path}" PARENT_SCOPE)
  else()
    set(${variable} "${path}" PARENT_SCOPE)
  endif()
endfunction()

# A game's CD (its origin directory) as a test's program sees it.
function(game_origin variable game)
  test_path(path ${REPOSITORY}/${GAME_ORIGIN_${game}})
  set(${variable} ${path} PARENT_SCOPE)
endfunction()

# A test of a part of the port alone, with a main of its own: a game's
# values without the program's choice of them (test/unit/<game>_values.cpp)
# and the system layer without a window. It links the libraries
# (PORT_LIBRARIES), of which it takes what it calls. A test over the whole
# game takes the program's choice instead (unified/src/system/values.cpp:
# the game CRUSADER_GAME names, which each directory of tests sets), the
# links of the games' data files (unified_data) and the program's new and
# delete.
set(REGRET_PART_SOURCES ${PROJECT_SOURCE_DIR}/test/unit/regret_values.cpp ${PROJECT_SOURCE_DIR}/system/nowindow.cpp)
set(REMORSE_PART_SOURCES ${PROJECT_SOURCE_DIR}/test/unit/remorse_values.cpp ${PROJECT_SOURCE_DIR}/system/nowindow.cpp)
set(GAME_TEST_SOURCES $<TARGET_OBJECTS:unified_values> $<TARGET_OBJECTS:unified_data>
  ${PROJECT_SOURCE_DIR}/system/nowindow.cpp ${PROJECT_SOURCE_DIR}/system/newdelete.cpp)
