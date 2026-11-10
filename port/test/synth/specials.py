"""The intrinsics that open a screen or leave the map, a scene each: the
keypad, the computer's text, another NPC
controlled, a map jump there and back by teleport eggs and No Remorse's
weasel shop (the credits are made but not a scene: SCENES). The egg's
usecode opens each at once; the recording's keys work it from frame START,
as a player's do, and close it. Each scene's document is its function's
docstring (docs.py)."""

import controls
import formats
import scenes
import ucasm
import world

LOCALS = {'S': -2, 'N': -4, 'T': -6}
FRAME = 6
START = 150
# A second map: the map jump's target.
OTHER_MAP = 101
# The teleport eggs (family 8): frame 1 an arrival, its low quality its
# number; frame 0 a departure, its map the target map, its low quality the
# arrival's number (egg.cpp Teleporter).
TELEPORT_EGG = scenes.TELEPORT_EGG


def keys(frame, steps):
    """inputs.tsv's lines for steps of (test, keys, held, wait[,
    footprint]), each step's window (scenes.windows_ran: its frames and its
    footprint, None for a key a screen takes, which no record keeps) and
    the frame after."""
    lines, windows = [], []
    for _, names, held, wait, *kind in steps:
        lines += press(frame, names, held)
        windows.append((frame, frame + held + wait, kind[0] if kind else None))
        frame += held + wait
    return lines, windows, frame


def other_acted(record, first, last):
    """The NPC controlled in the Silencer's place animates (the state log
    keeps the Silencer's place, not his)."""
    return bool(record.starters('AnimPrimitive', first, last) - {1})


SCAN = dict(controls.SCAN, **{str(d): 0x02 + (d - 1) % 10 for d in range(10)}, y=0x15, n=0x31, tab=0x0f)
SCAN['0'] = 0x0b


def press(frame, names, held):
    lines = []
    for verb, at in (('key', frame), ('release', frame + held)):
        for key in names if verb == 'key' else reversed(names):
            scan = SCAN[key]
            prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
            lines += prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}']
    return lines


def scene(name, game, lines, steps, maps=None):
    """A scene of the egg's lines and the steps' keys; None when the game
    has not one of the intrinsics it calls."""
    table = scenes.intrinsics_table(game)
    try:
        lines(table)
    except KeyError:
        return None
    code = ucasm.assemble('\n'.join([f'ENTER {FRAME}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}'] +
                                    controls.give(table, game, 'T', LOCALS) + lines(table) + ['RETURN']))
    inputs, windows, end = keys(START, steps)
    return scenes.Scene(name=name, maps=maps or {scenes.TEST_MAP: scenes.floor_and_egg(12)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code})}, arguments=scenes.START,
                        frames=end + 120, tests=[test for test, *_ in steps],
                        ran=scenes.windows_ran(windows), inputs=inputs)


def call(table, name, arguments):
    return scenes.call(table, name, arguments, local_offsets=LOCALS)


def keypad(game):
    """The keypad: the right code typed and entered, then a wrong digit and
    Escape.

    Checks: the keypad gump (keypad.cpp KeypadGump: its digits, Enter and
        Escape) and its result to the usecode (showKeypad's return).
    Drives: the test egg's event 7 opens it with the code 1234, and after
        400 passes again with 42 (each followed by its marker); the keys
        from frame START.
    Tests:
        code: <digit>
            A digit typed. No record shows it.
        code: Enter, the right code
            Enter: the keypad closed (the first marker in the trace).
        second code: 9
            A wrong digit typed. No record shows it.
        second code: Escape
            Escape: the keypad closed (the second marker).
    Original: Both follow.
    """
    def lines(table):
        return (call(table, '@showKeypad$qi', '1234') + scenes.marker(table, 0) + ['SUSPEND'] * 400 +
                call(table, '@showKeypad$qi', '42') + scenes.marker(table, 1))
    steps = [('code: 1', ['1'], 2, 8), ('code: 2', ['2'], 2, 8), ('code: 3', ['3'], 2, 8), ('code: 4', ['4'], 2, 8),
             ('code: Enter, the right code', ['enter'], 2, 420, scenes.reached(game, 0)),
             ('second code: 9', ['9'], 2, 8), ('second code: Escape', ['esc'], 2, 60, scenes.reached(game, 1))]
    return scene('keypad', game, lines, steps)


def computer(game):
    """The computer's text: rows, pauses and three pages, typed and turned
    by the keys, then closed.

    Checks: the computer's text (showComputerText; compgump.cpp
        ComputerGump: its rows, pauses and pages): while a page is typed
        Enter does nothing and Escape types the rest of it at once; a full
        page goes to the next by any key; the text typed, Enter closes it.
    Drives: the test egg's event 7 opens it with a text of three pages and
        waits for it (its marker after); the keys from frame START.
    Tests:
        page 1: Enter while typed
            Nothing. No record shows it.
        page <n>: Escape types the rest
            The page typed at once. No record shows it.
        page <n> full: Space, the next page
            The next page. No record shows it.
        the text done: Space does nothing
            Space once the text is typed: not a key that closes it. No
            record shows it.
        the text done: Enter closes it
            The text closed (the marker in the trace).
    Original: Both follow.
    Guards: the keys that close it come once the text is typed: while it
        is typed they do not close it.
    """
    text = ('^LOG ENTRY 1^^THE SYNTHETIC WORLD*^IS A TEST OF THE ENGINE.^' + 'LINE OF TEXT ' * 30 +
            '*^PAGE TWO^THE END.')
    def lines(table):
        return [f'PUSH_STRING "{text}"', 'STRING_TO_POINTER'] + [
            f'CALL_INTRINSIC 4 {table["@showComputerText$qnzc"][0]}', 'MOVE_TOP -4'] + scenes.marker(table, 0)
    steps = [('page 1: Enter while typed', ['enter'], 2, 30), ('page 1: Escape types the rest', ['esc'], 2, 30),
             ('page 1 full: Space, the next page', ['space'], 2, 30), ('page 2: Escape types the rest', ['esc'], 2, 30),
             ('page 2 full: Space, the next page', ['space'], 2, 30), ('page 3: Escape types the rest', ['esc'], 2, 30),
             ('the text done: Space does nothing', ['space'], 2, 30),
             ('the text done: Enter closes it', ['enter'], 2, 60, scenes.reached(game, 0))]
    return scene('computer', game, lines, steps)


def remote(game):
    """Another NPC controlled: walked, turned and fired by the keys, then
    the control given back with Escape, and the Silencer walks.

    Checks: setControlledNpcNumber and the controls of an NPC not the
        Silencer (control.cpp, Npc::isControlled), RETURN_CONTROL (Escape:
        setControlledNpc(1)), the eggs waking for the NPC controlled.
    Drives: the test egg's event 7 makes an actor east of the Silencer,
        gives it the control and moves the egg out of his way; the keys
        from frame START.
    Tests:
        the NPC walks
            The up arrow: the NPC animates (the state log keeps the
            Silencer's place, not its: its animation's process).
        the NPC turns
            The left arrow: it animates.
        the NPC fires
            Space: it animates.
        the NPC steps back
            The down arrow: it animates.
        Escape: the control back
            Escape gives the control back. No record shows it.
        the Silencer walks
            The up arrow: he moves (moved).
    Original: Both follow.
    Guards: the egg goes out of his way first: given back the control in
        its range, he would wake it again, its entry giving the control to
        another NPC (and the last walk would be that one's).
    """
    def lines(table):
        return (call(table, '@Npc@createActor$qusus', f'N {scenes.ACTOR} 0') +
                call(table, '@Item@popToCoords$qususuc', f'N {scenes.X0 + 120} {scenes.Y0} 0') +
                call(table, '@setControlledNpcNumber$qus', 'n') +
                call(table, '@Item@move$qususuc', f'E {scenes.X0} {scenes.Y0 + 300} 0'))
    steps = [('the NPC walks', ['up'], 30, 8, other_acted), ('the NPC turns', ['left'], 10, 8, other_acted),
             ('the NPC fires', ['space'], 20, 8, other_acted), ('the NPC steps back', ['down'], 20, 8, other_acted),
             ('Escape: the control back', ['esc'], 2, 60), ('the Silencer walks', ['up'], 20, 8, 'moved')]
    return scene('remote', game, lines, steps)


def mapjump(game):
    """A teleport to an egg of this map, a map jump to a map of its own,
    and a walk into its departure egg back to the arrival here.

    Checks: teleportToEgg on this map, startMapJump to another map (the
        maps' load, itemcach.cpp), the teleport eggs (egg.cpp Teleporter: an
        arrival by its number, a departure to a map's arrival), the walks
        after a map's load.
    Drives: the test egg's event 7: the teleport to this map's arrival
        egg 31, 60 passes, the jump to map 101; map 101 a floor with its
        arrival egg 30 and a departure egg west of it to this map's 31;
        the keys from frame START.
    Tests:
        there: west onto the departure, back here
            The keypad's 7 on map 101: he walks west onto the departure
            and is back on this map (moved).
        here: <direction>
            The keypad's 3, 9 and 1 back here: he walks (moved).
    Original: Both follow.
    Guards: this map's arrival is off the test egg's row: back on it, he
        walks on while the key is held (on its row he would walk into
        the egg's range, which runs the egg again: a second teleport and
        jump).
    """
    here = scenes.floor_and_egg(12) + [
        formats.item_record(scenes.X0 + 200, scenes.Y0 - 200, 0, TELEPORT_EGG, frame=1, quality=31)]
    there = [formats.item_record(scenes.X0 - 256 + 64 * i + 63, scenes.Y0 - 256 + 64 * j + 63, 0, scenes.FLOOR)
             for i in range(8) for j in range(8)]
    there += [formats.item_record(scenes.X0, scenes.Y0, 0, TELEPORT_EGG, frame=1, quality=30),
              formats.item_record(scenes.X0 - 96, scenes.Y0, 0, TELEPORT_EGG, frame=0, quality=31, npc=0x11,
                                  map=scenes.TEST_MAP)]

    def lines(table):
        return (call(table, '@teleportToEgg$qususuc', f'{scenes.TEST_MAP} 31 1') + ['SUSPEND'] * 60 +
                call(table, '@startMapJump$qus', f'{OTHER_MAP}'))
    # the arrival there faces some way: walk the four ways onto the departure
    steps = [('there: west onto the departure, back here', ['kp7'], 40, 20, 'moved'),
             ('here: east', ['kp3'], 40, 20, 'moved'), ('here: north', ['kp9'], 40, 20, 'moved'),
             ('here: south', ['kp1'], 40, 60, 'moved')]
    return scene('mapjump', game, lines, steps, maps={scenes.TEST_MAP: here, OTHER_MAP: there})


def credits(game):
    """The credits (playCredits) to their end, then again cut by a key."""
    def lines(table):
        return call(table, '@playCredits$qv', '') + ['SUSPEND'] * 30 + call(table, '@playCredits$qv', '')
    steps = [('to the end', [], 0, 600), ('cut by Escape', ['esc'], 2, 120)]
    return scene('credits', game, lines, steps)


def shop(game):
    """No Remorse's weasel shop of level 1 with no credits: browsed, every
    ware refused, left and back, then left.

    Checks: the shop (showWeaselShop; wezzy.cpp WeaselGump: browsing, buy
        refused for want of credits, the keys it ignores while browsing,
        Escape's leaving and "anything else?", its answers, the close).
    Drives: the test egg's event 7 opens the shop at level 1 (its marker
        after); the Silencer has no credits; the keys from frame START.
    Tests:
        buy: refused, no credits
            Enter on the first ware: refused. No record shows it.
        down: not a shop key
            Nothing. No record shows it.
        next ware
            The right arrow. No record shows it.
        buy the next: refused
            Enter: refused. No record shows it.
        Y while browsing: nothing
            No record shows it.
        down again: nothing
            No record shows it.
        buy again: refused
            No record shows it.
        N while browsing: nothing
            No record shows it.
        leave: anything else?
            Escape while browsing: the shop asks. No record shows it.
        Y: browsing again
            No record shows it.
        leave again
            Escape: it asks again. No record shows it.
        Escape: no, the shop closes
            Escape answers no: the shop closed (its marker in the trace).
    Original: Both follow.
    Guards: the last Escape closes the shop, which else stays open to the
        scene's end.
    """
    def lines(table):
        return call(table, '@showWeaselShop$qus', '1') + scenes.marker(table, 0)
    steps = [('buy: refused, no credits', ['enter'], 2, 30), ('down: not a shop key', ['down'], 2, 10),
             ('next ware', ['right'], 2, 10), ('buy the next: refused', ['enter'], 2, 30),
             ('Y while browsing: nothing', ['y'], 2, 30), ('down again: nothing', ['down'], 2, 10),
             ('buy again: refused', ['enter'], 2, 30), ('N while browsing: nothing', ['n'], 2, 30),
             ('leave: anything else?', ['esc'], 2, 30), ('Y: browsing again', ['y'], 2, 60),
             ('leave again', ['esc'], 2, 120), ('Escape: no, the shop closes', ['esc'], 2, 60, scenes.reached(game, 0))]
    return scene('shop', game, lines, steps)


def rich_shop(game):
    """The weasel shop of level 2 with 5000 credits on the Silencer: wares
    bought and refused, the ammunition's catalog and back, the purchase
    confirmed and delivered, more bought and cancelled, then no more.

    Checks: the shop's purchases (wezzy.cpp WeaselGump buy, canBuy: a
        weapon he has refused; deliver), the ammunition's catalog (Tab:
        level 1's wares, and Tab back), the confirmation, "anything
        else?", the cancel (the credits back), the close after a delivery.
    Drives: the test egg's event 7 gives him 5000 credits and opens the
        shop at level 2 (its marker after); the wares (world.stuff): two
        weapons he has (controls.give) and two items; the keys from frame
        START.
    Tests:
        buy a weapon he has: refused
            Enter on the first ware. No record shows it.
        next ware
            The right arrow. No record shows it.
        next ware: an item
            No record shows it.
        buy the item
            Bought (the credits less). No record shows it.
        previous ware: a weapon
            The left arrow. No record shows it.
        buy it: refused
            No record shows it.
        Tab: the ammunition
            The ammunition's catalog. No record shows it.
        ammunition: next
            No record shows it.
        ammunition: next again
            No record shows it.
        ammunition: the last ware, an item
            No record shows it.
        ammunition: buy the item <n>
            Bought, three times. No record shows it.
        Tab: the level's wares
            Back to level 2's. No record shows it.
        leave: confirm?
            Escape with purchases: the shop asks to confirm. No record
            shows it.
        Y: bought and delivered
            The purchases delivered (picked up). No record shows it.
        Y: anything else, browsing
            No record shows it.
        browsing: next ware
            No record shows it.
        browsing: an item
            No record shows it.
        browsing: buy the item
            No record shows it.
        leave again: confirm?
            No record shows it.
        N: cancelled, the credits back
            No record shows it.
        N: nothing else, the shop closes
            The shop closed (its marker in the trace).
    Original: Both follow.
    Guards: the shop opens at level 2: at level 1 Tab does nothing (level
        1's wares are the ammunition's catalog).
    """
    names = world.game_shapes(game)

    # Level 2: Tab shows the ammunition (level 1's wares) and Tab again the
    # level's (wezzy.cpp handleKeyboard; at level 1 Tab does nothing). The
    # last N closes the shop.
    def lines(table):
        return (call(table, '@Item@create$qusus', f'N {names["SHAPE_CREDITS"]} 0') +
                call(table, '@Item@setQuality$qus', 'N 5000') +
                call(table, '@Item@popToCoords$qususuc', f'N {scenes.X0} {scenes.Y0} 0') +
                call(table, '@pickUp$qusuc', 'n 0') + call(table, '@showWeaselShop$qus', '2') +
                scenes.marker(table, 0))
    # The wares (world.stuff): two weapons he has (controls.give: refused)
    # and two items; each catalog starts at the first.
    steps = [('buy a weapon he has: refused', ['enter'], 2, 30), ('next ware', ['right'], 2, 10),
             ('next ware: an item', ['right'], 2, 10), ('buy the item', ['enter'], 2, 20),
             ('previous ware: a weapon', ['left'], 2, 10), ('buy it: refused', ['enter'], 2, 20),
             ('Tab: the ammunition', ['tab'], 2, 20), ('ammunition: next', ['right'], 2, 10),
             ('ammunition: next again', ['right'], 2, 10), ('ammunition: the last ware, an item', ['right'], 2, 10),
             ('ammunition: buy the item 1', ['enter'], 2, 10), ('ammunition: buy the item 2', ['enter'], 2, 10),
             ('ammunition: buy the item 3', ['enter'], 2, 20), ("Tab: the level's wares", ['tab'], 2, 20),
             ('leave: confirm?', ['esc'], 2, 40), ('Y: bought and delivered', ['y'], 2, 60),
             ('Y: anything else, browsing', ['y'], 2, 40), ('browsing: next ware', ['right'], 2, 10),
             ('browsing: an item', ['right'], 2, 10), ('browsing: buy the item', ['enter'], 2, 20),
             ('leave again: confirm?', ['esc'], 2, 40), ('N: cancelled, the credits back', ['n'], 2, 60),
             ('N: nothing else, the shop closes', ['n'], 2, 120, scenes.reached(game, 0))]
    return scene('shop-rich', game, lines, steps)


# Not credits: the real data never runs them (they end the game), the
# port's credits pass frames inside a step (each turn's retrace wait) and
# the oracle cannot follow the original's polling of the retrace.
SCENES = {'keypad': keypad, 'computer': computer, 'remote': remote, 'mapjump': mapjump, 'shop': shop,
          'shop-rich': rich_shop}
