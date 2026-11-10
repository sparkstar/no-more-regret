#!/usr/bin/env python3
"""The movies: the intro the game plays when it starts, ORIGIN's logo
and ANIM01, from the CD's FLICS, with the sound on (install.py
--sound). No input: the movies play to their end and the main
menu comes. It runs twice in fresh installs: the screens (in ORIGIN, three
in ANIM01, the menu after them), the states and the WAV of the sound must
be the same bytes; each movie's screen must be a picture (many colours) and
each different; the WAV must have the movies' sound. Then twice with the
hi-colour movies (crusader.cfg's moviehicolor, VESA 111h), to ANIM01's
second screen: the same bytes again, pictures (the PNGs are of red, green
and blue), one of more colours than a palette holds, each different, and
the 8-bit run's sound as far as it goes (the movies keep to their sound).
Without the movie files (the repository has none) it is skipped (exit 77,
which ctest counts as skipped). --keep keeps the hi-colour run's in
hicolor/.

usage: movies.py REGRET [--cd CD] [--keep DIR]
"""
import argparse, hashlib, math, os, struct, subprocess, sys, tempfile, zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
SKIPPED = 77
RATE = 22050
# The screens by 1/60 s: ORIGIN (15.5 s from the start of the game), ANIM01
# (145 s after it), the menu once they have ended.
SCREENS = {'origin': 240, 'anim01a': 1500, 'anim01b': 5000, 'anim01c': 8600, 'menu': 12000}
QUIT = 12060
# The hi-colour run's: ORIGIN and the start of ANIM01.
HICOLOR_SCREENS = {'origin': 240, 'anim01a': 1500, 'anim01b': 5000}
HICOLOR_QUIT = 5060


def flics(cd):
    """The CD's FLICS directory, whatever the case of its name."""
    for entry in cd.iterdir():
        if entry.is_dir() and entry.name.upper() == 'FLICS':
            return entry
    return None


def run(regret, cd, work, name, hicolor=False):
    target = work / name
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(target / 'install'), '--cd', str(cd),
                    '--sound'], check=True, capture_output=True)
    shots, quit = (HICOLOR_SCREENS, HICOLOR_QUIT) if hicolor else (SCREENS, QUIT)
    if hicolor:
        config = target / 'install' / 'crusader.cfg'
        config.write_bytes(config.read_bytes().replace(b'moviehicolor=false', b'moviehicolor=true'))
    script = target / 'movies.script'
    script.write_text(''.join(f'{tick} screen {target}/{screen}.png\n' for screen, tick in shots.items())
                      + f'{quit} quit\n')
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'),
               REGRET_AUDIO=str(target / 'sound.wav'))
    env.pop('REGRET_SCREEN', None)
    result = subprocess.run([str(regret)], cwd=target / 'install', env=env, capture_output=True, encoding='latin-1',
                            timeout=1800)
    if result.returncode != 0:
        sys.exit(f'regret ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    screens = {n: (target / f'{n}.png').read_bytes() for n in shots}
    return (target / 'sound.wav').read_bytes(), screens, (target / 'state.txt').read_text()


def colours(png):
    """The colours a screen of the port's PNG (filter 0 rows) uses: its
    palette indexes, or its red, green and blue triples."""
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


def loudness(wav, start, end):
    data = wav[44:]
    first, last = int(start * RATE) * 4, int(end * RATE) * 4
    samples = struct.unpack(f'<{(last - first) // 2}h', data[first:last])[0::2]
    return math.sqrt(sum(s * s for s in samples) / max(1, len(samples)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('regret', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', ROOT / 'cd/noregret')))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s WAV, screens and states here')
    args = ap.parse_args()
    cd = args.cd.resolve()
    movies = flics(cd)
    names = {entry.name.upper() for entry in movies.iterdir()} if movies else set()
    if not {'ORIGIN.AVI', 'ANIM01.AVI'} <= names:
        print(f'movies: no ORIGIN.AVI and ANIM01.AVI in {cd}/FLICS, skipped')
        sys.exit(SKIPPED)
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH movies: {what}')

    with tempfile.TemporaryDirectory(prefix='port-movies') as work:
        work = Path(work)
        wav, screens, state = run(args.regret.resolve(), cd, work, 'first')
        wav2, screens2, state2 = run(args.regret.resolve(), cd, work, 'second')
        hwav, hscreens, hstate = run(args.regret.resolve(), cd, work, 'hicolor', True)
        hwav2, hscreens2, hstate2 = run(args.regret.resolve(), cd, work, 'hicolor2', True)
        if args.keep:
            for keep, (w, shots, st) in ((args.keep, (wav, screens, state)),
                                         (args.keep / 'hicolor', (hwav, hscreens, hstate))):
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
    origin, anim01 = loudness(wav, 2, 14), loudness(wav, 30, 150)
    check(origin > 500 and anim01 > 500, f'the movies\' sound: {origin:.0f}, {anim01:.0f}')
    print(f'sound.wav {hashlib.sha256(wav).hexdigest()[:16]}: {seconds:.1f} seconds, ORIGIN {origin:.0f}, '
          f'ANIM01 {anim01:.0f}')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(screens[name]).hexdigest()[:16]}: {counts[name]} colours')

    # The hi-colour movies.
    check(hwav == hwav2, 'hi-colour: the WAVs differ between two runs')
    for name in HICOLOR_SCREENS:
        check(hscreens[name] == hscreens2[name], f'hi-colour: {name}.png differs between two runs')
    check(hstate == hstate2, 'hi-colour: the states differ between two runs')
    hcounts = {name: colours(hscreens[name]) for name in HICOLOR_SCREENS}
    for name, count in hcounts.items():
        check(count >= 32, f'hi-colour: {name}.png has {count} colours')
    check(max(hcounts.values()) > 256, 'hi-colour: no screen has more colours than a palette')
    check(len(set(hscreens.values())) == len(HICOLOR_SCREENS), 'hi-colour: two screens are the same')
    horigin = loudness(hwav, 2, 14)
    check(horigin > 500, f'hi-colour: the movies\' sound: {horigin:.0f}')
    # The movies' pace is their sound's: the same bytes as the 8-bit run's, as far as it goes.
    check(len(hwav) < len(wav) and hwav[44:] == wav[44:len(hwav)], 'hi-colour: the sound is not the 8-bit run\'s')
    print(f'hi-colour sound.wav {hashlib.sha256(hwav).hexdigest()[:16]}: ORIGIN {horigin:.0f}')
    for name in HICOLOR_SCREENS:
        print(f'hi-colour {name}.png {hashlib.sha256(hscreens[name]).hexdigest()[:16]}: {hcounts[name]} colours')
    print(f'movies: {len(state.splitlines())} passes, {len(hstate.splitlines())} hi-colour, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
