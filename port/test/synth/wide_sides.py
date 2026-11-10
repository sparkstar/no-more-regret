#!/usr/bin/env python3
"""The wide screen's sides under movies and the credits, on the synthetic
scenes' data: the
movies scene (movies.py) and the credits (specials.credits) replayed with
enhanced.cfg's widescreen=1, the screen written at frames of the scene
(REGRET_SCRIPT's screen) and its sides seen:

- covered: the screen 640 wide (the window's frame black beside it): No
  Regret's movies on black, a Xan movie's screen cleared to its darkest
  colour (in either game), the credits;
- grey: No Remorse's movies on the greyed world (the cutscenes; the shop's
  too), the sides greyed as the middle is: each pixel the fixed grey
  (toFixedGray: 0-7, F8h-FFh by the luminance of its colour, BasePalette's
  toYiq) of the pixel of the world's sides at a frame of the world in the
  game's palette (WORLD);
- world: the world drawn in them, 854 wide: the scene's world stands still,
  so they are WORLD's, which has a tenth of them drawn (not black).

The frames are the scenes' (each movie played at a fixed pass); a change of
movies.py's timing moves them.

usage: wide_sides.py PROGRAM --game regret|remorse
"""
import argparse, os, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parents[1] / 'tools'))
import scenes  # noqa: E402
import world  # noqa: E402
import specials  # noqa: E402
import replays  # noqa: E402

# A frame of the movies scene with the world in the game's palette (No
# Remorse's: after XANA, before the fade to grey of JYVA), the sides of
# the others are seen against.
WORLD = {'remorse': 210, 'regret': 142}
# (frame, what the sides show) of the movies scene: No Remorse's movies are
# XANA (Xan: cleared, covered), RRVA (cutscene on grey), RRVB (the overlay:
# the world), JYVA (the cutscene with the fade to grey), XANB (Xan on grey:
# cleared, covered); No Regret's five on black.
MOVIES = {
    'remorse': [(165, 'covered'), (226, 'grey'), (276, 'world'), (442, 'grey'), (505, 'covered'), (600, 'world')],
    'regret': [(100, 'covered'), (160, 'covered'), (226, 'covered'), (311, 'covered'), (396, 'covered'),
               (600, 'world')],
}
# The credits start at the scene's start and go to the start menu.
CREDITS = [(300, 'covered'), (600, 'covered')]

def fixed_grey(colour):
    """toFixedGray's entry for a colour of the PNG's palette (8 bits, the
    VGA's 6 shifted by 2): the luminance's level, 0-7 or F8h-FFh."""
    red, green, blue = (c >> 2 for c in colour)
    level = (red * 299 + green * 587 + blue * 114) // 1000 >> 2
    return level if level < 8 else level + 0xf0


def shots(program, game, scene, frames, work):
    """The scene replayed wide, the PNG of the screen at each frame
    ({frame: bytes}, b'' where none was written) and what went wrong."""
    install, cd, recording = world.make(work, game, scene, None)
    settings = recording / 'files' / 'enhanced.cfg'
    settings.write_bytes((settings.read_bytes() if settings.exists() else b'') + b'widescreen=1\r\n')
    script = work / 'shots.script'
    script.write_text(''.join(f'{frame} screen shot{frame}.png\n' for frame in frames))
    env = {k: v for k, v in os.environ.items() if k not in ('DISPLAY', 'WAYLAND_DISPLAY', 'REGRET_STATE',
                                                            'REGRET_SCREEN', 'REGRET_USECODE_TRACE')}
    env.update(CRUSADER_CD=str(cd), REGRET_SCRIPT=str(script), REGRET_AUDIO_DRIVER='dummy')
    run = subprocess.run([str(program), '--debug-feature', '--replay', str(recording)], cwd=install, env=env,
                         capture_output=True, encoding='latin-1', timeout=600)
    wrong = '' if run.returncode == 0 else f'the replay: exit {run.returncode}, {run.stderr[-400:]}'
    found = {}
    for frame in frames:
        path = install / f'shot{frame}.png'
        found[frame] = path.read_bytes() if path.exists() else b''
    return found, wrong


def judged(png, expected, world_png):
    """What is wrong with the sides of png for what they should show, '' for
    nothing."""
    if not png:
        return 'no screen'
    width, pixels = replays.side_pixels(png)
    if expected == 'covered':
        return '' if width == 640 else f'{width} wide, the sides not covered'
    if width != 854:
        return f'{width} wide, not 854 with the sides'
    world_width, world_pixels = replays.side_pixels(world_png)
    world_palette = replays.png_palette(world_png)
    lit = sum(1 for p in world_pixels if world_palette[p] != (0, 0, 0)) / max(1, len(world_pixels))
    if world_width != width or lit <= 0.1:
        return f'the world\'s frame has no sides drawn ({world_width} wide, {lit:.0%} not black)'
    if expected == 'world':
        differ = sum(1 for a, b in zip(pixels, world_pixels) if a != b)
        return '' if differ == 0 else f'{differ} of {len(pixels)} side pixels not the world\'s'
    greys = bytes(fixed_grey(world_palette[p]) for p in world_pixels)
    differ = sum(1 for a, b in zip(pixels, greys) if a != b)
    return '' if differ == 0 else f'{differ} of {len(pixels)} side pixels not the world\'s fixed greys'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--game', choices=('regret', 'remorse'), required=True)
    args = ap.parse_args()
    program = args.program.resolve()
    failures = 0
    with tempfile.TemporaryDirectory(prefix='port-wide-sides') as temporary:
        for name, scene, expected in (('movies', scenes.all_scenes()['movies'](args.game), MOVIES[args.game]),
                                      ('credits', specials.credits(args.game), CREDITS)):
            world_frame = WORLD[args.game]
            frames = sorted({frame for frame, _ in expected} | {world_frame})
            found, wrong = shots(program, args.game, scene, frames, Path(temporary) / name)
            if wrong:
                failures += 1
                print(f'MISMATCH wide_sides {args.game} {name}: {wrong}')
                continue
            wrongs = 0
            for frame, what in expected:
                said = judged(found[frame], what, found[world_frame])
                if said:
                    wrongs += 1
                    print(f'MISMATCH wide_sides {args.game} {name} at {frame} ({what}): {said}')
            failures += wrongs
            print(f'wide_sides {args.game} {name}: {len(expected) - wrongs} of {len(expected)} screens as they should be')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
