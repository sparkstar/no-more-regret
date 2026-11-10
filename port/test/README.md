# Tests

The repository holds neither the games' data nor their original programs, so the tests come in three layers.

| Layer | Needs | Tests | What they show |
|---|---|---:|---|
| 1. Without the games' data | nothing | 32 | The parts of the port alone, the code's rules, the synthetic scenes |
| 2. With the games' data | the CD files of a copy you own, in `cd/` ([`../../cd/README.md`](../../cd/README.md)) | 73, layer 1's among them | The real games: menus, missions, movies, terminals, sound, snapshots, rewinding, recordings |
| 3. Against the originals | the original programs and a table of their functions' places, outside this repository | more, through [a hook](#tests-from-outside-the-repository) | That the port does what the original programs do, function by function and pass by pass |

Layers 1 and 2 compare with results kept in the repository: the synthetic scenes' states, and for the games' data
only digests (SHA-256) of the screens, states, sounds and saves, from which nothing of the data can be made. With the
GOG releases' files every machine should give the same results (checked on fresh GOG installs).

```sh
cmake -S port -B build/port -G Ninja
cmake --build build/port
ctest --test-dir build/port -j8                       # all that the data at hand allows
ctest --test-dir build/port -L regret                 # one game's (labels: common, regret, remorse)
ctest --test-dir build/port -R synth --output-on-failure
```

CMake makes a game's layer 2 tests when its folder in `cd/` holds the game's executable, and runs again when the
folder's files change. Every test also checks that the run gives back all it holds at its end
(`PORT_RELEASE_CHECK`: the game's arena; with the address sanitizer, the host's heap too). The tests run at the walk
speed SLOW (`REGRET_GAME_SPEED=slow`, [`../platform/clock.h`](../platform/clock.h)), a pass a sixtieth of a second,
at which their results came; a scene of another speed names it in its `enhanced.cfg` (`game_speed=`), which comes
after (`speed-lift-normal`, `speed-lift-fast`).

## Where they are

| Place | What |
|---|---|
| `test/unit` | A part of the port by itself: the platform's files and CD images, the virtual PC, the replay's pieces, the video, the window, the joystick, the gumps' layout, the usecode's memory, the arena's globals, the controls' bindings and settings, the save's schema, the sound's depth |
| `test/scenario` | The program on scripted input or a recording, with the games' data: the milestones, the missions, the movies' frames, the terminals, snapshots, rerecording, the wide screen, the sound |
| `test/synth` | The synthetic scenes: data made from nothing of the games', played by the program to their ends |
| `<game>/test/unit`, `<game>/test/scenario` | A game's own: the types' layouts against BCC's, the memory manager's pieces, its scenarios |
| `<game>/test/expected` | The kept results of a game's scenarios: `baseline.tsv`, `movieframes.tsv`, `terminals.tsv` |
| `test/setup.cmake` | What the test directories share: the CD folders, the programs as the scripts run them, the scenario tests |

The code's checks (the format, the trees' rules, the exceptions, the arena, the generators' files) are tests too,
and the build itself holds the game's boundary to its rules before the program links (`tools/boundary.py`);
[`../tools/README.md`](../tools/README.md) says what each tool checks.

## The synthetic scenes

Each scene is a function in `test/synth` that makes a world (maps, shapes, usecode, a CD of its own) and the inputs
that play it. `run.py` plays each scene twice to its end, checks that the two runs agree and that each of the
scene's tests left its mark in the run's records, and compares the run's states with
`test/synth/expected/<game>.json`.

A scene's docstring has fields, which `docs.py check` (ctest `synth_docs`) holds to their form: what it checks, what
drives it, its tests one by one, and **Original**. A test's entry says what it shows and the values the two games come
to; its footprint checks that result where the records keep it (the state lines' hit points, the usecode's globals in
the trace), not only that the test ran. `Original: Both follow` says that the two original programs,
given the same scene, go through it with the same state as the port at every pass (layer 3 runs this). `Original:
None` gives the reason they cannot: the scene uses what only the port has (its settings screen, its own keys, the
wide screen), or a part the originals play differently (their movie players).

## Changing the kept results

A change meant to keep the games' behaviour keeps every result. A change meant to change it writes the results again
in the same commit and says why:

| Results | Written again by |
|---|---|
| `test/synth/expected/<game>.json` | `python3 port/test/synth/run.py PROGRAM --game GAME --expected FILE --update` |
| `<game>/test/expected/baseline.tsv` | `CRUSADER_GAME=... python3 port/tools/baseline.py record PROGRAM SAVEFIELDS [SCENARIO ...]` |
| `<game>/test/expected/movieframes.tsv` | `CRUSADER_GAME=... python3 port/test/scenario/movieframes.py PROGRAM --write` |
| `<game>/test/expected/terminals.tsv` | `CRUSADER_GAME=... python3 port/test/scenario/terminals.py PROGRAM --write` |

`PROGRAM` is `build/port/crusader-port`; `SAVEFIELDS` is the game's `savefields_test` (`build/port/regret-test/`,
`build/port/remorse-test/remorse_savefields_test`). A change of the port's behaviour that the originals do not share
should also be one the originals' comparison (layer 3) explains before it is merged.

## Against the originals

The comparison with the original programs needs what the repository cannot hold: the games' executables and a
table of where their functions lie, which comes from the recovery of their code. It runs outside the repository, on
the original's machine code in an x86 emulator beside the port:

- the original's functions against the port's, each from the same state;
- the original program run from its start beside the port, the whole game's state compared at each pass of the
  game's loop (on the missions' recordings and on the synthetic scenes);
- the original sound and movie modules (ASYLUM.DLL, XANDRV32.DLL, No Remorse's player) against the port's, on the
  same calls;
- each function's calls against the original's.

The port has these ways in for it, which do nothing without it:

| Way in | What it gives |
|---|---|
| `PORT_EXTRA_TESTS` | More tests, from a CMake file outside the repository ([below](#tests-from-outside-the-repository)) |
| `REGRET_PASS_DUMP=FILE` | The run as records, for a comparer that reads them as the port runs (a FIFO): where the time passes, each interrupt delivered, and at each pass's end the state line and the game's state as the saves' objects ([`../platform/passdump.h`](../platform/passdump.h) gives the records) |
| `run.py --oracle PROGRAM [--symbols TABLE]` | Has PROGRAM follow each scene whose `Original` is `Both follow`: it is run as `PROGRAM --install DIR --cd CD --port PORT --port-install DIR --replay RECORDING --passes N` and its first line beginning `stop ` says how far it followed. The original's `unkcoff.dat` (its intrinsics' addresses) is made from TABLE, the original's functions' places (name, segment and offset a line; by default `symbols_<game>.tsv` beside the oracle's test directory), not taken from the player's files |
| `baseline.py --scenarios FILE` | More scenarios for the baseline, a line each: its name, its script and `savefields` when it takes the savefields test, a tab between |
| `savefields_test SAVES` | Also reads the original's saves in SAVES (`original_newgame.sav` and `saves/*.sav`) into the port's objects and writes them again, the same bytes |
| `usecode.py --symbols TABLE` | Names the intrinsics by the original's addresses (name, segment, offset a line) rather than by `tables/intrinsics.tsv` |

## Tests from outside the repository

`cmake -S port -B build/port -DPORT_EXTRA_TESTS=FILE` includes FILE at the end of `port/CMakeLists.txt`, after the
port's own tests, in that directory's scope. It can use what the port's tests use:

- the targets: `crusader` (the program), the libraries `game_unified`, `asm_unified`, `port_system`, `asylum`, `xan`,
  `compat`, `game_memory`, the objects `port_system_objects`, `unified_values` and `unified_data`, and
  `savefields_test`, `remorse_savefields_test`;
- the functions: `game_target`, `port_options`, `game_files`, `program_run`, `scenario_test`, `test_path`,
  `game_origin`, `release_checked`, `game_speed_slow`;
- the variables: `PYTHON`, `REPOSITORY`, `CRUSADER`, `PORT_LIBRARIES`, `SYSTEM_SOURCES`, `REGRET_PART_SOURCES`,
  `REMORSE_PART_SOURCES`, `GAME_TEST_SOURCES`, `unified_SOURCES`, `unified_ASM_SOURCES`, `unified_INCLUDES`,
  `GAME_ORIGIN_<game>`, `GAME_EXECUTABLE_<game>`, `GAME_DATA_<game>`, `CRUSADER_GAME_<game>`.

Its tests are the top directory's: they get no label and no `CRUSADER_GAME` by themselves. A file that adds a
game's tests sets them (`set_property(TEST ... APPEND PROPERTY LABELS remorse)`, `... ENVIRONMENT
CRUSADER_GAME=noremorse`) and calls `release_checked()` and `game_speed_slow()` after them, or adds a directory of
its own with `add_subdirectory(DIR BINARY_DIR)`, which sees the same scope.
