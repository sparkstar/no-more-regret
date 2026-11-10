#!/usr/bin/env python3
"""Writes the one program's run-time value headers.

Each game's values are the constexpr declarations of its value headers
(<game>/src/**/<name>_game.h, system/profile.h, usecode/searches.h). The
program's build compiles the common code with the headers this writes under
unified/src: for each value both games have, a variable of the same name,
which the game chosen at the start fills from its values (values_<game>.cpp,
the game's own declarations in a namespace of its own). A value whose two
games' arrays differ in size is a span; a <NAME>_CAPACITY, an array's room,
stays a constant, the larger of the two. The values only one game has (its
shapes) only fill that game's other values and get no variable. Each game's
data files are compiled in its namespace by a file of their own, which points
the program's tables at the game's at the start; values_data.cpp's calls link
them into the program (CMakeLists.txt's unified_data), and a test of a part
alone takes the values without them.

usage: make_unified_values.py [--check]"""
import argparse
import re
import sys
from pathlib import Path

from format import binary, formatted

ROOT = Path(__file__).resolve().parent.parent
GAMES = ('regret', 'remorse')
NAMESPACE = {'regret': 'regret_values', 'remorse': 'remorse_values'}
OUT = ROOT / 'unified' / 'src'
HEAD = '/* Written by port/tools/make_unified_values.py from {sources}: {what}; do not edit. */\n'


def value_headers(game):
    """The game's value headers by their path under <game>/src."""
    base = ROOT / game / 'src'
    paths = sorted(base.rglob('*_game.h')) + [base / 'system' / 'profile.h', base / 'usecode' / 'searches.h']
    return {str(p.relative_to(base)): p for p in paths}


def strip_comments(text):
    return re.sub(r'/\*.*?\*/', '', text, flags=re.S)


class Value:
    """A value's declaration: its type, name, initializer and the text that
    declares it in the game's header (without comments)."""

    def __init__(self, typ, name, init, text):
        self.typ = typ
        self.name = name
        self.init = init
        self.text = text
        self.array = False

    @property
    def extern(self):
        return self.init is None


def constexpr_declarations(text):
    """Each inline constexpr declaration of text, without its ;, to the ;
    outside its brackets (a lambda's body in a table has its own:
    remorse/src/world/savegame_game.h)."""
    out = []
    for m in re.finditer(r'^inline constexpr\b', text, flags=re.M):
        depth, i = 0, m.start()
        while i < len(text):
            c = text[i]
            if c in '([{':
                depth += 1
            elif c in ')]}':
                depth -= 1
            elif c == ';' and depth == 0:
                break
            i += 1
        out.append(text[m.start():i].strip())
    return out


def parse(path):
    """The includes and the values of a value header."""
    text = strip_comments(path.read_text())
    includes = [l.strip() for l in text.split('\n') if l.strip().startswith('#include')]
    values = {}
    pattern = r'^inline constexpr\s+([^;=]+?)\s*\b(\w+)\s*(\[[^\]]*\])?\s*(?:=\s*(.*)|(\{.*\}))$'
    for declaration in constexpr_declarations(text):
        m = re.match(pattern, declaration, flags=re.S)
        if not m:
            raise SystemExit(f'{path}: a value not read: {declaration[:80]}')
        typ = ' '.join(m.group(1).split())
        if m.group(3):
            typ += m.group(3)
        init = ' '.join((m.group(4) or m.group(5)).split())
        values[m.group(2)] = Value(typ, m.group(2), init, ' '.join(m.group(0).split()) + ';')
    # Only constant values: the game's fill the one program's before the
    # other static objects start (values.cpp), so they must be there
    # already.
    for m in re.finditer(r'^inline (?!constexpr)[^;(]*?\b(\w+)\s*[(=;]', text, flags=re.M):
        raise SystemExit(f'{path}: {m.group(1)} is not constexpr')
    # The game's tables and their counts, which its data files define. A table is a
    # reference to its place in the arena: extern T (&name)[N]; or extern T &name;.
    for m in re.finditer(r'^extern\s+([^;=(&]+?)\s*(?:\(\s*&\s*(\w+)\s*\)|&?\s*\b(\w+))\s*(\[[^\]]*\])?\s*;\s*$', text,
                         flags=re.M):
        typ = ' '.join(m.group(1).split())
        name = m.group(2) or m.group(3)
        values[name] = Value(typ, name, None, ' '.join(m.group(0).split()))
        values[name].array = m.group(4) is not None
    return includes, values


ARRAY = re.compile(r'^std::array<(.+),\s*(\w+)>$')


def unified_type(name, regret, remorse):
    """The variable's type in the one program, or None for a constant."""
    if name.endswith('_CAPACITY'):
        return None
    if regret.typ == remorse.typ:
        return regret.typ
    a, b = ARRAY.match(regret.typ), ARRAY.match(remorse.typ)
    if a and b and a.group(1) == b.group(1):
        return f'std::span<{a.group(1)} const>'
    raise SystemExit(f'{name}: the games\' types differ ({regret.typ} / {remorse.typ}) and are not two arrays')


def number(values, name, seen=()):
    """A _CAPACITY's base as a number (its game's literal or another value's)."""
    init = values[name].init
    if re.fullmatch(r'0[xX][0-9a-fA-F]+|\d+', init):
        return int(init, 0)
    if init in values and init not in seen:
        return number(values, init, seen + (name,))
    raise SystemExit(f'{name}: {init} is not a number')


def ordered(values):
    """The game's constant values with each after the values its initializer
    names."""
    values = {n: v for n, v in values.items() if not v.extern}
    names = set(values)
    done, out = set(), []

    def visit(name, stack):
        if name in done:
            return
        if name in stack:
            raise SystemExit(f'{name}: the values name each other')
        for used in re.findall(r'\b\w+\b', values[name].init):
            if used in names and used != name:
                visit(used, stack | {name})
        done.add(name)
        out.append(values[name])

    for name in sorted(values):
        visit(name, frozenset())
    return out


def data_files(game):
    """The game's data files and generated tables (<game>/src/**/*.cpp) by
    their path under <game>/src."""
    base = ROOT / game / 'src'
    return {str(p.relative_to(base)): p for p in sorted(base.rglob('*.cpp'))}


def defines(path, name):
    """Whether the file defines name at its top level (T name..., a
    reference to its place in the arena T &name or T (&name)[N])."""
    text = strip_comments(path.read_text())
    return re.search(rf'^(?!extern)[^\n;{{}}()]*(?:\(\s*&\s*)?\b{name}\b\)?\s*(\[[^\]]*\])?\s*(=|;|\{{)', text,
                     flags=re.M) is not None


# The functions each game's generated file defines (its namespace's), which
# the one program calls through the game it plays: the header that declares
# them, the return type, the parameters and the arguments.
DISPATCHED = {'callIntrinsic': ('intrinsics.h', 's32', 'u16 number, const UsecodeArguments &args', 'number, args')}


def generate():
    files = {}
    parsed = {g: {rel: parse(p) for rel, p in value_headers(g).items()} for g in GAMES}
    rels = sorted(set(parsed['regret']) | set(parsed['remorse']))
    every = {g: {} for g in GAMES}
    for g in GAMES:
        for rel, (_, values) in parsed[g].items():
            every[g].update(values)
    shared = []
    tables = []
    arrays = set()
    # The variables, declared extern in the headers and defined once in
    # values_storage.cpp: an inline variable's 0 lets a compiler initialize
    # statically what reads it (Clang did copyright[] in gamemain.cpp), before
    # the game's values are there; an extern one it cannot see.
    storage = []

    def variable(declaration):
        storage.append(declaration)
        return 'extern ' + declaration

    for rel in rels:
        includes = []
        for g in GAMES:
            for line in parsed[g].get(rel, ([], {}))[0]:
                if line not in includes:
                    includes.append(line)
        names = []
        for g in GAMES:
            for name in parsed[g].get(rel, ([], {}))[1]:
                if name not in names and all(name in every[x] for x in GAMES):
                    names.append(name)
        guard = 'PORT_' + re.sub(r'\W', '_', Path(rel).name).upper()
        if rel.endswith('profile.h'):
            guard = 'PORT_PROFILE_H'
        if rel.endswith('searches.h'):
            guard = 'PORT_SEARCHES_H'
        lines = [HEAD.format(sources=f'regret/src/{rel} and remorse/src/{rel}',
                             what="the one program's run-time values"),
                 f'#ifndef {guard}', f'#define {guard}', '#include <span>']
        lines += includes
        lines.append('')
        for name in names:
            r, m = every['regret'][name], every['remorse'][name]
            if r.extern or m.extern:
                if not (r.extern and m.extern and r.typ == m.typ and r.array == m.array):
                    raise SystemExit(f'{name}: declared differently by the games')
                if r.array:
                    lines.append(variable(f'std::span<{r.typ}> {name};'))
                else:
                    lines.append(variable(f'{r.typ.removeprefix("const ")} {name};'))
                tables.append(name)
                continue
            typ = unified_type(name, r, m)
            if typ is None:
                largest = max(number(every['regret'], name), number(every['remorse'], name))
                lines.append(f'inline constexpr {r.typ} {name} = {largest};')
                continue
            dims = re.match(r'^(.*?)(\[.*\])$', typ)
            if dims and dims.group(1) == 'char' and dims.group(2) == '[]':
                lines.append(variable(f'const char *{name};'))
            elif dims:
                lines.append(variable(f'{dims.group(1)} {name}{dims.group(2)};'))
                arrays.add(name)
            elif typ.endswith('*'):
                lines.append(variable(f'{typ}{name};'))
            else:
                lines.append(variable(f'{typ} {name};'))
            shared.append(name)
        lines.append('#endif')
        files[OUT / rel] = formatted('\n'.join(lines) + '\n', str(OUT / rel))
    # The common headers that include the value headers at their end (so
    # that the values see their types), else the value headers themselves:
    # the one program's variables with them.
    owners = []
    for rel in rels:
        name = Path(rel).name
        common = ROOT / 'common' / 'src' / Path(rel).parent / name.replace('_game.h', '.h')
        if name.endswith('_game.h') and common.exists():
            owners.append(f'#include "{common.name}"')
        owners.append(f'#include "{name}"')
    lines = [HEAD.format(sources='both games\' value headers', what="the one program's value variables, which the "
                                                                    "game it plays fills (values.cpp)")]
    lines += owners + [''] + storage
    path = OUT / 'system' / 'values_storage.cpp'
    files[path] = formatted('\n'.join(lines) + '\n', str(path))
    links = []
    for g in GAMES:
        namespace = NAMESPACE[g]
        title = g.capitalize()
        includes = []
        for rel in rels:
            for line in parsed[g].get(rel, ([], {}))[0]:
                if line not in includes:
                    includes.append(line)
        for line in owners:
            if line not in includes:
                includes.append(line)
        guard = f'PORT_VALUES_{g.upper()}_H'
        lines = [HEAD.format(sources=f'{g}/src\'s value headers',
                             what=f"{g}'s values, in a namespace of their own, which fill the one program's when it "
                                  f"plays {g}"),
                 f'#ifndef {guard}', f'#define {guard}'] + includes + ['', f'namespace {namespace} {{', '']
        for value in ordered(every[g]):
            lines.append(value.text)
        lines += ['', '}', '#endif']
        header = OUT / 'system' / f'values_{g}.h'
        files[header] = formatted('\n'.join(lines) + '\n', str(header))
        # Each data file in the namespace, which points the one program's
        # tables at the game's at its start, and the function that links it.
        for rel, path in data_files(g).items():
            text = strip_comments(path.read_text())
            data_includes = [l.strip() for l in text.split('\n') if l.strip().startswith('#include')]
            ident = re.sub(r'\W', '_', Path(rel).with_suffix('').as_posix())
            function = f'link{title}Data_{ident}'
            defined = [n for n in tables if defines(path, n)]
            target = OUT / Path(rel).parent / f'{Path(rel).stem}_{g}.cpp'
            back = '../' * (len(Path(rel).parts) + 1)
            # The functions the file defines that the one program calls
            # through the game (DISPATCHED), declared in the namespace.
            declared = [f'{result} {name}({parameters});' for name, (_, result, parameters, _) in sorted(DISPATCHED.items())
                        if re.search(rf'^\S[^\n]*\b{name}\(', text, flags=re.M)]
            lines = [HEAD.format(sources=f'{g}/src/{rel}',
                                 what=f"{g}'s data in its namespace in the one program"),
                     '#include "values.h"'] + data_includes + [f'#include "values_{g}.h"', '',
                     f'namespace {namespace} {{'] + declared + [
                     f'#include "{back}{g}/src/{rel}"', '}', '',
                     '/* Called by linkGameData (values_data.cpp) so that the program links this file. */',
                     f'void {function}();', '', f'void {function}() {{}}', '']
            # The one program's tables pointed at the game's by this file's
            # start when it plays the game: after the file's globals are
            # bound (the arena's) and after the values (values.cpp, which
            # runs before them).
            playing = 'playingRemorse' if g == 'remorse' else '!playingRemorse'
            lines += ['namespace {', '', '[[maybe_unused]] const bool POINTED = [] {', f'    if ({playing}) {{']
            for name in defined:
                lines.append(f'        ::{name} = {namespace}::{name};')
            lines += ['    }', '    return true;', '}();', '', '}']
            files[target] = formatted('\n'.join(lines) + '\n', str(target))
            links.append(function)
        lines = [HEAD.format(sources=f'{g}/src\'s value headers',
                             what=f"{g}'s values, which fill the one program's when it plays {g}"),
                 '#include "values.h"', '#include <algorithm>', '#include <iterator>', f'#include "values_{g}.h"', '',
                 f'void use{title}Values()', '{']
        for name in shared:
            if name in arrays:
                lines.append(f'    std::copy(std::begin({namespace}::{name}), std::end({namespace}::{name}), ::{name});')
            else:
                lines.append(f'    ::{name} = {namespace}::{name};')
        lines.append('}')
        path = OUT / 'system' / f'values_{g}.cpp'
        files[path] = formatted('\n'.join(lines) + '\n', str(path))
    # The calls that link every data file of both games: an object the
    # program takes as it is, which the linker then must resolve.
    lines = [HEAD.format(sources="both games' data files",
                         what='the calls that link them into the one program (CMakeLists.txt\'s unified_data)')]
    lines += [f'void {function}();' for function in links]
    lines += ['', '/* Never called: the program links this object, and with it every data file. */',
              'void linkGameData();', '', 'void linkGameData()', '{']
    lines += [f'    {function}();' for function in links]
    lines.append('}')
    path = OUT / 'system' / 'values_data.cpp'
    files[path] = formatted('\n'.join(lines) + '\n', str(path))
    # The functions the game's generated files define.
    lines = [HEAD.format(sources='DISPATCHED', what="the functions each game's generated file defines, called through "
                                                    "the game the one program plays"),
             '#include "values.h"']
    lines += sorted({f'#include "{header}"' for header, _, _, _ in DISPATCHED.values()})
    lines.append('')
    for g in GAMES:
        lines.append(f'namespace {NAMESPACE[g]} {{')
        for name, (_, result, parameters, _) in sorted(DISPATCHED.items()):
            lines.append(f'{result} {name}({parameters});')
        lines.append('}')
    for name, (_, result, parameters, arguments) in sorted(DISPATCHED.items()):
        lines += ['', f'{result} {name}({parameters})', '{', '    if (playingRemorse) {',
                  f'        return {NAMESPACE["remorse"]}::{name}({arguments});', '    }',
                  f'    return {NAMESPACE["regret"]}::{name}({arguments});', '}']
    path = OUT / 'system' / 'dispatch.cpp'
    files[path] = formatted('\n'.join(lines) + '\n', str(path))
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if binary() is None:
        print('make_unified_values: no clang-format 20 to write the files in the format')
        sys.exit(77)
    files = generate()
    stale = [p for p, text in files.items() if not p.exists() or p.read_text() != text]
    if args.check:
        for p in stale:
            print('stale', p.relative_to(ROOT))
        print(f'make_unified_values: {len(files)} files, {len(stale)} stale')
        sys.exit(1 if stale else 0)
    for p in stale:
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(files[p])
    print(f'make_unified_values: {len(files)} files, {len(stale)} written')


if __name__ == '__main__':
    main()
