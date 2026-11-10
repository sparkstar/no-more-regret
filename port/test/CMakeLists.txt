# The tests of what the games share: the platform's layer, compat, the
# sound's depth, the format and the trees' rules. Each game's own tests are
# in port/<game>/test. The setup they share is test/setup.cmake. The sources
# lie by what the tests do: unit/ a part of the port by itself, scenario/
# the program run on input, synth/ the program on data made from nothing of
# the original's.
set_directory_properties(PROPERTIES LABELS common)
game_origin(TEST_ORIGIN regret)

# The platform's files by their paths, a CD's image among them, and
# compat's path split, which have no oracle (the original's call INT 21h).
add_executable(files_test unit/files_test.cpp ${PROJECT_SOURCE_DIR}/system/files.cpp ${PROJECT_SOURCE_DIR}/system/iso9660.cpp
  ${PROJECT_SOURCE_DIR}/system/gamememory.cpp)
target_include_directories(files_test PRIVATE ${PROJECT_SOURCE_DIR}/system ${PROJECT_SOURCE_DIR}/platform)
target_link_libraries(files_test PRIVATE compat)
port_options(files_test)
add_test(NAME files COMMAND files_test)

# The game's globals in the arena (platform/gamestate.h): each kind's place
# and initial value, a copy of the arena put back, the destructors' order at
# the end.
add_executable(gamestate_test unit/gamestate_test.cpp ${PROJECT_SOURCE_DIR}/system/gamememory.cpp)
target_include_directories(gamestate_test PRIVATE ${PROJECT_SOURCE_DIR}/system ${PROJECT_SOURCE_DIR}/platform)
port_options(gamestate_test)
add_test(NAME game_globals COMMAND gamestate_test)
# The sanitizers' reports at the end may come between (the standard error).
set_tests_properties(game_globals PROPERTIES PASS_REGULAR_EXPRESSION
  "game_globals: [0-9]+ checks, 0 mismatches\n[^~]*destroyed b\ndestroyed a\n")

# File on the host's files: the game's file.cpp over the platform's files,
# with No Regret's CD files as the CD.
if(GAME_DATA_regret)
  game_files(FILE_SOURCES unified file.cpp)
  add_executable(file_test unit/file_test.cpp ${FILE_SOURCES} ${PROJECT_SOURCE_DIR}/system/files.cpp
    ${PROJECT_SOURCE_DIR}/system/iso9660.cpp ${PROJECT_SOURCE_DIR}/system/gamememory.cpp)
  game_target(file_test unified)
  target_include_directories(file_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
  target_link_libraries(file_test PRIVATE compat)
  target_compile_definitions(file_test PRIVATE PORT_ORIGIN="${TEST_ORIGIN}")
  add_test(NAME file COMMAND file_test)
endif()

# The port's PC.
add_executable(pc_test ${REGRET_PART_SOURCES} unit/pc_test.cpp)
game_target(pc_test unified)
target_include_directories(pc_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(pc_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME pc COMMAND pc_test)

# The replay's and the capture's pure parts: NUT's pieces, the capture's
# sound and picture, inputs.tsv's lines, the times and the days.
add_executable(replay_test ${REGRET_PART_SOURCES} unit/replay_test.cpp)
game_target(replay_test unified)
target_include_directories(replay_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(replay_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME replay COMMAND replay_test)

# The sound's trouble passed over, without the games' data: every card,
# the update before a card, a slot's sound replaced while it plays and the
# slot's stop, the data outside the tables, a division by 0, a channel
# without a mixer, a song's tick of no samples, finetune and instrument.
add_executable(soundtrouble_test unit/regret_values.cpp unit/soundtrouble_test.cpp)
game_target(soundtrouble_test unified)
target_include_directories(soundtrouble_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(soundtrouble_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME soundtrouble COMMAND soundtrouble_test)
set_tests_properties(soundtrouble PROPERTIES TIMEOUT 60)
# The chain of a sound the game plays again as it ends, whose channel
# SNDMGR.C loses, and its slot given to a shorter sound (SNDCACHE.C).
add_executable(soundchain_test ${REGRET_PART_SOURCES} unit/soundchain_test.cpp)
game_target(soundchain_test unified)
target_include_directories(soundchain_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(soundchain_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME soundchain COMMAND soundchain_test)
set_tests_properties(soundchain PROPERTIES TIMEOUT 60)

# The port's depth of the sound: through the game's calls, 8 bits against
# the 8-bit DMA's output for every song of both CDs, the effects, the
# movies' stream and a loud sum against the 16-bit output, a change while
# the sound plays; the program's menu at both depths and both DMA
# channels.
if(GAME_DATA_regret AND GAME_DATA_remorse)
  add_executable(sounddepth_test unit/regret_values.cpp unit/sounddepth_test.cpp)
  game_target(sounddepth_test unified)
  target_include_directories(sounddepth_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
  target_link_libraries(sounddepth_test PRIVATE ${PORT_LIBRARIES})
  game_origin(SOUNDDEPTH_REMORSE_ORIGIN remorse)
  game_origin(SOUNDDEPTH_REGRET_ORIGIN regret)
  target_compile_definitions(sounddepth_test PRIVATE PORT_REMORSE_CD="${SOUNDDEPTH_REMORSE_ORIGIN}"
    PORT_REGRET_CD="${SOUNDDEPTH_REGRET_ORIGIN}")
  add_test(NAME sounddepth COMMAND sounddepth_test)
endif()
if(GAME_DATA_regret)
  add_test(NAME soundprecision COMMAND ${CMAKE_COMMAND} -E env CRUSADER_GAME=regret ${PYTHON}
    ${PROJECT_SOURCE_DIR}/test/scenario/soundprecision.py ${CRUSADER})
  set_tests_properties(soundprecision PROPERTIES TIMEOUT 180)
endif()
if(GAME_DATA_remorse)
  add_test(NAME remorse_soundprecision COMMAND ${CMAKE_COMMAND} -E env CRUSADER_GAME=noremorse ${PYTHON}
    ${PROJECT_SOURCE_DIR}/test/scenario/soundprecision.py ${CRUSADER})
  set_tests_properties(remorse_soundprecision PROPERTIES TIMEOUT 180)
endif()

# The port's display: the DAC, the retrace, the VESA BIOS and the PNG,
# which zlib unpacks when the build has it.
add_executable(video_test ${REGRET_PART_SOURCES} unit/video_test.cpp)
game_target(video_test unified)
target_include_directories(video_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(video_test PRIVATE ${PORT_LIBRARIES})
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
  target_link_libraries(video_test PRIVATE ZLIB::ZLIB)
  target_compile_definitions(video_test PRIVATE PORT_HAVE_ZLIB)
endif()
add_test(NAME video COMMAND video_test)

# The window: the screen's picture in it, with SDL's dummy driver.
if(PORT_WINDOW)
  add_executable(window_test unit/regret_values.cpp unit/window_test.cpp ${PROJECT_SOURCE_DIR}/system/window.cpp)
  game_target(window_test unified)
  target_include_directories(window_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
  target_link_libraries(window_test PRIVATE ${PORT_LIBRARIES} SDL3::SDL3)
  add_test(NAME window COMMAND window_test)
  # It times the wall clock: alone, not beside the scenarios (baseline runs
  # nine at once).
  set_tests_properties(window PROPERTIES RUN_SERIAL TRUE)
  # The run's end on the game thread (window.h): the window thread closes
  # SDL and the run ends, neither hanging nor crashing.
  add_test(NAME window_exit COMMAND window_test --exit)
  set_tests_properties(window_exit PROPERTIES TIMEOUT 30 PASS_REGULAR_EXPRESSION "the game thread exits")
endif()

# The format: every C++ file of the port is as clang-format 20 writes it
# with port/.clang-format (skipped without clang-format 20;
# tools/format.py formats the changed files).
add_test(NAME format COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/format.py --check --all)
set_tests_properties(format PROPERTIES SKIP_RETURN_CODE 77)
# The exceptions to the rules: each in its place with the rule's name and
# why, none wider, the list as tools/exceptions.tsv has it.
add_test(NAME exceptions COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/exceptions.py)
# The synthetic scenes' documents: each scene's function's docstring in
# its form, every test of each game under an entry of its Tests and no
# entry over none.
add_test(NAME synth_docs COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/synth/docs.py check)
# The clang-tidy rules: nothing the rules of port/.clang-tidy find in the
# port's code, with this build's compile commands; in a build of Clang 20
# only (the ASan build), as clang-tidy 20 parses the code as it does, and
# once of the three (skipped without clang-tidy 20; tools/format.py applies
# the fixes the rules have).
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 20
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 21)
  add_test(NAME tidy COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/tidy.py --all --build ${CMAKE_BINARY_DIR})
  set_tests_properties(tidy PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)
endif()
# The trees' rules: each file in its group's place, no name twice on the
# include path, no include guard twice, no condition on a game.
add_test(NAME overlay COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/overlay.py --check)
# The one program's run-time value headers are as make_unified_values.py
# writes them from the games' value headers.
add_test(NAME unified_values_current COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/make_unified_values.py --check)
set_tests_properties(unified_values_current PROPERTIES SKIP_RETURN_CODE 77)
# The game's text in other languages: the Hangul font's glyphs and the
# games' translations are as make_text.py writes them from
# port/common/fonts and port/lang.
add_test(NAME text COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/make_text.py --check)
set_tests_properties(text PROPERTIES SKIP_RETURN_CODE 77)

# JOYSTICK.ASM in C++ on a clock and a joystick of the test's: the game
# port's axes, their time-outs and the buttons.
game_files(JOYSTICK_SOURCES unified joystick.cpp)
add_executable(joystick_test unit/joystick_test.cpp ${JOYSTICK_SOURCES})
game_target(joystick_test unified)
add_test(NAME joystick COMMAND joystick_test)

# The port's layout of gumps: a line's places, the stretches, the hits,
# a slider's track.
game_files(GUMP_LAYOUT_SOURCES unified layout.cpp)
add_executable(gump_layout_test unit/gump_layout_test.cpp ${GUMP_LAYOUT_SOURCES})
game_target(gump_layout_test unified)
add_test(NAME gump_layout COMMAND gump_layout_test)

# The usecode's memory: its selectors and far pointers.
game_files(USEMEM_SOURCES unified usemem.cpp)
add_executable(usemem_test unit/usemem_test.cpp ${USEMEM_SOURCES})
game_target(usemem_test unified)
target_link_libraries(usemem_test PRIVATE compat)
add_test(NAME usemem COMMAND usemem_test)
# The game's state in the arena: every writable object of the game's
# libraries' objects a gameGlobal reference its source binds, and every
# extern declaration of one in port/ a reference. On GCC's objects (objdump);
# a sanitizer's build adds data of its own.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CROSSCOMPILING AND NOT CMAKE_CXX_FLAGS MATCHES "coverage|sanitize")
  add_test(NAME gamestate COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/gamestate.py --build ${CMAKE_BINARY_DIR} --check)
endif()

release_checked()
game_speed_slow()
