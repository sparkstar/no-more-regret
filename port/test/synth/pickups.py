"""The things the Silencer picks up and uses:
the egg makes every kind of item at his feet and picks it up as the game's
usecode does (pickUp, shown among the pickups), so that world.cpp's pickUp
goes through each kind: the credits (the pack, then the stack), keycards
(below 20h and not), the three batteries twice each (the type, then the
energy), the three shields, a spider bomb to its 20, each ammunition to its
20, each weapon twice (the slots fill), each other item to its 10 (the pack
fills). Then the keys a player presses: the items' next and previous, use,
the medkit and energy cube, No Regret's VIR imager, mine detector and
radiation shield (No Remorse's R the reticle), the bomb's detonation, the
grab, and the selection (S) over the switches around him: next, previous,
the choice (the switch's usecode counts its uses in the globals), cancel."""
import controls
import formats
import scenes
import ucasm
import world

X0, Y0 = scenes.X0, scenes.Y0
# Where the Silencer goes after the pickups, among the switches.
SWITCHES = (X0 + 300, Y0)
LOCALS = {'N': -2}
# Where the pickUp results and the switch's uses go in the globals.
RESULTS, USES = 64, 120
START = 150


def plan(game):
    """[(shape, quality or None, times)] in order."""
    names = world.game_shapes(game)
    tables = world.table_shapes(game)
    special = {names[name] for name in ('SHAPE_CREDITS', 'SHAPE_KEYCARD', 'SHAPE_CHEMICAL_BATTERY',
                                        'SHAPE_FUSION_BATTERY', 'SHAPE_FISSION_BATTERY', 'SHAPE_IONIC_SHIELD',
                                        'SHAPE_PLASMA_SHIELD', 'SHAPE_GRAVITON_SHIELD', 'SHAPE_SPIDER_BOMB')}
    out = [(names['SHAPE_CREDITS'], 50, 2), (names['SHAPE_KEYCARD'], 3, 1), (names['SHAPE_KEYCARD'], 0x40, 1)]
    for name in ('SHAPE_CHEMICAL_BATTERY', 'SHAPE_FISSION_BATTERY', 'SHAPE_FUSION_BATTERY'):
        out.append((names[name], None, 2))
    for name in ('SHAPE_IONIC_SHIELD', 'SHAPE_PLASMA_SHIELD', 'SHAPE_GRAVITON_SHIELD', 'SHAPE_IONIC_SHIELD'):
        out.append((names[name], None, 1))
    out.append((names['SHAPE_SPIDER_BOMB'], None, 21))
    out += [(shape, None, 21) for shape in tables[11]]
    out += [(shape, None, 2) for shape in tables[10]]
    out += [(shape, None, 11) for shape in tables[12] + tables[13] if shape not in special]
    return out


def code(table, game):
    """Event 7: each thing of the plan made at the Silencer's feet and
    picked up, its result in the globals; after the selections there
    (SPOT_STEPS) he steps among the switches. What he has picked up keeps
    the place it lay at, which the selection takes as near (world.cpp
    selectItems, the original's too): there, the selection goes through his
    own things before a switch."""
    lines = ['ENTER 4', 'PUSH_WORD 1', 'POP_LOCAL_WORD -4']
    for k, (shape, quality, times) in enumerate(plan(game)):
        for _ in range(times):
            lines += scenes.call(table, '@Item@create$qusus', f'N {shape} 0', local_offsets=LOCALS)
            if quality is not None:
                lines += scenes.call(table, '@Item@setQuality$qus', f'N {quality}', local_offsets=LOCALS)
            lines += scenes.call(table, '@Item@popToCoords$qususuc', f'N {X0} {Y0} 0', local_offsets=LOCALS)
            lines += scenes.call(table, '@pickUp$qusuc', 'n 1', result=RESULTS + k % 48, local_offsets=LOCALS)
    lines += ['SUSPEND'] * (MOVED - DRIVEN)
    lines += scenes.call(table, '@Item@move$qususuc', f'S {SWITCHES[0]} {SWITCHES[1]} 0', local_offsets={'S': -4})
    return ucasm.assemble('\n'.join(lines + scenes.marker(table, 0) + ['RETURN']))


def switch_code():
    """The switch's use (event 1): one more use in the globals."""
    return ucasm.assemble('\n'.join(['ENTER 0', f'PUSH_GLOBAL {USES} 2', 'PUSH_WORD 1', 'ADD', f'POP_GLOBAL {USES} 2',
                                     'RETURN']))


def switch_used(record, first, last):
    """A switch chosen by the selection and used (its class's event 1 in
    the trace) from frame first to before frame last."""
    states = record.between(first, last)
    low, high = int(states[0][0]), int(states[-1][0])
    return any(line[1] == 'E' and line[3] == f'{scenes.SWITCH:04X}:0001' and low <= int(line[0]) <= high
               for line in record.trace)


# The selections where the things are picked up, a switch near: the first
# chooses one of his own things (no switch used), the second steps back
# from the first to the list's end, the switch, and uses it.
SPOT_STEPS = [(['s'], 2, 20), (['enter'], 2, 20), (['s'], 2, 20), (['comma'], 2, 20), (['enter'], 2, 20)]
# The pass the driver runs in (the test egg's hatching) and the one it
# moves the Silencer in, after SPOT_STEPS; the other steps' first frame.
DRIVEN, MOVED, AFTER = 63, 300, 330


SCAN = dict(controls.SCAN, comma=0x33, x=0x2d, f4=0x3e)


def press(frame, keys, held):
    lines = []
    for verb, at in (('key', frame), ('release', frame + held)):
        for key in keys if verb == 'key' else reversed(keys):
            scan = SCAN[key]
            prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
            lines += prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}']
    return lines


def steps(game):
    # No Remorse's R shows a box that takes Enter and Escape alone (a second
    # R would go to the box)
    own = [['v'], ['x'], ['r']] if game == 'regret' else [['r'], ['enter'], ['r'], ['enter']]
    keys = [['o'], ['o'], ['o'], ['i'], ['i'], ['u'], ['m'], ['e']] + own + [['g']]
    keys += [['s'], ['period'], ['period'], ['comma'], ['enter'], ['s'], ['period'], ['u'], ['s'], ['esc'],
             ['s'], ['enter'], ['b']]
    return [(k, 2, 20) for k in keys]


def scene(game):
    """Every kind of thing picked up as the games' usecode picks it up, then
    the items' keys and the selection over switches.

    Checks: world.cpp pickUp for each kind (the credits, the pack then the
        stack; keycards below 20h and not; the batteries twice, the type
        then the energy; the shields; a spider bomb to its 20; each
        ammunition to its 20; each weapon twice, the slots full; each other
        item to its 10, the pack full), the items' keys (next, previous,
        use, the medkit, the energy cube, No Regret's VIR imager, mine
        detector and radiation shield, No Remorse's reticle), the bomb's
        detonation, the grab, and the selection (world.cpp selectItems:
        next, previous, choose, use, cancel) of a switch.
    Drives: the test egg's event 7 (code): each thing of the plan made at
        his feet and picked up (pickUp, its result in the globals), then
        he is moved among the switches; the switch's event 1 counts its
        uses in the globals; the keys: the selections where he picked the
        things (SPOT_STEPS), then those among the switches (steps).
    Tests:
        pickups
            Every pickUp done: its marker in the trace.
        keys
            The keys among the switches: a switch chosen and used (its
            event 1 in the trace).
        his own things first
            The first selection where he picked the things chooses one of
            them, not the switch near (no switch used before it).
        the switch after them
            The second steps back past his things to the switch and uses
            it (its event 1 in the trace).
    Original: Both follow.
    Guards: what he has picked up keeps the place it lay at, which the
        selection takes as near (the original's too, which the scene pins).
    """
    table = scenes.intrinsics_table(game)
    # the switches, and nearer than them a clip on the floor, which the
    # last selection chooses (not selectable: chosen, not used)
    switches = [formats.item_record(SWITCHES[0] + dx, SWITCHES[1] + dy, 0, scenes.SWITCH)
                for dx, dy in ((70, 0), (0, 70), (-60, 0))]
    switches += [formats.item_record(SWITCHES[0] + 20, SWITCHES[1] + 20, 0, world.table_shapes(game)[11][0])]
    switches += [formats.item_record(X0 + 60, Y0, 0, scenes.SWITCH)]
    inputs, frame = [], START
    for keys, held, wait in SPOT_STEPS:
        inputs += press(frame, keys, held)
        frame += held + wait
    first = START + sum(held + wait for _, held, wait in SPOT_STEPS[:2])
    spot_end, frame = frame, AFTER
    for keys, held, wait in steps(game):
        inputs += press(frame, keys, held)
        frame += held + wait
    return scenes.Scene(name='pickups', maps={scenes.TEST_MAP: scenes.floor_and_egg(12) + switches},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table, game)}),
                                 scenes.SWITCH: ('SWITCH', {1: switch_code()})},
                        arguments=scenes.START, frames=frame + 60,
                        tests=['pickups', 'keys', 'his own things first', 'the switch after them'],
                        ran=lambda record, tests: [scenes.marked(game)(record, tests)[0],
                                                   switch_used(record, AFTER, frame),
                                                   not switch_used(record, START, first),
                                                   switch_used(record, first, spot_end)],
                        inputs=inputs)


SCENES = {'pickups': scene}
