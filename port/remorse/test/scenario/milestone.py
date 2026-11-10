#!/usr/bin/env python3
"""No Remorse's milestone 1 (No Regret's is
port/regret/test/scenario/milestone.py): the main menu as PNGs under scripted
input. The game is installed from No Remorse's CD (port/tools/install.py with
CRUSADER_GAME=noremorse, without its movies) into a fresh directory; the script
lets the menu come up, writes the screen, moves the choice with the arrow keys
and the cursor with the mouse, writing the screen after each, and quits. It runs
twice in the install, which must be as it was after each run (the menu writes no
file): the two runs must give the same bytes, and each input must change the
screen.

usage: milestone.py REMORSE [--cd CD] [--keep DIR]
"""
import argparse, hashlib, os, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
# Ticks are sixtieths of a second; the menu is up well before tick 600.
SCRIPT = '''\
600 screen {out}/menu.png
610 key e0 50
616 key e0 d0
640 screen {out}/down.png
650 key e0 4d
656 key e0 cd
680 screen {out}/right.png
690 mouse 120 180
720 screen {out}/mouse.png
721 quit
'''
SCREENS = ('menu', 'down', 'right', 'mouse')


def install(cd, work):
    target = work / 'install'
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(target), '--cd', str(cd), '--no-movies'],
                   check=True, capture_output=True, env=dict(os.environ, CRUSADER_GAME='noremorse'))
    return target


def contents(directory):
    """{path: sha256} of the files under directory."""
    return {str(p.relative_to(directory)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(directory.rglob('*')) if p.is_file()}


def run(remorse, cd, target, out):
    """The screens' bytes of one run in the install target."""
    out.mkdir(parents=True)
    script = out / 'milestone.script'
    script.write_text(SCRIPT.format(out=out))
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd))
    env.pop('REGRET_SCREEN', None)
    # The game's output is code page 437 (the loading bar).
    result = subprocess.run([str(remorse)], cwd=target, env=env, capture_output=True, encoding='latin-1',
                            timeout=600)
    if result.returncode != 0:
        sys.exit(f'remorse ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    return {name: (out / f'{name}.png').read_bytes() for name in SCREENS}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('remorse', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', ROOT / 'cd/noremorse')))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s screens here')
    args = ap.parse_args()
    args.remorse = args.remorse.resolve()
    args.cd = args.cd.resolve()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH remorse_milestone: {what}')

    with tempfile.TemporaryDirectory(prefix='port-remorse-milestone') as work:
        work = Path(work)
        target = install(args.cd, work)
        installed = contents(target)
        first = run(args.remorse, args.cd, target, work / 'first')
        check(contents(target) == installed, 'the first run changed the install')
        second = run(args.remorse, args.cd, target, work / 'second')
        check(contents(target) == installed, 'the second run changed the install')
        if args.keep:
            args.keep.mkdir(parents=True, exist_ok=True)
            for name, data in first.items():
                (args.keep / f'{name}.png').write_bytes(data)
    for name in SCREENS:
        check(first[name] == second[name], f'{name}.png differs between two runs')
    for before, after in zip(SCREENS, SCREENS[1:]):
        check(first[before] != first[after], f'{after}.png is {before}.png: the input changed nothing')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(first[name]).hexdigest()[:16]}')
    print(f'remorse_milestone: {len(SCREENS)} screens, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
