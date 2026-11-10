"""Saves (the scenes driven by inputs): the
NPCs of npcs.py fight the immortal Silencer while the recording saves and
loads the game by the keys a player presses: the save gump (F9: a slot
chosen with the arrows, a name typed), the load gump (F8), the quick save
and load (F5, F4), each a while apart, so that the processes of every kind
alive go through the save's fields and back."""
import controls
import npcs
import scenes
import ucasm

SCAN = dict(controls.SCAN, f4=0x3e, f5=0x3f, f8=0x42, f9=0x43, a=0x1e, b=0x30, n=0x31, y=0x15)
# Each step: its test's name, the keys, how long they are held, the wait
# after. The save gump starts at the slot chosen last (loadsave.cpp
# selectedSlot); the quick save and load ask (QuestionGump: Y for yes), and
# their notices close with a key (a notice left open takes the keys after
# it: the quick load's F4, Y and Enter would do nothing).
STEPS = [
    # a save in slot 2 named AB, and its load
    ('save: the gump', ['f9'], 2, 30), ('save: slot 1', ['down'], 2, 10), ('save: slot 2', ['down'], 2, 10),
    ('save: slot 2 chosen', ['enter'], 2, 10), ('save: A typed', ['a'], 2, 6), ('save: B typed', ['b'], 2, 6),
    ('save: saved as AB', ['enter'], 2, 120), ('load: the gump', ['f8'], 2, 30),
    ('load: slot 2 loaded', ['enter'], 2, 150),
    # the quick save and load, then a quick save refused
    ('quick save: asked', ['f5'], 2, 30), ('quick save: yes', ['y'], 2, 150),
    ('quick save: the notice closed', ['enter'], 2, 30), ('quick load: asked', ['f4'], 2, 30),
    ('quick load: yes', ['y'], 2, 90), ('quick load: the notice closed', ['enter'], 2, 150),
    ('quick save again: asked', ['f5'], 2, 30), ('quick save again: no', ['n'], 2, 60),
    # slot 2 again (its name goes on: ABN), an empty slot's load (the load
    # gump stays), and the gump left by Escape
    ('save again: the gump at slot 2', ['f9'], 2, 30), ('save again: slot 2 chosen', ['enter'], 2, 10),
    ('save again: N typed', ['n'], 2, 6), ('save again: saved as ABN', ['enter'], 2, 120),
    ('load again: the gump', ['f8'], 2, 30), ('load again: slot 1', ['up'], 2, 10),
    ('load again: the empty slot, the gump stays', ['enter'], 2, 60), ('load again: left by Escape', ['esc'], 2, 60),
]
START = 600


def quick(save_at, load_at):
    """inputs.tsv's lines of a quick save at save_at (F5, Y, the notice
    closed) and a quick load at load_at (F4, Y, the notice closed), and the
    frames the load gives back (it goes back to the save's state)."""
    lines = []
    for at, steps in ((save_at, [(['f5'], 2, 30), (['y'], 2, 60), (['enter'], 2, 60)]),
                      (load_at, [(['f4'], 2, 30), (['y'], 2, 60), (['enter'], 2, 60)])):
        for keys, held, wait in steps:
            lines += press(at, keys, held)
            at += held + wait
    return lines, load_at - save_at


def press(frame, keys, frames):
    lines = []
    for verb, at in (('key', frame), ('release', frame + frames)):
        for key in keys if verb == 'key' else reversed(keys):
            scan = SCAN[key]
            prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
            lines += prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}']
    return lines


def scene(game):
    """The game saved and loaded by the keys a player presses, while the
    NPCs of npcs.py stand about the immortal Silencer.

    Checks: the save gump (F9: a slot chosen with the arrows, a name
        typed), the load gump (F8), the quick save and load (F5, F4) with
        their questions and notices (loadsave.cpp, shapedlg.cpp
        QuestionGump, NoticeGump; world.cpp quickLoad), and the save and
        load of every kind of process alive (savegame.cpp saveGame,
        loadGame).
    Drives: inputs from frame START (STEPS); the test egg's event 7 makes
        the npcs scene's NPCs without their fights (npcs.driver); its map.
        No record shows the gumps' keys: what a load did is in the states
        after it.
    Tests:
        save: <what>
            F9, the arrows to slot 2, Enter, A and B typed, Enter: saved as
            AB.
        load: <what>
            F8 (the gump at slot 2, the slot chosen last) and Enter: loaded.
        quick save: <what>
            F5, Y, the notice closed: the quick save.
        quick load: <what>
            F4, Y, the notice closed: the quick save loaded.
        quick save again: <what>
            F5 refused by N: nothing saved.
        save again: <what>
            F9 (at slot 2), Enter, N typed after AB, Enter: saved as ABN.
        load again: <what>
            F8, the up arrow to slot 1 (empty), Enter (nothing: the gump
            stays), Escape: the gump left.
    Original: Both follow.
    Guards: the quick save's notice is closed before the quick load,
        which runs only then.
    """
    inputs, frame = [], START
    for _, keys, held, wait in STEPS:
        inputs += press(frame, keys, held)
        frame += held + wait
    code = ucasm.assemble(npcs.driver(game, fights=False))
    names = [name for name, *_ in STEPS]
    # (the save and load gumps' keys: no record keeps them)
    return scenes.Scene(name='saves', maps={scenes.TEST_MAP: npcs.floor_and_things()},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code})}, arguments=scenes.START,
                        frames=frame + 60, tests=names, ran=lambda record, tests: [None] * len(tests),
                        inputs=inputs)


def quick_saves(frames):
    """inputs.tsv's lines of a quick save (F5, Y, the notice closed) at each
    of frames: each writes the processes alive then."""
    lines = []
    for at in frames:
        for keys, held, wait in [(['f5'], 2, 30), (['y'], 2, 60), (['enter'], 2, 60)]:
            lines += press(at, keys, held)
            at += held + wait
    return lines
