#!/usr/bin/env python3
"""The experimental wide screen in the game, No Regret's first mission
with enhanced.cfg's widescreen=1: the sides drawn (the screen 854 wide, a
tenth of its sides not black); the game menu (F1) covering them (the
screen 640 wide, the window's frame black beside it) and the sides back
when it closes (Esc).

usage: widescreen.py PROGRAM
"""
import argparse, os, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import install, overlay, replays  # noqa: E402

# The script's passes: NEW GAME, LOOSE CANNON, the mission's start, F1,
# Esc.
SCRIPT = '''120 mouse 180 183
125 mouse 180 183 1
130 mouse 180 183 0
950 mouse 638 259
955 mouse 638 259 1
960 mouse 638 259 0
2100 screen start.png
2200 key 3b
2204 key bb
2300 screen menu.png
2400 key 01
2404 key 81
2500 screen back.png
2505 quit
'''


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    args = ap.parse_args()
    if overlay.GAME != 'regret':
        print('widescreen: No Regret only')
        return 77
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH widescreen: {what}')

    with tempfile.TemporaryDirectory(prefix='port-widescreen') as temp:
        target = Path(temp) / 'install'
        install.install(target, overlay.ORIGIN, sound=False, movies=False)
        (target / 'enhanced.cfg').write_text(f'game={overlay.GAME}\nwidescreen=1\n')
        script = target / 'wide.script'
        script.write_text(SCRIPT)
        env = dict(os.environ, CRUSADER_CD=str(overlay.ORIGIN), REGRET_SCRIPT=str(script))
        for key in ('REGRET_STATE', 'REGRET_SCREEN', 'REGRET_AUDIO', 'PORT_SNAPSHOT_TEST', 'PORT_REWIND_TEST',
                    'DISPLAY', 'WAYLAND_DISPLAY'):
            env.pop(key, None)
        result = subprocess.run([str(args.program.resolve()), '--game', overlay.GAME], cwd=target, env=env,
                                capture_output=True, timeout=300)
        check(result.returncode == 0, f'the run: exit {result.returncode}')
        shots = {name: (target / f'{name}.png').read_bytes() if (target / f'{name}.png').exists() else b''
                 for name in ('start', 'menu', 'back')}
        check(all(shots.values()), 'three screens')
        if all(shots.values()):
            width, share = replays.sides_drawn(shots['start'])
            check(width == 854 and share > 0.1, f'the start: 854 wide with its sides ({width}, {share:.0%})')
            width, _ = replays.sides_drawn(shots['menu'])
            check(width == 640, f'the game menu covers the sides ({width} wide)')
            width, share = replays.sides_drawn(shots['back'])
            check(width == 854 and share > 0.1, f'the sides again after the menu ({width}, {share:.0%})')
    print(f'widescreen: {failures} mismatches')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
