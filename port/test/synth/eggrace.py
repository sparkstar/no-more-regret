"""The egg events' race (by which the hero can vanish at No Remorse's
base pad): an egg's class is 0x900
plus its quality's low byte at the moment of each event (classOf, eggType),
and a trigger egg sets that byte to its trigger's number while its trigger
runs. Here the trigger egg (class 0x91D) sets it to 5 for 10 passes from
its entry (event 7); class 0x905 stands for No Remorse's CAMERA, whose
entry makes the camera follow the egg (setFollowedItem). The test egg's
usecode moves the Silencer into the trigger egg's range, out and in again:

- egg-race: a pass apart, so that the third entry goes to class 0x905 and
  the camera follows the egg; then teleportToEgg to map 101's egg 30 moves
  the Silencer to map 101, but Npc::teleport moves the camera (and so loads
  the map) only while the camera follows the Silencer: map 101 is never
  loaded, the teleporter finds this map's own arrival egg 30 and puts the
  Silencer there as of map 101, off the world (the bug, kept as the
  original's);
- egg-race-slow: 20 passes apart, the trigger done and the byte back, so
  that the entries go to the trigger egg's own class and the teleport loads
  map 101 with the Silencer on it.

The egg writes the Silencer's map and the camera's place into the globals
after the teleport. Both originals do the same (the code is theirs)."""
import dataclasses

import formats
import scenes
import ucasm

X0, Y0 = scenes.X0, scenes.Y0
TRIGGER_EGG_TYPE, CAMERA_TYPE, TRIGGER = 0x1d, 0x05, 5
OTHER_MAP, ARRIVAL = 101, 30
EGG_AT = (X0 + 300, Y0)
INSIDE, OUTSIDE = (X0 + 300, Y0 + 16), (X0 + 400, Y0 + 16)
HERE_ARRIVAL = (X0 - 200, Y0 + 200)
LOCALS = {'S': -2, 'W': -4}
RESULTS = 220


def trigger_code(table):
    """The trigger egg's entry (event 7): its byte the trigger's number for
    10 passes (the trigger running), then its own again."""
    lines = ['ENTER 0', 'PUSH_WORD 5', 'PUSH_LOCAL_LONG 6']
    number, size, _ = table['@Item@setQualityLow$qus']
    lines += [f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP -{size}'] + ['SUSPEND'] * 10
    lines += [f'PUSH_WORD {TRIGGER_EGG_TYPE}', 'PUSH_LOCAL_LONG 6', f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP -{size}']
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def camera_code(table):
    """Class 0x905's entry (No Remorse's CAMERA): the camera follows the egg
    (this item's number, read through this)."""
    number, size, _ = table['@setFollowedItem$qus']
    lines = ['ENTER 0', 'PUSH_LOCAL_LONG 6', 'PUSH_INDIRECT 2', f'CALL_INTRINSIC {size} {number}',
             f'MOVE_TOP -{size}', 'RETURN']
    return ucasm.assemble('\n'.join(lines))


def driver(table, apart):
    """The test egg's entry: the Silencer into the trigger egg's range, out
    and in again, apart passes apart; the teleport; the results."""
    call = lambda name, arguments, result=None: scenes.call(table, name, arguments, result=result,
                                                            local_offsets=LOCALS)
    lines = ['ENTER 4', 'PUSH_WORD 1', f'POP_LOCAL_WORD {LOCALS["S"]}'] + ['SUSPEND'] * 30
    for k, (x, y) in enumerate((INSIDE, OUTSIDE, INSIDE)):
        lines += call('@Item@move$qususuc', f'S {x} {y} 0') + ['SUSPEND'] * apart
    lines += scenes.marker(table, 0)
    lines += ['SUSPEND'] * 30 + call('@teleportToEgg$qususuc', f'{OTHER_MAP} {ARRIVAL} 1') + ['SUSPEND'] * 60
    lines += call('@Item@map$qv', 'S', RESULTS) + call('@cameraX$qv', '', RESULTS + 2)
    lines += call('@cameraY$qv', '', RESULTS + 4) + call('@Item@x$qv', 'S', RESULTS + 6)
    return ucasm.assemble('\n'.join(lines + ['SUSPEND'] * 30 + ['RETURN']))


def maps():
    # this map has an arrival egg 30 too (as No Remorse's base has): the
    # teleporter that could not load map 101 finds it here and puts the
    # Silencer there as of map 101, off the world (egg.cpp Teleporter::arrive)
    here = scenes.floor_and_egg(16) + [
        formats.item_record(*EGG_AT, 0, scenes.EGG, quality=TRIGGER_EGG_TYPE, npc=0x11),
        formats.item_record(*HERE_ARRIVAL, 0, scenes.TELEPORT_EGG, frame=1, quality=ARRIVAL)]
    there = [formats.item_record(X0 - 256 + 64 * i + 63, Y0 - 256 + 64 * j + 63, 0, scenes.FLOOR)
             for i in range(8) for j in range(8)]
    there += [formats.item_record(X0, Y0, 0, scenes.TELEPORT_EGG, frame=1, quality=ARRIVAL)]
    return {scenes.TEST_MAP: here, OTHER_MAP: there}


def teleported(record, race):
    """The teleport's footprint: the Silencer put on map 101, with the map
    loaded, or in the race without it (the camera on the egg: the hero
    gone)."""
    put = any(line[1] == 'T' and line[2] == '1' and line[-1] == str(OTHER_MAP) for line in record.trace)
    loaded = any(line[1] == 'M' and line[2] == str(OTHER_MAP) for line in record.trace)
    return put and loaded != race


def scene(game, apart=1, name='egg-race'):
    """The egg events' race: the Silencer into a trigger egg's range, out
    and in again a pass apart, so that the third entry runs another class;
    then a teleport that never loads its map.

    Checks: an egg's class at each event (uprocess.cpp classOf, eggType:
        0x900 plus its quality's low byte then), which a trigger egg sets to
        its trigger's number while it runs (here 5 for 10 passes, class
        0x905 standing for No Remorse's CAMERA, whose entry makes the camera
        follow the egg); Npc::teleport, which loads the map only while the
        camera follows the Silencer (egg.cpp Teleporter::arrive).
    Drives: the test egg's event 7 (driver): the Silencer moved in, out and
        in, apart passes apart, then teleportToEgg to map 101's egg 30; the
        map's own arrival egg 30 (as No Remorse's base has); the globals
        get his map and the camera's place after it.
    Tests:
        in, out and in
            The three entries: the driver's marker after them.
        teleport
            Put on map 101 without the map loaded (the trace's T and M
            lines): the camera on the egg, the hero gone.
    Original: Both follow: the race is the originals'.
    Guards: the bug kept as the original's, which the race scene pins.
    """
    table = scenes.intrinsics_table(game)
    classes = {0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: driver(table, apart)}),
               0x900 + TRIGGER_EGG_TYPE: ('TRIGEGG', {7: trigger_code(table)}),
               0x900 + CAMERA_TYPE: ('CAMERA', {7: camera_code(table)})}
    return scenes.Scene(name=name, maps=maps(), classes=classes, arguments=scenes.START,
                        frames=80 + 3 * apart + 130 + 60, tests=['in, out and in', 'teleport'],
                        ran=lambda record, tests: [scenes.marked(game)(record, tests)[0],
                                                   teleported(record, race=apart == 1)])


def slow(game):
    """The egg race's moves 20 passes apart: the trigger done and its byte
    back, each entry runs the trigger egg's own class, and the teleport
    loads map 101.

    Checks: the egg's class back to its own after the trigger, and the
        teleport with the camera following the Silencer (the map loaded).
    Drives: the egg-race scene with its moves 20 passes apart.
    Tests:
        in, out and in
            The three entries: the driver's marker after them.
        teleport
            Put on map 101 with the map loaded (the trace's T and M lines).
    Original: Both follow.
    """
    return dataclasses.replace(scene(game, 20, 'egg-race-slow'))


SCENES = {'egg-race': scene, 'egg-race-slow': slow}
