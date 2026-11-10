"""NPCs: the test egg makes the
Silencer immortal and armed, then twelve NPCs around him, most as the
games' usecode makes them (Item::createNpc from an item with the place, the
activities and the arming, and one with the type), the others by
Npc::createActor, each with an activity (standing, loitering, pacing, the
attacks with and without a combat tactic, surrender, guarding, rolling
thunder); waypoints for the tactics, barrels and boxes to break. The
recording turns the Silencer and fires around; the egg sounds the alert
after a while and silences it later."""
import re

import formats
import scenes
import ucasm
import world
import controls

# (not E: scenes.argument_code's E is the event's item)
LOCALS = {'S': -2, 'D': -4, 'Y': -6, 'N': -8, 'W': -10, 'T': -12} | {f'N{k}': -14 - 2 * k for k in range(12)}
FRAME = 36
# Each NPC: the activity, the combat tactic, how it is made, its angle (in
# eighths of a turn) and distance, armed.
NPCS = [(1, 0, 'egg', 0, 300, 0), (2, 0, 'egg', 1, 320, 0), (3, 0, 'egg', 2, 300, 0), (5, 0, 'egg', 3, 340, 1),
        (5, 1, 'egg', 4, 360, 0), (5, 2, 'egg', 5, 300, 0), (7, 0, 'actor', 6, 280, 0), (8, 0, 'egg', 7, 320, 0),
        (9, 1, 'egg', 0, 450, 1), (0xa, 0, 'actor', 2, 460, 0), (0xb, 0, 'egg', 4, 440, 0), (0xd, 0, 'egg', 6, 450, 0)]
DIRECTIONS = [(0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1)]
# The activities' names (common/src/item/npc.h), for the tests.
ACTIVITIES = {1: 'stand', 2: 'loiter', 3: 'pace', 5: 'attack', 7: 'surrender', 8: 'guard', 9: 'kneel and attack',
              0xa: 'aggressive attack', 0xb: 'aggressive kneel and attack', 0xd: 'rolling thunder'}


def npc_name(activity, tactic):
    """A test's name: the NPC's activity and its combat tactic."""
    return ACTIVITIES[activity] + (f', tactic {tactic}' if tactic else '')


def at(angle, distance):
    dx, dy = DIRECTIONS[angle]
    scale = distance if dx == 0 or dy == 0 else distance * 7 // 10
    return scenes.X0 + dx * scale, scenes.Y0 + dy * scale


WAITS = [0]


def counted_wait(passes):
    """The egg's process waits passes passes (a SUSPEND each)."""
    w = LOCALS['W']
    WAITS[0] += 1
    top, end = f'waiting{WAITS[0]}', f'waited{WAITS[0]}'
    return [f'PUSH_WORD {passes}', f'POP_LOCAL_WORD {w}', f'{top}:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1',
            'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', f'JUMP_IF_FALSE {end}', f'JUMP {top}', f'{end}:']


def weapon_fire_types(game):
    """The fire types of the game's weapons (<game>/src/game/dtable_game.cpp
    weaponTypes, each row's last field), the ones fireWeapon shoots in the
    game: others are the usecode's missiles (fire type 9's shot, a sprite
    without its item, halts at its first frame) or bombs."""
    text = (scenes.PORT / game / 'src/game/dtable_game.cpp').read_text()
    body = re.search(r'weaponTypes\)\[\w+\] =\s*gameGlobal<[^>]+>\(\{(.*?)\}\);', text, re.S).group(1)
    rows = re.findall(r'\{(SHAPE_\w+)[^}]*?,\s*(0x[0-9a-fA-F]+|\d+)\}', body)
    return sorted({int(kind, 0) for _, kind in rows})


# The shields by their types (pickUp's): ionic 1, plasma 2, graviton 3.
SHIELDS = {1: 'SHAPE_IONIC_SHIELD', 2: 'SHAPE_PLASMA_SHIELD', 3: 'SHAPE_GRAVITON_SHIELD'}


def driver(game, fights=True, shield=0):
    """Event 7: the Silencer immortal and armed (with shield, that shield, a
    battery and energy: hits on him then spend it, crusader.cpp takeHit),
    the NPCs; with fights the shots, the alert and the deaths after."""
    table = scenes.intrinsics_table(game)
    call = lambda name, arguments: scenes.call(table, name, arguments, local_offsets=LOCALS)
    lines = [f'ENTER {FRAME}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}'] + call('@Npc@setImmortal$qv', 'S')
    lines += controls.give(table, game, 'N', LOCALS)
    if shield:
        names = world.game_shapes(game)
        for name in ('SHAPE_FUSION_BATTERY', SHIELDS[shield]):
            lines += call('@Item@create$qusus', f'N {names[name]} 0')
            lines += call('@Item@popToCoords$qususuc', f'N {scenes.X0} {scenes.Y0} 0') + call('@pickUp$qusuc', 'n 0')
        lines += call('@Npc@setMana$qus', 'S 9000')
    for k, (activity, tactic, made, angle, distance, armed) in enumerate(NPCS):
        x, y = at(angle, distance)
        npc = f'N{k}'
        if made == 'egg':
            # The place, the second activity, the waypoints' group and the
            # arming (the map's bits: difficulty 1, armed 4) on one item, the
            # type, the first and third activities on the other.
            lines += call('@Item@create$qusus', f'D {scenes.ITEM} 0')
            lines += call('@Item@setQualityLow$qus', 'D 1') + call('@Item@setQualityHigh$qus', 'D 2')
            lines += call('@Item@setMap$qi', f'D {4 * armed}') + call('@Item@setNpcNumber$quc', f'D {angle}')
            lines += call('@Item@popToCoords$qususuc', f'D {x} {y} 0')
            lines += call('@Item@create$qusus', f'Y {scenes.ITEM} 0')
            lines += call('@Item@setNpcNumber$quc', 'Y 1') + call('@Item@setQualityHigh$qus', 'Y 1')
            lines += call('@Item@setMap$qi', 'Y 3')
            lines += call('@Item@createNpc$qus', 'D y') + ['PUSH_RETURNED_WORD', f'POP_LOCAL_WORD {LOCALS[npc]}']
        else:
            lines += call('@Npc@createActor$qusus', f'{npc} {scenes.ACTOR} 0')
            lines += call('@Item@popToCoords$qususuc', f'{npc} {x} {y} 0')
        lines += call('@Npc@setCombatTactic$qus', f'{npc} {tactic}') + call('@Npc@setActivity$qus', f'{npc} {activity}')
        lines += scenes.marker(table, k)
    if not fights:
        return '\n'.join(lines + ['RETURN'])
    # The fights go on a while, then each fire type once at an NPC from the
    # Silencer's place, toward it.
    lines += counted_wait(300)
    for kind in weapon_fire_types(game):
        target = kind % len(NPCS)
        npc = f'N{target}'
        lines += call('@Item@directionToItem$qus', f'S {npc.lower()}') + ['PUSH_RETURNED_BYTE', f'POP_LOCAL_WORD {LOCALS["T"]}']
        # x, y and z from the shooter's place (crusader.cpp fireWeapon), out
        # of his box toward the target, as an attack frame's point is (a
        # missile's item cannot be made inside him)
        dx, dy = DIRECTIONS[NPCS[target][3]]
        lines += call('@fireWeapon$qn4Itemiiizciuc', f'S {48 * dx} {48 * dy} 16 t {kind} 0') + counted_wait(25)
    lines += call('@setAlertActive$qv', '') + counted_wait(200) + call('@clearAlertActive$qv', '')
    # Hits that kill half the NPCs, then the Silencer.
    for k in range(0, len(NPCS), 2):
        lines += call('@receiveHitIntrinsic$qn4Itemusucius', f'N{k} 1 0 200 1') + counted_wait(10)
    lines += counted_wait(120) + call('@Npc@clearImmortal$qv', 'S')
    if not shield:
        # (a hit without a point on a shielded Silencer: No Remorse's original
        # sparks at the null pointer, the port at him)
        lines += call('@receiveHitIntrinsic$qn4Itemusucius', 'S 0 4 300 1')
    lines += ['RETURN']
    return '\n'.join(lines)


def floor_and_things():
    """A floor of 24 by 24 tiles, the egg, four waypoints of group 1, two
    barrels and two boxes."""
    records = scenes.floor_and_egg(24)
    for frame, (dx, dy) in enumerate(((-400, -400), (400, -400), (400, 400), (-400, 400))):
        records.append(formats.item_record(scenes.X0 + dx, scenes.Y0 + dy, 0, scenes.WAYPOINT, frame=frame, quality=1))
    for dx, dy, shape in ((150, 60, scenes.BARREL), (-150, 60, scenes.BARREL), (60, 150, scenes.CONTAINER),
                          (60, -150, scenes.CONTAINER)):
        records.append(formats.item_record(scenes.X0 + dx, scenes.Y0 + dy, 0, shape))
    return records


# The recording: the first weapon, then turns of an eighth with bursts of
# fire, a walk, and more.
START = 200


def inputs():
    lines, frame = [], START
    steps = [(['1'], 4)] + [step for _ in range(16) for step in ((['space'], 40), (['right'], 6))]
    steps += [(['up'], 60), (['shift', 'up'], 40)] + [step for _ in range(8) for step in ((['space'], 30),
                                                                                         (['left'], 6))]
    for keys, frames in steps:
        lines += controls.press(frame, keys, frames)
        frame += frames + 8
    return lines, frame


def scene(game):
    """Twelve NPCs about the immortal Silencer, each with an activity, while
    he turns and fires; each fire type shot at one, the alert, then hits
    that kill half of them and him.

    Checks: the NPCs made as the games' usecode makes them (Item::createNpc
        from an item with the place, the activities and the arming, and one
        with the type; Npc::createActor), their activities and combat
        tactics (activity.cpp, attack.cpp, tactics.py's scripts), the
        shots of each fire type (crusader.cpp fireWeapon), the alert,
        deaths, and the Silencer's own fire and death.
    Drives: the test egg's event 7 (driver): the Silencer immortal and armed
        (controls.give), the NPCs (NPCS: activity, tactic, how it is made,
        its place, armed), after 300 passes each of the game's weapons'
        fire types at an NPC, the alert set for 200 passes, hits of 200 on
        every second NPC, then the Silencer mortal and hit; the inputs
        (inputs): the first weapon, turns of an eighth with bursts of fire,
        a walk, a run, more turns and fire. A floor of 24 by 24 tiles, four
        waypoints of group 1, two barrels and two boxes.
    Tests:
        <activity>
            The NPC with that activity (and tactic) made: its marker after
            it in the trace. What it does after is in the records.
    Original: Both follow.
    Guards: a hit without a point on a shielded Silencer (No Remorse's original
        sparks at the null pointer) is left to the scenes without a shield.
    """
    lines, end = inputs()
    code = ucasm.assemble(driver(game))
    names = [npc_name(activity, tactic) for activity, tactic, *_ in NPCS]
    return scenes.Scene(name='npcs', maps={scenes.TEST_MAP: floor_and_things()},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code})}, arguments=scenes.START,
                        frames=max(end, 2200) + 60, tests=names, ran=scenes.marked(game), inputs=lines)


def saved(game):
    """The NPCs' scene without keys, quick saved while they fight and
    loaded later.

    Variant of: npcs
    Checks: the save and the load of what is alive while NPCs fight: their
        activities, animations and shots (savegame.cpp, each process's
        save fields, savefields).
    Drives: the npcs scene's data without its keys; a quick save at frame
        200 and a quick load at 700 (saves.quick: F5, Y, Enter; F4, Y,
        Enter).
    Original: Both follow.
    """
    import dataclasses
    import saves
    base = scene(game)
    inputs, back = saves.quick(200, 700)
    return dataclasses.replace(base, name='npcs-saved', inputs=inputs, frames=base.frames + back + 200)


def at_skill(game, skill):
    """The NPCs' scene at difficulty skill (-skill; the hits' damage scales
    with it), the Silencer with shield skill."""
    import dataclasses
    base = scene(game)
    code = ucasm.assemble(driver(game, shield=skill))
    return dataclasses.replace(base, name=f'npcs-skill{skill}', arguments=scenes.START + ['-skill', str(skill)],
                               classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code})})


def skill1(game):
    """The NPCs' scene at difficulty 1, the Silencer with the ionic shield.

    Variant of: npcs
    Checks: the difficulty's scale of the hits' damage (-skill 1), and the
        hits on a shielded Silencer spending his energy (crusader.cpp
        takeHit, silencerShieldTakes).
    Drives: the npcs scene, with -skill 1; the egg gives him a fusion
        battery, the ionic shield and 9000 energy.
    Original: Both follow.
    """
    return at_skill(game, 1)


def skill2(game):
    """The NPCs' scene at difficulty 2, the Silencer with the plasma shield.

    Variant of: npcs
    Checks: as npcs-skill1, at difficulty 2 with the plasma shield.
    Drives: the npcs scene, with -skill 2; the egg gives him a fusion
        battery, the plasma shield and 9000 energy.
    Original: Both follow, No Remorse's to pass 924: at pass 925 an NPC
        finds him in its shot's place, which the port takes as a hit and the
        original as the distance to a point it never set (point-blank).
    """
    import dataclasses
    return dataclasses.replace(at_skill(game, 2), followed_to=924 if game == 'remorse' else 0)


def shapes_driver(game):
    """Event 7 of npcs-shapes: the Silencer immortal and armed, an NPC of
    each shape the program knows (scenes.NPC_SHAPES) attacking him; after a
    while each is hit by each kind, then hard (a robot's feint, a woman's
    cries, an organic NPC spun dead)."""
    table = scenes.intrinsics_table(game)
    names = [f'K{k}' for k in range(len(scenes.NPC_SHAPES))]
    offsets = {name: -2 * (k + 1) for k, name in enumerate(names)} | {'S': -20, 'N': -22, 'W': -24}
    call = lambda name, arguments: scenes.call(table, name, arguments, local_offsets=offsets)
    lines = ['ENTER 24', 'PUSH_WORD 1', f'POP_LOCAL_WORD {offsets["S"]}'] + call('@Npc@setImmortal$qv', 'S')
    lines += controls.give(table, game, 'N', offsets)
    for k, (name, shape) in enumerate(zip(names, scenes.NPC_SHAPES)):
        x, y = at(k % 8, 260 + 40 * (k % 3))
        lines += call('@Npc@createActor$qusus', f'{name} {shape} 0') + call('@Item@popToCoords$qususuc', f'{name} {x} {y} 0')
        lines += call('@Npc@setActivity$qus', f'{name} 5')
    w = offsets['W']
    lines += [f'PUSH_WORD 300', f'POP_LOCAL_WORD {w}', 'fighting:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1',
              'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', 'JUMP_IF_FALSE fought', 'JUMP fighting', 'fought:']
    for kind in (1, 3, 4, 8, 13):
        for name in names:
            lines += call('@receiveHitIntrinsic$qn4Itemusucius', f'{name} 1 {(kind * 3) % 16} 12 {kind}')
            lines += ['SUSPEND'] * 4
    for k, name in enumerate(names):
        lines += call('@receiveHitIntrinsic$qn4Itemusucius', f'{name} 1 4 250 3') + ['SUSPEND'] * 10
        lines += scenes.marker(table, k)
    return '\n'.join(lines + ['SUSPEND'] * 60 + ['RETURN'])


def attackers_driver(game):
    """Event 7 of attackers: the Silencer immortal, an NPC of each shape
    that speaks before it fires (No Remorse's ready lines by shape: 2FDh,
    319h, 384h, 371h, 4D1h, 528h, 1B4h, and later ones: 2DFh, 383h) attacking
    him a while; in No Regret a crawling spider (584h) among them, which they
    shoot at."""
    table = scenes.intrinsics_table(game)
    # two of each: the first lines come one time in three
    speakers = (0x2fd, 0x319, 0x384, 0x371, 0x4d1, 0x528, 0x1b4, 0x2df, 0x383) * 2
    names = [f'K{k}' for k in range(len(speakers))]
    offsets = {name: -2 * (k + 1) for k, name in enumerate(names)} | {'S': -38, 'P': -40, 'W': -42}
    call = lambda name, arguments: scenes.call(table, name, arguments, local_offsets=offsets)
    lines = ['ENTER 42', 'PUSH_WORD 1', f'POP_LOCAL_WORD {offsets["S"]}'] + call('@Npc@setImmortal$qv', 'S')
    for k, (name, shape) in enumerate(zip(names, speakers)):
        x, y = at(k % 8, 280 + 70 * (k // 8))
        lines += call('@Npc@createActor$qusus', f'{name} {shape} 0') + call('@Item@popToCoords$qususuc', f'{name} {x} {y} 0')
        lines += call('@Npc@setActivity$qus', f'{name} 5') + scenes.marker(table, k)
    if game == 'regret':
        x, y = at(2, 200)
        lines += call('@Item@create$qusus', f'P {scenes.CRAWLING_SPIDER} 0')
        lines += call('@Item@popToCoords$qususuc', f'P {x} {y} 0')
    w = offsets['W']
    lines += [f'PUSH_WORD 900', f'POP_LOCAL_WORD {w}', 'fighting:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1',
              'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', 'JUMP_IF_FALSE fought', 'JUMP fighting', 'fought:']
    return '\n'.join(lines + ['RETURN']), speakers


def attackers(game):
    """Two NPCs of each shape that speaks before it fires, attacking the
    immortal Silencer a while.

    Checks: the attacks' ready lines by shape (attack.cpp: No Remorse's
        2FDh, 319h, 384h, 371h, 4D1h, 528h, 1B4h, and later ones, 2DFh and
        383h), the first lines one time in three; in No Regret the NPCs
        shooting at the crawling spider among them (584h).
    Drives: the test egg's event 7 (attackers_driver): the Silencer
        immortal, two of each speaker attacking (activity 5), No Regret's
        crawling spider near him, then 900 passes; the npcs scene's map,
        no keys.
    Tests:
        <shape>, again
            The second NPC of that shape made: its marker in the trace.
        <shape>
            The first.
    Original: Both follow, No Remorse's to pass 376: at pass 377 an NPC
        finds him in its shot's place, which the port takes as a hit and the
        original as the distance to a point it never set (point-blank).
    """
    import dataclasses
    base = scene(game)
    code, speakers = attackers_driver(game)
    names = [scenes.shape_name(shape) + (', again' if k >= len(speakers) // 2 else '') for k, shape in enumerate(speakers)]
    return dataclasses.replace(base, name='attackers', inputs=[], frames=200 + 900 + 100,
                               classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble(code)})},
                               tests=names, ran=scenes.marked(game), followed_to=376 if game == 'remorse' else 0)


def tactics_rest(game):
    """Tactic 3 (tactics.py's REST) for two NPCs, one near the Silencer and
    one far, a waypoint of frame 2 near them.

    Checks: the combat tactic REST (attack.cpp's tactics, combat.dat's
        script from tactics.py): walking to the waypoint and resting,
        near and far from the Silencer.
    Drives: the test egg's event 7: the Silencer immortal, two actors with
        tactic 3 attacking (activity 5), 700 passes; a floor of 24 by 24
        tiles and a waypoint of frame 2, no keys.
    Tests:
        near
            The NPC near him made: its marker in the trace.
        far
            The one far from him made: its marker.
    Original: Both follow.
    """
    import dataclasses
    import formats
    table = scenes.intrinsics_table(game)
    offsets = {'S': -2, 'A': -4, 'B': -6, 'W': -8}
    call = lambda name, arguments: scenes.call(table, name, arguments, local_offsets=offsets)
    lines = ['ENTER 8', 'PUSH_WORD 1', f'POP_LOCAL_WORD {offsets["S"]}'] + call('@Npc@setImmortal$qv', 'S')
    for k, (name, (x, y)) in enumerate((('A', at(2, 120)), ('B', at(6, 620)))):
        lines += call('@Npc@createActor$qusus', f'{name} {scenes.ACTOR} 0')
        lines += call('@Item@popToCoords$qususuc', f'{name} {x} {y} 0')
        lines += call('@Npc@setCombatTactic$qus', f'{name} 3') + call('@Npc@setActivity$qus', f'{name} 5')
        lines += scenes.marker(table, k)
    w = offsets['W']
    lines += ['PUSH_WORD 700', f'POP_LOCAL_WORD {w}', 'going:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1', 'SUB',
              f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', 'JUMP_IF_FALSE gone', 'JUMP going', 'gone:', 'RETURN']
    records = scenes.floor_and_egg(24) + [formats.item_record(scenes.X0 + 200, scenes.Y0 + 100, 0, scenes.WAYPOINT,
                                                              frame=2, quality=0)]
    base = scene(game)
    return dataclasses.replace(base, name='tactics-rest', inputs=[], frames=200 + 700 + 100,
                               maps={scenes.TEST_MAP: records},
                               classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(lines))})},
                               tests=['near', 'far'], ran=scenes.marked(game))


# point-blank: an NPC attacking the immortal Silencer from 40 east of him,
# their boxes overlapping as a fight pushes them (attackers' NPCs stood so):
# the shot's place at its muzzle (the firing frame's point, 32 west of it
# facing west, world.ATTACK_POINTS) is in his box.
POINT_BLANK = (scenes.X0 + 40, scenes.Y0)
POINT_BLANK_PASSES = 300


def point_blank_fired(record, marks, k):
    """Whether the NPC with the Silencer in its shot's place fired: shots
    made (SuperSprite processes; the Silencer fires none)."""
    return any(name == 'SuperSprite' for _, _, _, name in record.processes)


def point_blank(game):
    """An NPC attacking the immortal Silencer with the shot's place at its
    muzzle in his box: it fires.

    Checks: crusader.cpp fireDistance where the target itself stands in the
        shot's place (shapeFits blocked by the target): the shot meets it at
        once, 1, so the attack (attack.cpp: ready, drawWeapon, the turn and
        the burst) fires instead of walking to another place (PathFinder).
    Drives: the test egg's event 7 (point_blank_code): the Silencer immortal,
        an actor at POINT_BLANK (40 east of him) attacking (activity 5),
        POINT_BLANK_PASSES passes; the npcs scene's map, no keys. The actor
        finds him in its shot's place 4 times (both games).
    Tests:
        fires at the muzzle
            Shots made (SuperSprite in the trace): 36 in both games, with 4
            path finders between the bursts; taken as a miss (0) it would
            never fire and walk about (96 path finders, no shot).
    Original: None: where the target stands in the shot's place the
        original took the distance to a point aimPoint had not set in that
        call (its stack's leftovers), so whether it fired or walked away
        was the stack's chance, other in the originals' own runs of the npcs
        scenes; the ground is the code's: a shot that meets the target at
        once hits it.
    Traps: the boxes must overlap as a fight leaves them: from boxes that
        only touch (the actor 64 west of him, facing east, its muzzle 16
        east) the shot's place is free, and the case never comes.
    """
    import dataclasses
    table = scenes.intrinsics_table(game)
    offsets = {'S': -2, 'A': -4, 'W': -6}
    call = lambda name, arguments: scenes.call(table, name, arguments, local_offsets=offsets)
    x, y = POINT_BLANK
    lines = ['ENTER 6', 'PUSH_WORD 1', f'POP_LOCAL_WORD {offsets["S"]}'] + call('@Npc@setImmortal$qv', 'S')
    lines += call('@Npc@createActor$qusus', f'A {scenes.ACTOR} 0') + call('@Item@popToCoords$qususuc', f'A {x} {y} 0')
    lines += call('@Npc@setActivity$qus', 'A 5')
    w = offsets['W']
    lines += [f'PUSH_WORD {POINT_BLANK_PASSES}', f'POP_LOCAL_WORD {w}', 'fighting:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}',
              'PUSH_WORD 1', 'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', 'JUMP_IF_FALSE fought', 'JUMP fighting',
              'fought:']
    lines += scenes.marker(table, 0) + ['RETURN']
    base = scene(game)
    return dataclasses.replace(base, name='point-blank', inputs=[], frames=200 + POINT_BLANK_PASSES + 100, oracle=False,
                               classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(lines))})},
                               tests=['fires at the muzzle'], ran=scenes.checked(game, point_blank_fired))


def shapes(game):
    """An NPC of each shape the program knows by number, attacking the
    immortal Silencer, then hit by each kind of fire and hard.

    Checks: what the program does by an NPC's shape (scenes.NPC_SHAPES:
        crusader.cpp takeHit, attack.cpp): a woman's cries (FEMALE_SHAPES),
        the speaker of ready lines (2FDh), the robots' feints when hit
        (ROBOT_SHAPES), an organic NPC spun dead.
    Drives: the test egg's event 7 (shapes_driver): the Silencer immortal
        and armed, one NPC of each shape attacking; after 300 passes each
        hit lightly by fire types 1, 3, 4, 8 and 13, then hard (250, fire
        type 3); the npcs scene's map, no keys.
    Tests:
        <shape>
            The NPC of that shape after its hard hit: its marker in the
            trace.
    Original: Both follow.
    """
    import dataclasses
    base = scene(game)
    code = ucasm.assemble(shapes_driver(game))
    return dataclasses.replace(base, name='npcs-shapes', inputs=[], frames=300 + 5 * 7 * 6 + 7 * 12 + 300,
                               classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code})},
                               tests=[scenes.shape_name(shape) for shape in scenes.NPC_SHAPES], ran=scenes.marked(game))
