#!/usr/bin/env python3
"""No Remorse's help (WORLD.C's HelpGump): in a fresh install
(port/tools/install.py, without its movies) the script starts a new game
and in the first mission opens the help with F1: its first page
(static/help1.dat), the right arrow to the second (help2.dat) and again to
close it; then F1, E and three minus signs to the hidden page (help3.dat)
and Escape to close it.

Each page must be drawn (the two pages are text in a dozen colours, the
hidden one a photograph) other than the game and than the other pages,
the game must be on the screen again after each closing, the run must go
to the end of its script, and two runs must give the same screens and
states.

usage: help.py REMORSE [--cd CD] [--keep DIR]
"""
import argparse, hashlib, os, struct, subprocess, sys, tempfile, zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
PIT_HZ = 1193182
# Ticks are sixtieths of a second; the first mission is up at 700.
SCRIPT = '''\
300 key 1c
306 key 9c
420 key 1c
426 key 9c
890 screen {out}/game.png
900 key 3b
906 key bb
960 screen {out}/page1.png
1000 key e0 4d
1006 key e0 cd
1060 screen {out}/page2.png
1100 key e0 4d
1106 key e0 cd
1160 screen {out}/closed.png
1200 key 3b
1206 key bb
1260 key 12
1266 key 92
1280 key 0c
1286 key 8c
1300 key 0c
1306 key 8c
1320 key 0c
1326 key 8c
1380 screen {out}/hidden.png
1420 key 01
1426 key 81
1480 screen {out}/back.png
1500 quit
'''
SCREENS = ('game', 'page1', 'page2', 'closed', 'hidden', 'back')
PAGES = ('page1', 'page2', 'hidden')


def run(remorse, cd, work, name):
    """The screens' and the states' bytes of a run in a fresh install."""
    target = work / name
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(target / 'install'), '--cd', str(cd),
                    '--no-movies'], check=True, capture_output=True, env=dict(os.environ, CRUSADER_GAME='noremorse'))
    script = target / 'help.script'
    script.write_text(SCRIPT.format(out=target))
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'))
    env.pop('REGRET_SCREEN', None)
    env.pop('REGRET_AUDIO', None)
    result = subprocess.run([str(remorse)], cwd=target / 'install', env=env, capture_output=True, encoding='latin-1',
                            timeout=600)
    if result.returncode != 0:
        sys.exit(f'remorse ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    screens = {n: (target / f'{n}.png').read_bytes() for n in SCREENS}
    return screens, (target / 'state.txt').read_text()


def colours(png):
    """The colours of the port's PNG (filter 0 rows): palette indexes, or
    red, green and blue triples."""
    at, data, width, sample = 8, b'', 0, 1
    while at < len(png):
        size = struct.unpack_from('>I', png, at)[0]
        kind, body = png[at + 4:at + 8], png[at + 8:at + 8 + size]
        if kind == b'IHDR':
            width = struct.unpack_from('>I', body)[0]
            sample = 3 if body[9] == 2 else 1
        elif kind == b'IDAT':
            data += body
        at += 12 + size
    raw = zlib.decompress(data)
    row = width * sample + 1
    used = set()
    for y in range(len(raw) // row):
        line = raw[y * row + 1:(y + 1) * row]
        used.update(line[k:k + sample] for k in range(0, len(line), sample))
    return len(used)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('remorse', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', ROOT / 'cd/noremorse')))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s screens and states here')
    args = ap.parse_args()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH remorse_help: {what}')

    with tempfile.TemporaryDirectory(prefix='port-remorse-help') as work:
        work = Path(work)
        first, first_state = run(args.remorse.resolve(), args.cd.resolve(), work, 'first')
        second, second_state = run(args.remorse.resolve(), args.cd.resolve(), work, 'second')
        if args.keep:
            args.keep.mkdir(parents=True, exist_ok=True)
            for name, data in first.items():
                (args.keep / f'{name}.png').write_bytes(data)
            (args.keep / 'state.txt').write_text(first_state)
    for name in SCREENS:
        check(first[name] == second[name], f'{name}.png differs between two runs')
    check(first_state == second_state, 'the states differ between two runs')
    counts = {name: colours(first[name]) for name in SCREENS}
    for name in PAGES:
        check(counts[name] >= 8, f'{name}.png has {counts[name]} colours')
        check(first[name] != first['game'], f'{name}.png is the game')
    check(len({first[name] for name in PAGES}) == len(PAGES), 'two pages are the same')
    for name in ('closed', 'back'):
        check(first[name] not in [first[page] for page in PAGES], f'{name}.png is a page of the help')
        check(counts[name] >= 32, f'{name}.png has {counts[name]} colours')
    passes = first_state.splitlines()
    check(bool(passes) and int(passes[-1].split()[1]) >= 1480 * PIT_HZ // 60, 'the game did not go on')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(first[name]).hexdigest()[:16]}: {counts[name]} colours')
    print(f'state {hashlib.sha256(first_state.encode()).hexdigest()[:16]}')
    print(f'remorse_help: {len(SCREENS)} screens, {len(passes)} passes, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
