"""The PL-1's auto fire: while
the fire key is held, AutoFirer fires more shots than the fire animations'
(No Regret 12 a second, each on energy; No Remorse 6, on none), and it stops
when a shot of it finds the key let go (boboboom.cpp fireAtFrame's fireDown).
autofire-keys: by the keyboard, held then let go: it stops (the originals'
rule). autofire-mouse: by the mouse's button, held then let go: the release
is lost while the Silencer is busy firing (explode.cpp mouseFire returns on
isBusy before it sends the key's release), so the auto fire goes on, No
Regret's until the energy is gone, No Remorse's without end. That is the
originals' bug, not fixed yet: the scene pins it (both originals follow),
and its footprints turn when it is fixed."""
import dataclasses

import controls
import scenes
import ucasm
import world

LOCALS = {'N': -2, 'S': -4}
# The frames a shot of the fire animation still comes after the release,
# and those the scene goes on after the last counted (the shots' sprites
# end there, which the record counts by).
LATE, AFTER = 10, 150
# How long the fire is held: No Remorse's first shot comes 23 frames after
# the press, its auto fire's next 10 after that.
HOLD = 45


def code(table, game):
    """Event 7: the PL-1 and a fusion battery picked up, the energy all
    the battery holds."""
    def call(name, arguments):
        return scenes.call(table, name, arguments, local_offsets=LOCALS)
    names = world.game_shapes(game)
    lines = ['ENTER 4', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}']
    for shape in (names['SHAPE_PL_1'], names['SHAPE_FUSION_BATTERY']):
        lines += call('@Item@create$qusus', f'N {shape} 0')
        lines += call('@Item@popToCoords$qususuc', f'N {scenes.X0} {scenes.Y0} 0') + call('@pickUp$qusuc', 'n 0')
    lines += call('@Npc@setMana$qus', 'S 2000')
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def shots(at_least=None, none=False):
    """A footprint: the shots (the SuperSprites made) of a window, at least
    so many, or none."""
    def seen(record, first, last):
        count = record.made('SuperSprite', first, last)
        return count == 0 if none else count >= at_least
    return seen


def base(game, actions):
    scene = controls.of_actions(game, actions)
    table = scenes.intrinsics_table(game)
    return dataclasses.replace(scene, classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table, game)})})


def keys(game):
    """The PL-1's fire key held, then let go: the auto fire while it is
    held, none after.

    Checks: the PL-1's auto fire (boboboom.cpp fireAtFrame, AutoFirer: No
        Regret 12 a second on energy, No Remorse 6 on none) and its stop
        when a shot finds the key let go (fireDown).
    Drives: the test egg's event 7 (code) gives the PL-1, a fusion battery
        and 2000 energy; the keys: the weapon, Space held HOLD frames
        (controls.of_actions).
    Tests:
        weapon
            The PL-1 chosen. No record shows it.
        held: the auto fire
            Two shots or more while held (more than the one animation's:
            the recording does not repeat a held key; the SuperSprites
            made).
        let go: it stops
            No shot from LATE frames after the release.
    Original: Both follow.
    """
    actions = [('weapon', ['1'], 4, None), ('held: the auto fire', ['space'], HOLD, shots(at_least=2)),
               ('let go: it stops', [], 120, lambda record, first, last: shots(none=True)(record, first + LATE, last))]
    scene = base(game, actions)
    return dataclasses.replace(scene, name='autofire-keys', frames=scene.frames + AFTER)


def mouse(game):
    """The PL-1 fired by the mouse's button held, then let go: the auto fire
    goes on after the release (the originals' bug).

    Checks: the mouse's fire (explode.cpp mouseFire): its release is lost
        while the Silencer is busy firing (it returns on isBusy before it
        sends the key's release), so fireDown stays set and AutoFirer,
        which stops only on a shot that finds it clear, keeps firing: No
        Regret's until the energy is gone, No Remorse's without end.
    Drives: the test egg's event 7 (code) as autofire-keys'; the weapon's
        key, then the button pressed and let go HOLD frames later (motion
        lines).
    Tests:
        weapon
            The PL-1 chosen. No record shows it.
        held: the auto fire
            Two shots or more while held.
        let go: it goes on (the bug)
            A shot 40 to 100 frames after the release.
        later: none, the energy gone
            No Regret: no shot 300 to 400 frames after the release.
        later: still, it costs no energy
            No Remorse: a shot 300 to 400 frames after the release.
    Original: Both follow: the bug is the originals'.
    Guards: the originals' bug, to be fixed: the last two tests turn then, and
        the scene changes with the fix.
    """
    scene = base(game, [('weapon', ['1'], 4, None)])
    press = scene.frames
    release = press + HOLD
    inputs = list(scene.inputs) + [f'{press}\tmotion\t0\t0\t1', f'{release}\tmotion\t0\t0\t0']
    windows = scene.ran.windows + [
        (press, release, shots(at_least=2)),
        (release + LATE + 30, release + 100, shots(at_least=1)),
        (release + 300, release + 400, shots(none=True) if game == 'regret' else shots(at_least=1))]
    tests = scene.tests + ['held: the auto fire', 'let go: it goes on (the bug)',
                           'later: none, the energy gone' if game == 'regret' else 'later: still, it costs no energy']
    return dataclasses.replace(scene, name='autofire-mouse', inputs=inputs, frames=release + 400 + AFTER,
                               tests=tests, ran=scenes.windows_ran(windows))


SCENES = {'autofire-keys': keys, 'autofire-mouse': mouse}
