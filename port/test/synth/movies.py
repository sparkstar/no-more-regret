"""The movies: our own AVI files on the CD's
FLICS, played by the egg's usecode as the games' usecode plays them (No
Regret's playMovie, No Remorse's cutscenes and overlay). The originals have
their own players, which the oracle does not follow: the port alone runs
these scenes (twice the same), and their frames can go to the originals'
decoders. Both codecs in both games (the port's
decoder takes either): Xan frames (whole and changed, with chroma and the
luma's corrections) with Xan's sound in mono and stereo, RRV1 frames (runs,
pairs, slices left as they are) with 8-bit sound, RRV2 without sound, JYV1
with 16-bit stereo sound; subtitles in each game's form. One movie is cut by
Escape."""
import dataclasses
import struct

import formats
import menus
import scenes
import ucasm

FRAMES = 12
XAN_SIZE = (64, 32)
RRV_SIZE = (96, 60)
WAVE_FORMAT_XAN = 0x594a
# Xan's sound: 22050 a second, a block of 1471 bytes a channel each frame
# (audconv.cpp isXanFormat).
XAN_RATE, XAN_BLOCK = 22050, 22050 // 15 + 1
LEAD = 2


def xan_sound(channels):
    block = XAN_BLOCK * channels
    return (b'Axan', formats.wave_format(WAVE_FORMAT_XAN, channels, XAN_RATE, 16, block, block * 15)), \
        bytes((k * 7 + 3) & 0xff for k in range(block))


def pcm_sound(channels, bits, rate):
    align = bits // 8 * channels
    per_frame = rate * align // 15
    return (b'\0\0\0\0', formats.wave_format(1, channels, rate, bits)), bytes(0x80 if bits == 8 else 0
                                                                             for _ in range(per_frame))


def interleave(video, sound=None):
    """The movi chunks: the sound's lead, then each frame and its sound."""
    if sound is None:
        return video
    _, chunk = sound
    chunks = [(b'01wb', chunk)] * LEAD
    for frame in video:
        chunks += [frame, (b'01wb', chunk)]
    return chunks


def xan_movie(channels=1):
    width, height = XAN_SIZE
    steps = width // 2 * height
    corrections = bytes((k * 5) & 0x3f for k in range(32))
    table = [0x0204, 0x0806, 0x1010, 0x2020]
    video = [(b'00dc', formats.xan_frame(width, height, 0, [k % 3 for k in range(steps)],
                                         chroma=(0, table, bytes(k % 4 for k in range(width // 4 * height // 4))),
                                         corrections=corrections))]
    for frame in range(1, FRAMES):
        second = corrections[::-1] if frame % 3 == 2 else None
        chroma = (1, table, bytes(k % 4 for k in range(64))) if frame == 4 else None
        video.append((b'00dc', formats.xan_frame(width, height, 1, [(k + frame) % 3 for k in range(steps)],
                                                 chroma=chroma, corrections=corrections if frame % 2 else None,
                                                 second=second)))
    handler_format, chunk = xan_sound(channels)
    return formats.avi(width, height, b'Xxan', formats.bitmap_format(width, height),
                       interleave(video, (handler_format, chunk)), move=formats.xan_move(), sound=handler_format)


def rrv_movie(handler, slices, sound=None, pairs_from_frame=6):
    width, height = RRV_SIZE

    def runs(frame):
        def of(slice_index, count):
            half = count // 2
            colour = (frame * 7 + slice_index * 13) & 0x3f
            if (frame + slice_index) % 3 == 0:
                return [(0, bytes([colour + 96] * count))]
            return [(half, bytes([colour + 96] * (count - half)))]
        return of

    video = []
    for frame in range(FRAMES):
        pairs = handler != b'JYV1' and frame >= pairs_from_frame
        tag = b'00db' if pairs else b'00dc'
        unchanged = (frame % slices,) if frame > 0 else ()
        video.append((tag, formats.rrv_frame(width, height, slices, runs(frame), pairs=pairs, unchanged=unchanged)))
    format_of = formats.bitmap_format(width, height)
    if sound is None:
        return formats.avi(width, height, handler, format_of, video)
    handler_format, chunk = sound
    return formats.avi(width, height, handler, format_of, interleave(video, (handler_format, chunk)),
                       sound=handler_format)


LINES = [(1, b'THE FIRST LINE.'), (5, b'THE SECOND LINE.'), (9, b'')]


def subtitles(game):
    """name's subtitles in the game's form and extension."""
    if game == 'regret':
        return 'IFF', formats.iff_subtitles([(frame, text) for frame, text in LINES if text])
    return 'TXT', formats.text_subtitles(LINES)


def files(game):
    """The CD's FLICS: the movies and their subtitles."""
    kind, text = subtitles(game)
    pcm8 = pcm_sound(1, 8, 11025)
    pcm16 = pcm_sound(2, 16, 22050)
    out = {
        'XANA.AVI': xan_movie(1), f'XANA.{kind}': text,
        'RRVA.AVI': rrv_movie(b'RRV1', 5, pcm8), f'RRVA.{kind}': text,
        'RRVB.AVI': rrv_movie(b'RRV2', 15),
        'JYVA.AVI': rrv_movie(b'JYV1', 5, pcm16),
        'XANB.AVI': xan_movie(2),
    }
    return out


# Each movie by its name, and how the game's usecode plays it.
PLAYS = ['XANA', 'RRVA', 'RRVB', 'JYVA', 'XANB']
# Escape while RRVA plays (No Regret's from frame 152 to 186, No Remorse's
# from 222 to 242), held ESCAPE_HELD frames: No Remorse's player cancels on
# Escape, Space or Enter down when it looks, at a frame of the movie, and
# loses a press shorter than a frame; No Regret's on any key that comes.
CUT = 1
ESCAPE_AT = {'regret': 168, 'remorse': 228}
ESCAPE_HELD = 10


def shown(record, k, cut):
    """Whether movie k (of PLAYS) showed all its frames, or some of them for
    the one cut (cut, None for none): the frames log's movies
    (REGRET_FRAMES, Record.movies)."""
    if len(record.movies) != len(PLAYS):
        return False
    count = len(record.movies[k])
    return 0 < count < FRAMES if k == cut else count == FRAMES


def play(table, game, name, form):
    """The tokens of a play of name: No Regret's playMovie (name, restore
    the palette); No Remorse's cutscene, its other cutscene and the overlay
    by form (item, name, x, y)."""
    if game == 'regret':
        number, size, _ = table['@playMovie$qnzcuc']
        return ['PUSH_WORD 1', f'PUSH_STRING "{name}"', 'STRING_TO_POINTER', f'CALL_INTRINSIC {size} {number}',
                f'MOVE_TOP -{size}']
    intrinsic = ['@Item@playMovieCutscene$qnzcii', '@Item@playMovieCutsceneAlt$qnzcii',
                 '@Item@playMovieOverlay$qnzcii'][form % 3]
    number, size, _ = table[intrinsic]
    return ['PUSH_WORD 40', 'PUSH_WORD 60', f'PUSH_STRING "{name}"', 'STRING_TO_POINTER', 'PUSH_LOCAL_LONG 6',
            f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP -{size}']


def code(table, game):
    lines = ['ENTER 2']
    for form, name in enumerate(PLAYS):
        lines += play(table, game, name, form) + scenes.marker(table, form) + ['SUSPEND'] * 20
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def scene(game):
    """Our own movies played as the games' usecode plays them, one cut by
    Escape, with subtitles.

    Checks: the movies' player (movie/: avifile.cpp, the Xan and RRV
        decoders, audconv.cpp, the subtitles): Xan frames (whole and
        changed, with chroma and the luma's corrections) with Xan's sound in
        mono and stereo, RRV1 frames (runs, pairs, slices left as they are)
        with 8-bit sound, RRV2 without sound, JYV1 with 16-bit stereo
        sound, subtitles in each game's form; both codecs in both games.
    Drives: the CD's FLICS (files: our AVI files of FRAMES (12) frames
        and subtitles); the test egg's event 7 (code) plays each (No
        Regret's playMovie; No Remorse's cutscene, its other cutscene and
        the overlay), its marker after it, 20 passes apart; Escape held
        ESCAPE_HELD (10) frames from ESCAPE_AT while RRVA plays. The
        frames shown go to the records (REGRET_FRAMES: a hash of the screen
        a frame), whose hash and each movie's run.py keeps with the results.
    Tests:
        XANA
            Xan, mono sound, subtitles: all its 12 frames shown (the frames
            log), its marker after it.
        RRVA
            RRV1, 8-bit sound, subtitles; Escape held during it: cut, some
            of its frames shown and not all (No Regret 1, No Remorse 2).
        RRVB
            RRV2, no sound: all 12.
        JYVA
            JYV1, 16-bit stereo sound: all 12.
        XANB
            Xan, stereo sound: all 12.
    Original: None: the originals' own players are not followed; the
        frames can go to the originals' decoders instead (No Regret's
        XANDRV32.DLL, No Remorse's CRUSADER.EXE player).
    Traps: a movie plays inside one pass (the intrinsic's wait): the
        passes' frames jump over it, so a key meant for a movie is placed
        by the frames its play spans (ESCAPE_AT), not by passes. No
        Remorse's player looks at the keys down at its frames: a press of
        2 frames falls between them and cuts nothing, and No Regret's cuts
        XANA, not RRVA.
    """
    table = scenes.intrinsics_table(game)
    inputs = menus.press(ESCAPE_AT[game], ['esc'], ESCAPE_HELD)
    return scenes.Scene(name='movies', maps={scenes.TEST_MAP: scenes.floor_and_egg(8)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table, game)})},
                        arguments=scenes.START, frames=900, tests=PLAYS,
                        ran=scenes.checked(game, lambda record, marks, k: shown(record, k, CUT)),
                        inputs=inputs, oracle=False, movies=files(game),
                        config=['subtitles=true'])


def big(game):
    """The movies doubled, without subtitles, and No Regret's in high
    colour.

    Variant of: movies
    Checks: the doubled frames (bigvideo) and No Regret's high colour
        (moviehicolor).
    Drives: the movies scene with crusader.cfg's subtitles=false,
        bigvideo=true (and No Regret's moviehicolor=true), no keys: each
        movie shows all its 12 frames (the footprints check it, RRVA's
        too).
    Original: None: as movies (the originals' decoders can take the frames).
    """
    config = ['subtitles=false', 'bigvideo=true'] + (['moviehicolor=true'] if game == 'regret' else [])
    return dataclasses.replace(scene(game), name='movies-big', config=config, inputs=[],
                               ran=scenes.checked(game, lambda record, marks, k: shown(record, k, None)))


SCENES = {'movies': scene, 'movies-big': big}
