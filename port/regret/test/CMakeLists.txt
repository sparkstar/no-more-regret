# No Regret's tests: the types' layouts, the save's schema, the controls'
# bindings and settings, the synthetic scenes, and with the game's CD files
# the usecode's checks, the scenarios and the baseline. The shared code's
# tests are in port/test; the setup is test/setup.cmake. The places are
# port/test's (unit/, scenario/), with the scenarios' expected results in
# expected/.
set_directory_properties(PROPERTIES LABELS regret)
program_run(SAVEFIELDS savefields_test savefields)

# The layout of the types the files and the saved games hold against BCC's:
# layout_cases.h has BCC's offsets and sizes.
add_executable(layout_test unit/layout_test.cpp)
game_target(layout_test unified)
add_test(NAME layout COMMAND layout_test)

# CPROTMEM.C's sharing of the machine's memory: the original's fragments on
# its machine of 16 MB, the room the caches take.
add_executable(cprotmem_test ${REGRET_PART_SOURCES} unit/cprotmem_test.cpp)
game_target(cprotmem_test unified)
target_link_libraries(cprotmem_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME cprotmem COMMAND cprotmem_test)

# The saved game's schema: each member of each class under Process at BCC's
# offset (savefields_cases.h). Given a directory of the original's saves,
# the test reads them into the port's objects and writes them again.
add_executable(savefields_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/savefields_test.cpp)
game_target(savefields_test unified)
target_include_directories(savefields_test PRIVATE ${PROJECT_SOURCE_DIR}/system ${CMAKE_CURRENT_SOURCE_DIR}/unit)
target_link_libraries(savefields_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME savefields COMMAND savefields_test)

# The bindings of the controls (common/src/input/bindings.h) over the game's
# table: the original's give every key as it is; other bindings, the file's
# text.
add_executable(bindings_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/bindings_test.cpp)
game_target(bindings_test unified)
target_include_directories(bindings_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(bindings_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME bindings COMMAND bindings_test)

# A saved game's file checked before its load: loadable, and each thing
# the kernel's load cannot take.
add_executable(savecheck_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/savecheck_test.cpp)
game_target(savecheck_test unified)
target_include_directories(savecheck_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(savecheck_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME savecheck COMMAND savecheck_test)

# crusader.cfg's settings: GOG's without the file, a text's lines, one key
# alone.
add_executable(options_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/options_test.cpp)
game_target(options_test unified)
target_include_directories(options_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(options_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME options COMMAND options_test)

# The settings the player chose outside the arena: each changed and a copy
# of the arena put back, each kept.
add_executable(settings_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/settings_test.cpp)
game_target(settings_test unified)
target_include_directories(settings_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(settings_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME settings COMMAND settings_test)

# The synthetic scenes: data made from nothing of the original's, the port's
# run of each scene to its end with every test of it run, twice the same
# and the same as the results kept in test/synth/expected.
add_test(NAME synth COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/synth/run.py ${CRUSADER} --game regret
  --expected ${PROJECT_SOURCE_DIR}/test/synth/expected/regret.json)
# The wide screen's sides under the movies and the credits on the synthetic
# data (test/synth/wide_sides.py): covered, greyed with the world or the
# world's, at frames of the scenes.
add_test(NAME wide_sides COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/synth/wide_sides.py ${CRUSADER}
  --game regret)

if(GAME_DATA_regret)
  # The usecode: every function of EUSECODE.FLX decodes to its end as
  # INTER.C's tokens; its pointers stay where the port's design of them has
  # them; every call of an intrinsic passes the bytes its function takes
  # (tables/intrinsics.tsv); the loops' searches drop BCC's sizes; where
  # every literal's value goes is known.
  add_test(NAME usecode COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py check)
  add_test(NAME usecode_pointers COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py pointers)
  add_test(NAME usecode_loops COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py loops)
  add_test(NAME usecode_intrinsics COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py intrinsics)
  add_test(NAME usecode_text COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode_text.py --check)

  # Milestone 1: the main menu under scripted input, twice from fresh
  # installs, the same bytes each time.
  scenario_test(milestone regret milestone ${CRUSADER} ${SAVEFIELDS})

  # The first difficulty in NEW GAME: mouse selection must start the same
  # mission as keyboard selection (the original's mouse path leaves the
  # modal result unset, which the port sets as the Enter path does).
  add_test(NAME difficulty COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/scenario/difficulty.py ${CRUSADER})

  # Milestone 2: the first mission under scripted input, walking and firing,
  # twice from fresh installs, the same screens and states.
  scenario_test(milestone2 regret milestone2 ${CRUSADER} ${SAVEFIELDS})

  # Milestone 3: milestone 2's input in a sound install, twice from fresh
  # installs, the same WAV of the sound card on the virtual time, screens and
  # states.
  scenario_test(milestone3 regret milestone3 ${CRUSADER} ${SAVEFIELDS})

  # The replay and the capture: milestone 2's input as a recording, replayed
  # without a window, its states checked and its capture read by ffprobe
  # (when there is one).
  add_test(NAME replay_run COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/replay.py ${CRUSADER})
  # The usecode at the missions' starts: recordings made from keys with the
  # original's -warp and -egg in run.tsv and its cheat's immortality, the
  # trace's lines in order, each replay as recorded.
  add_test(NAME missions COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/missions.py ${CRUSADER})
  # The memory snapshots: milestone 2's input as a recording, a snapshot put
  # back a second on from frames in the menus, walking and fighting, each
  # replay as recorded to its end.
  add_test(NAME snapshots COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/snapshots.py ${CRUSADER})
  # Rerecording: milestone 2's input as a recording, replayed and recorded
  # again: the same recording.
  add_test(NAME rerecord COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/rerecord.py ${CRUSADER})
  # The wide screen in a new game: its sides drawn, covered under the game
  # menu and drawn again after it.
  add_test(NAME widescreen COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/widescreen.py ${CRUSADER})
  set_tests_properties(widescreen PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
  # The movies: the intro from the CD's FLICS to the menu, twice, the same
  # screens, states and WAV; skipped without the files.
  scenario_test(movies regret movies ${CRUSADER} ${SAVEFIELDS})
  set_tests_properties(movies PROPERTIES SKIP_RETURN_CODE 77)

  # The movies' frames: every movie of the CD in a mission, plain, big, in
  # hi-colour, in low resolution and without the subtitles, each frame's
  # screen against movieframes.tsv; skipped without the files. The runs go
  # six at once (movieframes.py's JOBS), about 20 seconds; they keep four
  # cores busy on the whole (PROCESSORS).
  add_test(NAME movieframes COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/movieframes.py ${CRUSADER})
  set_tests_properties(movieframes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800 PROCESSORS 4)

  # The terminals in other languages: mission 1's terminals in English and
  # in Korean, each page's screen against terminals.tsv; skipped without the
  # CD.
  add_test(NAME terminals COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/terminals.py ${CRUSADER})
  set_tests_properties(terminals PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)

  # The baseline: the deterministic outputs of the scenarios above (states,
  # screens, WAVs, saves) as expected/baseline.tsv has them, from the digests
  # their tests wrote (scenario_test: each runs once).
  add_test(NAME baseline COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/baseline.py check ${CRUSADER} ${SAVEFIELDS}
    --digests ${CMAKE_BINARY_DIR}/scenarios/regret)
  set_tests_properties(baseline PROPERTIES FIXTURES_REQUIRED regret_scenarios)
endif()

# The tools the tests run work on No Regret (tools/overlay.py), whatever
# the shell's CRUSADER_GAME says.
get_property(REGRET_TESTS DIRECTORY PROPERTY TESTS)
set_property(TEST ${REGRET_TESTS} APPEND PROPERTY ENVIRONMENT CRUSADER_GAME=regret)
release_checked()
game_speed_slow()
