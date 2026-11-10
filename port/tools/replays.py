#!/usr/bin/env python3
"""The player's recordings as a regression test: each recording under
replays/ of the game of CRUSADER_GAME (its run.tsv names the game), replayed
without a window by PROGRAM in a fresh install (tools/install.py --sound,
the CD the repository's), its states compared with those the recording holds
(checks.tsv, each 60th pass). It fails on a recording the replay is told is
not as recorded, that does not run to its end, or in which a frame passes
inside one of the game's waits: the host's work of a frame (the rewind's,
the state's) runs only between the game's steps.

usage: replays.py PROGRAM [--accept | --wide MODE] [--trace DIR] [--length short|long] [RECORDING...]

The recordings hold the player's files (their saves, crusader.cfg and
enhanced.cfg) and stay out of git (.gitignore's /replays/); without any of the
game it exits 77, which ctest counts as skipped. --accept writes a
recording's checks.tsv again from this program's states, after a change
meant to change the game's run (the old one kept as checks.tsv.<date>).
--wide replays a copy of each with the experimental wide screen on (its
enhanced.cfg's widescreen=MODE), whose rules stay the 640x480 screen's: as
recorded still, and the last screen with the sides drawn. A recording whose
game ended itself has no last picture (DOS's text screen): its sides are not
looked at, but at least one recording of the game's must have them. --trace
writes each one's usecode trace (REGRET_USECODE_TRACE, platform/trace.h) to
DIR/<recording>.trace, for tools/usecode.py coverage. RECORDING: recordings
to replay instead (directories elsewhere too).
"""
import argparse, datetime, os, re, shutil, struct, subprocess, sys, tempfile, zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
RECORDINGS = ROOT / 'replays'


def game_of(directory):
    """The game a recording is of (its run.tsv), None for no recording."""
    run = directory / 'run.tsv'
    if not run.exists():
        return None
    fields = dict(line.split('\t', 1) for line in run.read_text().splitlines() if '\t' in line)
    return fields.get('game')


# A recording of at least this many frames (ten minutes) is a long one:
# --length short replays the others (the quick tests'), --length long these
# (the tests labelled slow: ctest -LE slow leaves them out).
LONG_FRAMES = 10 * 60 * 60


def length_of(directory):
    """The recording's frames: its end line's frame, else its last input's."""
    last = 0
    for line in (directory / 'inputs.tsv').read_text().splitlines():
        words = line.split('\t')
        if line and not line.startswith('#') and words[0].isdigit():
            last = int(words[0])
    return last


def recordings(length=None):
    """The recordings of this game, in name order; with length 'short' or
    'long', those shorter than LONG_FRAMES or not."""
    found = [directory for directory in sorted(RECORDINGS.glob('*')) if game_of(directory) == overlay.GAME]
    if length == 'short':
        return [directory for directory in found if length_of(directory) < LONG_FRAMES]
    if length == 'long':
        return [directory for directory in found if length_of(directory) >= LONG_FRAMES]
    return found


def replay(program, install, recording, state=None, screen=None, trace=None):
    """The replay's return code and standard error."""
    env = dict(os.environ, CRUSADER_CD=str(overlay.ORIGIN))
    for name in ('REGRET_SCRIPT', 'REGRET_SCREEN', 'REGRET_AUDIO', 'REGRET_STATE', 'REGRET_USECODE_TRACE', 'DISPLAY',
                 'WAYLAND_DISPLAY'):
        env.pop(name, None)
    if state:
        env['REGRET_STATE'] = str(state)
    if screen:
        env['REGRET_SCREEN'] = str(screen)
    if trace:
        env['REGRET_USECODE_TRACE'] = str(trace)
    run = subprocess.run([str(program), '--debug-feature', '--replay', str(recording)], cwd=install, env=env,
                         capture_output=True, encoding='latin-1', timeout=3600)
    return run.returncode, run.stderr


def wide_copy(recording, work, mode):
    """A copy of the recording with the wide screen on in its enhanced.cfg."""
    copy = work / f'{recording.name}-wide'
    shutil.copytree(recording, copy)
    settings = copy / 'files' / 'enhanced.cfg'
    text = settings.read_bytes() if settings.exists() else b''
    settings.write_bytes(text + f'widescreen={mode}\r\n'.encode())
    return copy


def png_chunks(png):
    """The port's PNG's chunks: {kind: body}, the IDATs joined."""
    chunks = {}
    at = 8
    while at < len(png):
        size = struct.unpack_from('>I', png, at)[0]
        kind, body = png[at + 4:at + 8], png[at + 8:at + 8 + size]
        chunks[kind] = chunks.get(kind, b'') + body if kind == b'IDAT' else body
        at += 12 + size
    return chunks


def png_palette(png):
    """The port's PNG's palette: 256 (red, green, blue) of 8 bits."""
    body = png_chunks(png).get(b'PLTE', b'')
    return [tuple(body[3 * k:3 * k + 3]) for k in range(len(body) // 3)]


def side_pixels(png):
    """The screen's width and its sides' pixels (the palette's indices of
    the columns beyond the 640 in its middle, row by row), from the port's
    PNG (palette, rows unfiltered)."""
    chunks = png_chunks(png)
    width, height = struct.unpack_from('>II', chunks[b'IHDR'])
    raw = zlib.decompress(chunks[b'IDAT'])
    side = (width - 640) // 2
    pixels = b''
    for y in range(height if side > 0 else 0):
        row = raw[y * (width + 1) + 1:(y + 1) * (width + 1)]
        pixels += row[:side] + row[side + 640:]
    return width, pixels


def sides_drawn(png):
    """The screen's width and the share of its sides' pixels not of colour
    0. The map's void is black too: a tenth drawn is the sides drawn."""
    width, pixels = side_pixels(png)
    if not pixels:
        return width, 0.0
    return width, sum(1 for index in pixels if index != 0) / len(pixels)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--accept', action='store_true', help="write the recordings' checks again from this program")
    ap.add_argument('--wide', type=int, choices=(1, 2), help='replay copies with the wide screen on')
    ap.add_argument('--trace', type=Path, help="write each recording's usecode trace to this directory")
    ap.add_argument('--length', choices=('short', 'long'),
                    help='only the recordings shorter than ten minutes, or only the others')
    ap.add_argument('recordings', type=Path, nargs='*', help="instead of the game's under replays/")
    args = ap.parse_intermixed_args()
    found = [path.resolve() for path in args.recordings] or recordings(args.length)
    other = [path.name for path in found if game_of(path) != overlay.GAME]
    if other:
        print(f'replays: not recordings of {overlay.GAME}: {" ".join(other)}')
        return 2
    if args.trace:
        args.trace.mkdir(parents=True, exist_ok=True)
    if not found:
        print(f'replays: no {args.length + " " if args.length else ""}recording of {overlay.GAME} in '
              f'{RECORDINGS.relative_to(ROOT)}')
        return 77
    failures = 0
    sided = 0
    with tempfile.TemporaryDirectory(prefix='port-replays') as work:
        work = Path(work)
        install = work / 'install'
        subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(overlay.ORIGIN),
                        '--sound'], check=True, capture_output=True)
        for recording in found:
            frames = sum(1 for line in (recording / 'inputs.tsv').read_text().splitlines()
                         if line and not line.startswith('#'))
            state = work / 'state.txt' if args.accept else None
            screen = work / f'{recording.name}.png' if args.wide else None
            played = wide_copy(recording, work, args.wide) if args.wide else recording
            trace = args.trace.resolve() / f'{recording.name}.trace' if args.trace else None
            code, error = replay(args.program.resolve(), install, played, state, screen, trace)
            ended = [line for line in error.splitlines() if line.startswith('replay: ') and ' frames) in ' in line]
            told = [line for line in error.splitlines() if 'not as recorded after pass' in line]
            inside = re.search(r'(\d+) inside the game\'s waits', ended[-1]) if ended else None
            waits = int(inside.group(1)) if inside else 0
            if args.accept and code == 0 and ended:
                checks = ['\t'.join(line.split()) for line in state.read_text().splitlines()
                          if int(line.split()[0]) % 60 == 0]
                old = recording / 'checks.tsv'
                shutil.copy2(old, recording / f'checks.tsv.{datetime.date.today():%Y%m%d}')
                old.write_text('# pass\tstate\n' + '\n'.join(checks) + '\n')
                print(f'replays: {recording.name}: {len(checks)} checks written again')
                continue
            if code != 0 or not ended or told:
                failures += 1
                print(f'MISMATCH replays: {recording.name}: {code}, ' +
                      ' '.join(told[:1] + ended[-1:]) + (error[-500:] if not ended else ''))
            elif waits != 0:
                failures += 1
                print(f'MISMATCH replays: {recording.name}: {waits} frames inside the game\'s waits, '
                      'none of them between two steps')
            elif args.wide and screen.exists() and sides_drawn(screen.read_bytes())[1] < 0.1:
                failures += 1
                width, share = sides_drawn(screen.read_bytes())
                print(f'MISMATCH replays: {recording.name}: the wide screen\'s last picture {width} wide, '
                      f'{share:.0%} of its sides drawn')
            else:
                if args.wide and screen.exists():
                    sided += 1
                    width, share = sides_drawn(screen.read_bytes())
                    print(f'replays: {recording.name}: the last picture {width} wide, {share:.0%} of its sides drawn')
                elif args.wide:
                    print(f'replays: {recording.name}: no last picture (the game ended itself, back to the text '
                          'screen): its sides not looked at')
                print(f'replays: {recording.name}: {ended[-1][len("replay: "):]}, as recorded ({frames} inputs)')
    # The long recordings' part need not have one (the short part's does).
    if args.wide and not args.accept and sided == 0 and args.length != 'long':
        failures += 1
        print(f'MISMATCH replays: no recording of {overlay.GAME} with a last picture to look at the sides on')
    print(f'replays {overlay.GAME}: {len(found)} recordings, {failures} not as recorded or with frames inside a wait')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
