# No Remorse's tests: the save's schema, the controls' bindings and
# settings, the synthetic scenes, and with the game's CD files the usecode's
# checks, the scenarios and the baseline. The names begin with remorse_; the
# setup is test/setup.cmake. The places are No Regret's (unit/, scenario/,
# expected/).
set_directory_properties(PROPERTIES LABELS remorse)
program_run(REMORSE_SAVEFIELDS remorse_savefields_test remorse_savefields)

# The synthetic scenes: data made from nothing of the original's, the port's
# run of each scene to its end with every test of it run, twice the same
# and the same as the results kept in test/synth/expected.
add_test(NAME remorse_synth COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/synth/run.py ${CRUSADER} --game remorse
  --expected ${PROJECT_SOURCE_DIR}/test/synth/expected/remorse.json)
# The wide screen's sides under the movies and the credits on the synthetic
# data (test/synth/wide_sides.py): covered, greyed with the world (No
# Remorse's movies on grey) or the world's, at frames of the scenes.
add_test(NAME remorse_wide_sides COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/synth/wide_sides.py ${CRUSADER}
  --game remorse)

# CPROTMEM.C's sharing of the machine's memory: the original's fragments on
# its machine of 16 MB, the room the caches take.
add_executable(remorse_cprotmem_test ${REMORSE_PART_SOURCES} unit/cprotmem_test.cpp)
game_target(remorse_cprotmem_test unified)
target_link_libraries(remorse_cprotmem_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME remorse_cprotmem COMMAND remorse_cprotmem_test)

# The saved game's schema: each member of each class under Process at BCC's
# offset (savefields_cases.h). Given a directory of the original's saves,
# the test reads them into the port's objects and writes them again.
add_executable(remorse_savefields_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/savefields_test.cpp)
game_target(remorse_savefields_test unified)
target_include_directories(remorse_savefields_test PRIVATE ${PROJECT_SOURCE_DIR}/system ${CMAKE_CURRENT_SOURCE_DIR}/unit)
target_link_libraries(remorse_savefields_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME remorse_savefields COMMAND remorse_savefields_test)

# The bindings of the controls (common/src/input/bindings.h) over the game's
# table: the original's give every key as it is; other bindings, the file's
# text.
add_executable(remorse_bindings_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/bindings_test.cpp)
game_target(remorse_bindings_test unified)
target_include_directories(remorse_bindings_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(remorse_bindings_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME remorse_bindings COMMAND remorse_bindings_test)

# A saved game's file checked before its load: loadable, and each thing
# the kernel's load cannot take.
add_executable(remorse_savecheck_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/savecheck_test.cpp)
game_target(remorse_savecheck_test unified)
target_include_directories(remorse_savecheck_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(remorse_savecheck_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME remorse_savecheck COMMAND remorse_savecheck_test)

# crusader.cfg's settings: GOG's without the file, a text's lines, one key
# alone.
add_executable(remorse_options_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/options_test.cpp)
game_target(remorse_options_test unified)
target_include_directories(remorse_options_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(remorse_options_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME remorse_options COMMAND remorse_options_test)

# The settings the player chose outside the arena: each changed and a copy
# of the arena put back, each kept.
add_executable(remorse_settings_test ${GAME_TEST_SOURCES} ${PROJECT_SOURCE_DIR}/test/unit/settings_test.cpp)
game_target(remorse_settings_test unified)
target_include_directories(remorse_settings_test PRIVATE ${PROJECT_SOURCE_DIR}/system)
target_link_libraries(remorse_settings_test PRIVATE ${PORT_LIBRARIES})
add_test(NAME remorse_settings COMMAND remorse_settings_test)

if(GAME_DATA_remorse)
  # The usecode: every function of No Remorse's EUSECODE.FLX decodes to its
  # end (its table of its locals' names after it), its pointers stay where
  # the port's design has them, its loops drop BCC's sizes, its calls of
  # intrinsics pass the bytes the functions take, where every literal's
  # value goes is known.
  add_test(NAME remorse_usecode COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py check)
  add_test(NAME remorse_usecode_pointers COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py pointers)
  add_test(NAME remorse_usecode_loops COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py loops)
  add_test(NAME remorse_usecode_intrinsics COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode.py intrinsics)
  add_test(NAME remorse_usecode_text COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/usecode_text.py --check)

  # Milestone 1: the main menu under scripted input, twice from fresh
  # installs, the same bytes each time.
  scenario_test(remorse_milestone remorse milestone ${CRUSADER} ${REMORSE_SAVEFIELDS})

  # The movies: the intro from the CD's flics to the menu, twice, the same
  # screens, states and WAV, then with the subtitles and the big video;
  # skipped without the files.
  scenario_test(remorse_movies remorse movies ${CRUSADER} ${REMORSE_SAVEFIELDS})
  set_tests_properties(remorse_movies PROPERTIES SKIP_RETURN_CODE 77)

  # The movies' frames: every movie of the CD in a mission in its callers'
  # forms, big and without the subtitles, each frame's screen against
  # movieframes.tsv; skipped without the files. The runs go six at once
  # (movieframes.py's JOBS), about 15 seconds; they keep four cores busy on
  # the whole (PROCESSORS).
  add_test(NAME remorse_movieframes COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/movieframes.py ${CRUSADER})
  set_tests_properties(remorse_movieframes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800 PROCESSORS 4)
  add_test(NAME remorse_terminals COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/terminals.py ${CRUSADER})
  set_tests_properties(remorse_terminals PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)

  # Milestone 2: the first mission under scripted input, walking and firing
  # until a guard dies, twice from fresh installs, the same screens and
  # states.
  scenario_test(remorse_milestone2 remorse milestone2 ${CRUSADER} ${REMORSE_SAVEFIELDS})

  # Milestone 3: milestone 2's input in a sound install, twice, the same
  # WAV, screens and states.
  scenario_test(remorse_milestone3 remorse milestone3 ${CRUSADER} ${REMORSE_SAVEFIELDS})

  # The replay and the capture, as No Regret's replay_run.
  add_test(NAME remorse_replay_run COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/replay.py ${CRUSADER})
  # The missions' scenarios (No Regret's missions): the starts, the pads,
  # the fight, the death and the eggs' race at the base.
  add_test(NAME remorse_missions COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/missions.py ${CRUSADER})
  # The memory snapshots: milestone 2's input as a recording, a snapshot put
  # back a second on from frames in the menus, walking and fighting, each
  # replay as recorded to its end.
  add_test(NAME remorse_snapshots COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/snapshots.py ${CRUSADER})
  # Rerecording: milestone 2's input as a recording, replayed and recorded
  # again: the same recording.
  add_test(NAME remorse_rerecord COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/test/scenario/rerecord.py ${CRUSADER})

  # No Remorse's own: the help's pages and its hidden one (WORLD.C), the
  # weasel's shop (WEZZY.C) with a card's credits, twice the same.
  scenario_test(remorse_help remorse help ${CRUSADER} ${REMORSE_SAVEFIELDS})
  scenario_test(remorse_weasel remorse weasel ${CRUSADER} ${REMORSE_SAVEFIELDS})
  # The credits seen to their end with the cheats on, and their movie z03
  # (CREDITS.C); skipped without the movie.
  scenario_test(remorse_credits remorse credits ${CRUSADER} ${REMORSE_SAVEFIELDS})
  set_tests_properties(remorse_credits PROPERTIES SKIP_RETURN_CODE 77)

  # The baseline: the deterministic outputs of the scenarios above as
  # remorse/test/expected/baseline.tsv has them, from the digests their tests
  # wrote (scenario_test: each runs once).
  add_test(NAME remorse_baseline COMMAND ${PYTHON} ${PROJECT_SOURCE_DIR}/tools/baseline.py check ${CRUSADER}
    ${REMORSE_SAVEFIELDS} --digests ${CMAKE_BINARY_DIR}/scenarios/remorse)
  set_tests_properties(remorse_baseline PROPERTIES FIXTURES_REQUIRED remorse_scenarios)
endif()

# The tools the tests run work on No Remorse (tools/overlay.py), whatever
# the shell's CRUSADER_GAME says.
get_property(REMORSE_TESTS DIRECTORY PROPERTY TESTS)
if(REMORSE_TESTS)
  set_property(TEST ${REMORSE_TESTS} APPEND PROPERTY ENVIRONMENT CRUSADER_GAME=noremorse)
endif()
release_checked()
game_speed_slow()
