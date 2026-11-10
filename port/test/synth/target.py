"""The reticle's targets: eight standing
actors around the Silencer, one in each of the main directions, so that
whichever way he faces with his weapon drawn the reticle finds one
(tracker.cpp findTarget: an occluding target on the screen exactly in his
direction); he fires, turns and fires again, and the reticle's keys (No
Regret's Ctrl+R style, No Remorse's R on and off) are pressed."""
import dataclasses

import controls
import scenes
import ucasm

X0, Y0 = scenes.X0, scenes.Y0
LOCALS = {'K': -2, 'W': -4}
AROUND = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if (dx, dy) != (0, 0)]


def code(table, game):
    lines = ['ENTER 4'] + controls.give(table, game, 'K', LOCALS)
    for dx, dy in AROUND:
        lines += scenes.call(table, '@Npc@createActor$qusus', f'K {scenes.ACTOR} 0', local_offsets=LOCALS)
        lines += scenes.call(table, '@Item@popToCoords$qususuc', f'K {X0 + 128 * dx} {Y0 + 128 * dy} 0',
                             local_offsets=LOCALS)
        lines += scenes.call(table, '@Npc@setActivity$qus', 'K 0', local_offsets=LOCALS)
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def scene(game):
    """Eight actors around the Silencer: he fires, turns and fires with the
    reticle on them, the reticle's keys are pressed, and the mouse fires
    over them and the floor.

    Checks: the reticle's targets (tracker.cpp findTarget: an occluding
        target on the screen exactly in his direction), the reticle's
        keys (No Regret's Ctrl+R style, No Remorse's R on and off with its
        box), the turns that turn the reticle (control.cpp
        Controller::turn), the mouse's fire with the item under the
        pointer found by its frame's pixels (world.cpp itemAt, dlitem.cpp
        hit, framehit.cpp).
    Drives: the test egg's event 7 (code): his things (controls.give) and
        eight standing actors around him; the actions (controls.of_actions),
        then six mouse fires with the pointer moved.
    Tests:
        weapon
            A weapon of bullets (No Regret's 2, the BA-41; No Remorse's 1,
            the BA-40). No record shows it.
        fire
            Space: a shot (acted).
        turn <n>
            The right arrow: the reticle turns. No record shows it.
        fire <n>
            Space at the next target (acted).
        turn back
            The left arrow. No record shows it.
        fire again
            (acted).
        reticle style
            No Regret's Ctrl+R. No record shows it.
        targeting off
            No Remorse's R: its box. No record shows it.
        fire under the box
            Space while the box is up: nothing yet (idle).
        box closed: the fire after it
            Enter: the box closed, the fire under it shoots (acted).
        fire, targeting off
            (acted).
        targeting on
            R again: its box. No record shows it.
        box closed
            Enter. No record shows it.
        fire, targeting on
            (acted).
    Original: Both follow.
    Guards: the keys pressed while No Remorse's box is up act once it has
        closed, the original's too, which the scene pins; without its
        Enter the box stays up.
    """
    table = scenes.intrinsics_table(game)
    # No Remorse's R turns the targeting (findTarget's lock) off and on,
    # each time with a box that takes Enter and Escape alone (world.cpp
    # showMessage, msgbox.cpp); the keys pressed while it is up act once it
    # has closed (the original's too): the fire under it shoots after the
    # Enter (without it the box stays up).
    reticle = [('reticle style', ['ctrl', 'r'], 4)] if game == 'regret' else [
        ('targeting off', ['r'], 4), ('fire under the box', ['space'], 20, 'idle'),
        ('box closed: the fire after it', ['enter'], 50, 'acted'), ('fire, targeting off', ['space'], 20),
        ('targeting on', ['r'], 4), ('box closed', ['enter'], 30, None), ('fire, targeting on', ['space'], 20)]
    # A weapon of bullets that has them for all the shots: No Regret's 2,
    # the BA-41 (its 1, the XP-5, fires on energy the Silencer has not), No
    # Remorse's 1, the BA-40. With the reticle shown, left and right turn
    # the reticle (control.cpp Controller::turn), which no record keeps.
    weapon = '2' if game == 'regret' else '1'
    actions = [('weapon', [weapon], 4), ('fire', ['space'], 20)]
    for k in range(6):
        actions += [(f'turn {k}', ['right'], 6, None), (f'fire {k}', ['space'], 20)]
    after = [('turn back', ['left'], 12, None), ('fire again', ['space'], 20)]
    actions += reticle + after if game == 'regret' else after + reticle
    base = controls.of_actions(game, actions)
    classes = {0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table, game)})}
    # the mouse's fire with the pointer over the actors and the floor: the
    # item under it is found by its frame's pixels (world.cpp itemAt,
    # dlitem.cpp hit, framehit.cpp)
    inputs, frame = list(base.inputs), base.frames
    for dx, dy in ((0, 0), (40, 20), (-80, 0), (40, -60), (60, 60), (-30, 10)):
        inputs += [f'{frame}\tmotion\t{dx}\t{dy}\t0', f'{frame + 2}\tmotion\t0\t0\t1', f'{frame + 12}\tmotion\t0\t0\t0']
        frame += 30
    return dataclasses.replace(base, name='target', classes=classes, inputs=inputs, frames=frame + 60)


SCENES = {'target': scene}
