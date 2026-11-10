"""The controller's combinations: the roll
key (Ctrl) and the combat step (Alt) with each arrow, standing and
kneeling, a turn while walking and while running (keys held together), and
the keypad's moves with Num Lock on (control.cpp Controller::move). The
Silencer holds the controls scene's weapons. moves-keypad: the keypad's
moves with Alt and Ctrl, kneeling and armed, Num Lock off and on, the
strafe keys kneeling, a jump and a roll while running; moves-base: the
moves on the base's map."""
import dataclasses

import controls
import scenes

controls.SCAN.setdefault('numlock', 0x45)

ARROWS = ['up', 'down', 'left', 'right']


def roll(arrow):
    """A roll's footprint: the roll key with the down arrow animates the
    Silencer where he stands."""
    return 'acted' if arrow == 'down' else 'moved'


ACTIONS = ([(f'roll {arrow}', ['ctrl', arrow], 12, roll(arrow)) for arrow in ARROWS] + [('kneel', ['kp5'], 20)] +
           [(f'kneeling roll {arrow}', ['ctrl', arrow], 12, roll(arrow)) for arrow in ('left', 'right', 'down')] +
           [('kneel again', ['kp5'], 20)] +
           [(f'kneeling step {arrow}', ['alt', arrow], 16) for arrow in ARROWS] + [('stand', ['kp5'], 20)] +
           [(f'step {arrow}', ['alt', arrow], 16) for arrow in ARROWS] + [('weapon', ['1'], 4)] +
           [(f'armed roll {arrow}', ['ctrl', arrow], 12, roll(arrow)) for arrow in ARROWS] +
           [('num lock', ['numlock'], 2)] +
           [(f'keypad {key}', [key], 16) for key in ('kp8', 'kp2', 'kp4', 'kp6', 'kp7', 'kp9', 'kp1', 'kp3')] +
           [('num lock off', ['numlock'], 2)])


def held_together(frame, first, then, before, during, after):
    """first held from frame, then pressed before frames later for during
    frames, first let go after frames more."""
    lines = controls.press(frame + before, then, during)
    for verb, at in (('key', frame), ('release', frame + before + during + after)):
        for key in first:
            scan = controls.SCAN[key]
            prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
            lines += prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}']
    return lines


def scene(game):
    """The controller's combinations: the roll and combat-step keys with
    each arrow, standing, kneeling and armed, the keypad with Num Lock on,
    and keys held together.

    Checks: control.cpp Controller::move's combinations: Ctrl (roll) and Alt
        (combat step) with each arrow, standing and kneeling, armed; the
        keypad's moves with Num Lock on; a turn while walking and while
        running, a kneel while walking (keys held together).
    Drives: the controls scene's data; ACTIONS (controls.of_actions), then
        three pairs of keys held together (held_together).
    Tests:
        roll <arrow>
            Ctrl with an arrow: he rolls (moved; with the down arrow he
            animates where he stands: acted).
        kneel
            The keypad's 5 (acted).
        kneeling roll <arrow>
            Ctrl with an arrow while he kneels (as roll).
        kneel again
            The keypad's 5 (acted).
        kneeling step <arrow>
            Alt with an arrow while he kneels (moved).
        stand
            The keypad's 5 (acted).
        step <arrow>
            Alt with an arrow (moved).
        weapon
            The first weapon. No record shows it.
        armed roll <arrow>
            Ctrl with an arrow, the weapon drawn (as roll).
        num lock
            Num Lock on. No record shows it.
        keypad <key>
            A keypad key with Num Lock on (moved).
        num lock off
            No record shows it.
        walk and turn
            The up arrow held, the left pressed during it (moved).
        run and turn
            Shift and the up arrow held, the right pressed (moved).
        walk and kneel
            The up arrow held, the keypad's 5 pressed (moved).
    Original: Both follow.
    """
    base = controls.of_actions(game, ACTIONS)
    inputs, frame = list(base.inputs), base.frames
    windows = controls.timeline(ACTIONS)[1]
    for first, then in ((['up'], ['left']), (['shift', 'up'], ['right']), (['up'], ['kp5'])):
        inputs += held_together(frame, first, then, 20, 10, 10)
        windows.append((frame, frame + 60, 'moved'))
        frame += 60
    return dataclasses.replace(base, name='moves', inputs=inputs, frames=frame + 60,
                               tests=base.tests + ['walk and turn', 'run and turn', 'walk and kneel'],
                               ran=scenes.windows_ran(windows))


KEYPAD = ('kp8', 'kp2', 'kp4', 'kp6', 'kp7', 'kp9', 'kp1', 'kp3')
STRAFES = ('delete', 'pagedown', 'home', 'end', 'insert', 'pageup')
# The keypad's moves (Num Lock off, then on) standing, kneeling and
# armed; with the combat step (Alt) and the roll key (Ctrl), and those with
# the kneel key; the strafe keys kneeling; the run key with the run lock.
KEYPAD_ACTIONS = ([('run lock', ['capslock'], 2), ('run key locked', ['shift', 'up'], 30), ('run unlock', ['capslock'], 2),
                   ('keypad left standing', ['kp4'], 8), ('keypad right standing', ['kp6'], 8)] +
                  [(f'step {key}', ['alt', key], 16) for key in KEYPAD] +
                  [(f'roll {key}', ['ctrl', key], 12) for key in KEYPAD] +
                  [('step kneel', ['alt', 'kp5'], 16), ('step stand', ['alt', 'kp5'], 16),
                   ('roll kneel', ['ctrl', 'kp5'], 16), ('roll stand', ['ctrl', 'kp5'], 16), ('kneel', ['kp5'], 20)] +
                  [(f'kneeling step {key}', ['alt', key], 16) for key in KEYPAD] +
                  [('kneel again', ['kp5'], 20), ('kneeling roll forward', ['ctrl', 'up'], 12),
                   ('kneeling roll right', ['ctrl', 'right'], 12)] +
                  [action for key in STRAFES for action in ((f'kneel for {key}', ['kp5'], 20),
                                                            (f'kneeling {key}', [key], 16))] +
                  [('weapon', ['1'], 4), ('armed keypad left', ['kp4'], 8), ('armed keypad right', ['kp6'], 8),
                   ('armed walk', ['up'], 20), ('armed keypad walk left', ['kp4'], 12), ('num lock', ['numlock'], 2),
                   ('keypad jump', ['kpslash'], 12), ('keypad select', ['kp5'], 8), ('keypad run', ['shift', 'kp8'], 30)] +
                  # (with Num Lock, the keypad's 4 and 6 animate him where he stands)
                  [(f'num keypad {key}', [key], 12, 'acted' if key in ('kp4', 'kp6') else 'moved') for key in KEYPAD] +
                  [('kneel locked', ['ctrl', 'kp5'], 16)] +
                  [(f'kneeling num keypad {key}', [key], 12) for key in ('kp7', 'kp9', 'kp1', 'kp3')] +
                  [('num lock off', ['numlock'], 2), ('run lock again', ['capslock'], 2)])


# The keypad's actions' spacing: the kneel key pressed while the roll
# before it still runs is lost (No Remorse's rolls end later).
KEYPAD_GAP = 16


def keypad(game):
    """The keypad's moves with Alt and Ctrl, kneeling and armed, Num Lock
    off and on, the strafe keys kneeling, the run lock; then a jump, a roll
    and a turn while running.

    Checks: control.cpp Controller::move's keypad: its moves standing,
        kneeling and armed, with the combat step (Alt) and the roll key
        (Ctrl), and those with the kneel key; Num Lock's keypad (its 4 and
        6 animate him where he stands); the strafe keys kneeling; the run
        key with the run lock; a jump, a roll and a turn while running.
    Drives: the controls scene's data; KEYPAD_ACTIONS, KEYPAD_GAP apart,
        then three pairs held together.
    Tests:
        run lock
            Caps Lock. No record shows it.
        run key locked
            Shift and the up arrow, the run locked (moved).
        run unlock
            No record shows it.
        keypad <side> standing
            The keypad's 4 or 6, Num Lock off (moved).
        step <key>
            Alt with a keypad key (moved).
        roll <key>
            Ctrl with a keypad key (moved).
        step kneel
            Alt with the keypad's 5 (acted).
        step stand
            Again (acted).
        roll kneel
            Ctrl with the keypad's 5 (acted).
        roll stand
            Again (acted).
        kneel
            The keypad's 5 (acted).
        kneeling step <key>
            Alt with a keypad key while he kneels (moved).
        kneel again
            The keypad's 5 again (acted).
        kneeling roll forward
            Ctrl and the up arrow while he kneels (moved).
        kneeling roll right
            Ctrl and the right arrow while he kneels (moved).
        kneel for <key>
            The keypad's 5 before a strafe key (acted).
        kneeling num keypad <key>
            A keypad key with Num Lock while he kneels (moved).
        kneeling <key>
            A strafe key while he kneels (moved).
        weapon
            No record shows it.
        armed keypad <side>
            The keypad's 4 or 6, armed (moved).
        armed walk
            The up arrow, armed (moved).
        armed keypad walk left
            The keypad's 4 held, armed (moved).
        num lock
            No record shows it.
        keypad jump
            The keypad's / (moved).
        keypad select
            The keypad's 5 with Num Lock (acted).
        keypad run
            Shift and the keypad's 8 (moved).
        num keypad <key>
            A keypad key with Num Lock (moved; the 4 and 6 acted).
        kneel locked
            Ctrl and the keypad's 5 with Num Lock (acted).
        num lock off
            No record shows it.
        run lock again
            No record shows it.
        jump running
            The up arrow held, the keypad's / pressed (moved).
        roll running
            The up arrow held, Ctrl and the up arrow pressed (moved).
        turn running
            The up arrow held, the left pressed (moved).
    Original: Both follow.
    Guards: not Shift with the keypad's /: No Remorse's original reads it as
        ?, its help, where the port's keys jump.
    Traps: the kneel key pressed while a roll still runs is lost (No
        Remorse's rolls end later): KEYPAD_GAP.
    """
    base = controls.of_actions(game, KEYPAD_ACTIONS, KEYPAD_GAP)
    inputs, frame = list(base.inputs), base.frames
    windows = controls.timeline(KEYPAD_ACTIONS, KEYPAD_GAP)[1]
    for first, then in ((['up'], ['kpslash']), (['up'], ['ctrl', 'up']), (['up'], ['left'])):
        inputs += held_together(frame, first, then, 30, 10, 10)
        windows.append((frame, frame + 70, 'moved'))
        frame += 70
    return dataclasses.replace(base, name='moves-keypad', inputs=inputs, frames=frame + 60,
                               tests=base.tests + ['jump running', 'roll running', 'turn running'],
                               ran=scenes.windows_ran(windows))


# The game's base map (common/src/item/npc.cpp Npc::onBaseMap, BASE_MAP):
# its moves walk, run and stand only.
BASE_MAPS = {'regret': 25, 'remorse': 40}
BASE_ACTIONS = [('forward', ['up'], 30), ('turn left', ['left'], 12), ('run', ['shift', 'up'], 30),
                ('turn right', ['right'], 12), ('keypad', ['kp7'], 16), ('back', ['down'], 16)]


def base(game):
    """The moves on the base's map, in the room of walls: walk, turn, run,
    the keypad and back.

    Checks: the base's map (npc.cpp Npc::onBaseMap, BASE_MAP: its moves
        walk, run and stand only) and its animations (crusader.cpp npcAnim:
        a move the walls stop stands him in the combat stance, which the
        base's map takes as its own).
    Drives: BASE_ACTIONS (controls.of_actions) on the base's map (No
        Regret's 25, No Remorse's 40) with the room of walls (room).
    Tests:
        forward
            The up arrow (acted: the walls stop some).
        turn <side>
            An arrow (acted).
        run
            Shift and the up arrow (acted).
        keypad
            The keypad's 7 (acted).
        back
            The down arrow (acted).
    Original: Both follow.
    """
    scene_of = controls.of_actions(game, [(name, keys, frames, 'acted') for name, keys, frames in BASE_ACTIONS])
    number = BASE_MAPS[game]
    arguments = scenes.START[:-1] + [str(number)]
    return dataclasses.replace(scene_of, name='moves-base', maps={number: scene_of.maps[scenes.TEST_MAP] + room()},
                               arguments=arguments)


# The moves that walls stop (animprim.cpp AnimPrimitive::init's search for
# room, run's blocked steps): the rolls, slides, advances and retreats,
# standing, kneeling and armed, and runs, in a room of walls 96 from the
# Silencer's start.
WALL_ACTIONS = ([(f'roll {arrow}', ['ctrl', arrow], 12) for arrow in ARROWS] +
                [(f'strafe {key}', [key], 16) for key in STRAFES] + [('kneel', ['kp5'], 20)] +
                [(f'kneeling roll {arrow}', ['ctrl', arrow], 12) for arrow in ('left', 'right')] +
                [(f'kneeling {key}', [key], 16) for key in STRAFES] + [('stand', ['kp5'], 20), ('weapon', ['1'], 4)] +
                [(f'armed roll {arrow}', ['ctrl', arrow], 12) for arrow in ARROWS] +
                [(f'armed strafe {key}', [key], 16) for key in STRAFES] +
                [(f'turn and run {k}', ['right'], 8) for k in range(1)] + [('run', ['shift', 'up'], 40),
                                                                          ('walk', ['up'], 30), ('back', ['down'], 30)] +
                [(f'keypad {key}', [key], 16) for key in KEYPAD])


def room():
    """Walls around the start, 96 away: along y on the west and east,
    along x on the north and south (each 64 long, 16 thick)."""
    import formats
    x0, y0 = scenes.X0, scenes.Y0
    walls = []
    for k in range(4):
        along = y0 - 96 + 64 * k
        walls += [formats.item_record(x0 - 96, along, 0, scenes.WALL), formats.item_record(x0 + 112, along, 0, scenes.WALL)]
        across = x0 - 96 + 64 * k
        walls += [formats.item_record(across, y0 - 96, 0, scenes.WALL_ALONG),
                  formats.item_record(across, y0 + 112, 0, scenes.WALL_ALONG)]
    return walls


def walls(game):
    """The moves that walls stop, in a room of walls 96 from the start:
    rolls, slides, advances and retreats, standing, kneeling and armed,
    runs, the keypad.

    Checks: the moves the walls stop (animprim.cpp AnimPrimitive::init's
        search for room, run's blocked steps).
    Drives: WALL_ACTIONS (controls.of_actions) in the room of walls (room).
    Tests:
        roll <arrow>
            Ctrl with an arrow: he rolls, or the walls stop him (acted: each
            animates him, the walls stop some).
        strafe <key>
            A strafe key: a slide, an advance or a retreat (acted).
        kneel
            The keypad's 5 (acted).
        kneeling roll <side>
            Ctrl with the left or right arrow while he kneels (acted).
        kneeling <key>
            A strafe key while he kneels (acted).
        stand
            The keypad's 5 again (acted).
        weapon
            The first weapon. No record shows it.
        armed roll <arrow>
            Ctrl with an arrow, armed (acted).
        armed strafe <key>
            A strafe key, armed (acted).
        turn and run <n>
            The right arrow (acted).
        run
            Shift and the up arrow into a wall (acted).
        walk
            The up arrow (acted).
        back
            The down arrow (acted).
        keypad <key>
            A keypad key, Num Lock off (acted).
    Original: Both follow.
    """
    base_scene = controls.of_actions(game, [(name, keys, frames, 'acted' if scenes.footprint(keys) else None)
                                       for name, keys, frames in WALL_ACTIONS])
    records = base_scene.maps[scenes.TEST_MAP] + room()
    return dataclasses.replace(base_scene, name='moves-walls', maps={scenes.TEST_MAP: records})


SCENES = {'moves': scene, 'moves-keypad': keypad, 'moves-base': base, 'moves-walls': walls}
