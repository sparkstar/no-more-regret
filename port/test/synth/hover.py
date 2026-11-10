"""A process that outlives the map it began on (by which a wall of No
Regret's map 14 goes, and the Silencer can walk off the world there): No
Regret's HOVER (class 0A0Bh) flies a hover along its markers. Its route
(function 21h) keeps the hover's item number in a local, and its process
has no item (SET_INFO gives it none), so nothing kills it when the
Silencer's teleport changes the map: Npc::teleport loads the new map, and
killAll runs only on a map jump. On the new map the number is another
item's. When the route wakes and finds the hover's moving flag (1000h)
gone, it puts the hover back at its marker (function 23h), and with it
whatever stands on its surface (LOOP_SURFACE): on map 14, a wall.

hover-teleport: the test egg's usecode makes an item, the hover, and spawns
the route, a function of a class standing for HOVER, which marks the hover
moving and waits; the Silencer is teleported to map 101, where the number
goes to a platform tile with a wall on it. The route wakes, finds the mark
gone and moves the platform and the wall with it. That is the originals'
bug, not fixed yet: the scene pins it (both originals follow), and its
footprints turn when it is fixed."""
import formats
import scenes
import ucasm

X0, Y0 = scenes.X0, scenes.Y0
OTHER_MAP = 101
# The class standing for HOVER, and its route's function.
HOVER_CLASS, ROUTE = 0x0a0b, 0x20
# The hover, made on the test map.
HOVER_AT = (X0 + 200, Y0 + 200)
# Map 101's floor of 8 by 8 tiles, and the platform tile in the place of the
# one that gets the hover's number (the third of the third row: the free
# numbers' order, from those of the test map's items), with a wall on its
# surface (its top, 48 up; the wall's box from 63 short of its place).
PLATFORM_TILE = 2 * 8 + 2
PLATFORM_AT = (X0 - 256 + 64 * 2 + 63, Y0 - 256 + 64 * 2 + 63)
WALL_AT = (PLATFORM_AT[0] + 32, PLATFORM_AT[1], 48)
# How far the route's return moves the hover and what is on it (HOVER's to
# its marker).
DX, DY = -400, 200
MOVING = 0x1000
# The passes the route waits, over the teleport and the map's load.
WAIT = 120
# The globals the route writes: the shape of the item its number names after
# the teleport, that item's flags, how many items stood on it, the last one's
# shape and its x after the move.
RESULTS = 220
SHAPE_AFTER, FLAGS_AFTER, ON_IT, ON_SHAPE, ON_X = (RESULTS + 2 * k for k in range(5))
DRIVER_LOCALS = {'S': -2, 'H': -4}
ROUTE_LOCALS = {'H': -2, 'L': -4}


def moved_by(table, local, dx, dy):
    """The item in local pushed and put back dx and dy further (as HOVER's
    return does with each one)."""
    def get(name):
        number, size, _ = table[name]
        return [f'PUSH_LOCAL_ADDRESS {local}', f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP -{size}']
    number, size, _ = table['@Item@push$qv']
    lines = [f'PUSH_LOCAL_ADDRESS {local}', f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP -{size}']
    lines += get('@Item@z$qv') + ['PUSH_RETURNED_BYTE']
    lines += get('@Item@y$qv') + ['PUSH_RETURNED_WORD', f'PUSH_WORD {dy}', 'ADD']
    lines += get('@Item@x$qv') + ['PUSH_RETURNED_WORD', f'PUSH_WORD {dx}', 'ADD']
    number, size, _ = table['@Item@popToCoords$qususuc']
    return lines + [f'PUSH_LOCAL_ADDRESS {local}', f'CALL_INTRINSIC {size} {number}', f'MOVE_TOP -{size}']


def route(table):
    """The route (function 20h): no item of its own, the hover marked
    moving, a wait over the teleport; then the item its number names (its
    shape and flags into the globals) put back, with what is on its
    surface, when the mark is gone."""
    call = lambda name, arguments, result=None: scenes.call(table, name, arguments, result=result,
                                                            local_offsets=ROUTE_LOCALS)
    flags, size, _ = table['@Item@flags$qv']
    shape, _, _ = table['@Item@shape$qv']
    x, _, _ = table['@Item@x$qv']
    lines = ['ENTER 4', 'PUSH_BYTE 1', 'PUSH_BYTE 0', 'SET_INFO', 'PUSH_LOCAL_WORD 10',
             f'POP_LOCAL_WORD {ROUTE_LOCALS["H"]}']
    lines += call('@Item@orFlags$qus', f'H {MOVING}') + ['SUSPEND'] * WAIT
    lines += call('@Item@shape$qv', 'H', SHAPE_AFTER) + call('@Item@flags$qv', 'H', FLAGS_AFTER)
    lines += [f'PUSH_LOCAL_ADDRESS {ROUTE_LOCALS["H"]}', f'CALL_INTRINSIC {size} {flags}', f'MOVE_TOP -{size}',
              'PUSH_RETURNED_WORD', f'PUSH_WORD {MOVING}', 'BIT_AND', 'PUSH_BYTE 0', 'EQ', 'JUMP_IF_FALSE end']
    # Each item on its surface (a surface loop of all items, -1 on the other
    # side, as HOVER's), counted and moved, then the item itself.
    lines += ['LOOP_SCRIPT 0x24', 'PUSH_WORD -1', f'PUSH_LOCAL_WORD {ROUTE_LOCALS["H"]}',
              f'LOOP {ROUTE_LOCALS["L"]} 1 6', 'next:', 'JUMP_IF_FALSE done']
    lines += [f'PUSH_GLOBAL {ON_IT} 2', 'PUSH_WORD 1', 'ADD', f'POP_GLOBAL {ON_IT} 2']
    lines += [f'PUSH_LOCAL_ADDRESS {ROUTE_LOCALS["L"]}', f'CALL_INTRINSIC {size} {shape}', f'MOVE_TOP -{size}',
              'PUSH_RETURNED_WORD', f'POP_GLOBAL {ON_SHAPE} 2']
    lines += moved_by(table, ROUTE_LOCALS['L'], DX, DY)
    lines += [f'PUSH_LOCAL_ADDRESS {ROUTE_LOCALS["L"]}', f'CALL_INTRINSIC {size} {x}', f'MOVE_TOP -{size}',
              'PUSH_RETURNED_WORD', f'POP_GLOBAL {ON_X} 2']
    lines += ['LOOP_NEXT', 'JUMP next', 'done:', 'MOVE_TOP -67']
    lines += moved_by(table, ROUTE_LOCALS['H'], DX, DY)
    return ucasm.assemble('\n'.join(lines + ['end:', 'RETURN']))


def driver(table):
    """The test egg's entry: the hover made and its route spawned (the
    hover's number its argument, as HOVER's), then the Silencer teleported
    to map 101."""
    call = lambda name, arguments: scenes.call(table, name, arguments, local_offsets=DRIVER_LOCALS)
    lines = ['ENTER 4', 'PUSH_WORD 1', f'POP_LOCAL_WORD {DRIVER_LOCALS["S"]}'] + ['SUSPEND'] * 30
    lines += call('@Item@create$qusus', f'H {scenes.ITEM} 0') + call('@Item@popToCoords$qususuc',
                                                                     f'H {HOVER_AT[0]} {HOVER_AT[1]} 0')
    lines += [f'PUSH_LOCAL_WORD {DRIVER_LOCALS["H"]}', 'PUSH_LOCAL_LONG 6', f'SPAWN 2 2 {HOVER_CLASS} {ROUTE}']
    lines += scenes.marker(table, 0) + ['SUSPEND'] * 30
    lines += call('@Npc@teleport$qususucuc', f'S {X0} {Y0} 0 {OTHER_MAP}') + ['SUSPEND'] * (WAIT + 30)
    return ucasm.assemble('\n'.join(lines + ['RETURN']))


def maps():
    there = [formats.item_record(X0 - 256 + 64 * i + 63, Y0 - 256 + 64 * j + 63, 0, scenes.FLOOR)
             for i in range(8) for j in range(8)]
    there[PLATFORM_TILE] = formats.item_record(*PLATFORM_AT, 0, scenes.PLATFORM)
    there += [formats.item_record(*WALL_AT, scenes.WALL_ALONG)]
    return {scenes.TEST_MAP: scenes.floor_and_egg(), OTHER_MAP: there}


def footprints(game, record, tests):
    word = record.global_value
    teleported = any(line[1] == 'T' and line[2] == '1' and line[-1] == str(OTHER_MAP) for line in record.trace)
    loaded = any(line[1] == 'M' and line[2] == str(OTHER_MAP) for line in record.trace)
    return [scenes.marked(game)(record, tests)[0], teleported and loaded,
            word(SHAPE_AFTER) == scenes.PLATFORM and word(FLAGS_AFTER) & MOVING == 0,
            word(ON_IT) == 1 and word(ON_SHAPE) == scenes.WALL_ALONG and word(ON_X) == WALL_AT[0] + DX]


def scene(game):
    """A process of no item outliving the map it began on, its item number
    another item's on the new map: the hover's route of No Regret's HOVER,
    which moves map 14's wall.

    Checks: SET_INFO (a usecode process of no item); Npc::teleport to
        another map, which loads it and kills no process; the item numbers
        the new map's items get (itemcach.cpp: the old map's numbers freed,
        the last freed given first); the surface loop (itemfind.cpp
        SurfaceSearch: what stands on an item's top, its box over the
        item's).
    Drives: the test egg's event 7 (driver): an item made (the hover) and
        the route spawned, function 20h of class 0A0Bh standing for HOVER
        (route), the Silencer teleported to map 101 (a floor, a platform
        tile and a wall on it). The route writes into the globals the
        shape and flags of the item its number names after the wait, and
        what stood on it and where it went.
    Tests:
        route
            The route spawned: the driver's marker after it.
        teleport
            The Silencer on map 101 with the map loaded (the trace's T and
            M lines).
        number reused
            The route's number names map 101's platform tile, its moving
            flag gone.
        surface moved
            The wall on the platform moved with it by the route's return:
            the bug, kept as the originals'.
    Original: Both follow: the bug is the originals' (the route's class is
        the scene's; HOVER's usecode is the games' own).
    Guards: the bug kept as the originals', which the scene pins: a fix turns
        its last two footprints.
    """
    table = scenes.intrinsics_table(game)
    classes = {0x900 + scenes.TEST_EGG_TYPE: ('SYNTEGG', {7: driver(table)}),
               HOVER_CLASS: ('HOVER', {ROUTE: route(table)})}
    tests = ['route', 'teleport', 'number reused', 'surface moved']
    return scenes.Scene(name='hover-teleport', maps=maps(), classes=classes, arguments=scenes.START,
                        frames=30 + 30 + WAIT + 120, tests=tests,
                        ran=lambda record, tests: footprints(game, record, tests))


SCENES = {'hover-teleport': scene}
