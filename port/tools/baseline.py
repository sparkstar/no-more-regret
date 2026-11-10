#!/usr/bin/env python3
"""The baseline: the deterministic outputs of the scenario tests (milestones
1 to 3, movies, and No Remorse's help, weasel and credits): the states,
screens, WAVs and saves each keeps (--keep), by scenario, name, size and
SHA-256. A change that keeps the game's behaviour keeps them; one that
changes them updates the game's baseline (port/<game>/test/expected/
baseline.tsv, CRUSADER_GAME, tools/overlay.py) in the same commit and says
why in its message.

usage: baseline.py record|check PROGRAM SAVEFIELDS [--baseline FILE] [--keep DIR] [-j N] [--digests DIR]
                   [--scenarios FILE] [SCENARIO ...]
       baseline.py scenario NAME PROGRAM SAVEFIELDS --digests DIR [--scenarios FILE]

record runs the scenarios and writes FILE (default the game's baseline.tsv);
check runs them and compares with FILE, naming each file that differs, is
new or is missing (exit 1). A scenario that skips (movies without the CD's
FLICS, exit 77) keeps its rows in record and is left out of check. Of what
a scenario keeps, the files of its installs are the CD's (the inputs), but
for the saves the game writes in SAVEGAME; the scripts are the inputs too
and hold the run's paths; what a scenario keeps under original/ is another
program's (an exchange with the original game), not the port's. A state
(.txt) counts with its lines ending in LF, as the Windows build's end in CR
LF. The kept files go to a temporary directory beside the program (the
installs are large), or to DIR with --keep.

--scenarios FILE adds scenarios from outside this repository: a line each,
its name, its script (a path from FILE's directory) and "savefields" when it
takes the savefields test after the program, a tab between them.

The tests run each scenario once: scenario runs one as its test, its output
shown and its exit its own, and writes its files' rows to DIR/NAME.tsv (a
skipped one's rows say so; a failed one leaves none); check --digests DIR
compares those rows rather than running the scenarios again (ctest runs the
scenarios' tests first, its fixture <game>_scenarios).
"""
import argparse, concurrent.futures, hashlib, os, subprocess, sys, tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402

BASELINE = overlay.TESTS / 'expected/baseline.tsv'
SKIPPED = 77
# The game's scenarios: each one's script (the game's tests' or the shared
# port/test's) and whether it takes the savefields test after the game (and
# its own options).
SCENARIOS = {
    'regret': {
        'milestone': ('milestone.py', False, []),
        'milestone2': ('milestone2.py', False, []),
        'milestone3': ('milestone3.py', False, []),
        'movies': ('movies.py', False, []),
    },
    'remorse': {
        'milestone': ('milestone.py', False, []),
        'milestone2': ('milestone2.py', False, []),
        'milestone3': ('milestone3.py', False, []),
        'help': ('help.py', False, []),
        'weasel': ('weasel.py', False, []),
        'credits': ('credits.py', False, []),
        'movies': ('movies.py', False, []),
    },
}[overlay.GAME]


def more_scenarios(path):
    """Adds the scenarios of a --scenarios file to SCENARIOS."""
    for line in path.read_text().splitlines():
        if line and not line.startswith('#'):
            name, script, savefields = line.split('\t')
            SCENARIOS[name] = (str((path.parent / script).resolve()), savefields == 'savefields', [])


def script_path(script):
    """A scenario's script: the game's tests' own, else the shared one (a
    --scenarios file's by its path)."""
    own = overlay.TESTS / 'scenario' / script
    return own if own.exists() else overlay.PORT / 'test/scenario' / script


def output(name, path):
    """Whether a kept file of scenario name is an output of the game: not an
    input script, in an install only a save the game wrote, and not another
    program's (original/)."""
    parts = path.parts
    if path.name == 'script':
        return False
    if parts[0] == 'original':
        return False
    if 'install' in parts:
        rest = parts[parts.index('install') + 1:]
        return len(rest) == 2 and rest[0].upper() == 'SAVEGAME'
    return True


def command(name, regret, savefields, work):
    """The command line of a scenario keeping its files in work."""
    script, takes_savefields, options = SCENARIOS[name]
    line = [sys.executable, str(script_path(script)), str(regret)]
    if takes_savefields:
        line.append(str(savefields))
    return line + options + ['--keep', str(work)]


def digests(name, work):
    """{relative path: (size, sha256)} of the outputs a scenario kept."""
    files = {}
    for path in sorted(work.rglob('*')):
        relative = path.relative_to(work)
        if path.is_file() and output(name, relative):
            data = path.read_bytes()
            if path.suffix == '.txt':
                data = data.replace(b'\r\n', b'\n')      # the Windows build's text files
            files[relative.as_posix()] = (len(data), hashlib.sha256(data).hexdigest())
    return files


def run(name, regret, savefields, keep):
    """(name, {relative path: (size, sha256)}) of a scenario, or None when it
    skips; exits on a failure."""
    work = keep / name
    result = subprocess.run(command(name, regret, savefields, work), capture_output=True, text=True)
    if result.returncode == SKIPPED:
        return name, None
    if result.returncode:
        sys.exit(f'baseline: {name} failed ({result.returncode}):\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    return name, digests(name, work)


# A scenario's digests when it skipped (scenario's DIR/NAME.tsv).
SKIPPED_ROWS = 'skipped\n'


def write_digests(path, files):
    path.write_text(SKIPPED_ROWS if files is None else
                    ''.join(f'{file}\t{size}\t{digest}\n' for file, (size, digest) in sorted(files.items())))


def read_digests(name, folder):
    """(name, files) from scenario's DIR/NAME.tsv, None for a skipped one;
    exits without them (the scenario's test failed or did not run)."""
    path = folder / f'{name}.tsv'
    if not path.exists():
        sys.exit(f'baseline: no digests of {name} in {folder} (its test failed or did not run)')
    text = path.read_text()
    if text == SKIPPED_ROWS:
        return name, None
    files = {}
    for line in text.splitlines():
        file, size, digest = line.split('\t')
        files[file] = (int(size), digest)
    return name, files


def scenario(name, regret, savefields, folder):
    """One scenario as its test: its output shown, its exit returned, its
    digests written to folder (none when it fails)."""
    folder.mkdir(parents=True, exist_ok=True)
    rows = folder / f'{name}.tsv'
    rows.unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(dir=regret.parent, prefix=f'scenario-{name}-') as temporary:
        work = Path(temporary) / name
        sys.stdout.flush()
        code = subprocess.run(command(name, regret, savefields, work)).returncode
        if code == SKIPPED:
            write_digests(rows, None)
        elif code == 0:
            write_digests(rows, digests(name, work))
    return code


def read(path):
    rows = {}
    for line in path.read_text().splitlines():
        name, file, size, digest = line.split('\t')
        rows.setdefault(name, {})[file] = (int(size), digest)
    return rows


def main():
    if sys.argv[1:2] == ['scenario']:
        ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
        ap.add_argument('action')
        ap.add_argument('name')
        ap.add_argument('regret', type=Path)
        ap.add_argument('savefields', type=Path)
        ap.add_argument('--digests', type=Path, required=True)
        ap.add_argument('--scenarios', dest='more', type=Path)
        args = ap.parse_args()
        if args.more:
            more_scenarios(args.more)
        if args.name not in SCENARIOS:
            sys.exit(f'baseline: no scenario {args.name} (one of {", ".join(SCENARIOS)})')
        sys.exit(scenario(args.name, args.regret.resolve(), args.savefields.resolve(), args.digests.resolve()))
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('action', choices=['record', 'check'])
    ap.add_argument('regret', type=Path)
    ap.add_argument('savefields', type=Path)
    ap.add_argument('scenarios', nargs='*', help='the scenarios (all without)')
    ap.add_argument('--baseline', type=Path, default=BASELINE)
    ap.add_argument('--keep', type=Path, help='keep the scenarios\' files here')
    ap.add_argument('-j', type=int, default=os.cpu_count())
    ap.add_argument('--digests', type=Path, help='check the rows the scenarios\' tests wrote here')
    ap.add_argument('--scenarios', dest='more', type=Path, help='a file of more scenarios')
    args = ap.parse_args()
    if args.more:
        more_scenarios(args.more)
    names = args.scenarios or list(SCENARIOS)
    for name in names:
        if name not in SCENARIOS:
            sys.exit(f'baseline: no scenario {name} (one of {", ".join(SCENARIOS)})')
    if args.digests and args.action == 'check':
        results = dict(read_digests(name, args.digests.resolve()) for name in names)
    else:
        # The installs the scenarios keep are large: beside the game, not in /tmp.
        with tempfile.TemporaryDirectory(dir=args.regret.resolve().parent, prefix='baseline-') as temporary:
            keep = args.keep.resolve() if args.keep else Path(temporary)
            with concurrent.futures.ThreadPoolExecutor(args.j) as pool:
                results = dict(pool.map(lambda n: run(n, args.regret.resolve(), args.savefields.resolve(), keep),
                                        names))
    old = read(args.baseline) if args.baseline.exists() else {}
    if args.action == 'record':
        rows = dict(old)
        for name, files in results.items():
            if files is not None:
                rows[name] = files
        args.baseline.write_text(''.join(f'{name}\t{file}\t{size}\t{digest}\n'
                                         for name in SCENARIOS if name in rows
                                         for file, (size, digest) in sorted(rows[name].items())))
        for name, files in results.items():
            print(f'{name}: {"skipped, its rows kept" if files is None else f"{len(files)} files"}')
        print(f'written {args.baseline}')
        return
    wrong = []
    for name, files in results.items():
        if files is None:
            print(f'{name}: skipped')
            continue
        expected = old.get(name, {})
        for file in sorted(set(files) | set(expected)):
            if file not in expected:
                wrong.append(f'{name}/{file}: not in the baseline')
            elif file not in files:
                wrong.append(f'{name}/{file}: missing')
            elif files[file] != expected[file]:
                wrong.append(f'{name}/{file}: {files[file][0]} bytes {files[file][1][:12]}, '
                             f'the baseline {expected[file][0]} bytes {expected[file][1][:12]}')
        print(f'{name}: {len(files)} files')
    for w in wrong:
        print(f'DIFFERS {w}')
    print(f'baseline: {len(wrong)} differences')
    sys.exit(1 if wrong else 0)


if __name__ == '__main__':
    main()
