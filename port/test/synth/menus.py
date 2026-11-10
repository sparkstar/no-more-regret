"""The start and the game's menus: the game
starts without -warp, so the intro's movies come first (none on our CD:
each ends at once), then the start menu. The keys move over its buttons,
open the load gump and close it, try the save and the resume (nothing at
the start), open the difficulty dialog and cancel it, open it again and
choose a level; the new game goes to map 1's egg 30. There the game's menu
key opens the menu over the game: the save gump and back, the resume; the
help's pages; the menu again with the load gump."""
import controls
import formats
import scenes
import ucasm

# The original's keys of the game's menu and of the help (<game>/src/input/
# bindings_game.cpp GAME_ACTION_DEFAULTS).
MENU_KEY = {'regret': 'f1', 'remorse': 'esc'}
HELP_KEY = {'regret': 'f3', 'remorse': 'f1'}
SCAN = dict(controls.SCAN, f1=0x3b, f3=0x3d, k=0x25)
FIRST_MAP, FIRST_EGG = 1, 30
START = 120


def press(frame, keys, held):
    lines = []
    for verb, at in (('key', frame), ('release', frame + held)):
        for key in keys if verb == 'key' else reversed(keys):
            scan = SCAN[key]
            prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
            lines += prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}']
    return lines


def steps(game):
    """(keys, held, wait) in turn."""
    menu, help_key = MENU_KEY[game], HELP_KEY[game]
    at_start = [
        (['down'], 2, 20), (['enter'], 2, 60), (['esc'], 2, 60),          # load gump, closed
        (['down'], 2, 20), (['enter'], 2, 30),                            # save: nothing at the start
        (['right'], 2, 20), (['left'], 2, 20), (['esc'], 2, 30),          # quit and back; resume: nothing
        (['up'], 2, 20), (['up'], 2, 20), (['enter'], 2, 60),             # new game: the difficulty
        (['down'], 2, 20), (['esc'], 2, 60),                              # cancelled
        (['enter'], 2, 60), (['down'], 2, 20), (['up'], 2, 20), (['up'], 2, 20), (['enter'], 2, 240),
    ]
    in_game = [
        ([menu], 2, 60), (['down'], 2, 20), (['down'], 2, 20), (['enter'], 2, 60), (['esc'], 2, 60),
        (['esc'], 2, 90),                                                 # the save gump, then resume
        ([help_key], 2, 60), (['space'], 2, 40), (['space'], 2, 40), (['esc'], 2, 90),
        # the menu keeps its choice (SAVE): up to LOAD (OPTIONS opens the
        # port's own settings, which the originals have not)
        ([menu], 2, 60), (['up'], 2, 20), (['enter'], 2, 60), (['esc'], 2, 60), (['esc'], 2, 90),
    ]
    return at_start + in_game


def first_map():
    """Map 1: a floor, the arrival egg 30 the new game goes to, and the test
    egg there."""
    return scenes.floor_and_egg(12) + [
        formats.item_record(scenes.X0, scenes.Y0, 0, scenes.TELEPORT_EGG, frame=1, quality=FIRST_EGG)]


def code(table):
    """The egg writes the game's difficulty into the globals: the level the
    dialog chose."""
    lines = ['ENTER 2'] + scenes.call(table, '@gameDifficulty$qv', '', result=0, local_offsets={}) + ['RETURN']
    return ucasm.assemble('\n'.join(lines))


def new_game(record):
    """The new game chosen: its first map loaded (the trace's M line)."""
    return any(line[1] == 'M' and line[2] == str(FIRST_MAP) for line in record.trace)


def scene(game):
    """The game started without -warp: the start menu's buttons, the load
    gump, the difficulty dialog, a new game; then the game's menu, the help
    and the load gump.

    Checks: the intro's movies (none on our CD: each ends at once), the
        start menu (mainmenu.cpp MainMenuGump: its buttons, the load gump
        opened and closed, the save and the resume with nothing at the
        start, the difficulty dialog cancelled and chosen), the new game to
        map 1's egg 30; in the game the menu key (No Regret's F1, No
        Remorse's Escape): the save gump and the resume, the help's pages,
        the menu again with the load gump.
    Drives: no -warp; the keys from frame START (steps); map 1 a floor, its
        arrival egg 30 and the test egg, whose event 7 writes the game's
        difficulty into the globals.
    Tests:
        menus
            The new game chosen: map 1 loaded (the trace's M line).
    Original: Both follow.
    """
    table = scenes.intrinsics_table(game)
    inputs, frame = [], START
    for keys, held, wait in steps(game):
        inputs += press(frame, keys, held)
        frame += held + wait
    return scenes.Scene(name='menus', maps={FIRST_MAP: first_map()},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table)})}, arguments=[],
                        frames=frame + 60, tests=['menus'], ran=lambda record, tests: [new_game(record)], inputs=inputs)


SCENES = {'menus': scene}
