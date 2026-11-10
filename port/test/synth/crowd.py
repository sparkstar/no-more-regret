"""Many things at once: the egg starts 140
sprites together, so that the kernel's table of processes grows past its
100 (kernel.cpp growTable) and shrinks when they have ended (trimTable),
some of them blasts while the blasts are limited (sprite.cpp), and plays
150 sound effects one after another, so that the sound cache fills and
frees and compacts its blocks (sndcache.cpp)."""
import formats
import scenes
import ucasm

X0, Y0 = scenes.X0, scenes.Y0
BLASTS = (0x326, 0x31c, 0x31f, 0x320)
WAIT = 400


def code(table):
    lines = ['ENTER 2']
    for k in range(140):
        shape = BLASTS[k % 4] if k % 3 == 0 else scenes.ITEM
        x, y = X0 - 210 + 30 * (k % 14), Y0 - 150 + 30 * (k // 14)
        lines += scenes.call(table, '@createSpriteProcess$qususususususuc', f'{shape} 0 40 3 {x} {y} 0',
                             local_offsets={})
    # the big shapes' frames one after another: the shape cache fills,
    # evicts and compacts (cache/cache.cpp)
    for k, shape in enumerate(scenes.BIG_SHAPES):
        lines += scenes.call(table, '@createSpriteProcess$qususususususuc', f'{shape} 0 499 1 {X0 + 60 * k} {Y0} 0',
                             local_offsets={})
    lines += scenes.marker(table, 0)
    for k in range(1, 151):
        lines += scenes.call(table, '@playEffect$qi', f'{(k * 37) % 1000 + 1}', local_offsets={}) + ['SUSPEND']
    lines += scenes.marker(table, 1) + ['SUSPEND'] * WAIT
    for k in range(20):
        lines += scenes.call(table, '@createSpriteProcess$qususususususuc',
                             f'{scenes.ITEM} 0 10 1 {X0 + 30 * k} {Y0} 0', local_offsets={})
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def scene(game):
    """Many things at once: 140 sprites, the big shapes' frames, 150 sound
    effects in a row.

    Checks: the kernel's table of processes growing past its 100 and
        shrinking (kernel.cpp growTable, trimTable), the blasts limited
        (sprite.cpp), the shape cache filling, evicting and compacting
        (cache/cache.cpp makeRoom, compactCache), the sound cache's blocks
        (sndcache.cpp).
    Drives: the test egg's event 7 (code): 140 sprites at once (a third of
        them blasts), sprites of the big shapes, then a sound effect a pass;
        the map: 320 big items on the screen, each shape's frames 0-79.
    Tests:
        sprites
            The sprites started: its marker.
        sounds
            The 150 effects played: its marker.
    Original: Both follow.
    """
    table = scenes.intrinsics_table(game)
    # big items on the screen, each shape's frames 0-79: all in use at once,
    # more than the shape cache holds (cache.cpp makeRoom, compactCache)
    big = [formats.item_record(X0 - 240 + 24 * (k % 20), Y0 - 240 + 24 * (k // 20), 0, shape, frame=k % 80)
           for k, shape in enumerate(scenes.BIG_SHAPES[k // 80] for k in range(320))]
    return scenes.Scene(name='crowd', maps={scenes.TEST_MAP: scenes.floor_and_egg(12) + big},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table)})},
                        arguments=scenes.START, frames=150 + 150 + WAIT + 200, tests=['sprites', 'sounds'],
                        ran=scenes.marked(game))


SCENES = {'crowd': scene}
