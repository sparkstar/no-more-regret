#!/usr/bin/env python3
"""The port's clang-tidy: clang-tidy 20 with port/.clang-tidy on the port's
C++ files.

usage: tidy.py [--fix] [--build DIR] [--config FILE] [--all | FILE ...]

Without files, the C++ files of port/ that differ from git's HEAD (format.py's
choice); --all, every one. Without --fix names what the rules find and exits 1
if anything (ctest's tidy: --all); with --fix applies the fixes the rules have
(format.py runs it before clang-format) and then names what is left. Without
clang-tidy 20 exits 77, which ctest counts as skipped. --config takes another
config than port/.clang-tidy (a rule tried alone before it is added).

The compile commands are a Clang build's: --build DIR, by default build/tidy,
which tidy.py configures with clang++-20 when it has none (a configure,
nothing built). Of them the program's targets: the game's libraries, its
assembly, the system layer, the program's own, the DLLs and compat, without
the tests'. A header is checked through a .cpp that includes it. A game's
data files (<game>/src/**/*.cpp) are compiled through unified/src's files,
which include them in the game's namespace: they are checked through those,
and fixed by hand (unified/src is make_unified_values.py's, and the fixes of
its files' runs are not applied). The games' value headers
(<game>/src/**/<name>_game.h) are compiled in no file (make_unified_values.py
reads them) and not checked. The config is given as --config-file:
clang-tidy 20 takes the HeaderFilterRegex of a .clang-tidy it finds for
itself only for the main files, and the headers' findings were suppressed.

The fixes of readability-implicit-bool-conversion with
UseUpperCaseLiteralSuffix write nullptr as NULLPTR (clang-tidy 20); --fix puts
it back in the files it changed.
"""
import argparse, concurrent.futures, json, os, re, shutil, subprocess, sys, tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from format import PORT, ROOT, changed_files

CONFIG = PORT / '.clang-tidy'
VERSION = 20
MISSING = f'no clang-tidy {VERSION} (clang-tidy-{VERSION} and clang-apply-replacements-{VERSION} on the PATH)'
GAMES = ('regret', 'remorse')
# The port's own code: the directories clang-tidy checks and the targets that
# compile it.
DIRECTORIES = ('common', 'regret', 'remorse', 'system', 'platform', 'compat')
TARGET = re.compile(r'CMakeFiles/(?:game_unified|asm_unified|port_system_objects|crusader|game_memory|asylum|xan|compat)\.dir/')
# The program's include path's trees, its own unified/src first (CMakeLists.txt's
# game_trees): a header's name is found in them, and not in a game's tree.
INCLUDE_TREES = ('unified/src', 'common/src', 'common/asylum', 'common/xan', 'platform', 'system', 'compat')
FINDING = re.compile(r'^(/[^:\n]+):(\d+):(\d+): (?:warning|error): (.*?) \[([\w.,-]+)\]$', re.M)
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)


def binary(name):
    """The tool of major version 20, or None."""
    for candidate in (f'{name}-{VERSION}', name):
        path = shutil.which(candidate)
        if path:
            out = subprocess.run([path, '--version'], capture_output=True, text=True).stdout
            m = re.search(r'version (\d+)\.', out)
            if m and int(m.group(1)) == VERSION:
                return path
    return None


def configure(build):
    """build's compile commands, configured with clang++-20 when it has none."""
    commands = build / 'compile_commands.json'
    if not commands.exists():
        compiler = shutil.which(f'clang++-{VERSION}') or sys.exit(f'no clang++-{VERSION} to configure {build}')
        print(f'tidy: configuring {build.relative_to(ROOT)} for its compile commands')
        subprocess.run(['cmake', '-S', str(PORT), '-B', str(build), '-G', 'Ninja', f'-DCMAKE_CXX_COMPILER={compiler}',
                        f'-DCMAKE_C_COMPILER={compiler.replace("++", "")}', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'],
                       check=True, capture_output=True)
    return commands


def databases(build, reconfigure=False):
    """[unit]: the files of the program's targets, each once, their compile
    commands written as build/tidy_db/compile_commands.json for clang-tidy's -p."""
    commands = configure(build)
    if reconfigure:
        subprocess.run(['cmake', str(build)], check=True, capture_output=True)
    entries = {}
    for entry in json.loads(commands.read_text()):
        if TARGET.search(entry.get('output', '')):
            entries.setdefault(entry['file'], entry)
    out = build / 'tidy_db' / 'compile_commands.json'
    text = json.dumps(list(entries.values()), indent=1)
    if not out.exists() or out.read_text() != text:
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text)
    return sorted(entries)


def port_files():
    """The port's own C++ files, without the games' tests."""
    return [p for d in DIRECTORIES for p in sorted((PORT / d).rglob('*'))
            if p.suffix in ('.cpp', '.h') and 'test' not in p.relative_to(PORT).parts]


def included(path):
    """Whether a file is compiled only as another's include: a header, or a
    game's data file, which unified/src's files include."""
    return path.suffix == '.h' or path.relative_to(PORT).parts[0] in GAMES


def includers(units, wanted):
    """[unit]: a unit that includes each of the files wanted, the includes
    followed as the program's build finds them (by name in INCLUDE_TREES, or
    by their path from the file)."""
    by_name = {}
    for tree in INCLUDE_TREES:
        for path in sorted((PORT / tree).rglob('*.h')):
            by_name.setdefault(path.name, path)
    includes = {}

    def resolve(name, base):
        if '/' in name:
            path = (base.parent / name).resolve()
            return [path] if path.exists() else []
        return [by_name[name]] if name in by_name else []

    def reached(unit):
        seen, todo = set(), [Path(unit)]
        while todo:
            path = todo.pop()
            if path not in includes:
                includes[path] = INCLUDE.findall(path.read_text(errors='replace')) if path.exists() else []
            for name in includes[path]:
                for found in resolve(name, path):
                    if found not in seen:
                        seen.add(found)
                        todo.append(found)
        return seen

    chosen, left = set(), set(wanted)
    for unit in units:
        if not left:
            break
        hit = left & reached(unit)
        if hit:
            chosen.add(unit)
            left -= hit
    for path in sorted(left):
        print(f'tidy: no unit includes {path.relative_to(ROOT)}; not checked')
    return sorted(chosen)


def own(unit):
    """Whether a unit is the port's own code (DIRECTORIES), not unified/src's."""
    return Path(unit).relative_to(PORT).parts[0] in DIRECTORIES


def run(tool, build, units, fixes):
    """The findings of the units in the port's own files, [(path, line, column,
    message, rule)]; with fixes, a directory, their fixes exported to it."""
    def one(index_unit):
        index, unit = index_unit
        command = [tool, '-p', str(build / 'tidy_db'), f'--config-file={CONFIG}', '--quiet']
        if fixes:
            command.append(f'--export-fixes={Path(fixes) / f"{index}.yaml"}')
        result = subprocess.run(command + [unit], capture_output=True, text=True, errors='replace')
        # A game's data file is named through its includer's path (../): by its own.
        found = [(str(Path(path).resolve()), line, column, message, rule.split(',')[0])
                 for path, line, column, message, rule in FINDING.findall(result.stdout + result.stderr)]
        return [f for f in found if Path(f[0]).is_relative_to(PORT) and own(f[0])]

    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as pool:
        return [f for found in pool.map(one, enumerate(units)) for f in found]


def put_nullptr_back(fixes):
    """NULLPTR written by the fixes put back as nullptr in the files they change."""
    paths = set()
    for path in Path(fixes).glob('*.yaml'):
        paths.update(re.findall(r"FilePath:\s+'?([^'\n]+)'?", path.read_text(errors='replace')))
    for name in sorted(paths):
        path = Path(name)
        if path.exists():
            text = path.read_text()
            fixed = re.sub(r'\bNULLPTR\b', 'nullptr', text)
            if fixed != text:
                path.write_text(fixed)


def tidy(files, build, fix, every=False):
    """Checks (and with fix first fixes) files, every: all the units; the
    findings left, each once."""
    tool = binary('clang-tidy')
    apply = binary('clang-apply-replacements')
    units = databases(build)
    if any(str(p) not in units for p in files if not included(p)):
        units = databases(build, reconfigure=True)
    for path in files:
        if not included(path) and str(path) not in units:
            print(f'tidy: {path.relative_to(ROOT)} is in no target clang-tidy checks; not checked')
    if every:
        chosen = units
    else:
        chosen = sorted({str(p) for p in files if str(p) in units} |
                        set(includers(units, [p for p in files if included(p)])))
    if fix:
        with tempfile.TemporaryDirectory(prefix='tidy-fixes') as fixes:
            run(tool, build, [u for u in chosen if own(u)], fixes)
            subprocess.run([apply, fixes], check=True)
            put_nullptr_back(fixes)
    found = set(run(tool, build, chosen, None))
    return sorted(found, key=lambda f: (f[0], int(f[1]), int(f[2]), f[4]))


def report(found):
    for path, line, column, message, rule in found:
        print(f'{Path(path).relative_to(ROOT)}:{line}:{column}: {message} [{rule}]')


def available():
    return binary('clang-tidy') is not None and binary('clang-apply-replacements') is not None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--fix', action='store_true', help='apply the fixes the rules have, then name what is left')
    ap.add_argument('--all', action='store_true', help='every C++ file of the port')
    ap.add_argument('--build', type=Path, default=ROOT / 'build/tidy', help='a Clang build for the compile commands')
    ap.add_argument('--config', type=Path, help='another config than port/.clang-tidy')
    ap.add_argument('files', nargs='*', type=Path)
    args = ap.parse_args()
    if args.config:
        global CONFIG
        CONFIG = args.config.resolve()
    if not available():
        print(MISSING)
        return 77
    files = port_files() if args.all else [p.resolve() for p in args.files] if args.files else changed_files()
    files = [p for p in files if p.relative_to(PORT).parts[0] in DIRECTORIES and 'test' not in p.relative_to(PORT).parts]
    if not files:
        print('tidy: no files')
        return 0
    found = tidy(files, args.build.resolve(), args.fix, args.all)
    report(found)
    print(f'tidy: {len(files)} files, {len(found)} findings')
    return 1 if found else 0


if __name__ == '__main__':
    sys.exit(main())
