"""Things that break: one of each kind of
damage.flx's entries (world.DAMAGE: a frame's step at the first hit, worn
down by points to absolute frames, to nothing, to an explosion with damage
or destroying, to another item, and one whose hits its usecode takes), each
hit by the usecode three times, harder each time (receiveHitIntrinsic:
takeHit's item path, Item::gotHit, DamageTable::breakItem)."""
import scenes
import ucasm

X0, Y0 = scenes.X0, scenes.Y0
SHAPES = list(scenes.BREAKABLES) + [scenes.ITEM, scenes.CONTAINER, scenes.BARREL]
LOCALS = {f'B{k}': -2 * (k + 1) for k in range(len(SHAPES))}
LOCALS['W'] = -2 * (len(SHAPES) + 1)
USECODE_SHAPE = 316
# The tests' names: how each shape takes damage (world.DAMAGE).
KINDS = {310: 'a frame step at the first hit', 311: 'worn down by absolute frames', 312: 'worn down to nothing',
         313: 'an explosion with damage', 314: 'a destroying explosion', 315: 'worn down into a barrel',
         316: 'its hits to its usecode', scenes.ITEM: 'the test item: frames and a sound',
         scenes.CONTAINER: 'the container: explodes and goes', scenes.BARREL: 'the barrel: becomes a test item'}


def code(table):
    call = lambda name, arguments: scenes.call(table, name, arguments, local_offsets=LOCALS)
    lines = [f'ENTER {2 * len(LOCALS)}']
    for k, shape in enumerate(SHAPES):
        lines += call('@Item@create$qusus', f'B{k} {shape} 0')
        lines += call('@Item@popToCoords$qususuc', f'B{k} {X0 + 60 + 40 * (k % 5)} {Y0 - 120 + 60 * (k // 5)} 0')
    lines += ['SUSPEND'] * 20
    for damage in (6, 30, 250):
        for k in range(len(SHAPES)):
            lines += call('@receiveHitIntrinsic$qn4Itemusucius', f'B{k} 0 4 {damage} 3') + ['SUSPEND'] * 6
            # a shape's test ends with its third hit
            lines += scenes.marker(table, k) if damage == 250 else []
        lines += ['SUSPEND'] * 30
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def hit_code():
    """The usecode's hit (event 6): nothing more."""
    return ucasm.assemble('ENTER 0\nPUSH_WORD 1\nPOP_GLOBAL 200 2\nRETURN')


def scene(game):
    """One thing of each kind of damage, each hit three times, harder each
    time.

    Checks: an item's damage (receiveHitIntrinsic: takeHit's item path,
        Item::gotHit, itemdam.cpp DamageTable::breakItem) for each kind of
        damage.flx's entries (world.DAMAGE): a frame's step at the first
        hit, worn down by points to absolute frames, to nothing, to an
        explosion with damage or destroying, to another item, and one whose
        hits its usecode takes (event 6).
    Drives: the test egg's event 7 (code): the things in rows east of the
        Silencer, then each hit by 6, by 30 and by 250 points.
    Tests:
        <kind>
            The thing of that kind after its third hit: its marker.
    Original: Both follow.
    """
    table = scenes.intrinsics_table(game)
    return scenes.Scene(name='breakables', maps={scenes.TEST_MAP: scenes.floor_and_egg(12)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table)}),
                                 USECODE_SHAPE: ('BREAKS', {6: hit_code()})},
                        arguments=scenes.START, frames=150 + 3 * (len(SHAPES) * 7 + 30) + 100,
                        tests=[KINDS[shape] for shape in SHAPES], ran=scenes.marked(game))


SCENES = {'breakables': scene}
