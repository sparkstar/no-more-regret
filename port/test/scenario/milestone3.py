#!/usr/bin/env python3
"""Milestone 3: the sound of the main menu and the first mission. The game
is installed for the port's sound card (install.py --sound: a Sound Blaster
16, the sound effects and the music on; --no-movies: no movie) and driven
by milestone 2's input: the menu, NEW GAME, the mission, walking and
firing. Its sound comes out on the virtual time as a WAV (REGRET_AUDIO).
Two runs must give the same WAV, screens and states to the byte; the WAV
must be the card's format, of the run's length, with the menu's music and
the mission's (louder with the shots) in it. With --other the second run is
another build's (the other compiler's).

usage: milestone3.py PROGRAM [--other PROGRAM] [--cd CD] [--keep DIR]
"""
import argparse, hashlib, math, os, struct, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
import milestone2

SCREENS = milestone2.SCREENS
RATE = 22050
# The WAV's seconds: milestone 2's script quits at 961 sixtieths (No
# Remorse's at 1151); the card starts once the game has started the sound
# (ASS_Init), in its first second (No Remorse's in its second).
LENGTH = {'regret': (14.5, 16.1), 'remorse': (17.5, 19.2)}[overlay.GAME]


def run(program, cd, work, name):
    """The WAV's, screens' and states' bytes of a run in a fresh install."""
    target = work / name
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(target / 'install'), '--cd', str(cd),
                    '--sound', '--no-movies'], check=True, capture_output=True)
    script = target / 'milestone3.script'
    script.write_text(milestone2.SCRIPT.format(out=target))
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'),
               REGRET_AUDIO=str(target / 'sound.wav'))
    env.pop('REGRET_SCREEN', None)
    result = subprocess.run([str(program)], cwd=target / 'install', env=env, capture_output=True, encoding='latin-1',
                            timeout=600)
    if result.returncode != 0:
        sys.exit(f'{overlay.GAME} ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    screens = {n: (target / f'{n}.png').read_bytes() for n in SCREENS}
    return (target / 'sound.wav').read_bytes(), screens, (target / 'state.txt').read_text()


def loudness(wav, start, end):
    """The RMS of the left channel from start to end seconds."""
    data = wav[44:]
    first, last = int(start * RATE) * 4, int(end * RATE) * 4
    samples = struct.unpack(f'<{(last - first) // 2}h', data[first:last])[0::2]
    return math.sqrt(sum(s * s for s in samples) / max(1, len(samples)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--other', type=Path, help='the second run\'s build')
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', overlay.ORIGIN)))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s WAV, screens and states here')
    args = ap.parse_args()
    program = args.program.resolve()
    other = (args.other or args.program).resolve()
    cd = args.cd.resolve()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH milestone3: {what}')

    with tempfile.TemporaryDirectory(prefix='port-milestone3') as work:
        work = Path(work)
        wav, screens, state = run(program, cd, work, 'first')
        wav2, screens2, state2 = run(other, cd, work, 'second')
        if args.keep:
            args.keep.mkdir(parents=True, exist_ok=True)
            (args.keep / 'sound.wav').write_bytes(wav)
            for name, data in screens.items():
                (args.keep / f'{name}.png').write_bytes(data)
            (args.keep / 'state.txt').write_text(state)
    check(wav == wav2, 'the WAVs differ between two runs')
    check(screens == screens2, 'the screens differ between two runs')
    check(state == state2, 'the states differ between two runs')
    channels, rate, bits = struct.unpack_from('<H', wav, 22)[0], struct.unpack_from('<I', wav, 24)[0], wav[34]
    size = struct.unpack_from('<I', wav, 40)[0]
    check(wav[:4] == b'RIFF' and (channels, rate, bits) == (2, RATE, 16), f'the WAV is {channels}x{bits} at {rate}')
    check(size == len(wav) - 44, 'the WAV\'s data size')
    seconds = size / 4 / RATE
    check(LENGTH[0] < seconds < LENGTH[1], f'the WAV lasts {seconds:.2f} seconds')
    menu, mission = loudness(wav, 1, 4), loudness(wav, 12, seconds)
    check(menu > 200, f'the menu\'s music: {menu:.0f}')
    check(mission > menu, f'the mission with the shots ({mission:.0f}) louder than the menu ({menu:.0f})')
    s = milestone2.states(state)
    passes = sorted(s)
    at = lambda p: s[max(k for k in passes if k <= p)]
    start, walked, end = at(690), at(800), passes[-1]
    check((walked[5], walked[6]) != (start[5], start[6]), 'the Silencer did not move')
    check(s[end][11] > walked[11], 'no NPC died')
    print(f'sound.wav {hashlib.sha256(wav).hexdigest()[:16]}: {seconds:.2f} seconds, menu {menu:.0f}, '
          f'mission {mission:.0f}')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(screens[name]).hexdigest()[:16]}')
    print(f'state {hashlib.sha256(state.encode()).hexdigest()[:16]}')
    print(f'milestone3: {len(passes)} passes, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
