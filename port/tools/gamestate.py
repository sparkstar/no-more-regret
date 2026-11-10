#!/usr/bin/env python3
"""The game's state in the arena: every writable object of the game's
libraries (the game's, its TASM modules', asylum, xan, and compat's
borland.cpp) is a gameGlobal reference (platform/gamestate.h) its source
defines, so that none of the game's state lies outside the arena; or a
gameSetting reference, a setting the player chose, which is outside it on
purpose.

usage: gamestate.py [--build DIR] [--list] [--check]

Reads the objects of a GCC build (CMakeFiles/<library>.dir) with objdump:
the symbols of their writable sections (.data*,.bss*, not.data.rel.ro*),
each looked for in its source, and in the.cpp files its source includes (the
one program's wrappers), as a reference bound by gameGlobal or gameSetting
(name = gameGlobal<, (&name)[N] = gameGlobal<, Class::name the same). A
function's static, its guard and a reference of another size than a
pointer's are not one. Left out: the compiler's pointer to its exception
personality, and the one program's values, which it chooses before main and
keeps (values_storage.cpp, values.cpp). The data without a name there are
the compiler's: std::source_location's records of the halts' places
(.data.rel.local); a source makes none, with -Wpedantic an error (C's
compound literals).

Each extern declaration of a name a game's source binds by gameGlobal is a
reference too (extern T &name;) in every C++ file of port/: a declaration of
the object's type would read the reference's bytes as the object (the
names are the same, the linker sees nothing: statelog.cpp's processes).

Without --check it reports how many are left, by directory, and exits 0;
--list names each; --check exits 1 when any is left.
"""
import argparse, collections, re, subprocess, sys
from pathlib import Path

PORT = Path(__file__).resolve().parents[1]
LIBRARIES = ('game_unified', 'asm_unified', 'asylum', 'xan', 'compat')
# compat's objects of the game's: Borland's rand.
COMPAT_GAME = {'borland.cpp.o'}
# The one program's values, chosen before main (values.cpp, priority 101)
# and kept: the arena's references are bound after them.
KEPT = {'unified/src/system/values_storage.cpp', 'unified/src/system/values.cpp'}
POINTER = 8
SYMBOL = re.compile(r'^([0-9a-f]+) (.{7}) (\S+)\s+([0-9a-f]+) (.+)$')
INCLUDE = re.compile(r'^#include "([^"]+\.cpp)"', re.M)
DEFINED = re.compile(r'([A-Za-z_][\w:]*)\)?\s*((?:\[[^\]\n]*\]\s*)*)=\s*game(?:Global|Setting)<')
FUNCTION_STATIC = re.compile(r'\((?!anonymous namespace\))[^()]*\)(?: const)?::')


def code(text):
    """text without its comments."""
    return re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)


def symbol_name(m):
    """The name of objdump's symbol line, without the visibility before it
    (.hidden DW.ref.__gxx_personality_v0)."""
    return m.group(5).split()[-1]


def writable(section):
    return section.startswith(('.data', '.bss')) and not section.startswith('.data.rel.ro')


def objdump(*arguments):
    return subprocess.run(['objdump', '-w', *arguments], capture_output=True, text=True, check=True).stdout


def demangled(names):
    if not names:
        return {}
    out = subprocess.run(['c++filt'], input='\n'.join(names), capture_output=True, text=True, check=True).stdout
    return dict(zip(names, out.splitlines()))


def references(source, seen=None):
    """The names a source and the .cpp files it includes bind by
    gameGlobal or gameSetting, without their scopes."""
    seen = set() if seen is None else seen
    if source in seen or not source.exists():
        return set()
    seen.add(source)
    text = code(source.read_text(encoding='latin-1'))
    names = {m.group(1).split('::')[-1] for m in DEFINED.finditer(text)}
    for included in INCLUDE.findall(text):
        names |= references((source.parent / included).resolve(), seen)
    return names


def outside(build):
    """[(source, name, bytes, section, why)] of the writable objects not in
    the arena."""
    rows = []
    for library in LIBRARIES:
        directory = build / 'CMakeFiles' / f'{library}.dir'
        for obj in sorted(directory.rglob('*.o')):
            if library == 'compat' and obj.name not in COMPAT_GAME:
                continue
            relative = obj.relative_to(directory).with_suffix('').as_posix()
            if relative in KEPT:
                continue
            symbols = collections.defaultdict(list)
            for m in map(SYMBOL.match, objdump('-t', str(obj)).splitlines()):
                if m and m.group(2)[6] == 'O' and writable(m.group(3)):
                    symbols[m.group(3)].append((int(m.group(1), 16), int(m.group(4), 16), symbol_name(m)))
            names = demangled(sorted({s[2] for found in symbols.values() for s in found}))
            bound = references(PORT / relative)
            for section, found in symbols.items():
                for _, size, name in found:
                    plain = names[name]
                    if name.startswith('DW.ref.'):
                        continue
                    if name.startswith('_ZGV'):
                        why = 'a guard'
                    elif FUNCTION_STATIC.search(plain):
                        why = "a function's static"
                    elif plain.split('::')[-1] not in bound:
                        why = 'not a gameGlobal or gameSetting reference'
                    elif size != POINTER:
                        why = f'a gameGlobal name of {size} bytes'
                    else:
                        continue
                    rows.append((relative, plain, size, section, why))
    return rows


SOURCES = ('common', 'regret', 'remorse', 'unified', 'system', 'platform', 'compat', 'test')


def kept_names():
    """The names the one program's values define (KEPT): its own variables,
    of the names some of a game's tables have (its ammoTypes is a span the
    game it plays fills, each game's own a reference to the arena's)."""
    names = set()
    for relative in KEPT:
        for line in code((PORT / relative).read_text(encoding='latin-1')).splitlines():
            m = re.match(r'^[A-Za-z_][^;()]*?\b(\w+)\s*(?:\[[^\]]*\])?\s*(?:=[^;]*)?;\s*$', line)
            if m:
                names.add(m.group(1))
    return names


def plain_declarations():
    """[(file, line, name)] of the extern declarations, not references, of
    the names the game's sources bind by gameGlobal; the one program's own
    values left out in its files (unified/src)."""
    files = [f for d in SOURCES for f in sorted((PORT / d).rglob('*')) if f.suffix in ('.cpp', '.h')]
    bound = set()
    for f in files:
        bound |= {m.group(1).split('::')[-1] for m in DEFINED.finditer(code(f.read_text(encoding='latin-1')))}
    kept = kept_names()
    found = []
    for f in files:
        unified = f.relative_to(PORT).as_posix().startswith('unified/')
        for number, line in enumerate(code(f.read_text(encoding='latin-1')).splitlines(), 1):
            m = re.match(r'\s*extern\s+(?!"C")([^;=(]*?)[\s*]([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*;', line)
            if m and m.group(2) in bound and '&' not in m.group(1) and not (unified and m.group(2) in kept):
                found.append((f.relative_to(PORT).as_posix(), number, m.group(2)))
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--build', type=Path, default=PORT.parent / 'build/port')
    ap.add_argument('--list', action='store_true', help='name each object outside the arena')
    ap.add_argument('--check', action='store_true', help='fail when any is outside')
    args = ap.parse_args()
    rows = outside(args.build)
    plain = plain_declarations()
    # Each source once (the common code is in three libraries).
    distinct = sorted({(source, name): (size, section, why) for source, name, size, section, why in rows}.items())
    if args.list or args.check:
        for (source, name), (size, section, why) in distinct:
            print(f'MISMATCH gamestate: {source} {name} ({size} bytes, {section}): {why}')
    for file, number, name in plain:
        print(f'MISMATCH gamestate: {file}:{number}: extern {name}, not a reference to the arena\'s')
    by_directory = collections.Counter(str(Path(source).parent) for (source, _), _ in distinct)
    for directory, count in sorted(by_directory.items()):
        print(f'  {directory}: {count}')
    total = sum(size for (_, _), (size, _, _) in distinct)
    print(f'gamestate: {len(distinct)} writable objects of the game outside the arena ({total} bytes) '
          f'in {len({s for (s, _), _ in distinct})} files')
    if plain:
        print(f'gamestate: {len(plain)} extern declarations of the arena\'s globals not references')
    return 1 if (args.check and distinct) or plain else 0


if __name__ == '__main__':
    sys.exit(main())
