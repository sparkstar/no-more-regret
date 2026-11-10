#!/usr/bin/env python3
"""No Remorse's credits and their movie (CREDITS.C): with the cheats on
(-laurie) the credits seen to their end play the movie z03. In a fresh
install (port/tools/install.py) Escape held skips the intro (T01, T02: a
key down ends a movie), the mouse clicks the main menu's CREDITS (a key
would end the credits at once: they run while no key is down), and the
credits scroll to their end, z03 plays from the CD's flics, and the main
menu comes back.

The credits' screens must be drawn and each other, the movie's two
pictures (many colours) other than them and than each other, the menu on
the screen at the end, and two runs must give the same screens and states.
Without z03 in the CD's flics (the repository has none) it is skipped
(exit 77, which ctest counts as skipped).

usage: credits.py REMORSE [--cd CD] [--keep DIR]
"""
import argparse, concurrent.futures, hashlib, os, subprocess, sys, tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import help  # noqa: E402
import movies  # noqa: E402

ROOT = Path(__file__).resolve().parents[4]
SKIPPED = 77
# Ticks are sixtieths of a second. The menu is up at 300 once Escape has
# ended the intro; CREDITS is at 544, 276 on the screen, the mouse driver's
# x twice the screen's. The credits take some 11,500 ticks, z03 some 3,000.
SCRIPT = '''\
100 key 01
200 key 81
400 mouse 1088 276
450 mouse 1088 276 1
456 mouse 1088 276 0
1050 screen {out}/designer.png
6450 screen {out}/translations.png
12450 screen {out}/movie.png
14850 screen {out}/later.png
17250 screen {out}/menu.png
17300 quit
'''
SCREENS = ('designer', 'translations', 'movie', 'later', 'menu')


def run(remorse, cd, work, name):
    """The screens' and the states' bytes of a run in a fresh install."""
    target = work / name
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(target / 'install'), '--cd', str(cd)],
                   check=True, capture_output=True, env=dict(os.environ, CRUSADER_GAME='noremorse'))
    script = target / 'credits.script'
    script.write_text(SCRIPT.format(out=target))
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'))
    env.pop('REGRET_SCREEN', None)
    env.pop('REGRET_AUDIO', None)
    result = subprocess.run([str(remorse), '-laurie'], cwd=target / 'install', env=env, capture_output=True,
                            encoding='latin-1', timeout=1800)
    if result.returncode != 0:
        sys.exit(f'remorse ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    screens = {n: (target / f'{n}.png').read_bytes() for n in SCREENS}
    return screens, (target / 'state.txt').read_text()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('remorse', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', ROOT / 'cd/noremorse')))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s screens and states here')
    args = ap.parse_args()
    cd = args.cd.resolve()
    flics = movies.flics(cd)
    names = {entry.name.upper() for entry in flics.iterdir()} if flics else set()
    if 'Z03.AVI' not in names:
        print(f'remorse_credits: no Z03.AVI in {cd}/flics, skipped')
        sys.exit(SKIPPED)
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH remorse_credits: {what}')

    with tempfile.TemporaryDirectory(prefix='port-remorse-credits') as work:
        work = Path(work)
        # The two runs at once, each in its own install.
        with concurrent.futures.ThreadPoolExecutor(2) as pool:
            runs = [pool.submit(run, args.remorse.resolve(), cd, work, name) for name in ('first', 'second')]
            (first, first_state), (second, second_state) = (r.result() for r in runs)
        if args.keep:
            args.keep.mkdir(parents=True, exist_ok=True)
            for name, data in first.items():
                (args.keep / f'{name}.png').write_bytes(data)
            (args.keep / 'state.txt').write_text(first_state)
    for name in SCREENS:
        check(first[name] == second[name], f'{name}.png differs between two runs')
    check(first_state == second_state, 'the states differ between two runs')
    counts = {name: help.colours(first[name]) for name in SCREENS}
    for name in ('designer', 'translations'):
        check(counts[name] >= 4, f'{name}.png has {counts[name]} colours')
    check(len({first[name] for name in SCREENS}) == len(SCREENS), 'two screens are the same')
    for name in ('movie', 'later', 'menu'):
        check(counts[name] >= 32, f'{name}.png has {counts[name]} colours')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(first[name]).hexdigest()[:16]}: {counts[name]} colours')
    print(f'state {hashlib.sha256(first_state.encode()).hexdigest()[:16]}')
    print(f'remorse_credits: {len(SCREENS)} screens, {len(first_state.splitlines())} passes, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
