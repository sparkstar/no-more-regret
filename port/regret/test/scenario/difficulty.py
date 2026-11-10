#!/usr/bin/env python3
"""The start menu's first difficulty, chosen by Enter or a mouse click.

Both runs start a new game at the same tick in fresh installs. The mouse
path must reach the mission with the same screen and player state as the
keyboard path, rather than closing the difficulty dialog back to the menu.

usage: difficulty.py REGRET [--cd CD] [--keep DIR]
"""
import argparse, os, shutil, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
# The mouse driver's x is twice the GUI's; (600, 145) hits WEEKEND WARRIOR.
SCRIPTS = {
    'keyboard': '''\
300 key 1c
306 key 9c
350 key e0 48
356 key e0 c8
370 key e0 48
376 key e0 c8
420 key 1c
426 key 9c
690 screen {out}/mission.png
700 quit
''',
    'mouse': '''\
300 key 1c
306 key 9c
380 mouse 600 145
390 mouse 600 145 1
420 mouse 600 145 0
690 screen {out}/mission.png
700 quit
''',
}


def run(regret, cd, work, name):
    target = work / name
    target.mkdir()
    install = target / 'install'
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(cd), '--no-movies'],
                   check=True, capture_output=True)
    script = target / 'difficulty.script'
    script.write_text(SCRIPTS[name].format(out=target))
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'))
    env.pop('REGRET_SCREEN', None)
    result = subprocess.run([str(regret)], cwd=install, env=env, capture_output=True, encoding='latin-1', timeout=600)
    if result.returncode != 0:
        sys.exit(f'difficulty {name}: regret ended with {result.returncode}:\n'
                 f'{result.stdout[-2000:]}{result.stderr[-2000:]}')
    screen = (target / 'mission.png').read_bytes()
    state = (target / 'state.txt').read_text().splitlines()
    if not state:
        sys.exit(f'difficulty {name}: no game state was recorded')
    return screen, [int(value, 16) if i == 2 else int(value) for i, value in enumerate(state[-1].split())]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('regret', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', ROOT / 'cd/noregret')))
    ap.add_argument('--keep', type=Path, help='keep both scripts, screens and states here')
    args = ap.parse_args()
    regret, cd = args.regret.resolve(), args.cd.resolve()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH difficulty: {what}')

    with tempfile.TemporaryDirectory(prefix='port-difficulty') as temporary:
        work = Path(temporary)
        keyboard_screen, keyboard = run(regret, cd, work, 'keyboard')
        mouse_screen, mouse = run(regret, cd, work, 'mouse')
        if args.keep:
            for name in SCRIPTS:
                kept = args.keep.resolve() / name
                kept.mkdir(parents=True, exist_ok=True)
                for file in ('difficulty.script', 'mission.png', 'state.txt'):
                    shutil.copyfile(work / name / file, kept / file)

    check(keyboard[4] != 0 and keyboard[5] != 0 and keyboard[9] > 0, f'keyboard did not enter the mission: {keyboard}')
    check(mouse[4] != 0 and mouse[5] != 0 and mouse[9] > 0, f'mouse did not enter the mission: {mouse}')
    check(mouse[4:10] == keyboard[4:10], f'player differs: mouse {mouse[4:10]}, keyboard {keyboard[4:10]}')
    check(mouse_screen == keyboard_screen, 'mission screen differs between mouse and keyboard')
    print(f'difficulty: mouse and keyboard, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
