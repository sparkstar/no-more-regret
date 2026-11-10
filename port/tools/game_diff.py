#!/usr/bin/env python3
"""What is left of the games' own code: the functions of the games' files (regret/src and
remorse/src, the files of the same path paired; the generated
intrinsics.cpp and savefields.cpp left out), paired by name, and how many
lines of their bodies differ.

A function is a definition whose body opens with a line "{" and closes
with a line "}", as clang-format writes them (port/.clang-format); its name
is the last qualified name before "(" in the lines above the "{". A second
function of the same name in a file (an overload) is paired with the
other game's second. The lines that differ are those a diff of the two
bodies adds or removes.

usage: game_diff.py [--list]
(the counts by how much the functions differ; --list: each function, by
file, in Markdown)
"""
import argparse, difflib, re
from pathlib import Path

PORT = Path(__file__).resolve().parents[1]
GAMES = ('regret', 'remorse')
GENERATED = {'intrinsics.cpp', 'savefields.cpp'}
# The groups by the lines that differ: (title, the most lines, or None).
GROUPS = [('the same', 0), ('1-6 lines differ', 6), ('7-20 lines differ', 20), ('over 20 lines differ', None)]
NAME = re.compile(r'([~\w:]+)\s*\(')


def functions(path):
    """{name: the lines of its body, braces included} of a file; a second
    function of a name is name#2."""
    lines = path.read_text(errors='replace').split('\n')
    found = {}
    i = 0
    while i < len(lines):
        if lines[i] != '{' or i == 0:
            i += 1
            continue
        first = i - 1
        while first > 0 and lines[first - 1].strip() and not lines[first - 1].startswith(('}', '/*', ' *', '#', '//')) \
                and not lines[first - 1].endswith((';', '*/')):
            first -= 1
        head = ' '.join(line.strip() for line in lines[first:i])
        match = NAME.search(head)
        name = match.group(1) if match else head
        last = i + 1
        while last < len(lines) and lines[last] != '}':
            last += 1
        key, n = name, 2
        while key in found:
            key, n = f'{name}#{n}', n + 1
        found[key] = lines[i:last + 1]
        i = last + 1
    return found


def differing(a, b):
    """The lines a diff of two bodies adds or removes."""
    return sum(1 for line in difflib.unified_diff(a, b, lineterm='', n=0)
               if line[:1] in '+-' and not line.startswith(('+++', '---')))


def compare():
    """[(file, name, game or 'both', regret's lines, remorse's lines, lines
    that differ or None)] of the games' files."""
    trees = [PORT / game / 'src' for game in GAMES]
    files = sorted({p.relative_to(tree) for tree in trees for p in tree.rglob('*.cpp')}, key=str)
    rows = []
    for relative in files:
        if relative.name in GENERATED:
            continue
        found = [functions(tree / relative) if (tree / relative).exists() else {} for tree in trees]
        for name in sorted(set(found[0]) | set(found[1])):
            regret, remorse = found[0].get(name), found[1].get(name)
            if regret is not None and remorse is not None:
                rows.append((str(relative), name, 'both', len(regret), len(remorse), differing(regret, remorse)))
            else:
                rows.append((str(relative), name, GAMES[0] if regret is not None else GAMES[1],
                             len(regret or []), len(remorse or []), None))
    return rows


def group(row):
    """The row's group: a title of GROUPS, or the one game that has it."""
    if row[2] != 'both':
        return f'only {"No Regret" if row[2] == "regret" else "No Remorse"}'
    for title, most in GROUPS:
        if most is None or row[5] <= most:
            return title
    raise AssertionError(row)


def titles():
    return [t for t, _ in GROUPS] + ['only No Regret', 'only No Remorse']


def summary(rows):
    print('group\tfunctions\tNo Regret\tNo Remorse')
    for title in titles():
        mine = [r for r in rows if group(r) == title]
        print(f'{title}\t{len(mine)}\t{sum(r[3] for r in mine)}\t{sum(r[4] for r in mine)}')
    print(f'all\t{len(rows)}\t{sum(r[3] for r in rows)}\t{sum(r[4] for r in rows)}')


def listing(rows):
    for title in titles():
        mine = [r for r in rows if group(r) == title]
        print(f'### {title} ({len(mine)})\n')
        by_file = {}
        for f, name, game, regret, remorse, differ in mine:
            name = name.split('#')[0]
            if differ == 0:
                item = f'`{name}`'
            elif differ is not None:
                item = f'`{name}` ({regret}/{remorse}, {differ})'
            else:
                item = f'`{name}` ({regret or remorse})'
            by_file.setdefault(f, []).append(item)
        for f, items in by_file.items():
            print(f'- `{f}`: ' + ', '.join(items))
        print()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--list', action='store_true', help='each function, by file')
    args = ap.parse_args()
    rows = compare()
    if args.list:
        listing(rows)
    else:
        summary(rows)


if __name__ == '__main__':
    main()
