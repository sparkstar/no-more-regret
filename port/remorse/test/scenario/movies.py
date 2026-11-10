#!/usr/bin/env python3
"""No Remorse's movies (No Regret's are
port/regret/test/scenario/movies.py): the intro the game plays before the
main menu, T01 (640x480: ORIGIN, Vested Interest, the title) and T02
(320x120), from the CD's flics through PLAYER.C's player, with the sound on
(install.py --sound). No input: the movies play to their end and the main
menu comes. It runs twice in fresh installs: the screens (three in T01, two
in T02, the menu after them), the states and the WAV of the sound must be
the same bytes; each movie's screen must be a picture (many colours) and
each different; the WAV must have the movies' sound. Then twice with the
subtitles and the big video (crusader.cfg's subtitles and bigvideo: T02
each pixel 2 by 2, its t02.txt under it) into T02: the same bytes again,
pictures, other than the first run's, the subtitles' band drawn, and the
first run's sound as far as it goes (the movies keep to their sound).
Without the movie files (the repository has none) it is skipped (exit 77,
which ctest counts as skipped). --keep keeps the second run's in big/.

usage: movies.py REMORSE [--cd CD] [--keep DIR]
"""
import argparse, hashlib, math, os, struct, subprocess, sys, tempfile, zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
SKIPPED = 77
RATE = 22050
# The screens by 1/60 s: T01 (its three titles), T02, the menu once they
# have ended.
SCREENS = {'t01a': 300, 't01b': 900, 't01c': 1800, 't02a': 4000, 't02b': 5000, 'menu': 8000}
QUIT = 8060
# The big video's: T02 while its subtitles show.
BIG_SCREENS = {'t02a': 3600, 't02b': 4000, 't02c': 4500}
BIG_QUIT = 4560
# T02 doubled is 240 rows from row 120; the subtitles' 16 rows come 8 under
# it (PLAYER.C showSubtitle).
SUBTITLE_ROWS = range(368, 384)


def flics(cd):
    """The CD's flics directory, whatever the case of its name."""
    for entry in cd.iterdir():
        if entry.is_dir() and entry.name.upper() == 'FLICS':
            return entry
    return None


def run(remorse, cd, work, name, big=False):
    target = work / name
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(target / 'install'), '--cd', str(cd),
                    '--sound'], check=True, capture_output=True, env=dict(os.environ, CRUSADER_GAME='noremorse'))
    shots, quit = (BIG_SCREENS, BIG_QUIT) if big else (SCREENS, QUIT)
    if big:
        config = target / 'install' / 'crusader.cfg'
        config.write_bytes(config.read_bytes() + b'subtitles=true\r\nbigvideo=true\r\n')
    script = target / 'movies.script'
    script.write_text(''.join(f'{tick} screen {target}/{screen}.png\n' for screen, tick in shots.items())
                      + f'{quit} quit\n')
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'),
               REGRET_AUDIO=str(target / 'sound.wav'))
    env.pop('REGRET_SCREEN', None)
    result = subprocess.run([str(remorse)], cwd=target / 'install', env=env, capture_output=True, encoding='latin-1',
                            timeout=1800)
    if result.returncode != 0:
        sys.exit(f'remorse ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    screens = {n: (target / f'{n}.png').read_bytes() for n in shots}
    return (target / 'sound.wav').read_bytes(), screens, (target / 'state.txt').read_text()


def pixels(png):
    """(width, sample, the rows' bytes) of the port's PNG (filter 0 rows):
    palette indexes, or red, green and blue triples."""
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
    return width, sample, [raw[y * row + 1:(y + 1) * row] for y in range(len(raw) // row)]


def colours(png, rows=None):
    """The colours a screen uses, in rows (all by default)."""
    _, sample, lines = pixels(png)
    used = set()
    for y in rows if rows is not None else range(len(lines)):
        line = lines[y]
        used.update(line[k:k + sample] for k in range(0, len(line), sample))
    return len(used)


def loudness(wav, start, end):
    data = wav[44:]
    first, last = int(start * RATE) * 4, int(end * RATE) * 4
    samples = struct.unpack(f'<{(last - first) // 2}h', data[first:last])[0::2]
    return math.sqrt(sum(s * s for s in samples) / max(1, len(samples)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('remorse', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', ROOT / 'cd/noremorse')))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s WAV, screens and states here')
    args = ap.parse_args()
    cd = args.cd.resolve()
    movies = flics(cd)
    names = {entry.name.upper() for entry in movies.iterdir()} if movies else set()
    if not {'T01.AVI', 'T02.AVI', 'T02.TXT'} <= names:
        print(f'remorse_movies: no T01.AVI, T02.AVI and T02.TXT in {cd}/flics, skipped')
        sys.exit(SKIPPED)
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH remorse_movies: {what}')

    with tempfile.TemporaryDirectory(prefix='port-remorse-movies') as work:
        work = Path(work)
        wav, screens, state = run(args.remorse.resolve(), cd, work, 'first')
        wav2, screens2, state2 = run(args.remorse.resolve(), cd, work, 'second')
        bwav, bscreens, bstate = run(args.remorse.resolve(), cd, work, 'big', True)
        bwav2, bscreens2, bstate2 = run(args.remorse.resolve(), cd, work, 'big2', True)
        if args.keep:
            for keep, (w, shots, st) in ((args.keep, (wav, screens, state)),
                                         (args.keep / 'big', (bwav, bscreens, bstate))):
                keep.mkdir(parents=True, exist_ok=True)
                (keep / 'sound.wav').write_bytes(w)
                for name, data in shots.items():
                    (keep / f'{name}.png').write_bytes(data)
                (keep / 'state.txt').write_text(st)
    check(wav == wav2, 'the WAVs differ between two runs')
    for name in SCREENS:
        check(screens[name] == screens2[name], f'{name}.png differs between two runs')
    check(state == state2, 'the states differ between two runs')
    counts = {name: colours(screens[name]) for name in SCREENS}
    for name, count in counts.items():
        check(count >= 32, f'{name}.png has {count} colours')
    check(len(set(screens.values())) == len(SCREENS), 'two screens are the same')
    seconds = struct.unpack_from('<I', wav, 40)[0] / 4 / RATE
    t01, t02 = loudness(wav, 2, 20), loudness(wav, 70, 110)
    check(t01 > 300 and t02 > 300, f'the movies\' sound: {t01:.0f}, {t02:.0f}')
    print(f'sound.wav {hashlib.sha256(wav).hexdigest()[:16]}: {seconds:.1f} seconds, T01 {t01:.0f}, T02 {t02:.0f}')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(screens[name]).hexdigest()[:16]}: {counts[name]} colours')

    # The big video with the subtitles.
    check(bwav == bwav2, 'big: the WAVs differ between two runs')
    for name in BIG_SCREENS:
        check(bscreens[name] == bscreens2[name], f'big: {name}.png differs between two runs')
    check(bstate == bstate2, 'big: the states differ between two runs')
    bcounts = {name: colours(bscreens[name]) for name in BIG_SCREENS}
    for name, count in bcounts.items():
        check(count >= 32, f'big: {name}.png has {count} colours')
        check(colours(bscreens[name], SUBTITLE_ROWS) > 1, f'big: {name}.png has no subtitle under the movie')
    check(len(set(bscreens.values())) == len(BIG_SCREENS), 'big: two screens are the same')
    check(bscreens['t02b'] != screens['t02a'], 'big: T02 at 4000 is the first run\'s')
    # The movies' pace is their sound's: the first run's bytes, as far as it goes.
    check(len(bwav) < len(wav) and bwav[44:] == wav[44:len(bwav)], 'big: the sound is not the first run\'s')
    print(f'big sound.wav {hashlib.sha256(bwav).hexdigest()[:16]}')
    for name in BIG_SCREENS:
        print(f'big {name}.png {hashlib.sha256(bscreens[name]).hexdigest()[:16]}: {bcounts[name]} colours')
    print(f'remorse_movies: {len(state.splitlines())} passes, {len(bstate.splitlines())} big, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
