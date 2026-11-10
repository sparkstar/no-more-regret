#!/usr/bin/env python3
"""The port's trees for two games: which of the game's files both games
share and which each has its own, and the subsystem group each file lies
in; and, for the other tools, a game's files by name and its CD's files.

usage: overlay.py [--check]

A game's sources are port/common/src and port/<game>/src: what both games
have the same is in common, what differs is in each game's tree, in the same
subsystem groups (GROUPS). A file is in one place only: one that a game
changes leaves common for both games' trees, since an #include "x.h" looks
beside the including file first and would find a common x.h there before a
game's. The sound module (common/asylum) and the Xan decoder (common/xan)
are libraries of their own (LIBRARIES).

Without arguments prints the files of the trees by group (common / No
Regret / No Remorse).
--check fails (exit 1) and names each file of the trees that breaks the
rules: a file outside its group's directory or of no group, a name twice
in a game, a file both in common and in a game's tree, a file of both
games' trees with the same bytes (it belongs in common), a preprocessor
condition on a game's name (no #ifdef by game), a header of the trees named
as one of the other directories on the include path (an include in quotes
finds the first: video.h), an include guard two headers of a game's build
share.
"""
import argparse, collections, os, re, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / 'port'
GAMES = ('regret', 'remorse')
TREES = ('common', *GAMES)
# The game the tools work on: CRUSADER_GAME (regret, the default, or
# noremorse); the port's directories are regret and remorse.
CRUSADER_GAMES = {'regret': 'regret', 'noremorse': 'remorse'}
_NAME = os.environ.get('CRUSADER_GAME') or 'regret'
assert _NAME in CRUSADER_GAMES, f'CRUSADER_GAME={_NAME}: not one of {", ".join(CRUSADER_GAMES)}'
GAME = CRUSADER_GAMES[_NAME]
# Each game's CD: the player's own files in cd/<game>.
ORIGINS = {'regret': 'cd/noregret', 'remorse': 'cd/noremorse'}
ORIGIN = ROOT / ORIGINS[GAME]
# The game's tables of the tools (port/<game>/tables) and its tests
# (port/<game>/test).
TABLES = PORT / GAME / 'tables'
TESTS = PORT / GAME / 'test'
# Each game's libraries outside the game's sources (CMake's targets): the
# sound module, both games', and the Xan decoder, No Regret's movies'.
LIBRARIES = {'regret': ('common/asylum', 'common/xan'), 'remorse': ('common/asylum', 'common/xan')}
# The groups, by the port's file names without suffix. A file of either game's lies in its module's group.
GROUPS = {
    'kernel': 'kernel kprocess process kdaemon priority mlist notlist sublist queue stdinter stdint destroy intctl '
              'systimer dumbtimr biostimr waits',
    'input': 'keyboard keyio keypad keyqueue mouse mouseque mousectl u9mouse joydrv joystick dispatch bindings devices '
             'bindings_game',
    'usecode': 'inter intrinsics usemem usesearch searches uprocess yamm flag debugger unitfile',
    'item': 'item itemcach itemdam itemdata itemfind itemutil npc egg globs fixedflx refstore scrit preload quantity '
            'glob type alert fastfind tracker collide distance box hittest blocking framehit gravity',
    'game': 'control crusader actanim activity animprim animcach animfile animfrm astream animinit attack boboboom '
            'explode firetype missile pathfind sprite wpncache dtable anchor shapes',
    'world': 'world crusys gamemain gametime mapjump options savegame saveimage savefields gamefile enhanced help',
    'gump': 'newgump dialog button textbtn shapebtn shapedlg draggabl dragitem gumpdraw gumpfont gumpshap gumplist termgump '
            'msgbox compgump mainmenu loadsave status cafont credits bark irgump wezzy format layout widgets settings keysgump',
    'display': 'display dlist dlitem dlgrid spmgr vframe camera basecam snap widesides',
    'graph': 'basepal palette gray gamepal fade cycle rect shape shp32 shpm32 shpmr32 shpmt32 shpr32 shpt32 drawframe '
             'pixels supervga svgaprim view32 viewport vport x32code bmp targa',
    'sound': 'asylum assproc amfload music sndcache sndmgr sndsrc dsfxman audstrm audconv sounds',
    'movie': 'movie moviehost subtitles flicplay xanthunk rrv bitfield avifile videodecoder xanvideo',
    'text': 'language translate hangul hangul_glyphs translations utf8',
    'file': 'file fastflex filepars filespec flex flexedit tagflex',
    'cache': 'cache cacheblk cachehnd flexhand handler shaphand',
    'system': 'system halt random utility common diskfree hyperdsk cprotmem desc linklist pfloat huge profile',
}
GROUP = {module: group for group, modules in GROUPS.items() for module in modules.split()}
# The include path's directories besides the trees (CMakeLists.txt's).
INCLUDE_DIRECTORIES = ('compat', 'common/asylum', 'common/xan', 'platform', 'system')
GUARD = re.compile(r'^#ifndef (\w+_H)$', re.M)
GAME_MACRO = re.compile(r'^\s*#\s*(?:if|ifdef|ifndef|elif)\b.*\b(?:NO)?RE(?:GRET|MORSE)\w*\b', re.M)


def origin_path(relative, origin=ORIGIN):
    """The file of a game's CD at relative (as 'USECODE/EUSECODE.FLX'),
    each part in whatever case the CD has it (No Remorse's is in lower
    case); as named where there is none."""
    path = origin
    for part in Path(relative).parts:
        found = next((entry for entry in path.iterdir() if entry.name.upper() == part.upper()), None) \
            if path.is_dir() else None
        path = found if found is not None else path / part
    return path


def group(name):
    """The group of a file name, or None; a game's part of a file
    (<name>_game.h) is in the file's."""
    return GROUP.get(Path(name).stem.lower().removesuffix('_game'))


def source_dirs(game=GAME):
    """The directories of a game's sources: its own groups, then common's."""
    return [PORT / tree / 'src' / name for tree in (game, 'common') for name in GROUPS
            if (PORT / tree / 'src' / name).is_dir()]


def include_dirs(game=GAME):
    """The directories of a game's headers: its sources', its libraries'."""
    return source_dirs(game) + [PORT / library for library in LIBRARIES[game]]


def roots(game=GAME):
    """The roots of a game's files: its tree, common's, its libraries'."""
    return [PORT / game / 'src', PORT / 'common/src', *(PORT / library for library in LIBRARIES[game])]


def targets():
    """CMake's libraries of the game's sources (CMakeLists.txt), the one
    program's: the game's, its TASM modules', and its libraries' of their
    own."""
    return ('game_unified', 'asm_unified', *(Path(library).name for library in LIBRARIES[GAME]))


def files(game=GAME, libraries=False):
    """{lower-case name: path} of a game's sources (.cpp and .h), with its
    libraries' too when asked."""
    directories = source_dirs(game) + ([PORT / library for library in LIBRARIES[game]] if libraries else [])
    found = {}
    for directory in directories:
        for path in sorted(directory.rglob('*')):
            if path.suffix in ('.cpp', '.h'):
                found.setdefault(path.name.lower(), path)
    return found


def sources(game=GAME):
    """The .cpp files of a game's sources, by path."""
    return sorted(path for path in files(game).values() if path.suffix == '.cpp')


def find(name, game=GAME):
    """The path of a game's source file by its name."""
    path = files(game, libraries=True).get(name.lower())
    if path is None:
        sys.exit(f'overlay: no {name} in the sources of {game}')
    return path


def tree_files():
    """{tree: {lower-case name: path}} of the three trees now."""
    out = {}
    for tree in TREES:
        root = PORT / tree / 'src'
        out[tree] = {path.name.lower(): path for path in sorted(root.rglob('*')) if path.is_file()} \
            if root.exists() else {}
    return out


def check():
    """The breaches of the trees' rules."""
    problems = []
    for tree in TREES:
        root = PORT / tree / 'src'
        names = {}
        for path in sorted(root.rglob('*')) if root.exists() else ():
            if not path.is_file():
                continue
            relative = path.relative_to(root)
            name = path.name.lower()
            where = group(name)
            if where is None:
                problems.append(f'{path.relative_to(PORT)}: no group')
            elif len(relative.parts) != 2 or relative.parts[0] != where:
                problems.append(f'{path.relative_to(PORT)}: not in {tree}/src/{where}')
            if name in names:
                problems.append(f'{path.relative_to(PORT)}: also {names[name].relative_to(PORT)}')
            names[name] = path
    trees = tree_files()
    for game in GAMES:
        for name, path in trees[game].items():
            if name in trees['common']:
                problems.append(f'{path.relative_to(PORT)}: also {trees["common"][name].relative_to(PORT)}')
    for name, path in trees['regret'].items():
        other = trees['remorse'].get(name)
        if other is not None and path.read_bytes() == other.read_bytes():
            problems.append(f'{path.relative_to(PORT)}: the same as {other.relative_to(PORT)} (common)')
    others = {path.name.lower(): path for directory in INCLUDE_DIRECTORIES
              for path in sorted((PORT / directory).rglob('*.h'))}
    for tree in TREES:
        for name, path in trees[tree].items():
            if name in others:
                problems.append(f'{path.relative_to(PORT)}: also {others[name].relative_to(PORT)} on the include path')
    for game in GAMES:
        guards = {}
        headers = [path for directory in INCLUDE_DIRECTORIES for path in sorted((PORT / directory).rglob('*.h'))]
        headers += [path for tree in ('common', game) for path in trees[tree].values() if path.suffix == '.h']
        for path in headers:
            for guard in GUARD.findall(path.read_text(errors='replace')):
                if guard in guards and guards[guard] != path:
                    problems.append(f'{path.relative_to(PORT)}: guard {guard} also {guards[guard].relative_to(PORT)}')
                guards[guard] = path
    for directory in (*TREES, 'common/xan', 'common/asylum', 'system', 'platform', 'compat', 'test'):
        for path in sorted((PORT / directory).rglob('*')) if (PORT / directory).exists() else ():
            if path.suffix in ('.cpp', '.h') and GAME_MACRO.search(path.read_text(errors='replace')):
                problems.append(f'{path.relative_to(PORT)}: a condition on a game')
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.check:
        problems = check()
        for problem in problems:
            print(problem)
        print(f'overlay: {len(problems)} problems')
        return 1 if problems else 0
    trees = tree_files()
    counts = collections.defaultdict(collections.Counter)
    for tree in TREES:
        for name in trees[tree]:
            counts[group(name)][tree] += 1
    print('group     common regret remorse')
    for name in GROUPS:
        count = counts[name]
        print(f'{name:9} {count["common"]:6} {count["regret"]:6} {count["remorse"]:7}')
    print(f'{"total":9} {sum(c["common"] for c in counts.values()):6} '
          f'{sum(c["regret"] for c in counts.values()):6} {sum(c["remorse"] for c in counts.values()):7}')
    unplaced = sorted(name for tree in TREES for name in trees[tree] if group(name) is None)
    for name in unplaced:
        print(f'{name}: no group')
    return 1 if unplaced else 0


if __name__ == '__main__':
    sys.exit(main())
