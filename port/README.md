# The port's sources

English | [한국어](README.ko.md)

Everything of the port is under `port/`: one CMake project (`CMakeLists.txt`) that builds the program, its libraries
and its tests. The program's target is `crusader`, its file `crusader-port` (`crusader-port.exe` on Windows). How to
build it and run it: the repository's [`README.md`](../README.md).

## Layout

| Directory | What it holds |
|---|---|
| `common/src/<group>` | The game code both games share, in subsystem groups (below). All of the games' code is here once |
| `regret/src/<group>`, `remorse/src/<group>` | Each game's values and data: the value headers `<name>_game.h` the shared code takes per game, and the game's own tables and layouts (below) |
| `unified/src/<group>` | The one program's run-time choice of a game's values: the headers `tools/make_unified_values.py` writes from the value headers, and the wrappers that compile each game's data files in a namespace of the game's (`values_regret.cpp`, `values_remorse.cpp`) |
| `common/asylum` | The sound module, ASYLUM.DLL (the Asylum Sound System), in C++: a library of its own, over the platform's sound output |
| `common/xan` | No Regret's movie decoder, XANDRV32.DLL, in C++: a library of its own |
| `common/fonts` | The Hangul font, `ko.png` (Galmuri9, [`OFL.md`](common/fonts/OFL.md)), from which `tools/make_text.py` writes `common/src/text/hangul_glyphs.cpp` |
| `system` | The PC under the game, which the game's libraries run on (below) |
| `platform` | The system layer's interfaces to the game code: `audio.h`, `clock.h`, `files.h`, `gamestate.h`, `host.h`, `input.h`, `memory.h`, `passdump.h`, `screen.h`, `trace.h` |
| `compat` | Borland C++ 3.1's library and the DOS and Phar Lap headers the game code includes (`alloc.h`, `conio.h`, `dir.h`, `dos.h`, `io.h`, `mem.h`, `phapi.h`), `borland.h` (included before each of the game's files), the crash reports (`crash.h`) and the port's version (`portversion.h`) |
| `<game>/tables` | `intrinsics.tsv`, the usecode's intrinsics (number, name, module, argument and return bytes), which `tools/usecode.py intrinsics` checks the usecode's calls against |
| `lang` | The translations ([`lang/README.md`](lang/README.md)); the Korean glossary and story in [`lang/ko`](lang/ko) (in Korean) |
| `test`, `<game>/test` | The tests ([`test/README.md`](test/README.md)): `unit/` parts alone, `scenario/` the program on scripted input, `synth/` the synthetic scenes, `<game>/test/expected` the kept results of a game's scenarios |
| `tools` | Generators, checks and helpers ([`tools/README.md`](tools/README.md)) |
| `cmake` | The MinGW-w64 toolchain (`mingw-w64-x86_64.cmake`) and `wine.sh`, which runs the Windows tests under wine |

## The game's code

The game's code follows the original programs function by function: the C++ of the originals' sources, recovered
from their machine code, with the original's names where they are known. It keeps the originals' dialect (Borland
C++ 3.1's library from `compat`, `char` signed, the structures laid out as the originals' so that the saves are the
originals'), and its globals are references into the game's arena (`platform/gamestate.h`), which the snapshots and
the rewind take whole.

### The subsystem groups

Each file of the game's code is in one group, the same in every tree (`tools/overlay.py`'s `GROUPS`):

| Group | What it holds |
|---|---|
| `kernel` | The process kernel and its processes, the lists, the timers, the waits |
| `input` | The keyboard, the mouse, the joystick, the input's dispatch and the port's bindings |
| `usecode` | The usecode's interpreter, its intrinsics, memory, searches and processes |
| `item` | The items, the NPCs, the eggs, the item cache, collisions, distances, gravity |
| `game` | The controller, combat, animations, activities, attacks, missiles, path finding, sprites |
| `world` | The world, the game's start and main loop, the game's time, map jumps, the options, the saves, the port's `enhanced.cfg` |
| `gump` | The gumps: dialogs, buttons, menus, the load and save gump, the terminals, credits; the port's settings and key settings screens and their layout |
| `display` | The display list, the sprite manager, the camera, the wide screen's sides |
| `graph` | Palettes, fades, colour cycling, shapes and their drawing, viewports, the VGA and VESA screens, BMP and Targa |
| `sound` | The sound and music managers, the sound cache, the sound module's interface |
| `movie` | The movies (No Remorse's RRV movies, No Regret's Xan AVIs) and their subtitles, the game's flics (`FLICPLAY.C`) |
| `text` | The languages, the translations, Hangul |
| `file` | Files and FLEX archives |
| `cache` | The caches and their handles, the shapes' cache |
| `system` | The game's own system parts: halts, random numbers, utilities, the memory manager (`CPROTMEM.C`) over the host's heap |

The TASM modules of the originals (`ASM_MODULES` in `CMakeLists.txt`: the shape drawers, `view32`, `svgaprim`,
`fastfind`, `framehit`, `cacheblk`, `random`, `bitfield`, `xanthunk`) are C++ files in their groups, built as a library
of their own (`asm_unified`), which the tests link without the rest of the game.

### Two games, one program

The two games' differences are not split with preprocessor conditions such as `#ifdef`: one program chooses its
game at run time.

- The code is in `common/src` once. Where the games differ, it reads a value of the game's.
- A game's values are the `constexpr` declarations of its value headers (`<game>/src/**/<name>_game.h`,
  `system/profile.h`, `usecode/searches.h`). `tools/make_unified_values.py` writes, for each value both games have,
  a variable of the same name in `unified/src`, which the game chosen at the start fills (`unified/src/system/values.cpp`).
  After a change of a value header, run it again (ctest `unified_values_current` checks).
- A game's data are the `.cpp` files of its tree: the usecode's intrinsics table (`intrinsics.cpp`), the save's
  layout (`savefields.cpp`), the keys' defaults (`bindings_game.cpp`), the translations (`translations.cpp`, written
  by `tools/make_text.py`), the controller's saved globals (`control_game.cpp`), and the tables of the weapons,
  their ammunition and the inventory items (`dtable_game.cpp`), of the weapons' overlays (`wpncache_game.cpp`) and of
  the fire types (`firetype_game.cpp`).
- The game is chosen before anything else starts: `CRUSADER_GAME`, else the game of the folder the program is in (the
  original's executable there), else the last one played (`enhanced.cfg`'s `game=`), else No Remorse. `--game` runs
  the program again for its game (`system/restart.h`).

`tools/overlay.py --check` (ctest `overlay`) keeps to the rules: each file in its group's directory, in one place
only (an `#include "x.h"` looks beside the including file first), no file of both games' trees with the same bytes
(it belongs in common), no preprocessor condition on a game's name, no header named as one in the other directories
on the include path, no include guard two headers share.

## The system layer

`system/` is the PC the game runs on, under the interfaces of `platform/`:

| Files | What they are |
|---|---|
| `pc.h`, `pcstate.*`, `interrupts.cpp`, `timer.cpp`, `memory.cpp` | The virtual PC: the virtual time and the game's timer ticks, the interrupt controllers' order of events, the memory (16 MB, as DOSBox's), the PC's state as a value |
| `keyboard.cpp`, `mouse.cpp`, `mousedrv.h` | The keyboard's scan codes, the mouse and its driver (INT 33h) |
| `video.*`, `console.cpp`, `audio.cpp` | The screen's modes and the DAC's colours, the text screen of the start (on the standard output), the sound's output |
| `window.*`, `nowindow.cpp` | The SDL window: the picture, the sound's stream, the input's events, the gamepads; without a window, the scripts' runs |
| `files.cpp`, `filestate.h`, `iso9660.*`, `gamefolder.*`, `hostpath.h` | The DOS files on the host, the CD read from its image (`GAME.GOG`, `GAME.BIN`) or a directory, the game's folder |
| `commandline.*`, `main.cpp`, `end.*`, `restart.*` | The program's options, its start and end, the run of the other game |
| `replay.*`, `script.*`, `rewind.*`, `snapshot.*`, `capture.*`, `nut.*`, `png.*` | Recordings and their replays, the test scripts, the rewind and the states, the capture of a replay's picture and sound (NUT), the PNG writer |
| `report.*`, `unimplemented.*` | How a run ends: the messages, the box over the window and `regret.log` (the crash report itself is `compat/crash.h`'s), what the port cannot go on from |
| `trace.cpp`, `statelog.cpp`, `movieframes.cpp`, `passdump.cpp`, `passstate.cpp` | What the tests read: the usecode's trace, the state of each pass, the movies' frames, the pass dump for the comparison with the originals |
| `gamememory.*`, `newdelete.cpp` | The game's arena and the program's `new` and `delete` |

## The build's targets

| Target | What it is |
|---|---|
| `game_unified`, `asm_unified` | The game's code of both games (`common/src` and `unified/src`), and its TASM modules |
| `asylum`, `xan` | The sound module and the Xan decoder |
| `compat`, `game_memory` | Borland's library, the crash reports and the version; the game's arena |
| `port_system` | The system layer without the program's main, the window and the arena |
| `crusader` | The program, `crusader-port` |
| `boundary` | The game's boundary held to its rules (`tools/boundary.py`), before `crusader` links; `PORT_BOUNDARY_CHECK` |
| the tests | `test/CMakeLists.txt` and `<game>/test/CMakeLists.txt`; a game's build directory is `<game>-test` |

`-DPORT_EXTRA_TESTS=FILE` includes a CMake file of more tests after the port's own: the comparison with the original
programs plugs in there ([`test/README.md`](test/README.md#against-the-originals)).

## The game's boundary and the tables kept

### The game's boundary (`tools/boundary.py`)

The symbols the game's libraries (`game_unified`, `asm_unified`, `asylum`, `xan`) take from outside themselves are
held to rules. The build runs the check before the program links (the target `boundary` in `CMakeLists.txt`); a
symbol that breaks the rules stops the build, and `crusader-port` is not linked. It runs in every build: GCC, Clang,
the sanitizers, the coverage, MinGW-w64 and macOS.

| What is allowed | What it is | Now (GCC) |
|---|---|---|
| platform | The functions the headers of `port/platform` declare, which the system layer gives. The game reaches the host through these alone | 125 |
| library | The functions of `compat` the port keeps (`KEPT`: Borland's string functions and `rand`, the crash report) | 7 |
| runtime | The functions of the C and C++ runtimes with no effect on the host (`PURE`: memory, strings, characters, formatting into a buffer, parsing, `new` and `delete`; `PURE_STD`: the C++ strings and standard exceptions libc++ keeps out of its headers) | 27 |
| the compiler's | Exception handling, RTTI, stack protection, thread-local storage, the sanitizers' and the coverage's instrumentation, the C++ library's internal functions | not counted |

- Everything else breaks the rules: a symbol of the system layer that the platform does not declare (the game
  reaching the virtual PC directly), a `compat` function not in `KEPT`, a runtime function not known to be without
  effects (files, the standard output, environment variables, the clock, the process). The check names the symbol
  and the game's objects that use it.
- The fix: give the game a platform function and go through it. A function without effects goes into `PURE` in
  `boundary.py`.
- The toolchains name the same symbol differently, so the names are made one before the rules see them: c++filt
  demangles the C++ names; Mach-O's leading underscore, `std::__1` and `std::__cxx11`, and MinGW-w64's `__imp_` and
  `__mingw_` are dropped; `__memcpy_chk` or `__isoc23_sscanf` count as the plain function, `puts` as `printf`, and
  Clang's `bcmp` as `memcmp`'s. Neither the build nor the objects change: this is done for the comparison only.
- For a move to another host (libretro, the browser), the platform functions are the list of what the new host has to
  give. `python3 port/tools/boundary.py --list` shows every symbol with its state.
- To put a `printf` into the game code for a while when debugging, `-DPORT_BOUNDARY_CHECK=OFF` turns the check off.

### `<game>/tables/intrinsics.tsv`

The usecode intrinsics' table: their numbers, names, modules, and the bytes of their arguments and results. ctest
`usecode_intrinsics` checks every call of an intrinsic in the game data's usecode against it (only with the game
data). The table was set by the originals and hardly changes.

### Files not edited by hand

| File | What writes it | Its check |
|---|---|---|
| `unified/src/**` (but `system/values.cpp` and `system/values.h`) | `tools/make_unified_values.py` | ctest `unified_values_current` |
| `common/src/text/hangul_glyphs.cpp`, `<game>/src/text/translations.cpp` | `tools/make_text.py` (from the font and the translations in `lang/`) | ctest `text` |
| `tools/exceptions.tsv` | `tools/exceptions.py --write` (the list of the rules' exceptions) | ctest `exceptions` |
| `<game>/src/world/savefields.cpp`, `<game>/src/usecode/searches.h`, `<game>/src/usecode/intrinsics.cpp` | Made from the originals' data (BCC's structure layouts, the originals' vtables, `UNKCOFF.DAT`). The generators need the originals, so they are not in this repository | ctest `savefields`, `usecode_intrinsics` |
