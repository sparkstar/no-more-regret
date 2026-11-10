"""The port's own settings (the scenes driven by inputs): enhanced.cfg's
language, keys and wide screen, which the
player's recordings have and the originals have not, so the port alone
runs these scenes (twice the same). Variants of the other scenes in Korean
and on the wide screen, and the modern keys with the mouse."""
import dataclasses

import controls
import menus
import saves
import scenes
import specials
import npcs

KOREAN = ['language=ko']
WIDE = ['widescreen=1']
MODERN = ['keys=modern']
NO_MOUSE_ROLL = ['mouse_roll=0']


def variant(make, name, enhanced, same=True):
    """make's scene of a game as a port-only scene called name, with the
    enhanced.cfg lines; with same, its records must be those of make's
    scene, which the original follows (the port's own setting changes no
    pass of the game: Scene.same_as)."""
    def scene(game):
        base = make(game)
        if base is None:
            return None
        return dataclasses.replace(base, name=name, enhanced=enhanced, oracle=False, same_as=base.name if same else '')
    return scene


# The modern keys (bindings_game.cpp GAME_MODERN_BINDINGS): W, A, S and D,
# C kneels, Space jumps, the mouse turns and fires, its right button readies
# the weapon, the wheel picks the weapons, E selects, F grabs, G uses, H the
# medkit, T the cube, Tab the datalink.
SCAN = dict(controls.SCAN, a=0x1e, c=0x2e, f=0x21, h=0x23, t=0x14, tab=0x0f, k=0x25)
MOUSE_LEFT, MOUSE_RIGHT = 1, 2


def modern(game):
    """The modern keys with the mouse: moves by W, A, S and D, the mouse
    turning and firing, the wheel picking weapons, the items' keys.

    Checks: the port's modern layout (enhanced.cfg keys=modern;
        <game>/src/input/bindings_game.cpp GAME_MODERN_BINDINGS: W, S, A
        and D move forward, back and slide; C kneels; Space jumps; the
        mouse's left button fires, its right readies the weapon, the wheel
        picks the weapons; E selects, F grabs, G uses, H the medkit, T the
        energy cube, No Regret's Tab the datalink) and the mouse's turns
        (world.cpp WorldGump::handleMouseMotion: its sideways motion turns
        him, its motion up or down is bound to nothing).
    Drives: the controls scene's data (his things: controls.give); its own
        inputs, in the tests' order, each held a while and 8 frames apart.
    Tests:
        walk
            W held: he walks forward (moved).
        slide <side>
            A or D held: he slides to that side (moved).
        back
            S held: he steps back (moved).
        run
            Shift with W: he runs (moved).
        turn <side>
            The mouse moved right or left, 12 a frame for 30 frames: he
            turns (moved: his direction).
        mouse up
            The mouse moved up, 8 a frame for 10 frames: bound to nothing,
            he starts no animation (idle).
        kneel
            C: he kneels (acted).
        jump
            Space: he jumps (moved).
        weapon 1
            1: the first weapon. No record shows it.
        fire
            The left button held while the mouse stands still: he fires
            (acted).
        fire turning
            The left button held while the mouse moves right: he fires and
            turns (moved).
        ready weapon
            The right button: he readies or lowers the weapon (acted).
        wheel
            The wheel down three times and up once: the next weapons and
            the one before. No record shows it.
        items
            E, F, G, H and T: the selection, the grab, the item, the
            medkit, the energy cube. No record shows them.
        datalink
            No Regret's Tab, its datalink, closed by Escape. No record shows
            it.
    Original: None: the port's own keys; no original's like.
    Traps: the footprints' windows run from an input's first frame to the
        next one's: an animation an input starts that runs on into the next
        window is that one's movement too (the jump's landing, the run's
        stop), so the idle and acted ones follow inputs that end at rest.
    """
    lines, frame, windows = [], controls.START, []

    def keys(names, held, wait=8):
        nonlocal frame
        for verb, at in (('key', frame), ('release', frame + held)):
            for key in names if verb == 'key' else reversed(names):
                scan = SCAN[key]
                prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
                lines.extend(prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}'])
        frame += held + wait

    def mouse(steps, buttons=0, wait=8):
        """A run of mouse motions (mickeys), one a frame, with the buttons
        held, then let go."""
        nonlocal frame
        for dx, dy in steps:
            lines.append(f'{frame}\tmotion\t{dx}\t{dy}\t{buttons}')
            frame += 1
        lines.append(f'{frame}\tmotion\t0\t0\t0')
        frame += wait

    def test(name, kind, *inputs):
        """The inputs (functions that add them), as the test name whose
        footprint is kind from its first frame to the next test's."""
        first = frame
        for add in inputs:
            add()
        windows.append((name, first, kind))

    test('walk', 'moved', lambda: keys(['w'], 40))
    test('slide left', 'moved', lambda: keys(['a'], 20))
    test('slide right', 'moved', lambda: keys(['d'], 20))
    test('back', 'moved', lambda: keys(['s'], 20))
    test('run', 'moved', lambda: keys(['shift', 'w'], 30))
    test('turn right', 'moved', lambda: mouse([(12, 0)] * 30))
    test('turn left', 'moved', lambda: mouse([(-12, 0)] * 30))
    test('mouse up', 'idle', lambda: mouse([(0, -8)] * 10))
    test('kneel', 'acted', lambda: keys(['c'], 30))
    test('jump', 'moved', lambda: keys(['space'], 12))
    test('weapon 1', None, lambda: keys(['1'], 4))
    test('fire', 'acted', lambda: mouse([(0, 0)] * 30, MOUSE_LEFT))
    test('fire turning', 'moved', lambda: mouse([(6, 0)] * 20, MOUSE_LEFT))
    test('ready weapon', 'acted', lambda: mouse([(0, 0)] * 10, MOUSE_RIGHT))

    def wheel():
        nonlocal frame
        for _ in range(3):
            lines.append(f'{frame}\twheel\t-1')
            frame += 10
        lines.append(f'{frame}\twheel\t1')
        frame += 10
    test('wheel', None, wheel)
    test('items', None, *[lambda key=key: keys([key], 4) for key in 'efght'])
    if game == 'regret':
        # No Regret's datalink, closed by Escape (No Remorse has none: its
        # Tab jumps and its Escape opens the menu)
        test('datalink', None, lambda: keys(['tab'], 4, 30), lambda: keys(['esc'], 4))
    base = controls.scene(game)
    ends = [first for _, first, _ in windows[1:]] + [frame]
    return dataclasses.replace(base, name='modern', enhanced=MODERN, oracle=False, inputs=lines, frames=frame + 60,
                               tests=[name for name, _, _ in windows],
                               ran=scenes.windows_ran([(first, last, kind)
                                                       for (_, first, kind), last in zip(windows, ends)]))


# The settings gump's rows (settings.cpp gameSettingColumns): the left
# column's 7, then the right's 7, CONTROLS the right's fifth; then Done.
SETTINGS_ROWS, CONTROLS_ROW = 14, 11


def settings(game):
    """The port's settings from the start menu: each row's value stepped
    right and back, the key settings (K added to MOVE BACK, TURN LEFT's
    keys cleared), Done; then a new game, where K steps the Silencer back,
    the left arrow does nothing and the right arrow turns him.

    Checks: the port's settings screen (settings.cpp SettingsGump: each
        row's value), the key settings (keysgump.cpp KeysGump: Enter on a
        row waits for an input and adds it to the row's action, Escape
        while it waits clears the action's inputs, the groups one way and
        back; bindings.h captureKey, clearInputs) and that the game takes
        the bindings the screen made (bindings.cpp: the action of a key).
    Drives: the menus scene's start (no -warp); the keys: OPTIONS from the
        start menu, then each of the 14 rows right, left and down;
        CONTROLS' key settings: down to MOVE BACK, Enter, K; down to TURN
        LEFT, Enter, Escape; the groups right, right, left; Escape; the
        rest of the rows; Done. Then up to NEW GAME, Enter, Enter (the
        difficulty dialog's level), map 1; there K held, the left arrow
        held, the right arrow held.
    Tests:
        settings
            The settings screen's rows and the key settings' keys. No record
            shows them: their work shows in the game after.
        new game
            The new game after Done: map 1 loaded (the trace's M line).
        K steps back
            K, which the key settings added to MOVE BACK: he steps back
            (moved).
        left arrow cleared
            The left arrow, TURN LEFT's key until the key settings cleared
            it: his place and direction stay (not moved).
        right arrow turns
            The right arrow, which the key settings left: he turns (moved).
    Original: None: the port's own screen; no original's like.
    Traps: Escape while a row waits clears the action's inputs (the
        screen's hint says so); Escape after it closes the key settings.
    """
    steps = [(['down'], 2, 20)] * 3 + [(['enter'], 2, 60)]
    for row in range(SETTINGS_ROWS):
        steps += [(['right'], 2, 15), (['left'], 2, 15)]
        if row == CONTROLS_ROW:
            steps += [(['enter'], 2, 60), (['down'], 2, 10), (['enter'], 2, 20), (['k'], 2, 20), (['down'], 2, 10),
                      (['enter'], 2, 20), (['esc'], 2, 20), (['right'], 2, 20), (['right'], 2, 20), (['left'], 2, 20),
                      (['esc'], 2, 60)]
        steps += [(['down'], 2, 15)]
    steps += [(['enter'], 2, 60)]
    # the new game: up from OPTIONS to NEW GAME, the difficulty dialog and
    # its level, the map's load
    steps += [(['up'], 2, 20)] * 3 + [(['enter'], 2, 60), (['enter'], 2, 240)]
    inputs, frame = [], menus.START
    for keys, held, wait in steps:
        inputs += menus.press(frame, keys, held)
        frame += held + wait
    windows = []
    for key in ('k', 'left', 'right'):
        inputs += menus.press(frame, [key], 30)
        windows.append((frame, frame + 70))
        frame += 70
    base = menus.scene(game)
    (k_first, k_last), (left_first, left_last), (right_first, right_last) = windows

    def ran(record, tests):
        return [None, menus.new_game(record), record.moved(k_first, k_last),
                menus.new_game(record) and not record.moved(left_first, left_last),
                record.moved(right_first, right_last)]
    return dataclasses.replace(base, name='settings', oracle=False, inputs=inputs, frames=frame + 60,
                               tests=['settings', 'new game', 'K steps back', 'left arrow cleared',
                                      'right arrow turns'], ran=ran)


def controls_korean(game):
    """The controls scene in Korean.

    Variant of: controls
    Checks: the Korean text (enhanced.cfg language=ko: the status's and the
        boxes' text) changes no pass of the game.
    Drives: the controls scene with language=ko.
    Original: None: the port's own text; its records must be the controls
        scene's (Scene.same_as), which both originals follow.
    """
    return variant(controls.scene, 'controls-korean', KOREAN)(game)


def computer_korean(game):
    """The computer's text in Korean.

    Variant of: computer
    Checks: the Korean terminal (enhanced.cfg language=ko: its layout and
        its text typed).
    Drives: the computer scene with language=ko.
    Original: None: the port's own text; the Korean terminal's layout types
        the text in other times, so its passes are not the English one's:
        its ground is its footprints.
    """
    return variant(specials.computer, 'computer-korean', KOREAN, same=False)(game)


def saves_korean(game):
    """The saves scene in Korean.

    Variant of: saves
    Checks: the Korean save and load gumps and notices change no pass of
        the game.
    Drives: the saves scene with language=ko.
    Original: None: the port's own text; its records must be the saves
        scene's (Scene.same_as), which both originals follow.
    """
    return variant(saves.scene, 'saves-korean', KOREAN)(game)


def npcs_wide(game):
    """The NPCs' scene on the wide screen.

    Variant of: npcs
    Checks: the wide screen (enhanced.cfg widescreen=1: the sides drawn,
        the rules kept at 640 by 480) changes no pass of the game.
    Drives: the npcs scene with widescreen=1.
    Original: None: the port's own screen; its records must be the npcs
        scene's (Scene.same_as), which both originals follow.
    """
    return variant(npcs.scene, 'npcs-wide', WIDE)(game)


def key_lines(frame, names, verb):
    """The lines of the keys going down (key) or up (release) at frame, the
    extended ones after E0h."""
    lines = []
    for name in names if verb == 'key' else reversed(names):
        scan = SCAN[name]
        prefix = [f'{frame}\tkey\te0'] if scan >> 8 == 0xe0 else []
        lines += prefix + [f'{frame}\t{verb}\t{scan & 0xff:02x}']
    return lines


def held_while_moved(frame, names, across, frames):
    """The keys held down while the mouse moves across (mickeys) a frame
    for frames, then let go: the inputs and the frame after them."""
    lines = key_lines(frame, names, 'key')
    frame += 4
    for _ in range(frames):
        lines.append(f'{frame}\tmotion\t{across}\t0\t0')
        frame += 1
    lines.append(f'{frame}\tmotion\t0\t0\t0')
    lines += key_lines(frame + 2, names, 'release')
    return lines, frame + 3


def place_and_turn(record, first, last):
    """Whether the Silencer's place (x, y, z) and his direction changed
    from frame first to before frame last."""
    states = record.between(first, last)
    return len({tuple(state[5:8]) for state in states}) > 1, len({state[8] for state in states}) > 1


# The mouse's turns: a step across a frame large enough for a turn each
# (WorldGump::handleMouseMotion's shift of the cursor's move), and the
# frames a roll goes on after the keys are let go.
ACROSS, MOVES, SETTLE = 40, 20, 60


def mouse_no_roll(game):
    """The mouse that does not roll (the key settings' MOUSE ROLL off):
    with the roll's key (Ctrl) or the combat step's (Alt) held, the mouse
    moved sideways turns the Silencer where he stands.

    Checks: enhanced.cfg's mouse_roll=0 (bindings.cpp readBindingsText) and
        the mouse's turn key without the roll and the combat step
        (bindings.cpp deviceTurnShift; world.cpp
        WorldGump::handleMouseMotion; control.cpp chooseMove).
    Drives: the controls scene's data with keys=modern and mouse_roll=0;
        its own inputs: Ctrl held while the mouse moves right, then Alt
        held while it moves left.
    Tests:
        no roll with <mode>
            While the mode's key is held and a while after, the Silencer's
            direction changed and his place did not (the original's roll
            or side step would move him).
    Original: None: the port's own setting; the original's mouse rolls with
        Ctrl (mouse-roll-original).
    """
    lines, frame = held_while_moved(controls.START, ['ctrl'], ACROSS, MOVES)
    second = frame + SETTLE
    more, end = held_while_moved(second, ['alt'], -ACROSS, MOVES)
    windows = [(controls.START, second), (second, end + SETTLE)]
    base = controls.scene(game)
    return dataclasses.replace(base, name='mouse-no-roll', enhanced=MODERN + NO_MOUSE_ROLL, oracle=False,
                               inputs=lines + more, frames=end + 2 * SETTLE,
                               tests=['no roll with roll', 'no roll with combat step'],
                               ran=lambda record, tests: [place_and_turn(record, *w) == (False, True) for w in windows])


def mouse_no_roll_original(game):
    """The mouse-no-roll scene with the original layout: MOUSE ROLL is the
    same whichever the preset.

    Variant of: mouse-no-roll
    Checks: mouse_roll=0 apart from the keys' preset (bindings.cpp
        deviceTurnShift).
    Drives: the mouse-no-roll scene with mouse_roll=0 alone (the original
        layout).
    Original: None: the port's own setting; its records must be the
        mouse-no-roll scene's (Scene.same_as).
    """
    return variant(mouse_no_roll, 'mouse-no-roll-original', NO_MOUSE_ROLL)(game)


def mouse_roll_original(game):
    """The original layout's mouse with the roll's key: the mouse moved
    sideways with Ctrl held rolls the Silencer, as the original's.

    Checks: the original layout's turns on the mouse's motion
        (bindings_game.cpp GAME_ACTION_DEFAULTS: TURN_LEFT, TURN_RIGHT) and
        the mouse's turn key with the modes held, MOUSE ROLL's default
        (world.cpp WorldGump::handleMouseMotion, bindings.cpp
        deviceTurnShift; control.cpp chooseMove, roll).
    Drives: the controls scene's data (the original layout); its own
        inputs: Ctrl held while the mouse moves right.
    Tests:
        mouse roll
            While Ctrl is held and a while after, the Silencer's place
            changed: he rolled.
    Original: None: the port's mouse moves come as the window's frames
        give them; the ground is the original's handleMouseMotion, which
        hands the turn on with the shift state held (key.shift=keyShift).
    """
    lines, frame = held_while_moved(controls.START, ['ctrl'], ACROSS, MOVES)
    base = controls.scene(game)
    return dataclasses.replace(base, name='mouse-roll-original', oracle=False, inputs=lines, frames=frame + 2 * SETTLE,
                               tests=['mouse roll'],
                               ran=lambda record, tests: [place_and_turn(record, controls.START, frame + SETTLE)[0]])


def mouse_roll_modern(game):
    """The mouse-roll-original scene with the modern layout, whose mouse
    rolls with Ctrl as the original's while MOUSE ROLL is on (the default:
    the recordings made before the setting replay as they were).

    Variant of: mouse-roll-original
    Checks: the modern layout's turns on the mouse's motion
        (bindings_game.cpp GAME_MODERN_BINDINGS keeps the original's) and
        MOUSE ROLL's default with keys=modern.
    Drives: the mouse-roll-original scene with keys=modern.
    Original: None: the port's own layout; its records must be the
        mouse-roll-original scene's (Scene.same_as).
    """
    return variant(mouse_roll_original, 'mouse-roll-modern', MODERN)(game)


def roll_forward_key(game):
    """No Regret's roll forward on an input of its own (ROLL_FORWARD, none
    by default): bound to K in enhanced.cfg, K rolls the Silencer forward.

    Checks: the roll forward's action (bindings.cpp ACTION_INFOS
        roll_forward, keysgump.cpp's row; control.cpp keyMove, chooseMove,
        roll: the roll key with up's roll) and a binding of enhanced.cfg
        (bind_roll_forward=k).
    Drives: the controls scene's data with keys=modern and
        bind_roll_forward=k; its own input: K pressed.
    Tests:
        roll forward
            After K the Silencer's place changed and his direction did not.
    Original: None: the port's own input; the ground is the original's
        Ctrl with up, whose roll it makes.
    """
    if game != 'regret':
        return None
    lines = key_lines(controls.START, ['k'], 'key') + key_lines(controls.START + 4, ['k'], 'release')
    end = controls.START + 4 + SETTLE
    base = controls.scene(game)
    return dataclasses.replace(base, name='roll-forward-key', enhanced=MODERN + ['bind_roll_forward=k'], oracle=False,
                               inputs=lines, frames=end + SETTLE, tests=['roll forward'],
                               ran=lambda record, tests: [place_and_turn(record, controls.START, end) == (True, False)])


SCENES = {
    'controls-korean': controls_korean,
    'computer-korean': computer_korean,
    'saves-korean': saves_korean,
    'npcs-wide': npcs_wide,
    'modern': modern,
    'settings': settings,
    'mouse-no-roll': mouse_no_roll,
    'mouse-no-roll-original': mouse_no_roll_original,
    'mouse-roll-original': mouse_roll_original,
    'mouse-roll-modern': mouse_roll_modern,
    'roll-forward-key': roll_forward_key,
}
