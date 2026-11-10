#!/usr/bin/env python3
"""What the tests run of the game: the lines and
functions of the game's sources (tools/overlay.py), the usecode intrinsics (the 349 cases of
intrinsics.cpp) and the interpreter's opcodes (the 111 cases of
UsecodeThread::run) that a coverage build's ctest reaches.

The build is GCC's with --coverage (the default build/port-cov):
  cmake -S port -B build/port-cov -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPORT_WINDOW=OFF \\
    "-DCMAKE_CXX_FLAGS=--coverage -O0" -DCMAKE_EXE_LINKER_FLAGS=--coverage
--run builds it, clears the counts and runs its ctest (all tests, which
count into the same objects); without it the counts of the last run are
read. gcov (-j) reads them.

A case counts as run when the first line after its label with a count ran.
Opcodes whose labels share a body (the list unions and removals, CONCAT and
EQ_STRING, CALL_NEAR and CALL, the two FOREACH) count as run when the body
did.

usage: coverage.py [--build DIR] [--run] [--tests REGEX] [--exclude REGEX] [--out FILE]

Prints the sums (lines, functions, cases, and the lines by directory of
the game's sources) and writes one line per function to FILE (default
DIR/coverage.tsv): file, start line, name, times called, lines run, lines.
"""
import argparse, collections, json, re, subprocess, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / 'port'
ROOTS = overlay.roots()


def gather(build):
    """Line counts {(file, line): count} and functions {(file, line, name):
    count} of the game's objects, summed over the objects that share a
    header."""
    lines = collections.Counter()
    known = set()
    functions = collections.Counter()
    for library in overlay.targets():
        for data in sorted((build / 'CMakeFiles' / f'{library}.dir').rglob('*.gcda')):
            out = subprocess.run(['gcov', '-j', '-t', '-m', '-o', str(data.parent), str(data)], cwd=build,
                                 capture_output=True, text=True).stdout
            for document in out.splitlines():
                if not document.startswith('{'):
                    continue
                for entry in json.loads(document)['files']:
                    path = Path(entry['file'])
                    if not path.is_absolute():
                        path = (build / path).resolve()
                    if not any(root in path.parents for root in ROOTS):
                        continue
                    for line in entry['lines']:
                        known.add((path, line['line_number']))
                        lines[path, line['line_number']] += line['count']
                    for function in entry['functions']:
                        key = (path, function['start_line'], function['demangled_name'])
                        functions[key] += function['execution_count']
                        functions.setdefault(key, 0)
    counts = {key: lines[key] for key in known}
    return counts, functions


def cases(counts, path, label, start=None, end=None):
    """The cases of a switch in path whose labels match label (a regular
    expression with a group for the case's name), between lines start and
    end: {name: (run, shared)}; a case whose label is followed by another
    label shares the body after the last."""
    text = path.read_text(errors='replace').splitlines()
    found = {}
    pending = []
    for number, line in enumerate(text, 1):
        if start and number < start or end and number > end:
            continue
        match = re.match(label, line)
        if match:
            pending.append(match.group(1))
            continue
        if pending and (path, number) in counts:
            for name in pending:
                found[name] = (counts[path, number] > 0, len(pending) > 1)
            pending = []
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--build', type=Path, default=ROOT / 'build/port-cov')
    ap.add_argument('--run', action='store_true', help='build, clear the counts and run ctest first')
    ap.add_argument('--tests', help='ctest -R REGEX with --run')
    ap.add_argument('--exclude', help='ctest -E REGEX with --run')
    ap.add_argument('--out', type=Path)
    args = ap.parse_args()
    build = args.build.resolve()
    if not (build / 'CMakeCache.txt').exists():
        sys.exit(f'coverage: no build in {build} (see the usage)')
    if args.run:
        subprocess.run(['cmake', '--build', str(build)], check=True, stdout=subprocess.DEVNULL)
        for data in build.rglob('*.gcda'):
            data.unlink()
        subprocess.run(['ctest', '--test-dir', str(build), '-j8'] + (['-R', args.tests] if args.tests else [])
                       + (['-E', args.exclude] if args.exclude else []), stdout=subprocess.DEVNULL)
    counts, functions = gather(build)
    if not counts:
        sys.exit(f'coverage: no counts in {build}')

    run = sum(1 for c in counts.values() if c)
    print(f'lines: {run} of {len(counts)} ({100 * run / len(counts):.1f}%)')
    called = sum(1 for c in functions.values() if c)
    print(f'functions: {called} of {len(functions)} ({100 * called / len(functions):.1f}%)')

    intrinsics = cases(counts, overlay.find('intrinsics.cpp'), r'\s*case (0x[0-9A-F]{3}):')
    print(f'intrinsics: {sum(r for r, _ in intrinsics.values())} of {len(intrinsics)} cases')
    inter = overlay.find('inter.cpp')
    run_function = next(key for key in functions if key[0] == inter and key[2].startswith('UsecodeThread::run('))
    text = inter.read_text().splitlines()
    end = next(n for n in range(run_function[1], len(text) + 1) if text[n - 1] == '}')
    opcodes = cases(counts, inter, r'\s*case (OP_[A-Z_]+):', run_function[1], end)
    shared = sum(1 for _, s in opcodes.values() if s)
    print(f'opcodes: {sum(r for r, _ in opcodes.values())} of {len(opcodes)} '
          f'({shared} in shared bodies, run when the body ran)')
    never = sorted(n for n, (r, _) in opcodes.items() if not r)
    print(f'  not run: {" ".join(never)}')

    by = collections.defaultdict(lambda: [0, 0])
    for (path, _), count in counts.items():
        directory = path.parent.relative_to(PORT).as_posix()
        by[directory][0] += count > 0
        by[directory][1] += 1
    for directory, (r, n) in sorted(by.items()):
        print(f'  {directory}: {r} of {n} lines ({100 * r / n:.1f}%)')

    # The lines of each function: from its start to the next function's in the file.
    per = collections.defaultdict(lambda: [0, 0])
    starts = sorted(functions)
    by_file = collections.defaultdict(list)
    for key in starts:
        by_file[key[0]].append(key)
    file_lines = collections.defaultdict(list)
    for path, number in counts:
        file_lines[path].append(number)
    for path, keys in by_file.items():
        keys.sort(key=lambda k: k[1])
        numbers = sorted(file_lines[path])
        for i, key in enumerate(keys):
            last = keys[i + 1][1] if i + 1 < len(keys) else 1 << 30
            own = [n for n in numbers if key[1] <= n < last]
            per[key] = [sum(1 for n in own if counts[path, n]), len(own)]
    out = args.out or build / 'coverage.tsv'
    with out.open('w') as f:
        for key in starts:
            path, line, name = key
            f.write(f'{path.relative_to(ROOT)}\t{line}\t{name}\t{functions[key]}\t{per[key][0]}\t{per[key][1]}\n')
    print(f'{out}: {len(starts)} functions')


if __name__ == '__main__':
    main()
