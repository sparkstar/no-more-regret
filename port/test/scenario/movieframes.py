#!/usr/bin/env python3
"""The movies' frames (the movies and subtitles): every movie of the CD's flics
played the ways the game's callers play them (the script's movie, host.h's
hostScriptMovie) in a mission (-warp), with the sound on, and for each frame
finished on the screen a hash of the screen as the mode shows it (REGRET_FRAMES,
hostMovieFrame). The movies the game plays itself before the script's (No
Remorse's mission briefing) count as "(start)"; a frame shown again at once (a
movie stalled) counts once. The mission is saved before the first and loaded
before each, so that a movie starts from the same game whatever the movies
before it did (the time they took moves the world on, which the greyed screen
under the next shows).

No Remorse's player stops at the end of a movie whose data runs into the
file's last part of a 4K block (PLAYER.C's refill reads 4K blocks and takes
none of a shorter last one, and nextChunk waits for it): the movie stays on
its frame until Escape, Space or Enter, the original's as the port's. Each
such movie plays in a run of its own, Escape pressed once it should have
ended.

The runs of each game (RUNS): No Regret's (its intro's ORIGIN in form 2, the
others as the usecode plays them) with the subtitles, big (each pixel 2 by 2),
in hi-colour and in low resolution, and without the subtitles; No Remorse's with
the subtitles in each of its callers' forms (0 the usecode's cutscene, 1 its
other cutscene and the intro, 2 its overlay, 3 the weasel's shop at a place),
big, and without the subtitles. And in Korean (enhanced.cfg's language=ko:
port/lang/ko's translations of the subtitles, drawn with the Hangul font) the
movies it translates (KOREAN), No Regret's in hi-colour too. For each run and
movie the frames and a hash of their lines must be those of
port/<game>/test/expected/movieframes.tsv, which --write writes: the screens
kept, which a change meant to keep the movies must keep. Without the
movie files it is skipped (exit 77, which ctest counts as skipped).

usage: movieframes.py PROGRAM [--cd CD] [--write] [--runs NAME,...] [--keep DIR]
"""
import argparse, concurrent.futures, hashlib, os, struct, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402
SKIPPED = 77
TABLE = overlay.TESTS / 'expected/movieframes.tsv'
# The mission (No Remorse's second: its first plays a briefing first, whose
# length would set the palette's cycling for the movies after it) and the
# script's first movie, once the mission is up and its palette on (the
# monitors' movies draw with the game's palette); the movies queue at the end
# of that pass and play one after the other.
WARP = {'regret': 1, 'remorse': 2}[overlay.GAME]
START = 600
# The ticks of a frame (15 a second) and what a movie takes besides: the
# run quits once all have surely ended.
FRAME_TICKS = 4
MOVIE_TICKS = 300
# Escape's scan code, pressed and let go.
ESCAPE = ('01', '81')
# No Remorse's callers' forms (common/src/movie/moviehost.cpp's
# scriptPlayMovie): the movies the usecode plays as its other cutscene and
# the intro's, the overlay's, and the weasel's shop's (wezzy.cpp) at a place
# (x 200, or 80 in the 320x200 mode, below the shop's top).
ALT_CUTSCENES = ('t01', 't02', 'r01', 'b01', 'o01', 'o02', 'wec', '01d')
WEASEL = ('17a', '18d', '18e', '18f', '19c', '19d', '21a', '21b', '21c', '21d', '21e', '21f')
WEASEL_Y = 100
# The CD's movies no caller plays and the player cannot: No Regret's
# WEAVED.AVI has a sound stream of format 0, for which XANPLAY.C's player
# has no converter ("XDmovie init() failed"); the usecode names it nowhere.
UNPLAYABLE = {'regret': {'weaved'}, 'remorse': set()}[overlay.GAME]
# The runs at once, each a program of its own on a core (the tests'
# PROCESSORS count fewer: the short runs end early).
JOBS = 6


# The movies whose subtitles port/lang/ko translates (the game's text in
# other languages): the intro's and mission 1's.
KOREAN = {'regret': ['anim01', '01a', '01b', '01c', '01d', '01e', '01f', '01g', '01h', '01i'],
          'remorse': ['t02', '01a', '01b', '01c', '01d', '01e']}[overlay.GAME]


def regret_runs(names):
    # The intro's ORIGIN in its form (never doubled), the others the usecode's.
    plain = [(name, 2 if name == 'origin' else 0, 0, 0) for name in names]
    korean = [(name, 0, 0, 0) for name in KOREAN if name in names]
    return {
        'plain': (['subtitles=true'], plain),
        'big': (['subtitles=true', 'bigvideo=true'], plain),
        'hicolor': (['subtitles=true', 'moviehicolor=true'], plain),
        'lowres': (['subtitles=true', 'lowresmovies=true'], plain),
        'nosubtitles': ([], plain),
        'korean': (['subtitles=true', 'language=ko'], korean),
        'koreanhicolor': (['subtitles=true', 'moviehicolor=true', 'language=ko'], korean),
    }


def remorse_runs(names):
    cutscenes = [(name, 0, 0, 0) for name in names]
    forms = (cutscenes + [(name, 1, 0, 0) for name in ALT_CUTSCENES if name in names]
             + [(name, 2, 0, 0) for name in names if name.startswith('mva')]
             + [(name, 3, 200, WEASEL_Y) for name in WEASEL if name in names])
    big = cutscenes + [(name, 3, 80, WEASEL_Y) for name in WEASEL if name in names]
    # Each in the form its caller plays it (the intro's T02 and 01d in 1).
    korean = [(name, 1 if name in ALT_CUTSCENES else 0, 0, 0) for name in KOREAN if name in names]
    return {
        'plain': (['subtitles=true'], forms),
        'big': (['subtitles=true', 'bigvideo=true'], big),
        'nosubtitles': ([], cutscenes),
        'korean': (['subtitles=true', 'language=ko'], korean),
    }


RUNS = {'regret': regret_runs, 'remorse': remorse_runs}[overlay.GAME]


def flics(cd):
    """The CD's flics directory, whatever the case of its name."""
    for entry in cd.iterdir():
        if entry.is_dir() and entry.name.upper() == 'FLICS':
            return entry
    return None


def movie_frames(path):
    """The frames of an AVI file's main header."""
    data = path.read_bytes()[:0x400]
    at = data.find(b'avih')
    return struct.unpack_from('<I', data, at + 8 + 16)[0]


def stalls(path):
    """Whether No Remorse's player stalls at the end of the movie: its movi
    list ends past the file's last whole 4K block."""
    if overlay.GAME != 'remorse':
        return False
    data = path.read_bytes()
    at = data.find(b'movi') - 8
    return at + 8 + struct.unpack_from('<I', data, at + 4)[0] > len(data) // 0x1000 * 0x1000


def run(program, cd, movies, work, settings, plays, escape=False):
    """The frames' lines of a run, by movie: (name, form, x, y) to a list;
    with escape, Escape is pressed once the movies should have ended."""
    work.mkdir(parents=True)
    install = work / 'install'
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(cd), '--sound'],
                   check=True, capture_output=True)
    # The originals' settings in crusader.cfg, the port's (language=) in
    # enhanced.cfg.
    ours = [line for line in settings if line.startswith('language=')]
    config = install / 'crusader.cfg'
    config.write_bytes(config.read_bytes() + ''.join(f'{line}\r\n' for line in settings if line not in ours).encode())
    if ours:
        (install / 'enhanced.cfg').write_bytes(''.join(f'{line}\r\n' for line in ours).encode())
    files = {path.stem.lower(): path for path in movies.iterdir() if path.suffix.upper() == '.AVI'}
    ticks = sum(movie_frames(files[name]) * FRAME_TICKS + MOVIE_TICKS for name, _, _, _ in plays)
    script = work / 'script'
    end = START + ticks
    keys = f'{end} key {ESCAPE[0]}\n{end + 6} key {ESCAPE[1]}\n' if escape else ''
    script.write_text(f'{START} save 0\n'
                      + ''.join(f'{START} load 0\n{START} movie {name} {form} {x} {y}\n' for name, form, x, y in plays)
                      + keys + f'{end + 120} quit\n')
    frames = work / 'frames.txt'
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_FRAMES=str(frames))
    for name in ('REGRET_AUDIO', 'REGRET_SCREEN', 'REGRET_STATE'):
        env.pop(name, None)
    result = subprocess.run([str(program), '-warp', str(WARP)], cwd=install, env=env, capture_output=True,
                            encoding='latin-1', timeout=3600)
    if result.returncode != 0:
        sys.exit(f'{program.name} ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    lines = {('(start)', 0, 0, 0): []}
    current = lines[('(start)', 0, 0, 0)]
    for line in frames.read_text().splitlines():
        if line.startswith('movie '):
            _, name, form, x, y = line.split()
            current = lines.setdefault((name, int(form), int(x), int(y)), [])
        elif not current or current[-1] != line:
            current.append(line)
    return lines


def summary(lines):
    """A movie's frames and the first 16 digits of the SHA-256 of their
    lines."""
    return len(lines), hashlib.sha256('\n'.join(lines).encode()).hexdigest()[:16]


def read_table():
    rows = {}
    if TABLE.exists():
        for line in TABLE.read_text().splitlines():
            if line and not line.startswith('#'):
                run_name, name, form, x, y, frames, digest = line.split('\t')
                rows[(run_name, name, int(form), int(x), int(y))] = (int(frames), digest)
    return rows


def write_table(rows):
    header = ('# The movies\' frames (port/test/scenario/movieframes.py --write): for each run and movie (its name, the\n'
              '# callers\' form and place) the frames finished on the screen and the first 16 digits of the\n'
              '# SHA-256 of their lines (each the frame and the hash of the screen). The screens of the\n'
              '# movies as kept, which a change meant to keep them must keep.\n')
    body = ''.join(f'{r}\t{n}\t{f}\t{x}\t{y}\t{frames}\t{digest}\n'
                   for (r, n, f, x, y), (frames, digest) in rows.items())
    TABLE.write_text(header + body)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', overlay.ORIGIN)))
    ap.add_argument('--write', action='store_true', help='write the table from this build')
    ap.add_argument('--runs', help='only these runs, by name, comma-separated')
    ap.add_argument('--keep', type=Path, help='keep the runs here')
    args = ap.parse_args()
    cd = args.cd.resolve()
    movies = flics(cd)
    names = sorted(p.stem.lower() for p in movies.iterdir() if p.suffix.upper() == '.AVI') if movies else []
    names = [name for name in names if name not in UNPLAYABLE]
    if not names:
        print(f'movieframes: no movies in {cd}, skipped')
        sys.exit(SKIPPED)
    runs = RUNS(names)
    chosen = args.runs.split(',') if args.runs else list(runs)
    expected = read_table()
    found = {}
    program = args.program.resolve()
    with tempfile.TemporaryDirectory(prefix='port-movieframes') as temporary:
        base = args.keep.resolve() if args.keep else Path(temporary)
        files = {path.stem.lower(): path for path in movies.iterdir() if path.suffix.upper() == '.AVI'}
        # The runs, each a program of its own: all at once (JOBS).
        jobs = []
        for run_name in chosen:
            settings, plays = runs[run_name]
            stalling = [play for play in plays if stalls(files[play[0]])]
            plays = [play for play in plays if play not in stalling]
            jobs.append((run_name, None, base / run_name, settings, plays))
            for play in stalling:
                name, form, x, y = play
                jobs.append((run_name, play, base / f'{run_name}-{name}-{form}', settings, [play]))
        with concurrent.futures.ThreadPoolExecutor(max_workers=JOBS) as pool:
            done = [(run_name, alone, pool.submit(run, program, cd, movies, work, settings, plays, alone is not None))
                    for run_name, alone, work, settings, plays in jobs]
            for run_name, alone, future in done:
                lines = future.result()
                if alone is not None:
                    found[(run_name, *alone)] = summary(lines.get(alone, []))
                    continue
                for (name, form, x, y), frames in lines.items():
                    if name == '(start)' and not frames:
                        continue
                    found[(run_name, name, form, x, y)] = summary(frames)
    if args.write:
        rows = {key: value for key, value in expected.items() if key[0] not in chosen}
        rows.update(found)
        write_table(dict(sorted(rows.items())))
        print(f'movieframes: {len(found)} movies of {", ".join(chosen)} written to {TABLE.relative_to(ROOT)}')
        return
    failures = 0
    for key in sorted(set(found) | {k for k in expected if k[0] in chosen}):
        if found.get(key) != expected.get(key):
            failures += 1
            print(f'MISMATCH movieframes: {" ".join(map(str, key))}: {found.get(key)} for {expected.get(key)}')
    print(f'movieframes: {len(found)} movies of {", ".join(chosen)}, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
