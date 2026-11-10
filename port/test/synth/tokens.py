"""The interpreter's scene: the
tokens of INTER.C, each in a test of its own a pass apart, their results in
the usecode's globals (word and long arithmetic, comparisons, locals and
globals through pointers, jumps, strings and lists in the YAMM heap, calls
and their returns, processes spawned and waited for, the item searches of
the loops). Not here: CALL_NEAR (its return releases the function's code
the caller still runs), SPAWN_INLINE, AND_IMPLIES, OR_IMPLIES and
PUSH_STACK_ADDRESS, which the games' usecode never has."""
import scenes
import ucasm

CLASS = 0x900 + scenes.TEST_EGG_TYPE
# The driver's locals, from its frame's base: the test items, words, a
# long, strings, lists, a string list, 6 bytes, the loops' element, item
# and counter.
LOCALS = {'I': -2, 'J': -4, 'C': -6, 'K': -8, 'T1': -10, 'T2': -12, 'LONG': -16, 'S1': -18, 'S2': -20,
          'L1': -22, 'L2': -24, 'SL': -26, 'HUGE': -32, 'F': -34, 'FOUND': -36, 'COUNT': -38}
FRAME = 38
# The type the driver's process takes (as the games' events do), which the
# excluded process takes too.
TYPE = 524
# The search images' sizes on the stack (usecode/usesearch.cpp) and the
# local's offset word over them, which a loop's end drops.
SEARCH_SIZES = {2: 0x38, 3: 0x38, 4: 0x28, 5: 0x2a, 6: 0x41}
# Loop scripts' tokens (item/scrit.h).
END, INT, EQUAL, SHAPE, FAMILY, AND, NOT = ord('$'), ord('%'), ord('='), ord('@'), ord(':'), ord('&'), ord('!')
# The other script tokens (item/scrit.h), and the lists of shapes and of
# frames, whose token is SHAPE or FRAME_OF and the list's length.
OR, GREATER, LESS, GEQUAL, LEQUAL = ord('+'), ord('>'), ord('<'), ord(')'), ord('(')
NPCNUM, STATUS, QUALITY, FRAME_OF, TRUE, FALSE = ord('#'), ord('?'), ord('*'), ord('`'), 1, 0


class Driver:
    """The tests' code: each test's lines, a SUSPEND and the marker after
    it, and the globals' room for the results."""

    def __init__(self, table):
        self.table = table
        self.lines = []
        self.names = []
        self.offset = 0
        self.labels = 0

    def slot(self, size=4):
        at = self.offset
        self.offset += (size + 3) // 4 * 4
        return at

    def label(self):
        self.labels += 1
        return f'l{self.labels}'

    def test(self, name, lines):
        """A test: its lines, then the marker (the egg's frame, an
        intrinsic the trace shows) and a pass's pause."""
        self.names.append(name)
        self.lines += lines + scenes.marker(self.table, len(self.names) - 1) + ['SUSPEND']

    def store(self, size=2):
        """Pops a result of size bytes into a slot of its own."""
        return [f'POP_GLOBAL {self.slot(size)} {size}']


def script(*tokens):
    """A loop script's tokens pushed for OP_LOOP, which reads them from the
    stack's top: the last pushed first. A token is a byte or ('w', word).
    Returns the lines and the script's bytes."""
    lines, size = [], 0
    for token in reversed(tokens):
        if isinstance(token, tuple):
            lines.append(f'PUSH_WORD {token[1]}')
            size += 2
        else:
            lines.append(f'LOOP_SCRIPT {token}')
            size += 1
    return lines, size


def loop(d, kind, center, rest, script_tokens):
    """A count of the items an OP_LOOP of kind finds with the script; center
    and rest push its two words (the item, and the area's range or the
    container)."""
    top, end = d.label(), d.label()
    pushed, size = script(*script_tokens, END)
    return (['PUSH_WORD 0', f'POP_LOCAL_WORD {LOCALS["COUNT"]}'] + pushed +
            [rest, center, f'LOOP {LOCALS["FOUND"]} {size} {kind}', f'{top}:',
             f'JUMP_IF_FALSE {end}', f'PUSH_LOCAL_WORD {LOCALS["COUNT"]}', 'PUSH_WORD 1', 'ADD',
             f'POP_LOCAL_WORD {LOCALS["COUNT"]}', 'LOOP_NEXT', f'JUMP {top}', f'{end}:',
             f'MOVE_TOP {-(SEARCH_SIZES[kind] + 2)}', f'PUSH_LOCAL_WORD {LOCALS["COUNT"]}'] + d.store())


def foreach(d, token, list_local, element, push):
    """The sum (or count) of a list's elements over FOREACH_LIST or
    FOREACH_STRING_LIST; push copies the list onto the stack."""
    top, end = d.label(), d.label()
    add = [f'PUSH_LOCAL_WORD {LOCALS[element]}'] if token == 'FOREACH_LIST' else ['PUSH_WORD 1']
    return (['PUSH_WORD 0', f'POP_LOCAL_WORD {LOCALS["COUNT"]}', push, 'PUSH_WORD -1', f'{top}:',
             f'{token} {LOCALS[element]} 2 {end}', f'PUSH_LOCAL_WORD {LOCALS["COUNT"]}'] + add +
            ['ADD', f'POP_LOCAL_WORD {LOCALS["COUNT"]}', f'JUMP {top}', f'{end}:',
             f'PUSH_LOCAL_WORD {LOCALS["COUNT"]}'] + d.store())


def arithmetic(d):
    words = [('ADD', 30000, 10000), ('SUB', -5, 7), ('MUL', 300, 300), ('DIV', -7, 2), ('DIV', 5, 0),
             ('MOD', -7, 3), ('MOD', 5, 0), ('BIT_AND', 0x0ff0, 0x3c3c), ('BIT_OR', 0x0ff0, 0x3c3c),
             ('SHIFT_LEFT', 0x1234, 4), ('SHIFT_RIGHT', -32768, 3), ('SHIFT_RIGHT', 0x7ff0, 3), ('EQ', 5, 5),
             ('NE', 5, 6), ('LT', -1, 1), ('LE', 3, 3), ('GT', -1, 1), ('GE', -2, -2), ('AND', 0, 5), ('AND', 3, 5),
             ('OR', 0, 5), ('OR', 0, 0)]
    for token, a, b in words:
        d.test(f'{token} {a} {b}', [f'PUSH_WORD {a}', f'PUSH_WORD {b}', token] + d.store())
    for token, a in (('NOT', 0), ('NOT', 7), ('BIT_NOT', 0x1234)):
        d.test(f'{token} {a}', [f'PUSH_WORD {a}', token] + d.store())
    longs = [('ADD_LONG', 0x7fffffff, 1), ('SUB_LONG', 0, 1), ('MUL_LONG', 100000, 100000),
             ('DIV_LONG', -2**31, -1), ('DIV_LONG', 7, 0), ('DIV_LONG', -7, 2), ('MOD_LONG', -7, 3),
             ('MOD_LONG', 7, -1), ('MOD_LONG', 7, 0)]
    for token, a, b in longs:
        d.test(f'{token} {a} {b}', [f'PUSH_LONG {a}', f'PUSH_LONG {b}', token] + d.store(4))
    compares = [('EQ_LONG', 70000, 70000), ('NE_LONG', 70000, 70001), ('LT_LONG', -1, 0), ('LE_LONG', 5, 5),
                ('GT_LONG', 70000, -70000), ('GE_LONG', -3, -2), ('AND_LONG', 0, 70000), ('AND_LONG', 1, 70000),
                ('OR_LONG', 0, 0), ('OR_LONG', 0, 70000)]
    for token, a, b in compares:
        d.test(f'{token} {a} {b}', [f'PUSH_LONG {a}', f'PUSH_LONG {b}', token] + d.store())
    d.test('NOT_LONG', ['PUSH_LONG 0', 'NOT_LONG'] + d.store())
    d.test('WORD_TO_LONG', ['PUSH_WORD -5', 'WORD_TO_LONG'] + d.store(4))
    d.test('LONG_TO_WORD', ['PUSH_LONG 0x12345678', 'LONG_TO_WORD'] + d.store())
    # EQ_HUGE compares the two values on the stack and leaves them there.
    d.test('EQ_HUGE', ['PUSH_LONG 0x11223344', 'PUSH_LONG 0x11223344', 'EQ_HUGE 4'] + d.store() + ['MOVE_TOP -8'])
    d.test('PUSH_BYTE', ['PUSH_BYTE -3'] + d.store())


def locals_and_globals(d):
    t1, t2, long_, huge = LOCALS['T1'], LOCALS['T2'], LOCALS['LONG'], LOCALS['HUGE']
    d.test('POP_LOCAL_BYTE', ['PUSH_WORD 0', f'POP_LOCAL_WORD {t1}', 'PUSH_WORD 0x1234', f'POP_LOCAL_BYTE {t1}',
                              f'PUSH_LOCAL_WORD {t1}'] + d.store() + [f'PUSH_LOCAL_BYTE {t1}'] + d.store())
    d.test('POP_LOCAL_LONG', ['PUSH_LONG 0x12345678', f'POP_LOCAL_LONG {long_}', f'PUSH_LOCAL_LONG {long_}'] +
           d.store(4) + [f'PUSH_LOCAL_WORD {long_}'] + d.store())
    d.test('ASSIGN_LOCAL_HUGE', ['PUSH_LONG 0x11223344', 'PUSH_WORD 0x5566', f'ASSIGN_LOCAL_HUGE {huge} 6',
                                 'MOVE_TOP -6', f'PUSH_LOCAL_HUGE {huge} 6'] + d.store(6))
    d.test('PUSH_INDIRECT', ['PUSH_WORD 0x4321', f'POP_LOCAL_WORD {t2}', f'PUSH_LOCAL_ADDRESS {t2}',
                             'PUSH_INDIRECT 2'] + d.store())
    d.test('POP_INDIRECT', ['PUSH_WORD 0x0f0f', f'PUSH_LOCAL_ADDRESS {t2}', 'POP_INDIRECT 2',
                            f'PUSH_LOCAL_WORD {t2}'] + d.store())
    before = d.offset
    d.test('PUSH_GLOBAL', ['PUSH_WORD 0x1357'] + d.store() + [f'PUSH_GLOBAL {before} 2'] + d.store() +
           [f'PUSH_GLOBAL {before} 1'] + d.store() + [f'PUSH_GLOBAL {before} 4'] + d.store(4))
    d.test('PUSH_GLOBAL_ADDRESS', [f'PUSH_GLOBAL_ADDRESS {before}', 'PUSH_INDIRECT 2'] + d.store())
    taken, skipped, end = d.label(), d.label(), d.label()
    d.test('JUMP_IF_FALSE', ['PUSH_WORD 0', f'JUMP_IF_FALSE {taken}', 'PUSH_WORD 111'] + d.store() +
           [f'JUMP {end}', f'{taken}:', 'PUSH_WORD 222'] + d.store() + [f'{end}:', 'PUSH_WORD 1',
                                                                         f'JUMP_IF_FALSE {skipped}', 'PUSH_WORD 333'] +
           d.store() + [f'{skipped}:'])
    d.test('DEBUG', ['DEBUG_LINE 10', 'DEBUG_FUNCTION 0 SYNTEGG'])


def strings(d):
    s1, s2 = LOCALS['S1'], LOCALS['S2']
    d.test('PUSH_STRING CONCAT EQ_STRING', ['PUSH_STRING "hello"', f'POP_LOCAL_WORD {s1}', f'PUSH_LOCAL_STRING {s1}',
                                            'PUSH_STRING " world"', 'CONCAT', f'POP_LOCAL_WORD {s2}',
                                            f'PUSH_LOCAL_STRING {s2}', 'PUSH_STRING "hello world"', 'EQ_STRING'] +
           d.store() + ['PUSH_STRING "a"', 'PUSH_STRING "b"', 'EQ_STRING'] + d.store())
    d.test('LOCAL_STRING_TO_POINTER', [f'LOCAL_STRING_TO_POINTER {s2}', 'PUSH_INDIRECT 2'] + d.store())
    d.test('STRING_TO_POINTER POINTER_TO_STRING COPY_STRING',
           [f'PUSH_LOCAL_WORD {s2}', 'STRING_TO_POINTER', 'POINTER_TO_STRING', 'COPY_STRING', 'STRING_TO_POINTER',
            'PUSH_INDIRECT 2'] + d.store())
    d.test('FREE_STRING', ['PUSH_STRING "drop"', 'FREE_STRING 0', 'MOVE_TOP -2', f'FREE_LOCAL_STRING {s1}',
                           'GIVE_LOCAL ' + str(s2) + ' 1'])


def lists(d):
    l1, l2, sl = LOCALS['L1'], LOCALS['L2'], LOCALS['SL']
    d.test('CREATE_LIST IN_LIST', ['PUSH_WORD 30', 'PUSH_WORD 20', 'PUSH_WORD 10', 'CREATE_LIST 2 3',
                                   f'POP_LOCAL_WORD {l1}', 'PUSH_WORD 20', f'PUSH_LOCAL_LIST {l1} 2', 'IN_LIST 2 0'] +
           d.store() + ['PUSH_WORD 25', f'PUSH_LOCAL_LIST {l1} 2', 'IN_LIST 2 0'] + d.store())
    d.test('PUSH_ELEMENT', [f'PUSH_LOCAL_LIST {l1} 2', 'PUSH_WORD 2', 'PUSH_ELEMENT 2 2'] + d.store() +
           [f'PUSH_LOCAL_LIST {l1} 2', 'PUSH_WORD 9', 'PUSH_ELEMENT 2 2'] + d.store())
    d.test('ASSIGN_ELEMENT', ['PUSH_WORD 99', 'PUSH_WORD 1', f'ASSIGN_ELEMENT {l1} 2 0', 'MOVE_TOP -2'])
    d.test('FOREACH_LIST', foreach(d, 'FOREACH_LIST', l1, 'F', f'PUSH_LOCAL_LIST {l1} 2'))
    d.test('APPEND', [f'PUSH_LOCAL_LIST {l1} 2', 'PUSH_WORD 50', 'PUSH_WORD 40', 'CREATE_LIST 2 2', 'APPEND',
                      f'POP_LOCAL_WORD {l2}'] + foreach(d, 'FOREACH_LIST', l2, 'F', f'PUSH_LOCAL_LIST {l2} 2'))
    d.test('UNION_LIST', [f'PUSH_LOCAL_LIST {l2} 2', 'PUSH_WORD 60', 'PUSH_WORD 10', 'CREATE_LIST 2 2', 'UNION_LIST 2',
                          f'FREE_LOCAL_LIST {l2}', f'POP_LOCAL_WORD {l2}'] +
           foreach(d, 'FOREACH_LIST', l2, 'F', f'PUSH_LOCAL_LIST {l2} 2'))
    d.test('REMOVE_LIST', [f'PUSH_LOCAL_LIST {l2} 2', 'PUSH_WORD 99', 'PUSH_WORD 40', 'CREATE_LIST 2 2',
                           'REMOVE_LIST 2', f'FREE_LOCAL_LIST {l2}', f'POP_LOCAL_WORD {l2}'] +
           foreach(d, 'FOREACH_LIST', l2, 'F', f'PUSH_LOCAL_LIST {l2} 2'))
    d.test('FREE_LIST', [f'PUSH_LOCAL_LIST {l2} 2', 'FREE_LIST 0', 'MOVE_TOP -2', f'GIVE_LOCAL {l1} 3',
                         f'FREE_LOCAL_LIST {l2}'])
    d.test('string lists', ['PUSH_STRING "beta"', 'PUSH_STRING "alpha"', 'CREATE_LIST 2 2', f'POP_LOCAL_WORD {sl}',
                            f'PUSH_LOCAL_STRING_LIST {sl}', 'PUSH_STRING "gamma"', 'CREATE_LIST 2 1',
                            'UNION_STRING_LIST 2', f'FREE_LOCAL_STRING_LIST {sl}', f'POP_LOCAL_WORD {sl}'] +
           foreach(d, 'FOREACH_STRING_LIST', sl, 'F', f'PUSH_LOCAL_STRING_LIST {sl}'))
    d.test('REMOVE_STRING_LIST', [f'PUSH_LOCAL_STRING_LIST {sl}', 'PUSH_STRING "alpha"', 'CREATE_LIST 2 1',
                                  'REMOVE_STRING_LIST 2', f'FREE_LOCAL_STRING_LIST {sl}', f'POP_LOCAL_WORD {sl}'] +
           foreach(d, 'FOREACH_STRING_LIST', sl, 'F', f'PUSH_LOCAL_STRING_LIST {sl}') +
           ['PUSH_STRING "gamma"', f'PUSH_LOCAL_STRING_LIST {sl}', 'IN_LIST 2 1'] + d.store())
    d.test('FREE_STRING_LIST', [f'PUSH_LOCAL_STRING_LIST {sl}', 'FREE_STRING_LIST 0', 'MOVE_TOP -2',
                                f'GIVE_LOCAL {sl} 2', f'FREE_LOCAL_STRING_LIST {sl}', f'FREE_LOCAL_LIST {l1}'])


def calls(d):
    d.test('CALL', ['PUSH_WORD 50', 'PUSH_WORD 8', f'CALL {CLASS} 32', 'MOVE_TOP -4', 'PUSH_RETURNED_WORD'] +
           d.store())
    d.test('POP_RETURNED_LONG', [f'CALL {CLASS} 33', 'PUSH_RETURNED_LONG'] + d.store(4))
    d.test('recursion', ['PUSH_WORD 6', f'CALL {CLASS} 34', 'MOVE_TOP -2', 'PUSH_RETURNED_WORD'] + d.store())
    d.test('POP_RESULT', ['PUSH_LONG 0x0badbeef', 'POP_RESULT', 'PUSH_RESULT'] + d.store(4))


def processes(d, spawned_slots, excluded_slot):
    # A process spawned with a word for it, waited for: our pid under its pid.
    d.test('SPAWN', ['PUSH_PID', 'PUSH_WORD 14', 'PUSH_LONG 0', f'SPAWN 2 0 {CLASS} 35', 'MOVE_TOP -2',
                     'PUSH_RETURNED_WORD', 'IMPLIES 1 1', 'POP_RETURNED_WORD', 'SUSPEND',
                     f'PUSH_GLOBAL {spawned_slots[1]} 2'] + d.store())
    # A process of the egg with the driver's type: EXCLUDE ends it.
    d.test('EXCLUDE', ['PUSH_LOCAL_LONG 6', f'SPAWN 0 2 {CLASS} 36', 'PUSH_RETURNED_WORD'] + d.store() +
           [f'PUSH_GLOBAL {excluded_slot} 2'] + d.store())


def loops(d):
    """The searches of OP_LOOP: around the Silencer, recursive, by family,
    with NOT, and in the container; then each other token of the scripts
    and the lists of shapes and of frames."""
    item_shape = (INT, ('w', scenes.ITEM), SHAPE, EQUAL)
    silencer, area = 'PUSH_WORD 1', 'PUSH_WORD 512'
    container = f'PUSH_LOCAL_WORD {LOCALS["C"]}'
    d.test('LOOP area', loop(d, 2, silencer, area, item_shape))
    d.test('LOOP recursive area', loop(d, 3, silencer, area, item_shape))
    d.test('LOOP family', loop(d, 2, silencer, area, (INT, ('w', 6), FAMILY, EQUAL)))
    d.test('LOOP not', loop(d, 2, silencer, area, item_shape + (NOT,)))
    d.test('LOOP and', loop(d, 2, silencer, area, item_shape + (INT, ('w', 6), FAMILY, EQUAL, NOT, AND)))
    d.test('LOOP container', loop(d, 4, silencer, container, item_shape))
    d.test('LOOP recursive container', loop(d, 5, silencer, container, item_shape))
    # the other tokens (ItemSearch::matches), around the Silencer
    others = {'greater': (SHAPE, INT, ('w', 300), GREATER), 'less': (SHAPE, INT, ('w', 302), LESS),
              'gequal': (SHAPE, INT, ('w', 301), GEQUAL), 'lequal': (QUALITY, INT, ('w', 0), LEQUAL),
              'or': item_shape + (INT, ('w', 6), FAMILY, EQUAL, OR), 'status': (STATUS, INT, ('w', 0), GREATER),
              'npcnum': (NPCNUM, INT, ('w', 1), EQUAL), 'true': (TRUE,), 'false': (FALSE,),
              'frame': (FRAME_OF, INT, ('w', 1), EQUAL),
              'shape list': (SHAPE + 2, ('w', scenes.ITEM), ('w', scenes.CONTAINER)),
              'frame list': (FRAME_OF + 2, ('w', 0), ('w', 2))}
    for name, tokens in others.items():
        d.test(f'LOOP {name}', loop(d, 2, silencer, area, tokens))


# The functions beyond the events: 32 a difference of its two words, 33 a
# long, 34 a factorial by recursion, 35 a spawned process that waits two
# passes, 36 a process EXCLUDE ends.
def helpers(spawned_slots, excluded_slot):
    return {
        32: 'ENTER 0\nPUSH_LOCAL_WORD 8\nPUSH_LOCAL_WORD 6\nSUB\nPOP_RETURNED_WORD\nRETURN',
        33: 'ENTER 0\nPUSH_LONG 0x01020304\nPOP_RETURNED_LONG\nRETURN',
        34: f"""ENTER 0
            PUSH_LOCAL_WORD 6
            PUSH_WORD 1
            LE
            JUMP_IF_FALSE deeper
            PUSH_WORD 1
            POP_RETURNED_WORD
            RETURN
            deeper:
            PUSH_LOCAL_WORD 6
            PUSH_WORD 1
            SUB
            CALL {CLASS} 34
            MOVE_TOP -2
            PUSH_RETURNED_WORD
            PUSH_LOCAL_WORD 6
            MUL
            POP_RETURNED_WORD
            RETURN""",
        35: f"""ENTER 0
            PUSH_LOCAL_WORD 6
            PUSH_WORD 3
            MUL
            POP_GLOBAL {spawned_slots[0]} 2
            SUSPEND
            SUSPEND
            PUSH_WORD 1
            POP_GLOBAL {spawned_slots[1]} 2
            RETURN""",
        36: f"""ENTER 0
            PUSH_WORD {TYPE}
            PUSH_LOCAL_LONG 6
            PUSH_INDIRECT 2
            SET_INFO
            EXCLUDE
            PUSH_WORD 1
            POP_GLOBAL {excluded_slot} 2
            RETURN""",
    }


def driver(game):
    """Event 7's code and the tests' names: the process's type and item as
    the games' events take them, the test items (two on the floor, a
    container with one in it), then the tests."""
    table = scenes.intrinsics_table(game)
    d = Driver(table)
    spawned_slots = (d.slot(), d.slot())
    excluded_slot = d.slot()
    setup = [f'ENTER {FRAME}', f'PUSH_WORD {TYPE}', 'PUSH_LOCAL_LONG 6', 'PUSH_INDIRECT 2', 'SET_INFO', 'EXCLUDE']
    for local, shape, frame, x, y in (('I', scenes.ITEM, 0, 16448, 16384), ('J', scenes.ITEM, 1, 16448, 16448),
                                      ('C', scenes.CONTAINER, 0, 16384, 16448)):
        setup += scenes.call(table, '@Item@create$qusus', f'{local} {shape} {frame}', local_offsets=LOCALS)
        setup += scenes.call(table, '@Item@popToCoords$qususuc', f'{local} {x} {y} 0', local_offsets=LOCALS)
    setup += scenes.call(table, '@Item@create$qusus', f'K {scenes.ITEM} 2', local_offsets=LOCALS)
    setup += scenes.call(table, '@Item@popToContainer$qus', 'K c', local_offsets=LOCALS)
    setup += ['SUSPEND']
    arithmetic(d)
    locals_and_globals(d)
    strings(d)
    lists(d)
    calls(d)
    processes(d, spawned_slots, excluded_slot)
    loops(d)
    code = '\n'.join(setup + d.lines + ['RETURN'])
    functions = {7: ucasm.assemble(code)}
    for number, text in helpers(spawned_slots, excluded_slot).items():
        functions[number] = ucasm.assemble(text)
    return functions, d.names


def scene(game):
    """The interpreter's tokens, each in a test of its own a pass apart.

    Checks: INTER.C's tokens (usecode/inter.cpp, yamm.cpp, usesearch.cpp):
        word and long arithmetic and comparisons, locals and globals
        through pointers, jumps, strings and lists in the YAMM heap, calls
        and returns, recursion, processes spawned and waited for, EXCLUDE,
        and the item searches of OP_LOOP with each script token.
    Drives: the test egg's event 7 (driver): two items and a container with
        one in it, then the tests; functions 32-36 beside it (helpers);
        each result into a slot of the globals (the trace's G lines), each
        test ended by its marker (its footprint).
    Tests:
        LOOP <search>
            An OP_LOOP search (area, recursive, family, not, and, the
            container, the script tokens, the lists of shapes and frames):
            the count of the items it finds in the globals.
        recursion
            A factorial by CALL to itself.
        string lists
            UNION_STRING_LIST and FOREACH_STRING_LIST.
        <token> <n> <n>
            The token on those two numbers, its result in the globals.
        <token> <n>
            The token on that number, its result in the globals.
        <tokens>
            The tokens named, their results in the globals.
    Original: Both follow.
    Guards: not here, as the games' usecode never has them: CALL_NEAR (its
        return frees the code the caller runs), SPAWN_INLINE, AND_IMPLIES,
        OR_IMPLIES and PUSH_STACK_ADDRESS.
    """
    functions, names = driver(game)
    return scenes.Scene(name='tokens', maps={scenes.TEST_MAP: scenes.floor_and_egg()},
                        classes={CLASS: ('SYNTEGG', functions)}, arguments=scenes.START, frames=400, tests=names,
                        ran=scenes.marked(game))
