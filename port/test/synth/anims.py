"""Every animation by Npc::doAnim: the egg
has a test actor, then the Silencer (the NPC the player controls, holding
the controls scene's weapons), do each animation the engine knows in turn,
each killing the one before (npc.cpp doAnim's cases: the large weapons'
ready, fire, reload and unready, the bright fires, stopping a run, the
finish of firing, electrocution, the jumps, rolls and slides, surrender).
An animation doAnim has not (KNEEL_FIRE 1Ah and the others without a case)
halts, the originals too."""
import re

import controls
import saves
import scenes
import ucasm

X0, Y0 = scenes.X0, scenes.Y0
LOCALS = {'K': -2, 'S': -4, 'W': -6}
WAIT = 14


def anims(game):
    """The animations doAnim has (npc.cpp's newAnimProcess cases: another
    halts), less No Regret's own in No Remorse (the forward roll 5, the
    kneeling slides 1Bh and 35h, and 16h)."""
    source = (scenes.PORT / 'common/src/item/npc.cpp').read_text()
    body = source[source.index('AnimPrimitive *newAnimProcess('):]
    body = body[:body.index('    default:\n        return nullptr;')]
    values = dict((name, int(value, 16)) for name, value in
                  re.findall(r'(?:#define|constexpr int) (ANIM_\w+)(?: =)? (0x[0-9a-f]+)',
                             (scenes.PORT / 'common/src/game/animprim.h').read_text()))
    known = sorted({values[name] for name in re.findall(r'case (ANIM_\w+):', body)} |
                   {int(number, 16) for number in re.findall(r'case (0x[0-9a-f]+):', body)})
    return [anim for anim in known if not (game == 'remorse' and anim in (0x05, 0x16, 0x1b, 0x35))]


def wait(passes, label):
    w = LOCALS['W']
    return [f'PUSH_WORD {passes}', f'POP_LOCAL_WORD {w}', f'{label}:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}', 'PUSH_WORD 1',
            'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', f'JUMP_IF_FALSE {label}_end', f'JUMP {label}',
            f'{label}_end:']


def code(table, game):
    lines = ['ENTER 6', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}'] + controls.give(table, game, 'K', LOCALS)
    lines += scenes.call(table, '@Npc@createActor$qusus', f'K {scenes.ACTOR} 0', local_offsets=LOCALS)
    lines += scenes.call(table, '@Item@popToCoords$qususuc', f'K {X0 + 150} {Y0} 0', local_offsets=LOCALS)
    for test, who in enumerate(('K', 'S')):
        for anim in anims(game):
            lines += scenes.call(table, '@Npc@doAnim$qusususuc', f'{who} {anim} 4 0 1', local_offsets=LOCALS)
            lines += wait(WAIT, f'{who}{anim}')
        lines += scenes.marker(table, test)
    lines += scenes.call(table, '@Npc@doAnim$qusususuc', 'S 0 4 0 1', local_offsets=LOCALS)
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def scene(game):
    """Every animation the engine knows, done by a test actor and then by
    the armed Silencer, each killing the one before, with quick saves now
    and then.

    Checks: Npc::doAnim's cases (npc.cpp newAnimProcess, animprim.cpp): the
        large weapons' ready, fire, reload and unready, the bright fires,
        stopping a run, the finish of firing, electrocution, the jumps,
        rolls and slides, surrender; a save and load of each running.
    Drives: the test egg's event 7 (code): an actor made, then each
        animation of anims (from npc.cpp's cases) by doAnim on it and then
        on the Silencer, WAIT passes each; the keys: the second weapon
        drawn, then quick saves every 200 frames (saves.quick_saves).
    Tests:
        actor
            The test actor through every animation: its marker.
        Silencer
            The Silencer through every animation: its marker.
    Original: Both follow.
    Guards: an animation doAnim has not (KNEEL_FIRE 1Ah and the others
        without a case) halts, the originals too; No Regret's own are
        left out of No Remorse's (the forward roll 5, the kneeling slides
        1Bh and 35h, and 16h, which No Remorse's halts).
    """
    table = scenes.intrinsics_table(game)
    # the second weapon drawn first (the large anims of an armed Silencer),
    # then quick saves now and then: each writes the animation running
    inputs = controls.press(120, ['2'], 4) + saves.quick_saves(range(300, 1700, 200))
    return scenes.Scene(name='anims', maps={scenes.TEST_MAP: scenes.floor_and_egg(12)},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: code(table, game)})},
                        arguments=scenes.START, frames=200 + 2 * len(anims(game)) * (WAIT + 2) + 7 * 160, tests=['actor', 'Silencer'],
                        ran=scenes.marked(game), inputs=inputs)


SCENES = {'anims': scene}
