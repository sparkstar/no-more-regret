"""Deaths and hurts: NPCs hit by the usecode
(receiveHitIntrinsic, crusader.cpp takeHit) in the ways the games end them:
each fire type's death (burning or torn apart, also where the burning shape
has no room, plasma, skeleton, EM, frozen and shattered, LNR, No Regret's
gibs), the bosses' shields (No Regret's robot, No Remorse's Vargas) until
they are gone, the death cries of a woman, of the armoured guards and of the
others, the kneeling guards' death shapes, which cannot fall forwards, and
the hurts: a hit of no damage, a woman's cry, the robots' feints, the
Silencer's when immortal and at fire types 7, 8 and 15."""
import formats
import scenes
import ucasm


NAMES = scenes.named_shapes()
TROOPERS = ('SHAPE_STORM_TROOPER', 'SHAPE_ELITE_GUARD', 'SHAPE_ELITE_GRENADIER')
GUARDS = ('HQ_GUARD', 'ENFORCER', 'LMC_GUARD', 'STORM_TROOPER')
KNEELING = tuple(f'SHAPE_{kind}_KNEELING_{which}' for kind in GUARDS for which in 'AB')
# The NPCs the scene makes besides the test actor: as it is (world.py gives
# them its type, flags and animations), and crying out when they die.
ACTORS = tuple(NAMES[name] for name in ('SHAPE_BOSS_ROBOT', 'SHAPE_VARGAS') + TROOPERS + KNEELING)
# The shapes an NPC dies in, which move as the actors do and cry out too;
# the burning ones a box of 64 by 64, wider than the actors'.
DEAD = tuple(NAMES[name] for name in ('SHAPE_BURNING', 'SHAPE_BURNING_GIBS', 'SHAPE_GIBS', 'SHAPE_PLASMA_DEATH',
                                      'SHAPE_SKELETON_DEATH', 'SHAPE_EM_DEATH', 'SHAPE_LNR_DEATH', 'SHAPE_FROZEN',
                                      'SHAPE_FROZEN_SHATTERING'))
WIDE = (NAMES['SHAPE_BURNING'], NAMES['SHAPE_BURNING_GIBS'])
# The kneeling guards' death shapes: NPC types, flags and animations but
# the one of falling forwards (hasAnim).
STILL = tuple(NAMES[f'SHAPE_{kind}_KNEELING_DEATH'] for kind in GUARDS)
# The woman (No Regret's only woman, among scenes.NPC_SHAPES): she cries
# out when she dies too, and her NPC type is world.NPC_TYPES' second.
WOMAN = NAMES['SHAPE_SEATED_LADY']
WOMAN_TYPE = 2
# The activities (common/src/item/npc.h).
ACTIVITY_ATTACK, ACTIVITY_GUARD = 5, 8
# The fire types (common/src/game/firetype.h) whose deaths burn or tear an
# NPC apart by chance (GL-303, an explosion, AR-7 and 12), and those with a
# dead shape of their own (PL-1, UV-9, EM-4, BK-16 and 18, LNR-81, XP-5).
BURNING_TYPES = (3, 4, 10, 12)
SHAPED_TYPES = (6, 14, 15, 16, 18, 17, 20)
# The robots that feint when hit (crusader.cpp takeHit): VETRON, THERMATRON,
# CHEDITRON and ANDROID, which are among the program's (scenes.NPC_SHAPES).
FEINTING = tuple(NAMES[name] for name in ('SHAPE_VETRON', 'SHAPE_THERMATRON', 'SHAPE_CHEDITRON', 'SHAPE_ANDROID'))
# Where an NPC stands with a wall just west of it (its box is 32 wide, the
# burning shapes' 64): in two rows north of the Silencer.
CORNERS = [(scenes.X0 - 450 + 150 * (k % 6), scenes.Y0 - 400 - 150 * (k // 6)) for k in range(10)]
# (not E: scenes.argument_code's E is the event's item)
LOCALS = {'S': -2, 'K': -4, 'D': -6, 'T': -8, 'W': -10}
HIT = '@receiveHitIntrinsic$qn4Itemusucius'


def map_records():
    """A floor of 24 by 24 tiles, the test egg, and a wall west of each
    corner (16 by 64, ending 32 west of the NPC's place)."""
    return scenes.floor_and_egg(24) + [formats.item_record(x - 32, y, 0, scenes.WALL) for x, y in CORNERS]


class Driver:
    """The test egg's event 7, written case by case: the NPCs in rows south
    of the Silencer, each hit as its case says."""

    def __init__(self, game):
        self.table = scenes.intrinsics_table(game)
        self.lines = [f'ENTER {2 * len(LOCALS)}', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}']
        self.lines += self.call('@Npc@setImmortal$qv', 'S')
        self.npcs = 0
        self.waits = 0
        self.passes = 0
        self.tests = []

    def call(self, name, arguments):
        return scenes.call(self.table, name, arguments, local_offsets=LOCALS)

    def place(self):
        """The next NPC's place: rows of eleven, 96 apart."""
        k = self.npcs
        self.npcs += 1
        return scenes.X0 - 480 + 96 * (k % 11), scenes.Y0 + 150 + 96 * (k // 11)

    def actor(self, shape=scenes.ACTOR, at=None):
        x, y = at or self.place()
        self.lines += self.call('@Npc@createActor$qusus', f'K {shape} 0')
        self.lines += self.call('@Item@popToCoords$qususuc', f'K {x} {y} 0')

    def made(self, kind, activity2=3):
        """An NPC of type kind as the usecode makes one (Item::createNpc:
        Npc::create marks FEMALE_SHAPES, the type item's map is the third
        activity), as npcs.py's NPCs are made."""
        x, y = self.place()
        self.lines += self.call('@Item@create$qusus', f'D {scenes.ITEM} 0')
        self.lines += self.call('@Item@setQualityLow$qus', 'D 1') + self.call('@Item@setQualityHigh$qus', 'D 2')
        self.lines += self.call('@Item@setMap$qi', 'D 0') + self.call('@Item@setNpcNumber$quc', 'D 0')
        self.lines += self.call('@Item@popToCoords$qususuc', f'D {x} {y} 0')
        self.lines += self.call('@Item@create$qusus', f'T {scenes.ITEM} 0')
        self.lines += self.call('@Item@setNpcNumber$quc', f'T {kind}')
        self.lines += self.call('@Item@setQualityHigh$qus', 'T 1') + self.call('@Item@setMap$qi', f'T {activity2}')
        self.lines += self.call('@Item@createNpc$qus', 'D t') + ['PUSH_RETURNED_WORD', f'POP_LOCAL_WORD {LOCALS["K"]}']

    def hit(self, damage, kind, who='K'):
        """A hit from the Silencer, from a direction of the NPC's number."""
        self.lines += self.call(HIT, f'{who} 1 {(self.npcs * 5) % 16} {damage} {kind}')

    def wait(self, passes):
        """The egg's process waits passes passes (a SUSPEND each)."""
        w = LOCALS['W']
        self.waits += 1
        self.passes += passes
        top, end = f'waiting{self.waits}', f'waited{self.waits}'
        self.lines += [f'PUSH_WORD {passes}', f'POP_LOCAL_WORD {w}', f'{top}:', 'SUSPEND', f'PUSH_LOCAL_WORD {w}',
                       'PUSH_WORD 1', 'SUB', f'POP_LOCAL_WORD {w}', f'PUSH_LOCAL_WORD {w}', f'JUMP_IF_FALSE {end}',
                       f'JUMP {top}', f'{end}:']

    def done(self, name):
        """The end of the test name: its marker."""
        self.lines += scenes.marker(self.table, len(self.tests))
        self.tests.append(name)

    def code(self):
        return ucasm.assemble('\n'.join(self.lines + ['RETURN']))


def driver(game):
    d = Driver(game)
    d.wait(20)
    # The Silencer immortal, hit: no damage.
    d.hit(5, 1, who='S')
    d.wait(10)
    d.done('the Silencer immortal')
    # Each fire type's death of the test actor (organic): the burning ones
    # three times (torn apart or burning by chance and by the damage), and
    # once where the burning shapes have no room.
    for kind in BURNING_TYPES:
        for damage in (250, 250, 40):
            d.actor()
            d.hit(damage, kind)
            d.wait(30)
        d.done(f'fire type {kind}')
    for k, (damage, kind) in enumerate([(250, 3)] * 8 + [(40, 3), (40, 10)]):
        d.actor(at=CORNERS[k])
        d.hit(damage, kind)
        d.wait(30)
    d.done('burning without room')
    for kind in SHAPED_TYPES:
        d.actor()
        d.hit(250, kind)
        d.wait(30)
        d.done(f'fire type {kind}')
    # A frozen NPC hit again once it lies still shatters (No Regret).
    d.actor()
    d.hit(250, 16)
    d.wait(60)
    d.hit(5, 1)
    d.wait(30)
    d.done('frozen, hit again')
    # The bosses' shields: No Regret's robot sparks until its shield (1500)
    # is gone, then dies (GL-303: a robot's RP hits take a third); No
    # Remorse's Vargas teleports out and back, a hit while he does is lost,
    # and the shield (5000) goes below 0 after a first hit of less.
    d.actor(NAMES['SHAPE_BOSS_ROBOT'])
    for _ in range(8):
        d.hit(250, 3)
        d.wait(8)
    d.done('the boss robot')
    d.actor(NAMES['SHAPE_VARGAS'])
    d.hit(30, 1)
    d.hit(250, 1)
    d.wait(24)
    for _ in range(22):
        d.hit(250, 1)
        d.wait(24)
    d.done('Vargas')
    # The death cries: a woman's (also at EM-4, No Remorse's own), the
    # armoured guards', the kneeling guards' in their death shapes (which
    # cannot fall forwards), and one in such a shape from the start.
    for kind in (1, 15):
        d.made(WOMAN_TYPE)
        d.hit(250, kind)
        d.wait(30)
    d.done('a woman dies')
    for name in TROOPERS + KNEELING:
        d.actor(NAMES[name])
        d.hit(250, 1)
        d.wait(20)
        d.done(name)
    d.actor(STILL[0])
    d.hit(250, 1)
    d.wait(20)
    d.done('a shape that cannot fall forwards')
    # The hurts: a hit of no damage (one at least); a woman's cry; the
    # robots attacking, hit by EM-4 and the electric fire type (each feints
    # by chance); the Silencer mortal for a while, hit by fire types 8, 7
    # and 15.
    d.actor()
    d.hit(0, 1)
    d.wait(10)
    d.done('a hit of no damage')
    d.made(WOMAN_TYPE)
    for _ in range(2):
        d.hit(5, 1)
        d.wait(10)
    d.done('a woman hurt')
    # An NPC hit turns on the Silencer: one guarding, one standing whose
    # third activity is the attack.
    d.actor()
    d.lines += d.call('@Npc@setActivity$qus', f'K {ACTIVITY_GUARD}')
    d.wait(10)
    d.hit(5, 1)
    d.wait(20)
    d.made(1, ACTIVITY_ATTACK)
    d.wait(10)
    d.hit(5, 1)
    d.wait(20)
    d.done('NPCs turn on the Silencer')
    for shape in FEINTING:
        d.actor(shape)
        d.lines += d.call('@Npc@setActivity$qus', 'K 5')
        d.wait(40)
        for _ in range(4):
            for kind in (15, 7):
                d.hit(5, kind)
                d.wait(6)
        d.done(f'robot {shape:#x} hurt')
    d.lines += d.call('@Npc@clearImmortal$qv', 'S')
    for kind in (8, 7, 15):
        d.hit(5, kind, who='S')
        d.wait(20)
        d.done(f'the Silencer hurt by {kind}')
    d.lines += d.call('@Npc@setImmortal$qv', 'S')
    if game == 'regret':
        # No Regret's small shields (the ionic and the plasma) flash at the
        # Silencer (No Remorse's original at the null pointer of a hit
        # without one: npcs.py).
        import world
        names = world.game_shapes(game)
        for name in ('SHAPE_FUSION_BATTERY', 'SHAPE_IONIC_SHIELD', 'SHAPE_PLASMA_SHIELD'):
            d.lines += d.call('@Item@create$qusus', f'D {names[name]} 0')
            d.lines += d.call('@Item@popToCoords$qususuc', f'D {scenes.X0} {scenes.Y0} 0')
            d.lines += d.call('@pickUp$qusuc', 'd 0')
            d.lines += d.call('@Npc@setMana$qus', 'S 9000')
            d.hit(5, 1, who='S')
            d.wait(30)
        d.done('the small shields')
    # Another NPC controlled dies: the control goes back to the Silencer.
    d.actor()
    d.lines += d.call('@setControlledNpcNumber$qus', 'k')
    d.wait(10)
    d.hit(250, 1)
    d.wait(60)
    d.done('the controlled NPC dies')
    return d


def scene(game):
    """NPCs hit by the usecode in the ways the games end them: each fire
    type's death, the bosses' shields, the death cries, the kneeling
    guards' shapes, and the hurts.

    Checks: crusader.cpp takeHit and the deaths (receiveHitIntrinsic): each
        fire type's death (burning or torn apart, also where the burning
        shape has no room; plasma, skeleton, EM, frozen and shattered, LNR,
        No Regret's gibs), the bosses' shields (No Regret's robot, No
        Remorse's Vargas) until they are gone, the death cries of a woman,
        of the armoured guards and of the others, the kneeling guards'
        death shapes, which cannot fall forwards, and the hurts: a hit of
        no damage, a woman's cry, the robots' feints, NPCs turning on the
        Silencer, his hurts when mortal, the small shields, the controlled
        NPC's death.
    Drives: the test egg's event 7 (driver: Driver, case by case): NPCs
        made in rows south of the Silencer (actors, and some as the
        usecode makes them), each hit as its case says, each case ended by
        its marker (its footprint); walls west of the corners where the
        burning shapes have no room.
    Tests:
        the Silencer immortal
            Hit while immortal: no damage (its marker).
        fire type <n>
            The test actor's death by that fire type (the burning ones three
            times: torn apart or burning by chance and by the damage).
        burning without room
            Burning deaths with a wall just west.
        frozen, hit again
            A frozen NPC hit again once it lies still: it shatters (No
            Regret).
        the boss robot
            No Regret's robot sparks until its shield is gone, then dies.
        Vargas
            No Remorse's Vargas teleports out and back, his shield goes.
        a woman dies
            Her cry, also at EM-4.
        SHAPE_<guard>
            An armoured or kneeling guard's death cry and shape.
        a shape that cannot fall forwards
            A kneeling guard's death shape from the start.
        a hit of no damage
            One point at least.
        a woman hurt
            Her cry.
        NPCs turn on the Silencer
            One guarding and one standing, hit.
        robot <n> hurt
            A robot attacking, hit by EM-4 and the electric fire type: it
            feints by chance.
        the Silencer hurt by <n>
            Mortal, hit by that fire type.
        the small shields
            No Regret's ionic and plasma shields flash at him.
        the controlled NPC dies
            The control goes back to the Silencer.
    Original: Both follow.
    """
    d = driver(game)
    return scenes.Scene(name='deaths', maps={scenes.TEST_MAP: map_records()},
                        classes={0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: d.code()})}, arguments=scenes.START,
                        frames=200 + d.passes + 200, tests=d.tests, ran=scenes.marked(game))


SCENES = {'deaths': scene}
