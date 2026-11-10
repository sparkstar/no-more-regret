"""The player's controls (the scenes driven by inputs): the test egg gives the
Silencer weapons, their clips and items,
then the recording presses the original's keys (<game>/src/input/
bindings_game.cpp) one action after another: moves, turns, runs, slides,
rolls, jumps, kneeling, the keypad's moves, firing, the weapons' and the
items' keys and the Ctrl keys of the game."""
import scenes
import ucasm
import world

# Scan codes (set 1); extended ones have E0h above them.
SCAN = {
    'esc': 0x01, '1': 0x02, '2': 0x03, '3': 0x04, '4': 0x05, '5': 0x06, 'minus': 0x0c, 'equals': 0x0d,
    'q': 0x10, 'w': 0x11, 'e': 0x12, 'r': 0x13, 'u': 0x16, 'i': 0x17, 'o': 0x18, 'enter': 0x1c, 'ctrl': 0x1d,
    's': 0x1f, 'd': 0x20, 'g': 0x22, 'j': 0x24, 'l': 0x26, 'shift': 0x2a, 'z': 0x2c, 'v': 0x2f, 'b': 0x30,
    'm': 0x32, 'period': 0x34, 'alt': 0x38, 'space': 0x39, 'capslock': 0x3a, 'kp7': 0x47, 'kp8': 0x48,
    'kp9': 0x49, 'kp4': 0x4b, 'kp5': 0x4c, 'kp6': 0x4d, 'kp1': 0x4f, 'kp2': 0x50, 'kp3': 0x51,
    'up': 0xe048, 'down': 0xe050, 'left': 0xe04b, 'right': 0xe04d, 'home': 0xe047, 'end': 0xe04f,
    'pageup': 0xe049, 'pagedown': 0xe051, 'insert': 0xe052, 'delete': 0xe053, 'kpslash': 0xe035,
}

# Each action: its name, the keys held together, how many frames (and its
# footprint where it is not that of its keys: timeline).
ACTIONS = [
    ('forward', ['up'], 40), ('turn left', ['left'], 16), ('turn right', ['right'], 16), ('back', ['down'], 24),
    ('run', ['shift', 'up'], 40), ('combat step', ['alt', 'up'], 20), ('slide left', ['delete'], 20),
    ('slide right', ['pagedown'], 20), ('roll left', ['insert'], 12), ('roll right', ['pageup'], 12),
    ('advance', ['home'], 20), ('retreat', ['end'], 20), ('jump forward', ['kpslash'], 12), ('jump', ['j'], 12),
    ('kneel', ['kp5'], 30), ('kneeling forward', ['kp5', 'up'], 20), ('run lock', ['capslock'], 2),
    ('forward locked', ['up'], 30), ('run unlock', ['capslock'], 2),
    ('keypad north-west', ['kp8'], 16), ('keypad south-east', ['kp2'], 16), ('keypad south-west', ['kp4'], 16),
    ('keypad north-east', ['kp6'], 16), ('keypad west', ['kp7'], 16), ('keypad north', ['kp9'], 16),
    ('keypad south', ['kp1'], 16), ('keypad east', ['kp3'], 16),
    ('weapon 1', ['1'], 4), ('fire', ['space'], 30), ('fire again', ['space'], 30), ('weapon 2', ['2'], 4),
    ('fire 2', ['space'], 30), ('next weapon', ['w'], 4), ('previous weapon', ['q'], 4), ('weapon 3', ['3'], 4),
    ('fire 3', ['space'], 40), ('weapon 4', ['4'], 4), ('fire 4', ['space'], 40), ('kneel and fire', ['kp5'], 20),
    ('fire kneeling', ['space'], 30), ('stand', ['kp5'], 20),
    ('next item', ['o'], 4), ('previous item', ['i'], 4), ('use item', ['u'], 4), ('medkit', ['m'], 4),
    ('energy cube', ['e'], 4), ('select items', ['s'], 4), ('selection next', ['period'], 4),
    ('selection choose', ['enter'], 4), ('grab', ['g'], 4), ('detonate', ['b'], 4), ('camera', ['z'], 4),
]
# Each game's own keys after them (<game>/src/input/bindings_game.cpp): No
# Regret's Escape (no NPC to give back) and Ctrl keys; No Remorse has no
# datalink and no reticle style, its music keys have no Ctrl, its version
# shows a box that Enter closes, and its Escape is the menu's key too
# (No Regret's keys there would open the menu, which takes the rest).
OWN_ACTIONS = {
    'regret': [('camera back', ['esc'], 4), ('datalink', ['ctrl', 'd'], 4), ('reticle', ['ctrl', 'r'], 4),
               ('louder', ['ctrl', 'equals'], 4), ('quieter', ['ctrl', 'minus'], 4), ('next song', ['ctrl', 'm'], 4),
               ('version', ['ctrl', 'v'], 4), ('drop weapon', ['ctrl', 'l'], 4)],
    'remorse': [('louder', ['equals'], 4), ('quieter', ['minus'], 4), ('next song', ['ctrl', 'm'], 4),
                ('version', ['ctrl', 'v'], 4), ('version closed', ['enter'], 4), ('drop weapon', ['ctrl', 'd'], 4)],
}
# The inputs start once the egg has given the things.
START, GAP = 150, 8


def press(frame, keys, frames):
    """inputs.tsv's lines: the keys down at frame in turn, up frames later
    (an extended key's E0h before each)."""
    lines = []
    for verb, at in (('key', frame), ('release', frame + frames)):
        for key in keys if verb == 'key' else reversed(keys):
            scan = SCAN[key]
            prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
            lines += prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}']
    return lines


def give(table, game, local, local_offsets):
    """The Silencer picks up five weapons of those with overlays (one
    without halts the drawing of his firing), a clip of each ammunition,
    inventory items and a bomb, each made at his feet in the local."""
    shapes = world.table_shapes(game)
    weapons = [shape for shape, _ in world.overlay_weapons(game) if shape in shapes[10]][:5]
    lines = []
    for shape in weapons + shapes[11] + shapes[13][:6] + shapes[12][:1]:
        lines += scenes.call(table, '@Item@create$qusus', f'{local} {shape} 0', local_offsets=local_offsets)
        lines += scenes.call(table, '@Item@setQuality$qus', f'{local} 30', local_offsets=local_offsets)
        lines += scenes.call(table, '@Item@popToCoords$qususuc', f'{local} {scenes.X0} {scenes.Y0} 0',
                             local_offsets=local_offsets)
        lines += scenes.call(table, '@pickUp$qusuc', f'{local.lower()} 0', local_offsets=local_offsets)
    return lines


def gifts(game):
    """Event 7: the Silencer's things (give) and all the energy he can hold
    (255: No Regret's first weapon, the XP-5, fires once on 140 of it), and
    two items near him playing looped sounds, whose volumes follow his
    distance as he moves (sndsrc.cpp updateLoopVolumes)."""
    table = scenes.intrinsics_table(game)
    lines = ['ENTER 4', 'PUSH_WORD 1', 'POP_LOCAL_WORD -4']
    lines += scenes.call(table, '@Npc@setMana$qus', 'S 255', local_offsets={'S': -4})
    lines += give(table, game, 'N', {'N': -2})
    for sound, (x, y) in ((5, (scenes.X0 + 200, scenes.Y0)), (9, (scenes.X0 - 150, scenes.Y0 + 150))):
        lines += scenes.call(table, '@Item@create$qusus', f'N {scenes.ITEM} 0', local_offsets={'N': -2})
        lines += scenes.call(table, '@Item@popToCoords$qususuc', f'N {x} {y} 0', local_offsets={'N': -2})
        lines += scenes.call(table, '@Item@playLoop$qi', f'N {sound}', local_offsets={'N': -2})
    return '\n'.join(lines + ['RETURN'])


def timeline(actions, gap=GAP):
    """The actions one after another from START, gap frames apart: their
    inputs, each one's window (its first frame, the next one's, and its
    footprint: scenes.footprint of its keys, or an action's fourth element)
    and the frame after the last."""
    inputs, windows, frame = [], [], START
    for _, keys, frames, *kind in actions:
        inputs += press(frame, keys, frames)
        windows.append((frame, frame + frames + gap, kind[0] if kind else scenes.footprint(keys)))
        frame += frames + gap
    return inputs, windows, frame


def of_actions(game, actions, gap=GAP):
    """A scene of the actions gap frames apart, on the controls scene's map
    with its gifts: a key pressed while the Silencer's animation is still
    busy is lost (the kneel key's), so a scene whose actions follow long
    ones spaces them more."""
    inputs, windows, frame = timeline(actions, gap)
    return scenes.Scene(name='controls', maps={scenes.TEST_MAP: scenes.floor_and_egg(16) + scenes.animated_items()},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble(gifts(game))})},
                        arguments=scenes.START, frames=frame + 60, tests=[name for name, *_ in actions],
                        ran=scenes.windows_ran(windows), inputs=inputs)


def scene(game):
    """The player's controls: each action's keys pressed in turn, as the
    original binds them.

    Checks: the controller's actions (control.cpp Controller::move, turn,
        fire, cheatKey; <game>/src/input/bindings_game.cpp): moves, turns,
        runs, slides, rolls, jumps, kneeling, the keypad, the run lock,
        firing each weapon, the weapons' and the items' keys, the
        selection, the camera and the game's own Ctrl keys; looped sounds'
        volumes following the Silencer (sndsrc.cpp updateLoopVolumes); the
        animated shapes about (dlist.cpp animateItems).
    Drives: inputs from frame START, GAP apart (controls.timeline); the test
        egg's event 7 (gifts) gives him five weapons with overlays, a clip
        of each ammunition, items, a bomb and the energy, and makes two
        items playing looped sounds; a floor of 16 by 16 tiles and one of
        each animated shape.
    Tests:
        forward
            The up arrow held: he walks (moved).
        turn <side>
            An arrow held: he turns (moved).
        back
            The down arrow: he steps back (moved).
        run
            Shift with the up arrow: he runs (moved).
        combat step
            Alt with the up arrow (moved).
        slide <side>
            Delete or Page Down: he slides (moved).
        roll <side>
            Insert or Page Up: he rolls (moved).
        advance
            Home (moved).
        retreat
            End (moved).
        jump forward
            The keypad's / (moved).
        jump
            J (moved).
        kneel
            The keypad's 5: he kneels (acted).
        kneeling forward
            The keypad's 5 with the up arrow (moved).
        run lock
            Caps Lock: the run locked. No record shows it.
        forward locked
            The up arrow with the run locked: he runs (moved).
        run unlock
            Caps Lock again. No record shows it.
        keypad <direction>
            A keypad key, Num Lock off: he walks that way (moved).
        weapon <n>
            The weapon's key. No record shows it.
        fire
            Space with the first weapon drawn: a shot (acted).
        fire again
            Space again (acted); in No Regret no record shows it: the XP-5
            has no energy for a second shot.
        fire <n>
            Space with weapon n drawn (acted).
        next weapon
            W. No record shows it.
        previous weapon
            Q. No record shows it.
        kneel and fire
            The keypad's 5 with the weapon drawn (acted).
        fire kneeling
            Space while he kneels (acted).
        stand
            The keypad's 5 again: he stands (acted).
        next item
            O. No record shows it.
        previous item
            I. No record shows it.
        use item
            U. No record shows it.
        medkit
            M. No record shows it.
        energy cube
            E. No record shows it.
        select items
            S: the selection of the items about. No record shows it.
        selection next
            The period. No record shows it.
        selection choose
            Enter. No record shows it.
        grab
            G. No record shows it.
        detonate
            B: the bomb. No record shows it.
        camera
            Z: the camera to the Silencer. No record shows it.
        camera back
            No Regret's Escape: no other NPC to give the control back from,
            nothing. No record shows it.
        datalink
            No Regret's Ctrl+D. No record shows it.
        reticle
            No Regret's Ctrl+R, the reticle's style. No record shows it.
        louder
            The music louder (No Regret's Ctrl+=, No Remorse's =). No record
            shows it.
        quieter
            The music quieter (No Regret's Ctrl+-, No Remorse's -). No record
            shows it.
        next song
            Ctrl+M. No record shows it.
        version
            Ctrl+V (No Remorse shows a box). No record shows it.
        version closed
            No Remorse's Enter on the version's box. No record shows it.
        drop weapon
            No Regret's Ctrl+L, No Remorse's Ctrl+D. No record shows it.
    Original: Both follow.
    Guards: No Remorse's Escape is the menu's key too: No Regret's keys
        there would open the menu, which takes the rest.
    Traps: a key pressed while an animation runs is lost: GAP spaces them.
    """
    # No Regret's XP-5 has no energy for a second shot (gifts)
    actions = [(name, keys, frames, None) if (game, name) == ('regret', 'fire again') else (name, keys, frames)
               for name, keys, frames in ACTIONS + OWN_ACTIONS[game]]
    return of_actions(game, actions)


# crusader.cfg as the game writes it back (options.cpp writeOptions): every
# key its options have, which readOptions reads at the start.
WRITTEN_CONFIG = ['musicvolume=32', 'soundvolume=64', 'mousespeed=1', 'mixrate=22050', 'animation=0', 'button1=0',
                  'button2=10', 'button3=2', 'button4=7', 'button1_2=11', 'button1_3=6', 'button1_4=8', 'button2_3=5',
                  'button2_4=1', 'button3_4=9', 'limitblasts=true', 'frameskip=3', 'filter=true', 'subtitles=true',
                  'lowresmovies=false', 'bigvideo=false', 'videosize=small', 'cdicon=1', 'joystick=0', 'newvideo=true']


def configured(game):
    """The controls scene with crusader.cfg as the game writes it back.

    Variant of: controls
    Checks: the options read at the start (options.cpp readOptions) from a
        file with every key the game writes (WRITTEN_CONFIG), the mouse's
        buttons among them.
    Drives: the controls scene's inputs and data, with that crusader.cfg.
    Original: Both follow.
    """
    import dataclasses
    return dataclasses.replace(scene(game), name='controls-config', config=WRITTEN_CONFIG)
