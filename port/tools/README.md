# Tools

The port's generators, checks and helpers. They are Python 3 scripts, run from the repository's root
(`python3 port/tools/NAME.py`). The game they work on is `CRUSADER_GAME`'s: `regret` (the default) or
`noremorse`. Each one's docstring says more (`python3 port/tools/NAME.py --help`).

## Format and rules

| Tool | What it does | Run by |
|---|---|---|
| `format.py` | Formats the port's C++ files: clang-tidy 20's fixes, then clang-format 20 (`port/.clang-format`). `--check --all` changes nothing and names the files out of the format; `--staged` checks what is staged for a commit | ctest `format`; the hook `hooks/pre-commit` |
| `tidy.py` | Runs clang-tidy 20 with `port/.clang-tidy` on the port's files, with a build's compile commands; `--fix` applies the fixes the rules have | ctest `tidy` (a Clang 20 build only); `format.py` |
| `exceptions.py` | Checks that each exception to the rules (a `NOLINT`, a warning turned off at one place) is in its place with the rule's name and its reason, as `exceptions.tsv` lists them; `--write` writes the list | ctest `exceptions` |
| `overlay.py` | Knows the two games' source trees: which files both games share (`common/src`) and which are a game's own, and each file's subsystem group. `--check` names a file out of its group's place, a name twice, a file in both trees, two games' files with the same bytes (they belong in common), an `#ifdef` on a game, an include guard twice. The other tools take the game's files and its CD's from it | ctest `overlay` |
| `gamestate.py` | Checks that every writable object of the game's libraries is a `gameGlobal` reference (`platform/gamestate.h`), so that the game's whole state lies in its arena, where the snapshots and the rewind take it | ctest `gamestate` (GCC builds) |
| `boundary.py` | Holds what the game's libraries take from outside themselves to its rules: a function `port/platform` declares, a function of `port/compat` the port keeps, a function of the C and C++ runtime without side effects on the host, or the compiler's own; anything else (the system layer's past the platform, a file, the standard output, the environment, the clock) stops the build, named with its users. `--list` prints every symbol | the build, before the program links (`PORT_BOUNDARY_CHECK`, on every build) |

The hook: `git config core.hooksPath port/tools/hooks` runs `format.py --staged` before each commit.

## Generators

Their output is committed with the change that changes it; `--check` fails when the files differ from what they would
write.

| Tool | What it writes | Run by |
|---|---|---|
| `make_unified_values.py` | `unified/src`'s value headers: for each value the games' `<name>_game.h` give, the one program's run-time choice of the game's | ctest `unified_values_current` |
| `make_text.py` | The Hangul glyphs (`common/src/text/hangul_glyphs.cpp`, from `common/fonts/ko.png`) and each game's translations (`<game>/src/text/translations.cpp`, from `port/lang`) | ctest `text` |

## The games' data

These read the files of a game's CD (`cd/<game>`, [`../../cd/README.md`](../../cd/README.md)).

| Tool | What it does | Run by |
|---|---|---|
| `install.py DIR` | Installs a game for the port as the original's installer does: a copy of the CD's `STATIC` and `USECODE`, an empty `SAVEGAME` and a `crusader.cfg`, the CD read from where it is | the scenario tests |
| `usecode.py` | Disassembles the usecode (`EUSECODE.FLX`) into the interpreter's tokens. `check` decodes every function to its end; `pointers` checks where the usecode's pointers lie; `intrinsics` checks each call of an intrinsic against `<game>/tables/intrinsics.tsv`; `loops` checks the loops' searches; `list`, `dis CLASS` and `stats` print; `coverage TRACE` counts what a usecode trace ran | ctest `usecode*` |
| `usecode_text.py` | Writes every string of the usecode, with its place, where it shows (`terminal`, `movie`, ...) and the pieces it is joined with, as JSON for translating (`build/text/<game>_usecode_text.json`); `--check` checks that every string's use is known | ctest `usecode_text` |
| `subtitle_text.py` | Writes every line of the movies' subtitles as the port reads them, with its frames and hash, as JSON for translating (`build/text/<game>_subtitles.json`) | by hand ([`../lang/README.md`](../lang/README.md)) |
| `baseline.py` | The baseline: the states, screens, sounds and saves the scenario tests keep, by name, size and SHA-256, against `<game>/test/expected/baseline.tsv`. `scenario NAME` runs one scenario as its test; `check` compares; `record` writes the file again after a change meant to change them. `--scenarios FILE` adds scenarios kept outside the repository | ctest `milestone`, `movies`, ..., `baseline` |
| `replays.py PROGRAM` | Replays the player's recordings under `replays/` (as `--record` makes them, a debug feature: with `--debug-feature`) without a window and compares their states with those they hold; `--accept` takes the new states after a change meant to change the play; `--wide` replays them with the wide screen | by hand |

## Changes that change nothing

| Tool | What it does | Run by |
|---|---|---|
| `object_compare.py` | Builds the port's objects in three GCC configurations (with the window, without it, MinGW) with each function in a section of its own, and compares them by function before and after a change meant to change no code (the format, a rename, a mechanical fix): `save` before, `check` after. A function only renamed counts as the same | by hand |
| `coverage.py` | Builds the port with GCC's `--coverage`, runs the tests and counts the lines, the functions, the intrinsics and the interpreter's opcodes they reach | by hand |
| `game_diff.py` | Lists the functions the two games' trees (`regret/src`, `remorse/src`) still each have, and how much they differ. The two games' code is one now, so it finds none; it is there for a change that brings a game's own code back | by hand |

## Test scripts

The tests' own scripts lie beside the tests (`port/test`, `port/<game>/test`). Those of the synthetic scenes:

| Script | What it does |
|---|---|
| `test/synth/run.py PROGRAM --game GAME` | Makes each scene's data, plays it twice to its end and compares with `test/synth/expected/<game>.json`; `--update` writes them again. `--oracle PROGRAM` has a program outside the repository follow each scene with the original |
| `test/synth/docs.py check` | Checks each scene's docstring: its fields, and that each test of the scene is under one entry of its `Tests` (ctest `synth_docs`); `list` prints them |
| `test/synth/wide_sides.py PROGRAM --game GAME` | The wide screen's sides under the movies and the credits, at frames of the scenes |
| `test/synth/coverage.py` | What of the engine's code the synthetic scenes run and the games' own data runs, and the difference |
