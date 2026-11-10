"""Gravity, globs and the camera's snap eggs:
the egg's usecode drops items from the air, stacks crates with things on
them and breaks the lowest, throws items at a wall, along the floor (low
friction), straight up, throws an actor and a dying one, and puts the
Silencer on a raised platform again each time he has landed and has him
walk, run, jump and roll off its edges. Then the usecode takes him
far east and back, so that the chunks with glob eggs leave and enter the
fast area (a glob in the screen made at once, one outside it as the chunk's
records), and past the snap eggs, which hold the camera while he is in
their areas."""
import formats
import saves
import scenes
import ucasm
import world

X0, Y0 = scenes.X0, scenes.Y0
LOCALS = {'S': -2, 'N': -4, 'A': -6, 'B': -8, 'D': -10, 'W': -12, 'K': -14, 'C': -16}
FRAME = 16
# The platform: 2 by 2 tiles of 64 by 64 by 48 from PX east of the start
# (far enough that his runs and falls west keep out of the test egg's
# range); the Silencer stands on its middle.
PLATFORM, WALL, WALL_ALONG = scenes.PLATFORM, scenes.WALL, scenes.WALL_ALONG
PX, TOP = X0 + 384, 48
MIDDLE = (PX + 80, Y0 + 16)
# The yard west of the platform, on the screen while he is on it, and a
# wall west of it.
YX, YY = PX - 120, Y0
# Each round: where on the platform he stands, the animation the usecode
# has him do (as a player's keys would: he is the NPC the player controls,
# so the platform's edge holds him or throws him off it, animprim.cpp
# canStep) and its direction (0 north, 4 east, 8 south, 12 west). The
# usecode puts him back on the platform WAIT passes after each landing, or
# after LANDING passes without one: a walk east, north, a run west, a walk
# south, a jump on south, a roll forward east, a quick jump north from the
# edge, an advance west, and a walk north-east from the corner, which holds
# him at the edge. He keeps away from the test egg: it hatches again when he
# comes into its range.
WALK, RUN, ROLL_FORWARD, ADVANCE, JUMP_FORWARD, QUICK_JUMP = 0x01, 0x03, 0x05, 0x24, 0x38, 0x3a
ROUNDS = [(MIDDLE, WALK, 4), (MIDDLE, WALK, 0), (MIDDLE, RUN, 12), (MIDDLE, WALK, 8), (MIDDLE, JUMP_FORWARD, 8),
          (MIDDLE, ROLL_FORWARD, 4), ((PX + 80, Y0 - 30), QUICK_JUMP, 0), ((PX + 40, Y0 + 16), ADVANCE, 12),
          ((PX + 124, Y0 - 30), WALK, 2)]


def rounds(game):
    """ROUNDS of the game: No Remorse has no forward roll (animation 5 is only
    the engine's place; the port takes it as No Regret's): a quick jump east
    there instead."""
    if game == 'remorse':
        return [(spot, QUICK_JUMP if anim == ROLL_FORWARD else anim, direction) for spot, anim, direction in ROUNDS]
    return ROUNDS
WAIT, LANDING = 40, 120
# The rounds' names (the tests'): the animation and its direction.
ANIM_NAMES = {WALK: 'walk', RUN: 'run', ROLL_FORWARD: 'roll forward', ADVANCE: 'advance', JUMP_FORWARD: 'jump forward',
              QUICK_JUMP: 'quick jump'}
DIRECTION_NAMES = {0: 'north', 2: 'north-east', 4: 'east', 8: 'south', 12: 'west'}


def round_names(game):
    """The tests' names of the game's rounds."""
    return [f'{ANIM_NAMES[anim]} {DIRECTION_NAMES[direction]}' for _, anim, direction in rounds(game)]


# Glob 4's eggs in a row of chunks along his steps back west. A chunk that
# comes into the fast area is off the screen (the area reaches 3 chunks
# across the screen and 5 down): a glob is made at once only when a map is
# loaded (specials.py mapjump), and here goes into its chunk's records.
ROW = range(26, 46)
STEPS, STEP = 10, 256


def layout():
    """The test map: the floor, the platform, a wall west, the glob eggs (one
    in the start's chunk, one in the fast area off the screen, one beyond
    it) and the snap eggs (over the platform, and one moved by its map and
    NPC bytes)."""
    records = scenes.floor_and_egg(20)
    records += [formats.item_record(PX + 64 * i + 63, Y0 - 64 + 64 * j + 63, 0, PLATFORM)
                for i in range(2) for j in range(2)]
    records += [formats.item_record(YX - 185, YY - 200 + 64 * k + 63, 0, WALL) for k in range(4)]
    records += [formats.item_record(YX - 120 + 64 * k + 63, YY - 265, 0, WALL_ALONG) for k in range(4)]
    records += [formats.item_record(X0 - 64, Y0 - 300, 0, scenes.GLOB_EGG, quality=1),
                formats.item_record(X0 + 1536 + 100, Y0, 0, scenes.GLOB_EGG, quality=2),
                formats.item_record(X0 + 2560 + 40, Y0 + 20, 0, scenes.GLOB_EGG, quality=3),
                formats.item_record(X0 + 700, Y0 - 400, 0, scenes.GLOB_EGG, quality=5)]
    records += [formats.item_record(512 * column + 300, Y0 + 512 * row + 300, 0, scenes.GLOB_EGG, quality=4)
                for column in ROW for row in (-1, 0, 1)]
    records += [formats.item_record(PX + 64, Y0, TOP - 8, scenes.SNAP_EGG, quality=0x2200),
                formats.item_record(X0 - 100, Y0 + 300, 0, scenes.SNAP_EGG, quality=0x3100, map=2, npc=0xfe),
                formats.item_record(X0 + 2560, Y0, 0, scenes.SNAP_EGG, quality=0x4400),
                formats.item_record(X0 + 1300, Y0 - 200, 0, scenes.SNAP_EGG, quality=0x1100)]
    return records


def call(table, name, arguments):
    return scenes.call(table, name, arguments, local_offsets=LOCALS)


def wait(passes, label):
    w = LOCALS['W']
    return [f'PUSH_WORD {passes}', f'POP_LOCAL_WORD {w}', f'{label}:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1',
            'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', f'JUMP_IF_FALSE {label}_end', f'JUMP {label}',
            f'{label}_end:']


def landed(table, label):
    """Waits until the Silencer stands on the floor (his z 0), or LANDING
    passes."""
    number, size, _ = table['@Item@z$qv']
    c = LOCALS['C']
    return [f'PUSH_WORD {LANDING}', f'POP_LOCAL_WORD {c}', f'{label}:', 'SUSPEND', f'PUSH_LOCAL_WORD {c}', 'PUSH_WORD 1',
            'SUB', f'POP_LOCAL_WORD {c}', f'PUSH_LOCAL_WORD {c}', f'JUMP_IF_FALSE {label}_end',
            f'PUSH_LOCAL_ADDRESS {LOCALS["S"]}', f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP {-size}',
            'PUSH_RETURNED_BYTE', 'PUSH_BYTE 0', 'EQ', f'JUMP_IF_FALSE {label}', f'{label}_end:']


def made(table, shape, x, y, z, local='N'):
    """An item of shape made at x, y, z in the local."""
    return (call(table, '@Item@create$qusus', f'{local} {shape} 0') +
            call(table, '@Item@popToCoords$qususuc', f'{local} {x} {y} {z}'))


def things(table, game):
    """The items' physics in the yard west of the platform."""
    lines = []
    # things in the air fall
    for x, y, z in [(YX, YY - 100, 80), (YX - 40, YY - 100, 40), (YX, YY - 160, 120)]:
        lines += made(table, scenes.ITEM, x, y, z) + call(table, '@fall$qn4Item', 'N')
    # crates: A on the floor, B on A, an item on B; D with an item on it
    crate = world.crate(game)
    lines += made(table, crate, YX, YY + 200, 0, 'A')
    lines += made(table, crate, YX, YY + 200, 16, 'B')
    lines += made(table, scenes.ITEM, YX - 8, YY + 192, 32)
    lines += made(table, crate, YX - 60, YY + 200, 0, 'D')
    lines += made(table, scenes.ITEM, YX - 68, YY + 192, 16)
    # thrown: at the walls (west, north), along the floor with low
    # friction, straight up, and thrown again while it flies
    lines += made(table, scenes.ITEM, YX - 100, YY - 100, 0) + call(table, '@hurl$qn4Itemiiii', 'N -12 0 6 2')
    lines += made(table, scenes.ITEM, YX + 20, YY - 200, 0) + call(table, '@hurl$qn4Itemiiii', 'N 1 -12 6 2')
    lines += made(table, scenes.ITEM, YX - 100, YY - 40, 0) + call(table, '@hurl$qn4Itemiiii', 'N -8 2 0 2')
    lines += made(table, scenes.ITEM, YX - 60, YY - 140, 0) + call(table, '@Item@orFlags$qus', 'N 16384')
    lines += call(table, '@hurl$qn4Itemiiii', 'N 0 6 4 2')
    lines += made(table, scenes.ITEM, YX - 60, YY + 20, 0) + call(table, '@Item@orFlags$qus', 'N 16384')
    lines += call(table, '@hurl$qn4Itemiiii', 'N -10 -4 8 2')
    lines += made(table, scenes.ITEM, YX + 40, YY - 60, 0) + call(table, '@hurl$qn4Itemiiii', 'N 0 0 12 2')
    lines += call(table, '@hurl$qn4Itemiiii', 'N 4 4 4 2')
    # an actor thrown, and one thrown as it dies
    lines += call(table, '@Npc@createActor$qusus', f'K {scenes.ACTOR} 0')
    lines += call(table, '@Item@popToCoords$qususuc', f'K {YX - 120} {YY - 120} 0')
    lines += call(table, '@hurl$qn4Itemiiii', 'K -10 0 10 2')
    lines += call(table, '@Npc@createActor$qusus', f'K {scenes.ACTOR} 0')
    lines += call(table, '@Item@popToCoords$qususuc', f'K {YX - 60} {YY - 220} 0')
    lines += call(table, '@receiveHitIntrinsic$qn4Itemusucius', 'K 1 0 200 1')
    lines += call(table, '@hurl$qn4Itemiiii', 'K 0 4 12 2')
    return lines


def code(table, game):
    lines = [f'ENTER {FRAME}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}']
    # The camera to the map's corner for a pass and back: the chunks here
    # leave the fast area and what falls in them lands at once (gravity.cpp
    # landNow): one thing falls at the start, the others come after it.
    lines += made(table, scenes.ITEM, X0 + 40, Y0 + 40, 120) + call(table, '@fall$qn4Item', 'N')
    lines += call(table, '@moveCamera$qususucus', '512 512 0 0') + ['SUSPEND']
    lines += call(table, '@moveCamera$qususucus', f'{X0} {Y0} 0 0')
    first, _, _ = rounds(game)[0]
    lines += call(table, '@Item@move$qususuc', f'S {first[0]} {first[1]} {TOP}') + wait(30, 'settle')
    lines += things(table, game)
    # the Silencer on the platform, each round
    for k, ((x, y), anim, direction) in enumerate(rounds(game)):
        lines += call(table, '@Item@move$qususuc', f'S {x} {y} {TOP}') + wait(10, f'stand{k}')
        lines += call(table, '@Npc@doAnim$qusususuc', f'S {anim} {direction} 0 0')
        lines += landed(table, f'round{k}') + wait(WAIT, f'rest{k}')
        if k == 0:
            # the lowest crate broken: what lies on it falls
            lines += call(table, '@Item@destroy$qv', 'A')
        if k == 1:
            lines += call(table, '@Item@destroy$qv', 'D')
        lines += scenes.marker(table, k)
    # dropped from the air beside the platform: he lands as from a jump
    lines += call(table, '@Item@move$qususuc', f'S {PX - 40} {Y0 + 16} {TOP}') + call(table, '@fall$qn4Item', 'S')
    lines += landed(table, 'dropped') + wait(WAIT, 'rest_dropped') + scenes.marker(table, len(ROUNDS))
    # thrown up half over the platform's edge: he lands pushed off it
    lines += call(table, '@Item@move$qususuc', f'S {PX + 144} {Y0 + 16} {TOP}')
    lines += call(table, '@hurl$qn4Itemiiii', 'S 0 0 8 2') + landed(table, 'pushed') + wait(WAIT, 'rest_pushed')
    lines += scenes.marker(table, len(ROUNDS) + 1)
    # Into the area of the snap egg moved by its bytes (it holds the
    # camera), then far east at once: the egg leaves the fast area as the
    # current one, the globs' chunks leave and enter. There, in the area of
    # another, thrown up out of its height, then back west in steps.
    lines += call(table, '@Item@move$qususuc', f'S {X0 - 36} {Y0 + 236} 0') + wait(20, 'snapped')
    lines += call(table, '@moveCamera$qususucus', f'{X0 + 2560} {Y0} 0 {scenes.TEST_MAP}') + wait(10, 'looked')
    lines += call(table, '@Item@move$qususuc', f'S {X0 + 2560} {Y0} 0') + wait(60, 'east')
    lines += call(table, '@hurl$qn4Itemiiii', 'S 0 0 16 2') + wait(60, 'up')
    for k in range(1, STEPS + 1):
        lines += call(table, '@Item@move$qususuc', f'S {X0 + 2560 - STEP * k} {Y0 + 400} 0') + wait(8, f'step{k}')
    lines += call(table, '@Item@move$qususuc', f'S {X0 + 3600} {Y0 + 1200} 0') + wait(60, 'further')
    lines += call(table, '@Item@move$qususuc', f'S {X0 - 100} {Y0 + 300} 0') + wait(60, 'home')
    return lines + scenes.marker(table, len(ROUNDS) + 2) + ['RETURN']


def scene(game):
    """Things that fall, stack, break and are thrown; the Silencer off a
    raised platform in each way he moves; then far east and back past glob
    and snap eggs.

    Checks: gravity and landing (gravity.cpp: fall, hurl, landNow, friction,
        bounces), what lies on a crate when it goes, the platform's edge
        (animprim.cpp canStep: it holds him or he falls off), the glob eggs
        made when their chunks enter the fast area (glob.cpp) and the snap
        eggs that hold the camera (snap.cpp), with a quick save and load
        while things fall.
    Drives: the test egg's event 7 (code): an item dropped while the camera
        is at the map's corner (it lands at once), the yard's things
        (things), then each round on the platform (ROUNDS: where he stands,
        the animation by Npc::doAnim and its direction, put back after
        each landing), a drop beside it, a throw half over its edge, then
        the snap egg's area, far east, a throw up, steps back west; a
        quick save at frame 120 and its load at 600 (saves.quick). The map
        (layout): a floor, the platform, walls, glob and snap eggs.
    Tests:
        <round>
            The round's animation from the platform (No Remorse has no
            forward roll: a quick jump east there), his landing and the
            rest after it: its marker in the trace. The first two rounds
            also break the lowest crates.
        dropped
            Dropped from the air beside the platform: he lands as from a
            jump (its marker).
        pushed
            Thrown up half over the edge: he lands pushed off it.
        globs
            The snap egg's area, far east and back west in steps: the globs'
            chunks leave and enter the fast area (its marker).
    Original: Both follow.
    Guards: the camera goes to the map's corner for a pass at the start,
        which lands what falls there at once (gravity.cpp landNow).
    Traps: keep him out of the test egg's range: it hatches again.
    """
    table = scenes.intrinsics_table(game)
    frames = 110 + len(ROUNDS) * (10 + LANDING + WAIT) + 2 * (LANDING + WAIT) + 30 + 5 * 60 + STEPS * 8 + 120
    # a quick save while things fall and the usecode waits, its load later
    inputs, back = saves.quick(120, 600)
    frames += back + 200
    return scenes.Scene(name='physics', maps={scenes.TEST_MAP: layout()},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(code(table, game)))})},
                        arguments=scenes.START, frames=frames, inputs=inputs,
                        tests=round_names(game) + ['dropped', 'pushed', 'globs'], ran=scenes.marked(game))


def throws_code(table):
    """physics-throws' egg: the Silencer thrown at the west wall, plain and
    with low friction (he bounces and slides); dropped half over each edge of
    the platform (the slope under him pushes him off); low-friction things
    thrown at the wall (they turn by chance); a weightless thing let fall
    (No Regret's gravity stops at once)."""
    lines = [f'ENTER {FRAME}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}']
    for k, (speed, low) in enumerate([(-12, False), (-16, False), (-12, True), (-6, True), (-16, True)]):
        lines += call(table, '@Item@move$qususuc', f'S {YX - 60} {YY - 100} 0') + wait(10, f'set{k}')
        if low:
            lines += call(table, '@Item@orFlags$qus', 'S 16384')
        lines += call(table, '@hurl$qn4Itemiiii', f'S {speed} 0 8 2') + landed(table, f'thrown{k}')
        lines += wait(WAIT, f'rest_thrown{k}')
        if low:
            lines += call(table, '@Item@andFlags$qus', f'S {0xffff & ~0x4000}')
    lines += scenes.marker(table, 0)
    edges = [(PX + 16, Y0 + 16), (PX + 64, Y0 - 48), (PX + 64, Y0 + 80), (PX + 144, Y0 - 16)]
    for k, (x, y) in enumerate(edges):
        lines += call(table, '@Item@move$qususuc', f'S {x} {y} {TOP + 24}') + call(table, '@fall$qn4Item', 'S')
        lines += landed(table, f'edge{k}') + wait(WAIT, f'rest_edge{k}')
    lines += scenes.marker(table, 1)
    for k in range(6):
        lines += made(table, scenes.ITEM, YX - 60, YY - 40 + 20 * k, 0) + call(table, '@Item@orFlags$qus', 'N 16384')
        lines += call(table, '@hurl$qn4Itemiiii', f'N {-12 - k} {k - 3} 4 2') + wait(20, f'slid{k}')
    lines += made(table, scenes.WAYPOINT, YX, YY + 100, 40) + call(table, '@fall$qn4Item', 'N') + wait(20, 'weightless')
    return lines + scenes.marker(table, 2) + ['RETURN']


def throws(game):
    """The Silencer thrown at a wall and dropped over the platform's edges,
    low-friction things thrown, a weightless one let fall.

    Checks: the Silencer hurled (gravity.cpp: plain, and with low friction
        he bounces and slides), the slope under him over an edge pushing
        him off, low-friction things that turn by chance at a wall, a
        weightless thing's fall (No Regret's gravity stops at once).
    Drives: the test egg's event 7 (throws_code) on the physics scene's map.
    Tests:
        thrown
            Five throws at the west wall, two plain, three with low
            friction: its marker after them.
        edges
            Dropped half over each edge of the platform: its marker.
        slid
            Six low-friction things thrown and a weightless one let fall:
            its marker.
    Original: Both follow.
    """
    table = scenes.intrinsics_table(game)
    frames = 120 + 5 * (10 + LANDING + WAIT) + 4 * (LANDING + WAIT) + 6 * 20 + 20 + 100
    return scenes.Scene(name='physics-throws', maps={scenes.TEST_MAP: layout()},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(throws_code(table)))})},
                        arguments=scenes.START, frames=frames, tests=['thrown', 'edges', 'slid'],
                        ran=scenes.marked(game))


# The hits with no direction (physics-no-direction): fineDirection's and
# direction's none, which an item at the hit's x and y gets (tracker.cpp
# hurtArea, crusader.cpp fireWeapon). Where each test's crate stands: east
# of the Silencer, apart by more than an explosion's reach (fire type 4's
# range 10, halved for the middle blast: 80).
NO_DIRECTION = 16
CENTRE_SPOTS = [(X0 + 200 + 160 * k, Y0 - 160) for k in range(4)]


def no_direction_code(table, game):
    """physics-no-direction's egg: a crate hit by the usecode with no
    direction, by a grenade's fire type and by an explosion's; a crate on an
    item that explodes, at its x and y; a crate hit from the last
    direction."""
    crate = world.crate(game)
    hit = '@receiveHitIntrinsic$qn4Itemusucius'
    (x0, y0), (x1, y1), (x2, y2), (x3, y3) = CENTRE_SPOTS
    lines = [f'ENTER {FRAME}']
    lines += made(table, crate, x0, y0, 0) + call(table, hit, f'N 0 {NO_DIRECTION} 10 3')
    lines += wait(60, 'grenade') + scenes.marker(table, 0)
    lines += made(table, crate, x1, y1, 0) + call(table, hit, f'N 0 {NO_DIRECTION} 10 4')
    lines += wait(60, 'explosion') + scenes.marker(table, 1)
    lines += made(table, scenes.ITEM, x2, y2, 0, 'A') + made(table, crate, x2, y2, 16)
    lines += call(table, '@explodeIntrinsic$qn4Itemiuc', 'A 1 1') + wait(60, 'blast') + scenes.marker(table, 2)
    lines += made(table, crate, x3, y3, 0) + call(table, hit, f'N 0 {NO_DIRECTION - 1} 10 4')
    lines += wait(60, 'last') + scenes.marker(table, 3)
    return lines + ['RETURN']


def no_direction(game):
    """Crates hit with no direction, by the usecode and by an explosion
    under one, and one from the last direction.

    Checks: a thing that a grenade or an explosion hits with no direction
        (16: the hit at its x and y) hurled by the hurl tables
        (crusader.cpp thingTakesHit, hurlX and hurlY), which have 16
        directions: the originals read past them, the words after them in
        their data segment.
    Drives: the test egg's event 7 (no_direction_code): crates made at
        CENTRE_SPOTS and hit by receiveHitIntrinsic, one on a test item that
        explodeIntrinsic blows up (hurtArea: fineDirection from the blast
        gives none).
    Tests:
        a grenade with no direction
            A crate hit by fire type 3 from direction 16, hurled for 60
            passes: its marker.
        an explosion with no direction
            The same with fire type 4: its marker.
        an explosion under it
            A crate on a test item at its x and y that explodes (hurtArea's
            hit with no direction): its marker.
        the last direction
            A crate hit by fire type 4 from direction 15, the tables' last:
            its marker.
    Original: Both follow.
    """
    table = scenes.intrinsics_table(game)
    return scenes.Scene(name='physics-no-direction', maps={scenes.TEST_MAP: scenes.floor_and_egg(20)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(no_direction_code(table, game)))})},
                        arguments=scenes.START, frames=120 + 4 * 60 + 100,
                        tests=['a grenade with no direction', 'an explosion with no direction', 'an explosion under it',
                               'the last direction'], ran=scenes.marked(game))


# physics-blast: a large blast (explode's type 2: fire type 4's whole
# range, 10) among things around it, as a player's det pac among the
# things of a room: where it goes off, far from the Silencer; the things
# on rings round it, each ring's directions apart by an eighth (the outer's
# between the inner's), a crate with a thing on it, one on the blast (no
# direction) and one out of its reach.
BLAST = (X0 + 300, Y0 - 250)
INNER, OUTER, BEYOND = 48, 112, 220
BLAST_WAIT = 240
# The tests: the groups of blast_things, by their indexes there.
BLAST_GROUPS = {'the inner ring': range(1, 9), 'the outer ring': range(9, 17),
                'the crate and the thing on it': range(17, 19), 'the crate on the blast': range(0, 1),
                'the thing beyond its reach': range(19, 20)}
# The globals the egg writes: each thing's number (a word), and after the
# flights its x and y (words) and z (a byte), 6 bytes a thing.
NUMBERS, PLACES = 256, 320


def blast_things(game):
    """physics-blast's things: (shape, x, y, z) each, 20: the crate on the
    blast, the inner ring, the outer ring (crates and things in turn), a
    crate and the thing on it, the thing beyond the blast's reach."""
    import math
    crate = world.crate(game)
    cx, cy = BLAST
    ring = lambda radius, k, turn: (cx + round(radius * math.cos(math.pi * (k + turn) / 4)),
                                    cy + round(radius * math.sin(math.pi * (k + turn) / 4)))
    things = [(crate, cx, cy, 16)]
    things += [(scenes.ITEM, *ring(INNER, k, 0), 0) for k in range(8)]
    things += [(scenes.ITEM if k % 2 else crate, *ring(OUTER, k, 0.5), 0) for k in range(8)]
    things += [(crate, cx + 80, cy + 40, 0), (scenes.ITEM, cx + 80, cy + 40, 16)]
    return things + [(scenes.ITEM, cx - BEYOND, cy, 0)]


def blast_code(table, game):
    """physics-blast's egg: the things made (their numbers into the
    globals), a test item at the blast's place blown up, then the things'
    flights to their rest; their places into the globals, and the groups'
    markers."""
    cx, cy = BLAST
    n = LOCALS['N']
    lines = [f'ENTER {FRAME}'] + made(table, scenes.ITEM, cx, cy, 0, 'A')
    for k, (shape, x, y, z) in enumerate(blast_things(game)):
        lines += made(table, shape, x, y, z) + [f'PUSH_LOCAL_WORD {n}', f'POP_GLOBAL {NUMBERS + 2 * k} 2']
    lines += wait(20, 'set') + call(table, '@explodeIntrinsic$qn4Itemiuc', 'A 2 1') + wait(BLAST_WAIT, 'flown')
    for k in range(len(blast_things(game))):
        lines += [f'PUSH_GLOBAL {NUMBERS + 2 * k} 2', f'POP_LOCAL_WORD {n}']
        for name, offset in (('@Item@x$qv', 0), ('@Item@y$qv', 2), ('@Item@z$qv', 4)):
            lines += scenes.call(table, name, 'N', result=PLACES + 6 * k + offset, local_offsets=LOCALS)
    for k in range(len(BLAST_GROUPS)):
        lines += scenes.marker(table, k)
    return lines + ['RETURN']


def blast_rest(game, record):
    """[(start, rest)] of blast_things, each an (x, y, z), from the globals at
    the run's end."""
    return [((x, y, z), (record.global_value(PLACES + 6 * k), record.global_value(PLACES + 6 * k + 2),
                         record.global_value(PLACES + 6 * k + 4, 1)))
            for k, (_, x, y, z) in enumerate(blast_things(game))]


def blast_landed(game, record, k):
    """Whether physics-blast's group k came to rest as the blast throws it:
    the rings and the crate with its thing thrown on the floor farther from
    the blast than they lay, the thing that lay on the crate farther than
    the crate; the crate on the blast down on the floor where it lay (its
    push, which has no direction, takes it out of the fast area at once:
    none); the thing beyond the blast's reach where it lay."""
    rest = blast_rest(game, record)
    name = list(BLAST_GROUPS)[k]
    group = [rest[j] for j in BLAST_GROUPS[name]]
    away = lambda place: max(abs(place[0] - BLAST[0]), abs(place[1] - BLAST[1]))
    if name == 'the crate on the blast':
        (start, end), = group
        return end == (start[0], start[1], 0)
    if name == 'the thing beyond its reach':
        (start, end), = group
        return end == start
    thrown = all(end[2] == 0 and away(end) > away(start) for start, end in group)
    if name == 'the crate and the thing on it':
        return thrown and away(group[1][1]) > away(group[0][1])
    return thrown


def blast(game):
    """A large blast among twenty things, as a det pac's in a room: the
    things about it thrown away from it, the crate on it dropped where it
    lay, the thing beyond its reach left as it was.

    Checks: what a blast's hits throw: explode.cpp explode (type 2, the
        large blast: fire type 4's whole damage, 150, and range, 10 or 160
        in the map's units); tracker.cpp hurtArea (what lies in the square
        of 160 round the blast, each hit by the damage over its distance
        in 48s, from the direction distance.cpp fineDirection gives, none
        for a thing at the blast's x and y); crusader.cpp thingTakesHit
        (the hurl tables' push for the direction, 10 to 25 times; for none
        the words the originals read past the tables); gravity.cpp's
        flights, bounces and landings with many things in the air at once,
        a thing on a crate thrown with it, the crates' and the things'
        damage on the way; collide.cpp Sweep: a step out of the map or the
        fast area drops a thrown thing's push.
    Drives: the test egg's event 7 (blast_code): the things of blast_things
        made, each one's number into the globals at NUMBERS: a crate on
        the blast's place (z 16, on the test item), 8 things INNER (48)
        from it and 8 crates and things in turn OUTER (112) from it, each
        ring's directions an eighth apart (the outer's between the
        inner's), a crate at 80, 40 from it with a thing on it, a thing
        BEYOND (220) west of it; a test item at BLAST, which
        explodeIntrinsic blows up (type 2, the item destroyed); BLAST_WAIT
        passes for the flights; each thing's x, y and z into the globals at
        PLACES, 6 bytes a thing (the trace's G lines); then the groups'
        markers (BLAST_GROUPS, in the tests' order). The values below are
        the same in both games (the hurl tables' 16 directions are).
    Tests:
        the inner ring
            The 8 things 48 from the blast, each hit by 150 (48 is 1 in
            48s): each on the floor farther from the blast than it lay
            (flights of 22 to 63), the globals at PLACES; the marker after
            them.
        the outer ring
            The 8 crates and things 112 from it, each hit by 75 (2 in
            48s): each on the floor farther (flights of 24 to 53).
        the crate and the thing on it
            The crate 80, 40 from it and the thing on it, each hit by 150:
            both on the floor farther, the thing farther than the crate
            (thrown from the crate's top it flies 228, the crate 34).
        the crate on the blast
            Hit by 150 with no direction (at the blast's x and y): its push
            is the words past the hurl tables (No Regret's y -24576 a step,
            No Remorse's 2720), whose first step ends out of the map (No
            Regret) or of the fast area (No Remorse); the sweep drops its x
            and y speeds (gravity.cpp, sweep.outside), and it falls to the
            floor where it lay: the same x and y, z 0.
        the thing beyond its reach
            220 west, outside the square of 160: not hit, where it lay.
    Original: Both follow: the things' places are in the globals, which the
        pass oracle compares with the original's at each pass.
    Guards: the crate on the blast keeps the originals' push with no
        direction and its end (physics-no-direction holds the words past
        the tables).
    Traps: the state lines keep none of the things' places: the footprints
        read them from the globals, and a thing whose number or place is
        left out of them is a flight the port's own test does not see. The
        floor (floor_and_egg(20)) reaches 640 from the start: the
        farthest landing (the thing from the crate) is 44 short of its east
        edge, and a thing thrown off it or out of the fast area stops, as
        the crate on the blast does; keep BLAST and the rings so that the
        flights stay on it.
    """
    table = scenes.intrinsics_table(game)
    return scenes.Scene(name='physics-blast', maps={scenes.TEST_MAP: scenes.floor_and_egg(20)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(blast_code(table, game)))})},
                        arguments=scenes.START, frames=120 + 20 + BLAST_WAIT + 100, tests=list(BLAST_GROUPS),
                        ran=scenes.checked(game, lambda record, marks, k: blast_landed(game, record, k)))


# physics-blast-hurt: blasts round the Silencer, from far to near, that
# hurt him and leave him alive (fire type 4's 150 points, his 150 hit
# points): explode's type, the blast's place from his, and the damage the
# round does at difficulty 3. explode divides fire type 4's range (10) and
# damage by 3, 2 and 1 for its small, medium and large blasts (type 0, 1,
# 2): their reach (the range in 16s) 48, 80 and 160, their damage 50, 75
# and 150. hurtArea hurts what is in the square of the reach round the
# blast, each by the damage over its distance (the largest of x's, y's and
# z's) in 48s, at least 1: just beyond the large blast's reach (0), its
# square's corner (160 in 48s: 3, a third: 50), beyond the medium's and the
# small's reach (0), within the large one's half (120 in 48s: 2, a half:
# 75). Nearer than 96 the large blast's whole 150 kills him.
HURT_SPOT = (X0 + 400, Y0 + 100)
HURT_ROUNDS = [(2, 170, 0, 0), (2, 160, 160, 50), (1, 90, 0, 0), (0, 60, 0, 0), (2, 120, 0, 75)]
HURT_WAIT = 90
BLAST_SIZES = {0: 'small', 1: 'medium', 2: 'large'}
# What a hit on the Silencer becomes at each difficulty (crusader.cpp
# scaledDamage): a fifth at 1, a third at 2, the whole at 3 and 4; a hit
# that comes to 0 is 1.
SILENCER_SCALE = {1: 5, 2: 3, 3: 1, 4: 1}


def hurt_names():
    """physics-blast-hurt's tests' names: each round's blast and place."""
    return [f'{BLAST_SIZES[kind]} blast at {dx}, {dy}' for kind, dx, dy, _ in HURT_ROUNDS]


def hurt_code(table):
    """physics-blast-hurt's egg: each round, the Silencer put at HURT_SPOT,
    a test item blown up beside him, then the hit's end."""
    sx, sy = HURT_SPOT
    lines = [f'ENTER {FRAME}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}']
    for k, (kind, dx, dy, _) in enumerate(HURT_ROUNDS):
        lines += call(table, '@Item@move$qususuc', f'S {sx} {sy} 0') + wait(20, f'placed{k}')
        lines += made(table, scenes.ITEM, sx + dx, sy + dy, 0, 'A')
        lines += call(table, '@explodeIntrinsic$qn4Itemiuc', f'A {kind} 1') + wait(HURT_WAIT, f'hurt{k}')
        lines += scenes.marker(table, k)
    return lines + ['RETURN']


def hurt_taken(record, marks, k, skill):
    """Whether physics-blast-hurt's round k took from the Silencer what its
    blast does at difficulty skill (his hit points at its marker against the
    last round's, his whole at the first), and left him alive."""
    damage = HURT_ROUNDS[k][3]
    lost = 0 if damage == 0 else max(1, damage // SILENCER_SCALE[skill])
    before = int(record.state(marks[k - 1])[scenes.STATE_HP]) if k else world.NPC_TYPES[0]['maxHp']
    after = int(record.state(marks[k])[scenes.STATE_HP])
    return after > 0 and before - after == lost


def blast_hurt(game, skill=3):
    """Blasts round the Silencer that hurt him and leave him alive: one just
    beyond each size's reach, one at the large blast's corner, one at half
    its reach.

    Checks: what a blast does to the Silencer: explode.cpp explode's three
        sizes (small, medium and large: fire type 4's range and damage over
        3, 2 and 1, a reach of 48, 80 and 160 and a damage of 50, 75 and
        150); tracker.cpp hurtArea (the square of the reach, the damage
        over the distance in 48s); crusader.cpp takeHit, scaledDamage and
        npcTakesHit for the controlled NPC (his hit points, the status
        bar's health, his hurt cry). An explosion's hit does not push him,
        turn him or start an animation (only EM-4's and the electric
        fire's do: noRegretHurt, noRemorseHurt): his place and direction
        stay.
    Drives: the test egg's event 7 (hurt_code): for each round of
        HURT_ROUNDS the Silencer moved to HURT_SPOT (he does not move: the
        rounds' places are from the same spot), 20 passes, a test item made
        at the round's place from him and blown up by explodeIntrinsic (the
        round's type, the item destroyed), HURT_WAIT passes, the round's
        marker.
    Tests:
        <size> blast at <x>, <y>
            The round's blast at that place from him: at its marker his hit
            points (the state lines') less by the round's damage in
            HURT_ROUNDS as the difficulty scales it (SILENCER_SCALE), and he
            alive. At difficulty 3, in both games: large at 170, 0 (beyond
            the reach of 160) 0; large at 160, 160 (the square's corner: 3
            in 48s) 50; medium at 90, 0 (beyond 80) 0; small at 60, 0
            (beyond 48) 0; large at 120, 0 (2 in 48s) 75: of his 150, 25
            left.
    Original: Both follow.
    Traps: nearer than 96 a large blast's whole 150 kills him, and the
        rounds after it have no one to hurt; the rounds go from far to
        near, so that a reach misjudged shows in its own round.
    """
    table = scenes.intrinsics_table(game)
    return scenes.Scene(name='physics-blast-hurt', maps={scenes.TEST_MAP: scenes.floor_and_egg(20)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(hurt_code(table)))})},
                        arguments=scenes.START, frames=120 + len(HURT_ROUNDS) * (20 + HURT_WAIT) + 100,
                        tests=hurt_names(),
                        ran=scenes.checked(game, lambda record, marks, k: hurt_taken(record, marks, k, skill)))


def blast_hurt_at_skill(game, skill):
    """physics-blast-hurt at difficulty skill (-skill)."""
    import dataclasses
    base = blast_hurt(game, skill)
    return dataclasses.replace(base, name=f'physics-blast-hurt-skill{skill}',
                               arguments=scenes.START + ['-skill', str(skill)])


def blast_hurt_skill1(game):
    """The blasts round the Silencer at difficulty 1.

    Variant of: physics-blast-hurt
    Checks: the difficulty's scale of the hits on the Silencer
        (crusader.cpp scaledDamage: a fifth at difficulty 1): the corner's
        50 is 10, the half reach's 75 is 15; of his 150, 125 left. The
        footprints check these.
    Drives: the physics-blast-hurt scene with -skill 1.
    Original: Both follow.
    """
    return blast_hurt_at_skill(game, 1)


def blast_hurt_skill2(game):
    """The blasts round the Silencer at difficulty 2.

    Variant of: physics-blast-hurt
    Checks: as physics-blast-hurt-skill1, a third at difficulty 2: 50 is
        16, 75 is 25; of his 150, 109 left.
    Drives: the physics-blast-hurt scene with -skill 2.
    Original: Both follow.
    """
    return blast_hurt_at_skill(game, 2)


def blast_hurt_skill4(game):
    """The blasts round the Silencer at difficulty 4.

    Variant of: physics-blast-hurt
    Checks: difficulty 4, which scales no hit (crusader.cpp scaledDamage),
        as the default 3: 50 and 75; of his 150, 25 left.
    Drives: the physics-blast-hurt scene with -skill 4.
    Original: Both follow.
    """
    return blast_hurt_at_skill(game, 4)


SCENES = {'physics': scene, 'physics-throws': throws, 'physics-no-direction': no_direction, 'physics-blast': blast,
          'physics-blast-hurt': blast_hurt, 'physics-blast-hurt-skill1': blast_hurt_skill1,
          'physics-blast-hurt-skill2': blast_hurt_skill2, 'physics-blast-hurt-skill4': blast_hurt_skill4}
