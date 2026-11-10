"""The hits of the weapons and the Silencer's shields: a
gun (an item that shoots, as the games' wall guns do) shoots each of the
game's weapons' fire types at standing targets of many hit points (an
organic one near and one far, a robot near), at the difficulty of the
scene; then, while the Silencer wears the ionic, the plasma and the
graviton shield in turn, it shoots him with the fire types each shield
stops and one it does not, and at last with too little energy.

The gun is an item, not the Silencer: the controlled NPC's shots go to the
reticle (crusader.cpp fireWeapon, launchAtReticle), an NPC's take its
weapon's damage; an item's take the fire type's and fly in the direction
given (launchInDirection). It is a waypoint, which no blast breaks."""
import re

import npcs
import scenes
import ucasm
import world

X0, Y0 = scenes.X0, scenes.Y0
LOCALS = {'S': -2, 'N': -4, 'T': -6, 'F': -8, 'R': -10, 'D': -12, 'W': -14, 'K': -16}
FRAME = 2 * len(LOCALS)
DIRECTIONS = npcs.DIRECTIONS
EAST, SOUTH, WEST, NORTH = 2, 4, 6, 0
# hits-weapons: where the gun shoots from (away from the Silencer and the
# test egg), and its targets: the organic one NEAR east and FAR south, the
# robot NEAR north. A shot leaves from OUT toward its target (crusader.cpp
# fireWeapon's x, y and z from the shooter's place), 16 up.
SHOOTER = (X0 + 400, Y0 - 300)
NEAR, FAR, OUT = 96, 320, 48
SHOT_WAIT = 40


def call(table, name, arguments):
    return scenes.call(table, name, arguments, local_offsets=LOCALS)


def wait(passes, label):
    w = LOCALS['W']
    return [f'PUSH_WORD {passes}', f'POP_LOCAL_WORD {w}', f'{label}:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1',
            'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', f'JUMP_IF_FALSE {label}_end', f'JUMP {label}',
            f'{label}_end:']


def place(spot, direction, distance):
    """The place distance from spot in direction (of the 8)."""
    dx, dy = DIRECTIONS[direction]
    return spot[0] + dx * distance, spot[1] + dy * distance


def fire_table(game):
    """{fire type: (low damage, high damage, shield cost, shield mask)} of
    <game>/src/game/firetype_game.cpp's table (game/firetype.h FireType)."""
    text = re.sub(r'\s+', '', (scenes.PORT / game / 'src/game/firetype_game.cpp').read_text())
    number = r'(0x[0-9a-fA-F]+|\d+)'
    rows = re.findall(r'\{' + ','.join([number] * 11) + r'\}', text)
    out = {}
    for row in rows:
        values = [int(value, 0) for value in row]
        out[values[0]] = (values[1], values[2], values[5], values[6])
    return out


def actor(table, local, shape, spot):
    """An NPC of shape made at spot in the local."""
    return call(table, '@Npc@createActor$qusus', f'{local} {shape} 0') + \
        call(table, '@Item@popToCoords$qususuc', f'{local} {spot[0]} {spot[1]} 0')


def shot(table, shooter, target, direction, kind, label):
    """shooter's shot of fire type kind at target (the direction of the 8
    it is in): from OUT beyond its box that way, then SHOT_WAIT passes."""
    dx, dy = DIRECTIONS[direction]
    lines = call(table, '@Item@directionToItem$qus', f'{shooter} {target.lower()}')
    lines += ['PUSH_RETURNED_BYTE', f'POP_LOCAL_WORD {LOCALS["D"]}']
    lines += call(table, '@fireWeapon$qn4Itemiiizciuc', f'{shooter} {OUT * dx} {OUT * dy} 16 d {kind} 0')
    return lines + wait(SHOT_WAIT, label)


def gun(table, spot):
    """The gun (a waypoint) made at spot in K."""
    return call(table, '@Item@create$qusus', f'K {scenes.WAYPOINT} 0') + \
        call(table, '@Item@popToCoords$qususuc', f'K {spot[0]} {spot[1]} 0')


def weapons_code(table, game):
    """hits-weapons' egg: the gun at SHOOTER, the targets; each fire type
    of the game's weapons at each target, the targets put back at their
    places first (no hit here moves them: so that one that did would not
    change the next fire type's shots)."""
    near, far, robot = place(SHOOTER, EAST, NEAR), place(SHOOTER, SOUTH, FAR), place(SHOOTER, NORTH, NEAR)
    lines = [f'ENTER {FRAME}'] + gun(table, SHOOTER)
    lines += actor(table, 'T', scenes.TARGET, near) + actor(table, 'F', scenes.TARGET, far)
    lines += actor(table, 'R', scenes.ROBOT_TARGET, robot)
    for k, kind in enumerate(npcs.weapon_fire_types(game)):
        for local, spot in (('T', near), ('F', far), ('R', robot)):
            lines += call(table, '@Item@move$qususuc', f'{local} {spot[0]} {spot[1]} 0')
        lines += wait(10, f'placed{k}')
        lines += shot(table, 'K', 'T', EAST, kind, f'near{k}') + shot(table, 'K', 'F', SOUTH, kind, f'far{k}')
        lines += shot(table, 'K', 'R', NORTH, kind, f'robot{k}') + scenes.marker(table, k)
    return lines + ['RETURN']


def hit_points_lost(record, marks, k, hp):
    """The hit points (the state line's field hp) lost from test k-1's
    marker to test k's, from the most before the first marker for k 0."""
    before = (int(record.state(marks[k - 1])[hp]) if k else
              max(int(state[hp]) for state in record.states if int(state[0]) < marks[0]))
    return before - int(record.state(marks[k])[hp])


def weapons(game):
    """A gun shoots each of the game's weapons' fire types at an organic
    target near and one far, and at a robot near: every shot hits its
    target.

    Checks: each fire type's shots (crusader.cpp fireWeapon: bullets,
        pellets, energy, rockets, grenades; the fire type's shots, its
        damage from low to high, its range for a blast:
        <game>/src/game/firetype_game.cpp), their flights (sprite.cpp
        SuperSprite: the shot's shape SHAPE_SHOT, solid 1 by 1 by 1, meets
        the target's box) and reach, and their hits on NPCs: takeHit,
        scaledDamage (by the difficulty; a third of the RP's, SG-A1's, BA's
        and AC-88's on a robot; at most 250), npcTakesHit; a blast's
        (tracker.cpp hurtArea) on what else lies in its reach.
    Drives: the test egg's event 7 (weapons_code): the gun (gun, a
        waypoint) at SHOOTER; scenes.TARGET NEAR (96) east and FAR (320)
        south of it, scenes.ROBOT_TARGET NEAR north (NPCs of
        scenes.TARGET_HP, 20000, hit points that stand: no shot kills
        them); for each fire type of the game's weapons
        (npcs.weapon_fire_types: No Regret's 13, No Remorse's 10), the
        targets put back at their places (no hit here moves them), then a
        shot at each from OUT (48) beyond the gun toward it, 16 up,
        SHOT_WAIT passes apart; the fire type's marker.
    Tests:
        fire type <type>
            Its three shots, each on its target (the far one too: every
            fire type reaches 320): at its marker the NPCs' hit points (the
            state lines' total) at least 3 less than at the last marker.
            At difficulty 3, in both games but where said: the RP's (1)
            and the BA's (11) bullet, the fire type's damage (18 to 25, 14
            to 20), a third on the robot; the SG-A1's (2) pellets, No
            Regret's 10 of 15 to 20, No Remorse's 8 of 10 to 17, each a
            third on the robot; the AC-88's (13), No Regret's 10 of 15 to
            20, No Remorse's 5 of 10 to 17, as the SG-A1's; the GL-303's
            (3) grenade, a blast: 200 on its target and 100 on the other
            near target, 96 away (2 in 48s), none on the far one; the
            AR-7's (10) three rockets, each a blast: three hits of 45 to 80
            on its target and three of half on the other near one; the
            PA's (5), PL-1's (6), UV-9's (14), EM-4's (15) and No Regret's
            BK-16's (16), LNR-81's (17) and XP-5's (20) one shot of the
            fire type's damage, the whole on the robot too (the UV-9's
            250).
    Original: Both follow.
    Guards: the footprints' hit points: when the synthetic shots' shape had
        no box, every shot of every scene flew through what it met, and
        nothing said so.
    Traps: the gun is an item, not an NPC: the controlled NPC's shots go to
        the reticle (crusader.cpp fireWeapon, launchAtReticle) and an NPC's
        take its weapon's damage (weaponDamage), not the fire type's; an
        item's take the fire type's and fly in the direction given. The
        targets' hit points are many, so that no shot kills one: a target
        falling dead would change the next fire types' shots.
    """
    table = scenes.intrinsics_table(game)
    kinds = npcs.weapon_fire_types(game)
    return scenes.Scene(name='hits-weapons', maps={scenes.TEST_MAP: scenes.floor_and_egg(24)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(weapons_code(table, game)))})},
                        arguments=scenes.START, frames=120 + len(kinds) * (10 + 3 * SHOT_WAIT) + 100,
                        tests=[f'fire type {kind}' for kind in kinds],
                        ran=scenes.checked(game, lambda record, marks, k:
                                           hit_points_lost(record, marks, k, scenes.STATE_ALL_HP) >= 3))


def weapons_skill1(game):
    """The weapons' hits at difficulty 1.

    Variant of: hits-weapons
    Checks: the difficulty's scale of the hits on NPCs (crusader.cpp
        scaledDamage: five times at difficulty 1, then a third of the RP's,
        SG-A1's, BA's and AC-88's on a robot, at most 250): the RP's bullet
        90 to 125 (on the robot 30 to 41), an SG-A1's pellet 75 to 100 (No
        Remorse's 50 to 85); the GL-303's grenade (its blast's half on the
        other target too), the UV-9's shot and No Regret's EM-4's, BK-16's,
        LNR-81's and XP-5's 250.
    Drives: the hits-weapons scene with -skill 1.
    Original: Both follow.
    """
    import dataclasses
    base = weapons(game)
    return dataclasses.replace(base, name='hits-weapons-skill1', arguments=scenes.START + ['-skill', '1'])


# hits-shields: where the Silencer stands, and the gun that shoots him
# from east of him.
WEARER = (X0 + 400, Y0 + 200)
SHOOTER_DISTANCE = 128
# The shields in the order the Silencer takes them (pickUp fits a better
# one): their types and the bits of FireType::shieldMask they stop
# (crusader.cpp silencerShieldTakes: the graviton shield, type 3, is mask 4).
SHIELDS = [(1, 1), (2, 2), (3, 4)]
ENERGY = 9000
# The last round: the graviton shield with too little energy for the first
# shot (its cost more), enough for the second, too little after it.
LOW_ENERGY, LOW_SHOTS = 100, (1, 2, 11)


def shield_rounds(game):
    """[(shield type or None, energy, [fire types])]: for each shield, the
    weapons' fire types it stops and the weakest it does not; then the
    graviton shield with LOW_ENERGY and LOW_SHOTS."""
    table = fire_table(game)
    kinds = npcs.weapon_fire_types(game)
    rounds = []
    for shield, mask in SHIELDS:
        stopped = [kind for kind in kinds if table[kind][3] & mask]
        through = sorted((kind for kind in kinds if not table[kind][3] & mask), key=lambda kind: table[kind][1])
        rounds.append((shield, ENERGY, stopped + through[:1]))
    rounds.append((None, LOW_ENERGY, [kind for kind in LOW_SHOTS if kind in kinds]))
    return rounds


def shields_code(table, game):
    """hits-shields' egg: a fusion battery for the Silencer, then each round
    of shield_rounds: its shield taken, his energy set, and each fire type
    shot at him by the gun, he put back at his place first."""
    names = world.game_shapes(game)
    shooter = place(WEARER, EAST, SHOOTER_DISTANCE)
    lines = [f'ENTER {FRAME}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}']
    lines += call(table, '@Item@move$qususuc', f'S {WEARER[0]} {WEARER[1]} 0')
    lines += gun(table, shooter)
    taken = ['SHAPE_FUSION_BATTERY']
    for k, (shield, energy, kinds) in enumerate(shield_rounds(game)):
        if shield:
            taken.append(npcs.SHIELDS[shield])
        for name in taken:
            lines += call(table, '@Item@create$qusus', f'N {names[name]} 0')
            lines += call(table, '@Item@popToCoords$qususuc', f'N {WEARER[0]} {WEARER[1]} 0')
            lines += call(table, '@pickUp$qusuc', 'n 0')
        taken = []
        lines += call(table, '@Npc@setMana$qus', f'S {energy}')
        for j, kind in enumerate(kinds):
            lines += call(table, '@Item@move$qususuc', f'S {WEARER[0]} {WEARER[1]} 0')
            lines += wait(10, f'placed{k}_{j}') + shot(table, 'K', 'S', WEST, kind, f'shot{k}_{j}')
        lines += scenes.marker(table, k)
    return lines + ['RETURN']


def shield_names(game):
    names = {1: 'the ionic shield', 2: 'the plasma shield', 3: 'the graviton shield'}
    return [names[shield] if shield else 'too little energy' for shield, _, _ in shield_rounds(game)]


def shields(game):
    """The Silencer shot while he wears each shield in turn, at difficulty
    1, then with too little energy: each shield stops its fire types for
    their energy, the weakest it does not stop goes through.

    Checks: the Silencer's shields: world.cpp pickUp (the battery, then
        each shield fitted over the worse one he wears); crusader.cpp
        silencerShieldTakes (the fire types whose shieldMask has the
        shield's bit: ionic 1, plasma 2, graviton 4; the fire type's
        shieldCost from his energy, for each pellet; the ionic and the
        plasma shield's flash, the graviton's sphere; a cost his energy
        does not cover goes through); the hits it does not stop
        (npcTakesHit, scaledDamage: a fifth at difficulty 1).
    Drives: the test egg's event 7 (shields_code): the Silencer at WEARER,
        the gun (gun) SHOOTER_DISTANCE (128) east of him; a fusion battery;
        then each round of shield_rounds: its shield made at his feet and
        picked up, his energy set (ENERGY, 9000; the last round's
        LOW_ENERGY, 100), and for each of its fire types he put back at
        WEARER, 10 passes, the gun's shot at him, SHOT_WAIT passes; the
        round's marker. -skill 1, so that the shots no shield stops leave
        him alive.
    Tests:
        <shield>
            The round of that shield, or of too little energy: at its
            marker his hit points (the state lines') less than at the last
            marker (the shot no shield stops), and he alive. The ionic
            shield stops the RP's bullet (130 energy), the SG-A1's pellets
            and the AC-88's (80 each) and the BA's bullet (80); the PL-1's
            shot goes through (No Regret 5, No Remorse 5: 150 to 145). The
            plasma shield stops those and the PA's (400) and the PL-1's (No
            Regret 300, No Remorse 375); the EM-4's goes through (No Regret
            11: to 134; No Remorse 7: to 138). The graviton shield stops all
            those and the UV-9's (2500), the EM-4's (750) and No Regret's
            BK-16's (500), LNR-81's (1250) and XP-5's (900); the AR-7's
            three rockets go through (No Regret 3 of 10: to 104; No Remorse
            3 of 11: to 105). Too little energy (the graviton shield, 100):
            the RP's bullet goes through (130 more than 100), the SG-A1's
            first pellet is stopped (20 left), the others and the BA's
            bullet go through (No Regret to 71, No Remorse to 78). The hit
            points are this run's (the damages are drawn from the fire
            types' ranges; the kept results pin them).
    Original: Both follow.
    Traps: the shield he picks up last is the one he wears only when it is
        better (pickUp): the rounds take them in the order ionic, plasma,
        graviton. His energy is set each round, so that only the last
        round runs short.
    """
    table = scenes.intrinsics_table(game)
    shots = sum(len(kinds) for _, _, kinds in shield_rounds(game))
    return scenes.Scene(name='hits-shields', maps={scenes.TEST_MAP: scenes.floor_and_egg(24)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: ucasm.assemble('\n'.join(shields_code(table, game)))})},
                        arguments=scenes.START + ['-skill', '1'], frames=120 + shots * (10 + SHOT_WAIT) + 100,
                        tests=shield_names(game),
                        ran=scenes.checked(game, lambda record, marks, k:
                                           hit_points_lost(record, marks, k, scenes.STATE_HP) > 0 and
                                           int(record.state(marks[k])[scenes.STATE_HP]) > 0))


SCENES = {'hits-weapons': weapons, 'hits-weapons-skill1': weapons_skill1, 'hits-shields': shields}
