#!/usr/bin/env python3
"""What the synthetic scenes leave of the engine's code that the real data
runs: the lines and branches of the
engine's libraries (game_unified, asm_unified, asylum) a coverage build's
program runs in a workload, and the difference of two sets of workloads.

The build is GCC's with --coverage of the one program:
  cmake -S port -B build/port-cov -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPORT_WINDOW=OFF \\
    "-DCMAKE_CXX_FLAGS=--coverage -O0" "-DCMAKE_C_FLAGS=--coverage -O0" \\
    -DCMAKE_EXE_LINKER_FLAGS=--coverage
  ninja -C build/port-cov crusader

usage: coverage.py run [--build DIR] OUT.json -- COMMAND...
         clears the counts, runs COMMAND, writes what ran: {file: {line: [count, [branch counts]]}}
       coverage.py diff --have A.json... --want B.json... [--files] [--out FILE]
         the lines (and branches) the --want workloads run and the --have ones do not, by file and function
       coverage.py areas --have A.json... --want B.json...
         of the lines the --want workloads run, how many the --have ones run, by area of the engine
"""
import argparse, concurrent.futures, json, os, subprocess, sys
from collections import defaultdict
from pathlib import Path

PORT = Path(__file__).resolve().parents[2]
ROOT = PORT.parent
LIBRARIES = ('game_unified', 'asm_unified', 'asylum')


def counts(build):
    """The .gcda files of the engine's libraries."""
    return [data for library in LIBRARIES for data in sorted((build / 'CMakeFiles' / f'{library}.dir').rglob('*.gcda'))]


def gcov(build, data):
    """gcov's JSON of one object's counts: [(file, line, count, branches, function)]."""
    out = subprocess.run(['gcov', '-j', '-t', '-m', '-b', '-o', str(data.parent), str(data)], cwd=build,
                         capture_output=True, text=True).stdout
    found = []
    for document in out.splitlines():
        if not document.startswith('{'):
            continue
        for entry in json.loads(document)['files']:
            path = Path(entry['file'])
            if not path.is_absolute():
                path = (build / path).resolve()
            try:
                name = str(path.relative_to(PORT))
            except ValueError:
                continue
            for line in entry['lines']:
                found.append((name, line['line_number'], line['count'],
                              [branch['count'] for branch in line.get('branches', [])],
                              line.get('function_name', '')))
    return found


def gather(build):
    """{file: {line: [count, branch counts, function]}} summed over the
    objects (a header's lines are in each object that includes it)."""
    lines = defaultdict(dict)
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as pool:
        for found in pool.map(lambda data: gcov(build, data), counts(build)):
            for name, number, count, branches, function in found:
                key = str(number)
                old = lines[name].get(key)
                if old is None:
                    lines[name][key] = [count, branches, function]
                else:
                    old[0] += count
                    if len(old[1]) == len(branches):
                        old[1] = [a + b for a, b in zip(old[1], branches)]
                    old[2] = old[2] or function
    return lines


def run(args):
    build = args.build.resolve()
    for data in counts(build):
        data.unlink()
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    code = subprocess.run(command).returncode
    lines = gather(build)
    args.out.write_text(json.dumps(lines))
    ran = sum(1 for file in lines.values() for value in file.values() if value[0])
    print(f'coverage {args.out.name}: the command returned {code}; {ran} lines ran')
    return 0


def union(paths):
    """{file: {line: (ran, branches taken as a set of indices, function)}} of workloads."""
    out = defaultdict(dict)
    for path in paths:
        for name, file in json.loads(Path(path).read_text()).items():
            for line, (count, branches, function) in file.items():
                ran, taken, known = out[name].get(line, (False, set(), function))
                out[name][line] = (ran or count > 0, taken | {i for i, c in enumerate(branches) if c}, known or function)
    return out


def diff(args):
    have, want = union(args.have), union(args.want)
    by_file = defaultdict(lambda: defaultdict(lambda: [0, 0]))
    total_lines = total_branches = 0
    for name, file in sorted(want.items()):
        for line, (ran, taken, function) in file.items():
            mine = have.get(name, {}).get(line, (False, set(), function))
            missing_line = ran and not mine[0]
            missing_branches = len(taken - mine[1]) if ran else 0
            if missing_line or missing_branches:
                entry = by_file[name][function or '?']
                entry[0] += missing_line
                entry[1] += missing_branches
                total_lines += missing_line
                total_branches += missing_branches
    report = []
    for name, functions in sorted(by_file.items(), key=lambda item: -sum(v[0] for v in item[1].values())):
        lines = sum(v[0] for v in functions.values())
        branches = sum(v[1] for v in functions.values())
        report.append(f'{lines:6d} {branches:6d}  {name}')
        if not args.files:
            for function, (n, b) in sorted(functions.items(), key=lambda item: -item[1][0]):
                report.append(f'{n:6d} {b:6d}      {function}')
    wanted = sum(1 for file in want.values() for v in file.values() if v[0])
    head = (f'lines the --want workloads run: {wanted}; of them the --have ones do not: {total_lines} '
            f'(branches taken there but not here: {total_branches})')
    text = head + '\n  lines branches  file / function\n' + '\n'.join(report) + '\n'
    if args.out:
        args.out.write_text(text)
        print(head)
    else:
        print(text, end='')
    return 0


# The engine's areas, by the paths of their files (the first that matches).
AREAS = [
    ('NPC, movement, combat', r'/game/(control|animprim|activity|attack|pathfind|crusader|sprite|explode|wpncache|'
                              r'dtable|actanim|animcach|animfile|astream|animfrm|alert|firetype|missile|boboboom)|'
                              r'/item/npc'),
    ('items, world, physics', r'/item/|/world/(?!savefields|savegame|loadsave)'),
    ('sound, music', r'/sound/|common/asylum/'),
    ('gumps, menus', r'/gump/'),
    ('drawing', r'/display/|/graph/'),
    ('movies', r'/movie/'),
    ('kernel', r'/kernel/'),
    ('input', r'/input/'),
    ('usecode', r'/usecode/'),
    ('saves', r'savefields|savegame|loadsave'),
    ('the rest', r'.'),
]


def areas(args):
    import re
    have, want = union(args.have), union(args.want)
    rows = {name: [0, 0] for name, _ in AREAS}
    for name, file in want.items():
        area = next(area for area, pattern in AREAS if re.search(pattern, name))
        for line, (ran, _, _) in file.items():
            if ran:
                rows[area][1] += 1
                rows[area][0] += have.get(name, {}).get(line, (False,))[0]
    total = [sum(row[0] for row in rows.values()), sum(row[1] for row in rows.values())]
    for name, (got, of) in [('all', total)] + list(rows.items()):
        print(f'{name:24s} {got:6d} / {of:6d}  {100 * got / max(of, 1):5.1f}%')
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    sub = ap.add_subparsers(dest='action', required=True)
    r = sub.add_parser('run')
    r.add_argument('out', type=Path)
    r.add_argument('--build', type=Path, default=ROOT / 'build/port-cov')
    r.add_argument('command', nargs=argparse.REMAINDER)
    d = sub.add_parser('diff')
    d.add_argument('--have', nargs='+', required=True)
    d.add_argument('--want', nargs='+', required=True)
    d.add_argument('--files', action='store_true', help='by file only')
    d.add_argument('--out', type=Path)
    a = sub.add_parser('areas')
    a.add_argument('--have', nargs='+', required=True)
    a.add_argument('--want', nargs='+', required=True)
    args = ap.parse_args()
    return {'run': run, 'diff': diff, 'areas': areas}[args.action](args)


if __name__ == '__main__':
    sys.exit(main())
