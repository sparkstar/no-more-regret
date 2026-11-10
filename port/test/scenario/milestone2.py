#!/usr/bin/env python3
"""Milestone 2: the first mission under scripted
input. The game of CRUSADER_GAME is installed from its CD
(port/tools/install.py, without its movies) into a fresh directory; the
script starts a new game (NEW GAME, the first difficulty), and when the
Silencer has walked in, turns him with the keypad's 8 and walks him with
it (NumLock off: the game's isometric directions), turns back with the 2
and walks, and fires with the space bar four times (No Remorse's eight).
Each pass of the game's loop writes the state (REGRET_STATE: the random
seed, the processes, the Silencer's place and hit points, the NPCs' hit
points and dead). It runs twice: the screens and the states must be the
same bytes, the Silencer must have moved, the other NPCs lost hit points
and one of them died while the Silencer lives.

usage: milestone2.py PROGRAM [--cd CD] [--keep DIR]
"""
import argparse, hashlib, os, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402
# Ticks are sixtieths of a second. The menu is up well before 300; the
# mission's first screen comes at 700, when the Silencer has walked in. The
# same walk in both games; No Remorse's guards take more shots (and send
# another, whose hit points count from his coming).
WALK = '''\
300 key 1c
306 key 9c
420 key 1c
426 key 9c
690 screen {out}/mission.png
700 key 48
704 key c8
712 key 48
742 key c8
760 key 50
764 key d0
772 key 50
790 key d0
800 screen {out}/walked.png
810 key 39
816 key b9
850 key 39
856 key b9
890 key 39
896 key b9
930 key 39
936 key b9
'''
SCRIPT = WALK + {
    'regret': '''\
960 screen {out}/fired.png
961 quit
''',
    'remorse': '''\
970 key 39
976 key b9
1010 key 39
1016 key b9
1050 key 39
1056 key b9
1090 key 39
1096 key b9
1150 screen {out}/fired.png
1151 quit
''',
}[overlay.GAME]
SCREENS = ('mission', 'walked', 'fired')


def run(program, cd, work, name):
    """The screens' and the states' bytes of a run in a fresh install."""
    target = work / name
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(target / 'install'), '--cd', str(cd), '--no-movies'],
                   check=True, capture_output=True)
    script = target / 'milestone2.script'
    script.write_text(SCRIPT.format(out=target))
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'))
    env.pop('REGRET_SCREEN', None)
    result = subprocess.run([str(program)], cwd=target / 'install', env=env, capture_output=True, encoding='latin-1',
                            timeout=600)
    if result.returncode != 0:
        sys.exit(f'{overlay.GAME} ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    screens = {name: (target / f'{name}.png').read_bytes() for name in SCREENS}
    return screens, (target / 'state.txt').read_text()


def states(text):
    """The state lines as {pass: fields}: pass, time, seed, processes,
    Silencer's item, x, y, z, direction, hit points, NPCs' hit points, dead,
    alert."""
    out = {}
    for line in text.splitlines():
        fields = line.split()
        out[int(fields[0])] = [int(f, 16) if k == 2 else int(f) for k, f in enumerate(fields)]
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', overlay.ORIGIN)))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s screens and states here')
    args = ap.parse_args()
    args.program = args.program.resolve()
    args.cd = args.cd.resolve()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH milestone2: {what}')

    with tempfile.TemporaryDirectory(prefix='port-milestone2') as work:
        work = Path(work)
        first, first_state = run(args.program, args.cd, work, 'first')
        second, second_state = run(args.program, args.cd, work, 'second')
        if args.keep:
            args.keep.mkdir(parents=True, exist_ok=True)
            for name, data in first.items():
                (args.keep / f'{name}.png').write_bytes(data)
            (args.keep / 'state.txt').write_text(first_state)
    for name in SCREENS:
        check(first[name] == second[name], f'{name}.png differs between two runs')
    check(first_state == second_state, 'the states differ between two runs')
    for before, after in zip(SCREENS, SCREENS[1:]):
        check(first[before] != first[after], f'{after}.png is {before}.png')
    s = states(first_state)
    passes = sorted(s)
    at = lambda p: s[max(k for k in passes if k <= p)]
    start, walked, end = at(690), at(800), passes[-1]
    check((walked[5], walked[6]) != (start[5], start[6]), f'the Silencer did not move: {start[5:7]}')
    others = lambda f: f[10]-f[9]
    # The fewest hit points the others had after the walk (a guard who
    # comes later brings his).
    fewest = min(others(s[p]) for p in passes if p >= 800)
    check(fewest < others(walked), f'no NPC was hit: {others(walked)} then {fewest}')
    check(s[end][11] > walked[11], 'no NPC died')
    check(s[end][9] > 0, 'the Silencer died')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(first[name]).hexdigest()[:16]}')
    print(f'state {hashlib.sha256(first_state.encode()).hexdigest()[:16]}: moved {start[5:7]} to {walked[5:7]}, '
          f'NPCs {others(walked)} to {fewest} hit points, {s[end][11]-walked[11]} dead, '
          f'Silencer {s[end][9]}')
    print(f'milestone2: {len(SCREENS)} screens, {len(passes)} passes, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
