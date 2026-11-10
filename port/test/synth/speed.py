"""The games' speed where it hangs on the machine's: a process that waits
for another goes on in the pass after the wait ends. The originals ran many
passes a tick on a fast machine, so the pass cost them nothing there; the
port's pass at the game speed SLOW, the tests' (platform/clock.h,
REGRET_GAME_SPEED), is a sixtieth of a second, a whole tick
(system/timer.cpp clockPass); NORMAL and FAST run two a tick, NORMAL one in
three ticks of ten. Two waits show it: each frame of an actor's animation waits
its repeat's ticks (AnimPrimitive::advanceFrame's AccWait), and the
usecode's step movers wait a tick a step through a process that counts the
ticks (No Regret's 0A1Dh:003Bh, No Remorse's 0A0Ch:0032h; here the test
egg's class's TICKS, written the same way).

speed-lift: No Remorse's mission 14 in small: the switch pressed and a run
at once to a lift that rises after a window of ticks. The Silencer walks
north-west with the up arrow held (his walk waits its frames' ticks as the
games' do); the test egg's driver is the switch: from his first step it
waits WINDOW ticks, the lift's time to rise, and writes where he is; then it
raises a mover STEPS steps of a tick each and writes the ticks they took."""
import dataclasses

import controls
import formats
import scenes
import ucasm

X0, Y0 = scenes.X0, scenes.Y0
EGG_CLASS = 0x900 + scenes.TEST_EGG_TYPE
# The egg's class's tick wait (its argument the ticks), as the games'
# usecode waits: the ticks read each pass and counted when they changed.
TICKS = 0x20
# The walk: 8 frames of 4 units (8 half units), each shown for two ticks as
# the games' walks and runs are (anim.dat's repeat), looping.
WALK = 0x01
MOVES = {WALK: (8, 8, {'looping': 1, 'two_step': 1, 'repeat': 2})}
# The window from the switch to the lift's rise, in ticks, and the steps of
# the mover, a tick each.
WINDOW = 90
STEPS = 20
# The frame the walk's key goes down (the driver waits for his first step)
# and how long it is held.
KEY_AT = 90
HOLD = WINDOW + 60
# The lift, north-west of the start: its near corner LIFT units off along
# each axis. A walk of 2 ticks a frame reaches it in the window, one that
# loses a tick a frame does not.
LIFT = 480
FLOOR = 24
# Where the mover rises: north of the start, out of the walk's way.
MOVER_AT = (X0, Y0 - 160)
LOCALS = {'S': -2, 'M': -4, 'Z': -6, 'K': -8}
# The globals the driver writes: the ticks at the switch, at the rise and
# after the mover's steps, and the Silencer's place at the rise.
RESULTS = 240
AT_SWITCH, AT_RISE, AT_STEPS_END = RESULTS, RESULTS + 4, RESULTS + 8
X_AT_RISE, Y_AT_RISE = RESULTS + 12, RESULTS + 14


def wait(ticks):
    """The driver waits for the egg class's tick wait of ticks to end (our
    pid under its pid, as the games' usecode waits for a process it
    spawns)."""
    return ['PUSH_PID', f'PUSH_WORD {ticks}', 'PUSH_LONG 0', f'SPAWN 2 0 {EGG_CLASS} {TICKS}', 'MOVE_TOP -2',
            'PUSH_RETURNED_WORD', 'IMPLIES 1 1', 'POP_RETURNED_WORD', 'SUSPEND']


def tick_wait(table):
    """TICKS: its argument (local 6) the ticks; the timer's tick read each
    pass, the count down when it changed."""
    now = scenes.call(table, '@currentTimerTick$qv', '')
    lines = ['ENTER 8'] + now + ['PUSH_RETURNED_LONG', 'POP_LOCAL_LONG -4',
                                 'PUSH_LOCAL_WORD 6', 'PUSH_BYTE 1', 'LT', 'JUMP_IF_FALSE poll', 'JUMP done',
                                 'poll:', 'SUSPEND'] + now
    lines += ['PUSH_RETURNED_LONG', 'POP_LOCAL_LONG -8', 'PUSH_LOCAL_LONG -8', 'PUSH_LOCAL_LONG -4', 'NE_LONG',
              'JUMP_IF_FALSE poll', 'PUSH_LOCAL_LONG -8', 'POP_LOCAL_LONG -4',
              'PUSH_LOCAL_WORD 6', 'PUSH_BYTE 1', 'SUB', 'POP_LOCAL_WORD 6',
              'PUSH_LOCAL_WORD 6', 'PUSH_BYTE 1', 'LT', 'JUMP_IF_FALSE poll', 'done:', 'RETURN']
    return ucasm.assemble('\n'.join(lines))


def driver(table):
    """The test egg's event 7: the switch's tick, the window, the Silencer's
    place and the tick at the rise; then the mover made and raised STEPS
    steps of 2, a tick each, and the tick after them."""
    def call(name, arguments, result=None):
        return scenes.call(table, name, arguments, result=result, local_offsets=LOCALS)
    lines = ['ENTER 8'] + call('@controlledNpcNumber$qv', '') + ['PUSH_RETURNED_WORD', f'POP_LOCAL_WORD {LOCALS["S"]}']
    # the switch: his first step from the start
    lines += ['still:', 'SUSPEND'] + call('@Item@x$qv', 'S') + ['PUSH_RETURNED_WORD', f'PUSH_WORD {X0}', 'EQ',
                                                                   'JUMP_IF_FALSE moving', 'JUMP still', 'moving:']
    lines += call('@currentTimerTick$qv', '', AT_SWITCH) + scenes.marker(table, 0) + wait(WINDOW)
    lines += call('@currentTimerTick$qv', '', AT_RISE)
    lines += call('@Item@x$qv', 'S', X_AT_RISE) + call('@Item@y$qv', 'S', Y_AT_RISE) + scenes.marker(table, 1)
    lines += ['SUSPEND'] + call('@Item@create$qusus', f'M {scenes.ITEM} 0')
    lines += call('@Item@popToCoords$qususuc', f'M {MOVER_AT[0]} {MOVER_AT[1]} 0')
    lines += ['PUSH_WORD 0', f'POP_LOCAL_WORD {LOCALS["Z"]}', f'PUSH_WORD {STEPS}', f'POP_LOCAL_WORD {LOCALS["K"]}',
              'step:', f'PUSH_LOCAL_WORD {LOCALS["Z"]}', 'PUSH_BYTE 2', 'ADD', f'POP_LOCAL_WORD {LOCALS["Z"]}']
    lines += call('@Item@push$qv', 'M') + call('@Item@popToCoords$qususuc', f'M {MOVER_AT[0]} {MOVER_AT[1]} z')
    lines += wait(1) + [f'PUSH_LOCAL_WORD {LOCALS["K"]}', 'PUSH_BYTE 1', 'SUB', f'POP_LOCAL_WORD {LOCALS["K"]}',
                        f'PUSH_LOCAL_WORD {LOCALS["K"]}', 'PUSH_BYTE 0', 'GT', 'JUMP_IF_FALSE stepped', 'JUMP step',
                        'stepped:']
    lines += call('@currentTimerTick$qv', '', AT_STEPS_END) + scenes.marker(table, 2) + ['SUSPEND', 'RETURN']
    return ucasm.assemble('\n'.join(lines))


def results(record):
    """The driver's results: the window's ticks, the Silencer's place at the
    rise, the mover's ticks."""
    switch, rise, end = (record.global_value(at, 4) for at in (AT_SWITCH, AT_RISE, AT_STEPS_END))
    return dict(window=rise - switch, x=record.global_value(X_AT_RISE), y=record.global_value(Y_AT_RISE),
                steps=end - rise)


def footprints(game, record, tests):
    found = results(record)
    marked = scenes.marked(game)(record, tests)
    short = X0 - found['x'] < LIFT and Y0 - found['y'] < LIFT
    return [marked[0], marked[1] and found['window'] >= WINDOW, marked[1] and short,
            marked[2] and found['steps'] >= 2 * STEPS]


def scene(game):
    """No Remorse's mission 14 lift in small: the switch, a window of ticks
    while the Silencer walks to the lift, and a mover that rises a tick a
    step. At the game speed SLOW (the tests', a pass a sixtieth of a second)
    the port loses a pass at each wait's end, a whole tick: the walk falls
    short and the steps take twice their ticks (a slow machine's speed: a
    fast machine's original reaches the lift).

    Checks: the actors' animation frames that wait their repeat's ticks
        (animprim.cpp AnimPrimitive::advanceFrame, systimer.cpp AccWait), a
        usecode process waiting for the one it spawned (IMPLIES; kernel.cpp
        runProcessesFrom runs the waiter in the pass after), the timer's
        ticks (gametime.cpp currentTimerTick) and the pass's time
        (timer.cpp clockPass).
    Drives: the test egg's event 7 (driver) with the egg class's tick wait
        (tick_wait, function 20h); the walk (MOVES: 2 ticks a frame, 16
        units a frame along each axis north-west) by the up arrow held from
        KEY_AT; the switch is his first step. Results in the globals at
        RESULTS (results).
    Tests:
        switch
            His first step seen: the driver's first marker.
        window
            WINDOW ticks waited: its marker, and at least WINDOW ticks
            between the two in the globals (91: the timer's ticks keep).
        short of the lift
            Where he is at the rise: short of LIFT along each axis (400; a
            fast machine's original walks 608). SLOW's, pinned.
        steps twice their ticks
            STEPS steps of a tick each took at least twice their ticks (41;
            a fast machine's original takes 20). SLOW's, pinned.
    Original: Both follow under the pass oracle, which gives the original
        the port's time (a pass a tick): with the same time the code is the
        same. In DOSBox at cycles=max, with the unkcoff.dat made from the
        address table (world.unkcoff), both originals walk 608 and step 20
        ticks (window 90): a pass there costs a fraction of a millisecond.
    Guards: the other game speeds are speed-lift-normal's and
        speed-lift-fast's (a build running 8 passes a tick: 624 and 20).
    Traps: the synthetic data's animations have no repeat (a frame every
        pass), hence the scene's own walk (Scene.moves); an original run in
        DOSBox needs the unkcoff.dat made from the address table, not the
        synthetic data's zeros, or its intrinsics are other functions.
    """
    table = scenes.intrinsics_table(game)
    classes = {EGG_CLASS: ('SYNTEGG', {7: driver(table), TICKS: tick_wait(table)})}
    return scenes.Scene(name='speed-lift', maps={scenes.TEST_MAP: scenes.floor_and_egg(FLOOR)}, classes=classes,
                        arguments=scenes.START, frames=KEY_AT + HOLD + 2 * STEPS + 60,
                        tests=['switch', 'window', 'short of the lift', 'steps twice their ticks'],
                        inputs=controls.press(KEY_AT, ['up'], HOLD),
                        ran=lambda record, tests: footprints(game, record, tests), moves=MOVES)


def at_speed(game, speed, tests, judge):
    """speed-lift at a game speed of the port's (enhanced.cfg game_speed=),
    its tests judged by judge(found, marked) from the driver's results and
    markers."""
    def ran(record, ran_tests):
        return judge(results(record), scenes.marked(game)(record, ran_tests))
    return dataclasses.replace(scene(game), name=f'speed-lift-{speed}', enhanced=[f'game_speed={speed}'],
                               oracle=False, tests=tests, ran=ran)


def reached(found):
    return X0 - found['x'] >= LIFT and Y0 - found['y'] >= LIFT


def normal_scene(game):
    """speed-lift at the game speed NORMAL: two passes a tick, one in three
    ticks of ten (platform/clock.h). The walk reaches the lift in the
    window and the steps take more than their ticks and less than twice.

    Checks: the passes of NORMAL (system/timer.cpp clockPass, passLength,
        onePassTick: the ticks of one pass by a hash of their numbers) under
        speed-lift's waits (an animation's frame, a usecode process waiting
        for the one it spawned); enhanced.cfg's game_speed= (enhanced.cpp
        readEnhancedText) over the tests' REGRET_GAME_SPEED.
    Drives: speed-lift's driver, walk and lift, with enhanced.cfg's
        game_speed=normal.
    Tests:
        switch
            His first step seen: the driver's first marker.
        window
            WINDOW ticks waited: its marker, and at least WINDOW ticks
            between the two in the globals.
        at the lift
            Where he is at the rise: LIFT or more along each axis (a walk
            of NORMAL at about 90% of FAST's).
        steps between
            STEPS steps of a tick each took more than STEPS ticks and fewer
            than twice: a step loses a tick where its tick has one pass.
    Original: None: the pass oracle gives the original a pass a tick, SLOW's;
        the ground is the hash's share of ticks, which the port alone has.
    Guards: a regular pattern of one-pass ticks falls in step with the
        walk's frames; the hash's share is NORMAL's, the slowest speed at
        which an ordinary run boards No Remorse's mission 14 lift.
    """
    def judge(found, marked):
        return [marked[0], marked[1] and found['window'] >= WINDOW, marked[1] and reached(found),
                marked[2] and STEPS < found['steps'] < 2 * STEPS]
    return at_speed(game, 'normal', ['switch', 'window', 'at the lift', 'steps between'], judge)


def fast_scene(game):
    """speed-lift at the game speed FAST: two passes every tick
    (platform/clock.h). The walk reaches the lift in the window and each
    step takes its tick, as a fast machine's original.

    Checks: the passes of FAST (system/timer.cpp clockPass, passLength: to
        the tick's middle and its end) under speed-lift's waits; enhanced.cfg's
        game_speed= (enhanced.cpp readEnhancedText) over the tests'
        REGRET_GAME_SPEED.
    Drives: speed-lift's driver, walk and lift, with enhanced.cfg's
        game_speed=fast.
    Tests:
        switch
            His first step seen: the driver's first marker.
        window
            WINDOW ticks waited: its marker, and at least WINDOW ticks
            between the two in the globals.
        at the lift
            Where he is at the rise: LIFT or more along each axis (a fast
            machine's original walks 608).
        steps a tick each
            STEPS steps of a tick each took STEPS ticks, or one more for the
            tick the first began in (a fast machine's original takes 20).
    Original: None: the pass oracle gives the original a pass a tick, SLOW's;
        the ground is the original at cycles=max in DOSBox (speed-lift's
        Original), which walks 608 and steps 20 ticks.
    """
    def judge(found, marked):
        return [marked[0], marked[1] and found['window'] >= WINDOW, marked[1] and reached(found),
                marked[2] and STEPS <= found['steps'] <= STEPS + 1]
    return at_speed(game, 'fast', ['switch', 'window', 'at the lift', 'steps a tick each'], judge)


SCENES = {'speed-lift': scene, 'speed-lift-normal': normal_scene, 'speed-lift-fast': fast_scene}
