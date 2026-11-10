"""The cheats: the codes typed as a player
types them (control.cpp typeCheat: No Remorse's JASSICA16; No Regret's
LOOSECANNON16, PIX twice and MEMORY twice, each closing its box with
Enter), then F10 and its Ctrl and Alt forms (control.cpp cheatKey: the
gifts, immortality on and off), and, after the egg's usecode has killed the
Silencer, F10 bringing him back."""
import controls
import scenes
import ucasm

LOCALS = {'S': -2, 'W': -4}
SCAN = dict(controls.SCAN, a=0x1e, c=0x2e, n=0x31, p=0x19, x=0x2d, y=0x15, **{'1': 0x02, '6': 0x07}, f10=0x44,
            f1=0x3b, f3=0x3d, f4=0x3e, f5=0x3f, f7=0x41, f8=0x42, f9=0x43, t=0x14, f=0x21, h=0x23, grave=0x29,
            slash=0x35)
START = 150
# The passes the egg waits before it kills the Silencer, after the codes.
KILL = 520


def press(frame, keys, held):
    lines = []
    for verb, at in (('key', frame), ('release', frame + held)):
        for key in keys if verb == 'key' else reversed(keys):
            scan = SCAN[key]
            prefix = [f'{at}\tkey\te0'] if scan >> 8 == 0xe0 else []
            lines += prefix + [f'{at}\t{verb}\t{scan & 0xff:02x}']
    return lines


def typed(word):
    return [([letter], 2, 6) for letter in word[:-1]] + [([word[-1]], 2, 60), (['enter'], 2, 40)]


def steps(game):
    if game == 'regret':
        codes = typed('loosecannon16') + typed('pix') + typed('pix') + typed('memory') + typed('memory')
    else:
        codes = typed('jassica16')
    gifts = [(['f10'], 2, 60), (['enter'], 2, 30), (['ctrl', 'f10'], 2, 60), (['enter'], 2, 60),
             (['alt', 'f10'], 2, 60), (['enter'], 2, 30), (['ctrl', 'f10'], 2, 60), (['enter'], 2, 60)]
    return codes, gifts


def code(table):
    """The egg waits, then hits the Silencer: the cheat's gifts (F10: a
    graviton shield, a fusion battery, the energy filled) stop the first
    hit with his energy (crusader.cpp silencerShieldTakes); with his energy
    taken the second kills him."""
    w = LOCALS['W']
    lines = ['ENTER 4', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}', f'PUSH_WORD {KILL}', f'POP_LOCAL_WORD {w}',
             'waiting:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1', 'SUB', f'POP_LOCAL_WORD {w}',
             f'PUSH_LOCAL_WORD {w}', 'JUMP_IF_FALSE waited', 'JUMP waiting', 'waited:']
    lines += scenes.call(table, '@Npc@clearImmortal$qv', 'S', local_offsets=LOCALS)
    lines += scenes.call(table, '@receiveHitIntrinsic$qn4Itemusucius', 'S 0 4 300 1', local_offsets=LOCALS)
    lines += ['SUSPEND'] * 30 + scenes.call(table, '@Npc@setMana$qus', 'S 0', local_offsets=LOCALS)
    lines += scenes.call(table, '@receiveHitIntrinsic$qn4Itemusucius', 'S 0 4 300 1', local_offsets=LOCALS)
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def revived(record):
    """The Silencer dead (no hit points left), then alive again."""
    hits = [int(state[9]) for state in record.states if state[4] == '1']
    died = [k for k in range(1, len(hits)) if hits[k] == 0 and hits[k - 1] > 0]
    return bool(died) and any(hits[died[0]:])


def scene(game):
    """The cheats' codes typed, F10's gifts and immortality, and F10 bringing
    the Silencer back after the egg has killed him.

    Checks: the codes typed as a player types them (control.cpp typeCheat:
        No Remorse's JASSICA16; No Regret's LOOSECANNON16, PIX twice and
        MEMORY twice, each box closed by Enter), F10 and its Ctrl and Alt
        forms (cheatKey: the gifts, immortality on and off), the gifts'
        shield taking a hit (crusader.cpp silencerShieldTakes), his death,
        and F10 after it.
    Drives: the keys from frame START (steps); the test egg's event 7
        (code) waits, then hits him: the gifts' shield stops the first hit,
        and with his energy taken the second kills him; then F10 twice.
    Tests:
        codes
            The codes typed. No record shows them.
        gifts
            F10 (the gifts), Ctrl+F10 (immortality, its box), Alt+F10 and
            Ctrl+F10 again, each with an Enter (the boxes' close). No record
            shows them.
        revived
            His hit points 0, then more again: F10 brought him back.
    Original: Both follow.
    """
    table = scenes.intrinsics_table(game)
    codes, gifts = steps(game)
    inputs, frame = [], START
    for keys, held, wait in codes + gifts:
        inputs += press(frame, keys, held)
        frame += held + wait
    # after the death: F10 brings him back, then the gifts once more
    frame = max(frame, START + KILL + 200)
    for keys, held, wait in [(['f10'], 2, 60), (['enter'], 2, 30), (['f10'], 2, 60), (['enter'], 2, 60)]:
        inputs += press(frame, keys, held)
        frame += held + wait
    return scenes.Scene(name='cheats', maps={scenes.TEST_MAP: scenes.floor_and_egg(12)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table)})},
                        arguments=scenes.START, frames=frame + 60, tests=['codes', 'gifts', 'revived'],
                        ran=lambda record, tests: [None, None, revived(record)], inputs=inputs)


# The world screen's keys (world.cpp WorldGump::keyboardRest), with the
# cheats on: the selection ended by Escape and by an arrow; the cheats' keys
# (the eggs, the snap eggs and the chunks shown and hidden, the item boxes,
# No Remorse's ~, Ctrl+Q and T, the hack mover), each box closed by Enter;
# the help (No Regret's F3, No Remorse's F1), the load and save screens
# left by Escape, the quick load and save asked and refused (N); the next
# song once; the version; the weapon dropped (No Regret's Ctrl+L, No
# Remorse's Ctrl+D); No Remorse's targeting reticle (R) and position
# (Ctrl+L). In world-options, without the original: the options (F2, the
# port's settings screen) and the next song round all the songs (both games
# play the songs as No Regret, so No Remorse's original picks others from
# the second on).
def world_keys(game):
    """The world screen's keys with the cheats on: the selection, the
    cheats' keys and boxes, the help, the load and save screens, the quick
    load and save refused, the songs, the version, the dropped weapon.

    Checks: WorldGump::keyboardRest and cheatKey (world.cpp): the selection
        ended by Escape and by an arrow; the eggs, the snap eggs and the
        chunks shown and hidden (Ctrl+F7, Alt+F7, F7), the item boxes (F),
        No Remorse's ~ (cheats off and on), Ctrl+Q (the CD's transfer
        display) and T, the hack mover (H), each box closed by Enter; the
        help (No Regret's F3, No Remorse's F1), the load and save screens
        left by Escape, the quick load and save refused (N; No Regret's F4
        asks nothing without a quick save), the next song, the version,
        the weapon dropped (No Regret's Ctrl+L, No Remorse's Ctrl+D); No
        Remorse's targeting reticle (R) and position (Ctrl+L).
    Drives: the cheats scene's codes and gifts, then the keys (keys); the
        egg does nothing.
    Tests:
        codes
            The codes typed. No record shows them.
        gifts
            F10 and its forms. No record shows them.
        keys
            The world's keys after them. No record shows them.
    Original: Both follow.
    Guards: No Remorse's Escape after the selection is the menu's key: it
        would open the menu, the cheats' keys go into it and its Enter
        begin a new game.
    """
    if game == 'regret':
        help_key, drop, own = ['f3'], ['ctrl', 'l'], []
    else:
        help_key, drop, own = ['f1'], ['ctrl', 'd'], [(['r'], 2, 30), (['enter'], 2, 20), (['r'], 2, 30),
                                                      (['enter'], 2, 20), (['ctrl', 'l'], 2, 30), (['enter'], 2, 20)]
    # (No Remorse's Escape after the selection is the menu's key: it would
    # open the menu, the cheats' keys go into it and its Enter begin a new
    # game)
    after_selection = [(['esc'], 2, 20)] if game == 'regret' else []
    keys = [(['s'], 2, 20), (['esc'], 2, 20), (['s'], 2, 20), (['right'], 2, 20), (['g'], 2, 20),
            (['period'], 2, 10)] + after_selection + [
            (['ctrl', 'f7'], 2, 20), (['ctrl', 'f7'], 2, 20), (['alt', 'f7'], 2, 20), (['alt', 'f7'], 2, 20),
            (['f7'], 2, 20), (['f7'], 2, 20), (['f'], 2, 20), (['f'], 2, 20), (['shift', 'grave'], 2, 30),
            (['enter'], 2, 20), (['shift', 'grave'], 2, 30), (['enter'], 2, 20), (['ctrl', 'q'], 2, 30),
            (['enter'], 2, 20), (['ctrl', 'q'], 2, 30), (['enter'], 2, 20), (['t'], 2, 30), (['h'], 2, 30),
            (['enter'], 2, 20), (['h'], 2, 30), (['enter'], 2, 20),
            (help_key, 2, 40), (['esc'], 2, 40), (['f8'], 2, 60), (['esc'], 2, 60), (['f9'], 2, 60),
            (['esc'], 2, 60), (['f5'], 2, 40), (['n'], 2, 40), (['f4'], 2, 40), (['n'], 2, 40),
            (['ctrl', 'v'], 2, 30), (['enter'], 2, 20)]
    keys += [(['ctrl', 'm'], 2, 30), (drop, 2, 30)] + own
    table = scenes.intrinsics_table(game)
    codes, gifts = steps(game)
    inputs, frame = [], START
    for keys_held, held, wait in codes + gifts + keys:
        inputs += press(frame, keys_held, held)
        frame += held + wait
    return scenes.Scene(name='world-keys', maps={scenes.TEST_MAP: scenes.floor_and_egg(12)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: idle_code()})},
                        arguments=scenes.START, frames=frame + 60, tests=['codes', 'gifts', 'keys'],
                        ran=lambda record, tests: [None] * len(tests), inputs=inputs)


def world_options(game):
    """The options key in the world, the port's settings screen left by
    Escape, then the next song round all the songs.

    Checks: F2 in the world (the port's settings screen, settings.cpp: it
        takes the keys while it is up) and the songs' order (music.cpp: both
        games play the songs as No Regret).
    Drives: the controls scene's data; the keys (controls.of_actions): F2,
        the up arrow held under the screen, Escape, the up arrow held in
        the world, then Ctrl+M and a while, 24 times.
    Tests:
        options
            F2: the settings screen. No record shows it.
        up under the screen
            The up arrow while the screen is up: the screen takes it, the
            Silencer stays (not moved).
        leave
            Escape: the screen closed. No record shows it.
        walk after
            The up arrow again: back in the world, he walks (moved).
        next song <n>
            Ctrl+M: the next song. No record shows it.
        song <n> playing
            The song a while. No record shows it.
    Original: None: the original's options are its own, and No Remorse's
        original picks other songs from the second on; no original's like.
    Traps: the song playing is music.cpp's currentSong, which the saves keep
        but neither the state lines nor the trace: the songs' order is the
        kept results' only in what it changes of the game (nothing), so a
        change of it passes the port's own test.
    """
    import dataclasses
    controls.SCAN.setdefault('f2', 0x3c)
    songs = [action for k in range(1, 25) for action in ((f'next song {k}', ['ctrl', 'm'], 2), (f'song {k} playing', [], 20))]
    stays = lambda record, first, last: not record.moved(first, last)
    base = controls.of_actions(game, [('options', ['f2'], 2), ('up under the screen', ['up'], 30, stays),
                                      ('leave', ['esc'], 2), ('walk after', ['up'], 30)] + songs)
    return dataclasses.replace(base, name='world-options', oracle=False)


def idle_code():
    """The egg does nothing."""
    return ucasm.assemble('ENTER 0\nRETURN')


SCENES = {'cheats': scene, 'world-keys': world_keys, 'world-options': world_options}
