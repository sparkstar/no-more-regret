#!/usr/bin/env python3
"""The usecode's text (the game's text in other languages): every string
literal of EUSECODE.FLX (OP_PUSH_STRING) with its place, where its value
goes and the pieces it is joined with, as JSON for the translation's tools.

A literal's place is its class, function and the offset of its
OP_PUSH_STRING in the function's code: the id "CCCC:FFFF:OOOO" in hex,
which the interpreter knows when it runs the token (process->classId,
process->function and the offset from the function's code). Its hash is the
FNV-1a 64-bit hash of its bytes: the translation checks it before it
replaces the text, as another version of the usecode has other places.

Where the value goes is followed through the code by the tokens' stack
effects (inter.cpp): on the interpreter's stack, into locals and their
copies (OP_PUSH_LOCAL_STRING), into the callee's locals as an argument of a
CALL (local 6 + its depth, the this pointer pushed last) or a SPAWN (local
10 + its depth, local 6 being the pointer to the this copy; local 6 + its
depth when the this has no bytes, as UsecodeProcess pushes no pointer
then), and as what a function returns (OP_POP_RETURNED_WORD) to its
callers' PUSH_RETURNED_WORD.
It ends at an intrinsic (showComputerText: the terminal; Item::nothing,
which does nothing: not shown; the movies' names), a comparison, a list,
or the engine (an event function's result).

A terminal text's display is the terminal (displays.terminal: the gump, its
font, cells, columns, rows and waits), and its layout the pages and rows
the original shows it in (ComputerGump::setText's breaks, then typeText's
rows and pages; the rows hold the shown characters): the literal's own, and
the joined text's when it is joined with texts and numbers only (each
number at its widest, five characters).

The pieces are the OP_CONCAT chain the literal is in, in order: literals,
numbers (numberString of a local, a global or a constant), string locals
(text built up in a local) and other values. The conditions are those of
the branches the literal is in, written out; the case is the innermost
one's tests of a local against constants (the text functions, TEXTFIL1-3
and TEXTFILE 21h and 23h, pick a text by its number in local 12).

Where a terminal text is shown (shown) comes from the maps' items
(static/fixed.dat): the items of the text's class, or for a text picked by
number the terminals whose quality gives one of its numbers (see Places).

The output holds the game's English text, which is not ours to give: it
goes to build/text (outside the repository) by default.

--check writes nothing and fails (exit 1) when a literal's value goes
where the trace cannot follow it (a use "unknown: TOKEN"), has no use but
the one the original overwrites before it is read (TEXTFIL2's first "Bad
command or file name...", No Remorse), or a terminal's text number comes
from what the tool cannot write out (not a quality's high byte, or a link
search without its shape or range).

usage: usecode_text.py [--flx F] [-o OUT] [--check]
"""
import argparse, json, re, struct, sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402
import usecode as uc  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
NAMES = uc.token_names()

# The tokens' stack effects in bytes (popped, pushed), as inter.cpp's run
# does them. CALL_INTRINSIC and CALL leave their arguments (MOVE_TOP drops
# them); SPAWN pops the this pointer and leaves its arguments.
FIXED = {
    'PUSH_BYTE': (0, 2), 'PUSH_WORD': (0, 2), 'PUSH_LONG': (0, 4), 'PUSH_STRING': (0, 2),
    'PUSH_LOCAL_BYTE': (0, 2), 'PUSH_LOCAL_WORD': (0, 2), 'PUSH_LOCAL_LONG': (0, 4), 'PUSH_LOCAL_STRING': (0, 2),
    'PUSH_LOCAL_LIST': (0, 2), 'PUSH_LOCAL_STRING_LIST': (0, 2), 'PUSH_LOCAL_ADDRESS': (0, 4),
    'PUSH_GLOBAL_ADDRESS': (0, 4), 'PUSH_PID': (0, 2), 'PUSH_RETURNED_BYTE': (0, 2), 'PUSH_RETURNED_WORD': (0, 2),
    'PUSH_RETURNED_LONG': (0, 4), 'PUSH_RESULT': (0, 4), 'POP_RESULT': (4, 0), 'POP_LOCAL_BYTE': (2, 0),
    'POP_LOCAL_WORD': (2, 0), 'POP_LOCAL_LONG': (4, 0), 'POP_RETURNED_WORD': (2, 0), 'POP_RETURNED_LONG': (4, 0),
    'CONCAT': (4, 2), 'EQ_STRING': (4, 2), 'APPEND': (4, 2), 'NOT': (2, 2), 'BIT_NOT': (2, 2),
    'WORD_TO_LONG': (2, 4), 'LONG_TO_WORD': (4, 2), 'STRING_TO_POINTER': (2, 4), 'LOCAL_STRING_TO_POINTER': (0, 4),
    'SET_INFO': (4, 0), 'LOOP_SCRIPT': (0, 1), 'LOOP_NEXT': (0, 2), 'JUMP_IF_FALSE': (2, 0), 'ASSIGN_ELEMENT': (2, 0),
    'UNION_LIST': (4, 2), 'UNION_STRING_LIST': (4, 2), 'REMOVE_STRING_LIST': (4, 2), 'REMOVE_LIST': (4, 2),
    'SPAWN': (4, 0),
}
for _name in ('ADD', 'SUB', 'MUL', 'DIV', 'MOD', 'EQ', 'LT', 'LE', 'GT', 'GE', 'AND', 'OR', 'NE', 'BIT_AND',
              'BIT_OR', 'SHIFT_LEFT', 'SHIFT_RIGHT'):
    FIXED[_name] = (4, 2)
for _name in ('ADD_LONG', 'SUB_LONG', 'MUL_LONG', 'DIV_LONG', 'MOD_LONG', 'AND_LONG', 'OR_LONG'):
    FIXED[_name] = (8, 4)
for _name in ('EQ_LONG', 'LT_LONG', 'LE_LONG', 'GT_LONG', 'GE_LONG', 'NE_LONG'):
    FIXED[_name] = (8, 2)
for _name in ('CALL_INTRINSIC', 'CALL', 'ENTER', 'DEBUG_LINE', 'DEBUG_FUNCTION', 'SUSPEND', 'EXCLUDE', 'GIVE_LOCAL',
              'FREE_LOCAL_STRING', 'FREE_LOCAL_STRING_LIST', 'FREE_LOCAL_LIST', 'FREE_STRING', 'JUMP', 'RETURN'):
    FIXED[_name] = (0, 0)


# The debugger's tokens (No Remorse's usecode has them between statements).
DEBUG = ('DEBUG_LINE', 'DEBUG_FUNCTION')


def effect(ins):
    """(popped, pushed) bytes of an instruction; None for a token whose
    effect the trace does not model (LOOP: its search's size by kind)."""
    name, ops = NAMES[ins.op], ins.operands
    if name in FIXED:
        return FIXED[name]
    if name == 'CREATE_LIST':
        return ops[0] * ops[1], 2
    if name == 'IN_LIST':
        return 2 + ops[0], 2
    if name == 'PUSH_ELEMENT':
        return 4, 2 if ops[0] == 1 else ops[0]
    if name == 'PUSH_INDIRECT':
        return 4, ops[0]
    if name == 'POP_INDIRECT':
        return 4 + ops[0], 0
    if name == 'PUSH_GLOBAL':
        return 0, 2 if ops[1] == 1 else ops[1]
    if name == 'POP_GLOBAL':
        return 2 if ops[1] == 1 else ops[1], 0
    if name == 'MOVE_TOP':
        return (-ops[0], 0) if ops[0] < 0 else (0, ops[0])
    if name == 'IMPLIES':
        return 2 * ops[0] + 2, 2 * ops[0]
    return None


OPERATORS = {
    'ADD': '+', 'SUB': '-', 'MUL': '*', 'DIV': '/', 'MOD': '%', 'EQ': '==', 'NE': '!=', 'LT': '<', 'LE': '<=',
    'GT': '>', 'GE': '>=', 'AND': '&&', 'OR': '||', 'BIT_AND': '&', 'BIT_OR': '|', 'SHIFT_LEFT': '<<',
    'SHIFT_RIGHT': '>>', 'EQ_STRING': '==', 'ADD_LONG': '+', 'SUB_LONG': '-', 'MUL_LONG': '*', 'DIV_LONG': '/',
    'EQ_LONG': '==', 'NE_LONG': '!=', 'LT_LONG': '<', 'GT_LONG': '>',
}
LONG_RESULTS = ('ADD_LONG', 'SUB_LONG', 'MUL_LONG', 'DIV_LONG')


def top_values(stack, size):
    """The texts of the values in the top size bytes, first pushed first."""
    out, total = [], 0
    for text, width in reversed(stack):
        if total >= size:
            break
        out.append(text)
        total += width
    return list(reversed(out))


def drop(stack, size):
    total = 0
    while stack and total < size:
        total += stack.pop()[1]


def wrap(text):
    return f'({text})' if ' ' in text and not text.startswith("'") else text


def fnv1a64(data):
    value = 0xcbf29ce484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return value


def short_name(symbol):
    """'@Item@nothing$qv' as 'Item::nothing'."""
    return symbol.lstrip('@').split('$')[0].replace('@', '::') if symbol else ''


class Function:
    def __init__(self, class_id, function, code, file_offset):
        self.class_id, self.function, self.file_offset = class_id, function, file_offset
        self.ins = uc.decode(code)
        self.index = {i.offset: k for k, i in enumerate(self.ins)}

    def target(self, k):
        """The index a jump at k goes to."""
        i = self.ins[k]
        return self.index[i.offset + i.size + i.operands[-1]]


class Program:
    def __init__(self, path):
        self.usecode = uc.Usecode(path)
        self.intrinsics = {number: short_name(symbol) for number, symbol in uc.intrinsic_names().items()}
        self.functions = {}
        for c in self.usecode.classes():
            unit_offset = self.usecode.objects[c + 2][0]
            unit = self.usecode.unit(c)
            count, _, code_base = struct.unpack_from('<III', unit, 0)
            for f in range(count + 32):
                size, offset = struct.unpack_from('<HI', unit, 20 + 6 * f)
                if size and offset & 0xffff:
                    start = offset + code_base - 1
                    self.functions[c, f] = Function(c, f, unit[start:start + size], unit_offset + start)
        self.callers = defaultdict(list)
        for fn in self.functions.values():
            for k, i in enumerate(fn.ins):
                if NAMES[i.op] == 'CALL':
                    self.callers[tuple(i.operands)].append((fn, k))
        self.local_uses = {}
        self.returned_uses = {}

    def name(self, class_id):
        return self.usecode.names[class_id]

    def place(self, class_id, function):
        return f'{self.name(class_id)}:{function:04X}'

    # Following a value.
    def trace(self, fn, k, depth, size, locals_, path):
        """The uses of a value from instruction k on: on the stack at depth
        bytes from the top (None when it is only in locals) and size bytes
        wide (2 a string, 4 a pointer to its text), and in locals_. A set
        of (use, path)."""
        uses = set()
        seen = set()
        work = [(k, depth, size, frozenset(locals_), tuple(path))]
        while work:
            k, depth, size, locals_, path = work.pop()
            while k < len(fn.ins):
                state = (k, depth, size, locals_)
                if state in seen:
                    break
                seen.add(state)
                i = fn.ins[k]
                name, ops = NAMES[i.op], i.operands
                if name in ('PUSH_LOCAL_STRING', 'PUSH_LOCAL_WORD') and ops[0] in locals_:
                    work.append((k + 1, 0, 2, locals_, path + (f'copy of local {ops[0]}',)))
                elif name == 'LOCAL_STRING_TO_POINTER' and ops[0] in locals_:
                    work.append((k + 1, 0, 4, locals_, path + (f'pointer to local {ops[0]}',)))
                elif name == 'FREE_LOCAL_STRING' and ops[0] in locals_:
                    locals_ = locals_ - {ops[0]}
                elif name == 'POP_LOCAL_WORD' and depth != 0 and ops[0] in locals_:
                    locals_ = locals_ - {ops[0]}
                if name == 'RETURN':
                    break
                if name == 'JUMP':
                    k = fn.target(k)
                    continue
                if depth is None:
                    if name == 'JUMP_IF_FALSE':
                        work.append((fn.target(k), None, size, locals_, path))
                    elif name in ('FOREACH_LIST', 'FOREACH_STRING_LIST'):
                        work.append((fn.target(k), None, size, locals_, path))
                    if not locals_:
                        break
                    k += 1
                    continue
                # The value is on the stack.
                if name == 'CALL_INTRINSIC':
                    if depth < ops[0]:
                        uses.add((f'intrinsic {self.intrinsics.get(ops[1], hex(ops[1]))}', path))
                elif name == 'CALL':
                    nxt = fn.ins[k + 1] if k + 1 < len(fn.ins) else None
                    arguments = -nxt.operands[0] if nxt and NAMES[nxt.op] == 'MOVE_TOP' and nxt.operands[0] < 0 else 0
                    if depth < arguments and size == 2:
                        callee = tuple(ops)
                        step = f'argument of {self.place(*callee)} (local {6 + depth})'
                        for use, inner in self.local(callee, 6 + depth):
                            uses.add((use, path + (step,) + inner))
                elif name == 'SPAWN':
                    if depth < 4:
                        uses.add(('unknown: the this pointer of a SPAWN', path))
                        break
                    depth -= 4
                    if depth < ops[0] and size == 2:
                        callee = (ops[2], ops[3])
                        local = (10 if ops[1] else 6) + depth
                        step = f'argument of spawned {self.place(*callee)} (local {local})'
                        for use, inner in self.local(callee, local):
                            uses.add((use, path + (step,) + inner))
                    k += 1
                    continue
                elif name == 'CONCAT':
                    if depth < 4:
                        depth, size, path = 0, 2, path + ('joined (CONCAT)',)
                        k += 1
                        continue
                elif name == 'EQ_STRING':
                    if depth < 4:
                        uses.add(('compared', path))
                        break
                elif name == 'IN_LIST':
                    if 2 <= depth < 2 + ops[0]:
                        uses.add(('compared', path))
                        break
                elif name == 'STRING_TO_POINTER':
                    if depth == 0:
                        size, path = 4, path + ('pointer',)
                        k += 1
                        continue
                elif name == 'POP_LOCAL_WORD':
                    if depth == 0 and size == 2:
                        locals_ = locals_ | {ops[0]}
                        depth, path = None, path + (f'local {ops[0]}',)
                        k += 1
                        continue
                elif name == 'POP_RETURNED_WORD':
                    if depth == 0:
                        step = f'returned by {self.place(fn.class_id, fn.function)}'
                        for use, inner in self.returned((fn.class_id, fn.function)):
                            uses.add((use, path + (step,) + inner))
                        depth = None
                        k += 1
                        continue
                elif name == 'FREE_STRING':
                    if depth == ops[0]:
                        depth = None
                        if not locals_:
                            break
                        k += 1
                        continue
                elif name == 'CREATE_LIST':
                    if depth < ops[0] * ops[1]:
                        uses.add(('list', path))
                        break
                elif name == 'ASSIGN_ELEMENT':
                    if 2 <= depth < 2 + ops[1]:
                        uses.add(('list', path))
                elif name == 'POP_RESULT':
                    if depth < 4:
                        uses.add(('result of the process', path))
                        break
                elif name == 'MOVE_TOP' and ops[0] < 0 and depth < -ops[0]:
                    break
                change = effect(i)
                if change is None:
                    uses.add((f'unknown: {name}', path))
                    break
                popped, pushed = change
                if depth < popped:
                    uses.add((f'unknown: {name}', path))
                    break
                depth = depth - popped + pushed
                if name == 'JUMP_IF_FALSE':
                    work.append((fn.target(k), depth, size, locals_, path))
                elif name in ('FOREACH_LIST', 'FOREACH_STRING_LIST'):
                    if depth >= 4:
                        work.append((fn.target(k), depth - 4, size, locals_, path))
                k += 1
        return uses

    def local(self, place, local):
        """The uses of a function's local (an argument) from its start."""
        key = (place, local)
        if key not in self.local_uses:
            self.local_uses[key] = set()
            fn = self.functions.get(place)
            if fn:
                self.local_uses[key] = self.trace(fn, 0, None, 2, {local}, ())
        return self.local_uses[key]

    def returned(self, place):
        """The uses of what a function returns: at its callers'
        PUSH_RETURNED_WORD after the CALL; an event function's goes to the
        engine."""
        if place not in self.returned_uses:
            self.returned_uses[place] = set()
            uses = set()
            if place[1] < 32 and not self.callers[place]:
                uses.add(('returned to the engine (an event)', ()))
            for fn, k in self.callers[place]:
                for j in range(k + 1, min(k + 4, len(fn.ins))):
                    name = NAMES[fn.ins[j].op]
                    if name == 'PUSH_RETURNED_WORD':
                        step = f'in {self.place(fn.class_id, fn.function)}'
                        uses |= self.trace(fn, j + 1, 0, 2, set(), (step,))
                        break
                    if name != 'MOVE_TOP':
                        uses.add(('returned, not used', ()))
                        break
            self.returned_uses[place] = uses
        return self.returned_uses[place]

    # The conditions a literal is shown under.
    def render(self, fn, start, end):
        """The expression the instructions start..end leave on the stack,
        written out (C-like); None when a token is not one it knows."""
        stack = []
        pending = None
        for i in fn.ins[start:end]:
            name, ops = NAMES[i.op], i.operands
            if name in DEBUG:
                continue
            if name in ('PUSH_BYTE', 'PUSH_WORD', 'PUSH_LONG'):
                stack.append((str(ops[0]), 4 if name == 'PUSH_LONG' else 2))
            elif name in ('PUSH_LOCAL_BYTE', 'PUSH_LOCAL_WORD', 'PUSH_LOCAL_STRING'):
                stack.append((f'local {ops[0]}', 2))
            elif name == 'PUSH_LOCAL_LONG':
                stack.append(('this' if ops[0] == 6 else f'local {ops[0]}', 4))
            elif name == 'PUSH_LOCAL_ADDRESS':
                stack.append((f'&local {ops[0]}', 4))
            elif name == 'PUSH_GLOBAL':
                stack.append((f'global {ops[0]:X}', 2 if ops[1] == 1 else ops[1]))
            elif name == 'PUSH_GLOBAL_ADDRESS':
                stack.append((f'&global {ops[0]:X}', 4))
            elif name == 'PUSH_STRING':
                stack.append((repr(ops[0]), 2))
            elif name == 'PUSH_PID':
                stack.append(('pid', 2))
            elif name == 'PUSH_INDIRECT' and stack:
                stack.append((f'*{stack.pop()[0]}', ops[0]))
            elif name == 'CALL_INTRINSIC':
                pending = f'{self.intrinsics.get(ops[1], hex(ops[1]))}({", ".join(top_values(stack, ops[0]))})'
            elif name == 'CALL':
                pending = (self.place(*ops),)
            elif name == 'MOVE_TOP' and ops[0] < 0:
                values = top_values(stack, -ops[0])
                if isinstance(pending, tuple):
                    pending = f'{pending[0]}({", ".join(values)})'
                drop(stack, -ops[0])
            elif name in ('PUSH_RETURNED_BYTE', 'PUSH_RETURNED_WORD', 'PUSH_RETURNED_LONG'):
                stack.append((pending if isinstance(pending, str) else 'returned', 4 if name.endswith('LONG') else 2))
            elif name in OPERATORS and len(stack) >= 2:
                right, left = stack.pop(), stack.pop()
                stack.append((f'{wrap(left[0])} {OPERATORS[name]} {wrap(right[0])}', 4 if name in LONG_RESULTS else 2))
            elif name == 'NOT' and stack:
                stack.append((f'!{wrap(stack.pop()[0])}', 2))
            elif name in ('WORD_TO_LONG', 'LONG_TO_WORD') and stack:
                stack.append((stack.pop()[0], 4 if name == 'WORD_TO_LONG' else 2))
            else:
                return None
        return stack[-1][0] if len(stack) == 1 else None

    def assigned(self, fn, k, local):
        """The expression last popped into local before instruction k (in
        the code's order); None when there is none it can write out."""
        for j in range(k - 1, -1, -1):
            i = fn.ins[j]
            if NAMES[i.op] == 'POP_LOCAL_WORD' and i.operands[0] == local:
                start = self.start_of(fn, j, 2)
                return self.render(fn, start, j) if start is not None else None
        return None

    def conditions(self, fn, k):
        """The conditions of the branches the instruction at k is in,
        outermost first: each JUMP_IF_FALSE before it that jumps past it."""
        out = []
        for j in range(k):
            if NAMES[fn.ins[j].op] == 'JUMP_IF_FALSE' and fn.target(j) > k:
                start = self.start_of(fn, j, 2)
                text = self.render(fn, start, j) if start is not None else None
                out.append(text or '?')
        return out

    # The pieces of a CONCAT chain.
    def start_of(self, fn, end, size):
        """The index of the first instruction of those before end that
        leave size bytes on the stack, a returned value with the call that
        made it (its arguments, the call, MOVE_TOP); None where it cannot
        tell."""
        need = size
        for j in range(end - 1, -1, -1):
            name = NAMES[fn.ins[j].op]
            change = effect(fn.ins[j])
            if change is None or name in ('JUMP', 'JUMP_IF_FALSE', 'RETURN'):
                return None
            popped, pushed = change
            need = need - pushed + popped
            if need < 0:
                return None
            if need == 0 and name not in DEBUG and not (name.startswith('PUSH_RETURNED') and self.after_call(fn, j)):
                return j
        return None

    @staticmethod
    def after_call(fn, j):
        """Whether the instruction before j (past the debugger's tokens) is
        a call or the MOVE_TOP that drops its arguments."""
        j -= 1
        while j >= 0 and NAMES[fn.ins[j].op] in DEBUG:
            j -= 1
        return j >= 0 and NAMES[fn.ins[j].op] in ('MOVE_TOP', 'CALL_INTRINSIC', 'CALL')

    def describe(self, fn, start, end):
        """A number's source: a local, a global, a constant or an
        expression."""
        if start is None:
            return 'unknown'
        tokens = [i for i in fn.ins[start:end] if NAMES[i.op] not in DEBUG]
        if len(tokens) == 1:
            i = tokens[0]
            name = NAMES[i.op]
            if name in ('PUSH_LOCAL_WORD', 'PUSH_LOCAL_BYTE'):
                return f'local {i.operands[0]}'
            if name == 'PUSH_GLOBAL':
                return f'global {i.operands[0]:04X}'
            if name in ('PUSH_WORD', 'PUSH_BYTE'):
                return str(i.operands[0])
        return ' '.join(NAMES[i.op] for i in tokens)

    def pieces(self, fn, end):
        """The pieces of the string value the instructions before end leave
        on the stack, and the index where they start (None: not known)."""
        while end > 0 and NAMES[fn.ins[end - 1].op] in DEBUG:
            end -= 1
        i = fn.ins[end - 1]
        name = NAMES[i.op]
        if name == 'PUSH_STRING':
            return [{'text': text_id(fn, i)}], end - 1
        if name == 'PUSH_LOCAL_STRING':
            return [{'local': i.operands[0]}], end - 1
        if name == 'CONCAT':
            right, start = self.pieces(fn, end - 1)
            if start is None:
                return [{'unknown': ''}] + right, None
            left, start = self.pieces(fn, start)
            return left + right, start
        if name == 'PUSH_RETURNED_WORD':
            j = end - 2
            while j >= 0 and NAMES[fn.ins[j].op] in DEBUG:
                j -= 1
            if j >= 0 and NAMES[fn.ins[j].op] == 'MOVE_TOP':
                j -= 1
            call = fn.ins[j] if j >= 0 else None
            if call and NAMES[call.op] == 'CALL_INTRINSIC':
                symbol = self.intrinsics.get(call.operands[1], '')
                start = self.start_of(fn, j, call.operands[0]) if call.operands[0] else j
                if symbol == 'numberString':
                    return [{'number': self.describe(fn, start, j)}], start
                return [{'intrinsic': symbol}], start
            if call and NAMES[call.op] == 'CALL':
                move = fn.ins[j + 1]
                arguments = -move.operands[0] if NAMES[move.op] == 'MOVE_TOP' else 0
                start = self.start_of(fn, j, arguments) if arguments else j
                return [{'call': self.place(*call.operands)}], start
        start = self.start_of(fn, end, 2)
        return [{'value': self.describe(fn, start, end)}], start


def path_of(use, steps):
    """The steps to a use, a run of the same step once with its count."""
    out = []
    for step in list(steps) + [use]:
        if out and out[-1][0] == step:
            out[-1][1] += 1
        else:
            out.append([step, 1])
    return [step if count == 1 else f'{step} x{count}' for step, count in out]


# A test of a local against a constant in a written-out condition.
CASE = re.compile(r'\(local (-?\d+)\) == (\d+)')


def case_of(conditions):
    """The innermost condition's tests of one local against constants: the
    case the literal is in ({'local', 'values'}); None when there is none.
    The text functions (TEXTFIL1-3, TEXTFILE 21h and 23h) pick a text by
    its number in local 12, a text sometimes for several numbers."""
    for text in reversed(conditions):
        pairs = CASE.findall(text)
        if pairs and len({local for local, _ in pairs}) == 1:
            return {'local': int(pairs[0][0]), 'values': sorted({int(value) for _, value in pairs})}
    return None


def numbers_of(conditions, local=12):
    """The values the innermost condition that tests local allows."""
    for text in reversed(conditions):
        values = {int(value) for name, value in CASE.findall(text) if int(name) == local}
        if values:
            return sorted(values)
    return []


def text_id(fn, i):
    return f'{fn.class_id:04X}:{fn.function:04X}:{i.offset:04X}'


# The literals the original overwrites before it reads them (--check).
UNREAD = {'0A1F:0020:0662'}

# The terminal (ComputerGump, compgump.cpp and compgump.h): CAFont 6 in cells
# of 8x9 pixels, 20 columns and 13 rows from (41, 38) in the gump, <MORE> on
# the 14th row when a page is full; each shown character waits and plays
# effect 33h, '*' waits longer (the games' waits differ).
COLUMNS, ROWS = 20, 13
TERMINAL = {
    'gump': 'ComputerGump',
    'font': 6,
    'cell': [8, 9],
    'columns': COLUMNS,
    'rows': ROWS,
    'origin': [41, 38],
    'more': '<MORE>',
    'controls': {'^': 'starts a row', '*': 'a pause, not shown', '\u0001': 'not shown'},
    'wrap': 'setText turns the last space into ^ once 20 characters ran since the last ^ (* counted, the run 1 '
            'after a ^); typeText starts a row at ^ and after 20 shown characters, and a page after 13 rows',
}
WAITS = {
    'regret': {'character': 'clockDelay(30)', 'pause': 'clockDelay(1800)'},
    'remorse': {'character': 'systemTimer->delay(1)', 'pause': 'clockDelay(4800)'},
}
# A number's widest text (numberString of a 16-bit value).
NUMBER = '#####'


def terminal_pages(text):
    """The pages the terminal shows text in, each a list of its rows'
    shown characters, as ComputerGump lays it out: setText's breaks, then
    typeText's rows and pages."""
    chars = list(text)
    space, run = -1, 0
    for i, ch in enumerate(text):
        if ch == ' ':
            space = i
        if ch == '^':
            run, space = 0, -1
        if run >= COLUMNS and space >= 0:
            chars[space] = '^'
            run, space = i - space, -1
        if ch != '\x01':
            run += 1
    pages, rows, column, row = [], [''], 0, 0
    for ch in chars:
        if ch == '^':
            column, row = 0, row + 1
            rows.append('')
        elif ch not in ('\x01', '*'):
            rows[-1] += ch
            column += 1
        if column >= COLUMNS:
            column, row = 0, row + 1
            rows.append('')
        if row == ROWS:
            pages.append(rows[:ROWS])
            rows, column, row = [''], 0, 0
    while len(rows) > 1 and not rows[-1]:
        rows.pop()
    if rows != [''] or not pages:
        pages.append(rows)
    return pages


def layout_of(text):
    pages = terminal_pages(text)
    rows = [row for page in pages for row in page]
    return {
        'pageCount': len(pages),
        'rowCount': len(rows),
        'widest': max(len(row) for row in rows),
        'fullRows': sum(1 for row in rows if len(row) == COLUMNS),
        'pages': pages,
    }


# An item of a map file (itemdata.h's ItemRecord): x, y, z, shape, frame,
# flags, quality, npc, map, next.
ITEM = struct.Struct('<HHBHBHHBBH')
# The functions that pick a text by its number (local 12) and return it.
BY_NUMBER = ('TEXTFILE', (0x21, 0x23))


class Places:
    """Where the terminal texts are shown: the items of the maps
    (static/fixed.dat, a FLEX of the maps, each a list of ItemRecords) of the
    texts' classes (an item's shape is its usecode class), and for a text
    picked by its number the terminals whose quality gives that number.

    The terminals that pick a text by number (No Regret's WATCHNS and
    WATCHEW, No Remorse's SURCAMNS and SURCAMEW) call TEXTFILE's 21h or 23h
    with the text number in local 12, the high byte of a quality: their own
    when their map field is 0, else that of the item their link search finds
    (the shape and range of the search's script; the item's quality's low
    byte the terminal's, or any when the terminal's is 0). The code shows
    that test (Item::map(this) != 0) before the search."""

    def __init__(self, program, path):
        self.program = program
        self.file = path
        self.by_shape = defaultdict(list)
        self.by_map = defaultdict(list)
        # The items of a map by shape, in the map's order: what a link
        # search looks through (the tests' times).
        self.by_map_shape = defaultdict(list)
        if Path(path).exists():
            data = Path(path).read_bytes()
            self.hash = f'{fnv1a64(data):016x}'
            for number, entry in enumerate(uc.flex_objects(data)):
                if not entry:
                    continue
                offset, size = entry
                for at in range(offset, offset + size - ITEM.size + 1, ITEM.size):
                    x, y, z, shape, frame, flags, quality, npc, field, _ = ITEM.unpack_from(data, at)
                    item = {'map': number, 'x': x, 'y': y, 'z': z, 'shape': shape, 'frame': frame,
                            'quality': quality, 'mapField': field}
                    self.by_shape[shape].append(item)
                    self.by_map[number].append(item)
                    self.by_map_shape[(number, shape)].append(item)
        self.sources = self.number_sources()

    def number_sources(self):
        """The calls of the text-by-number functions: by the caller's
        place, its terminal's class, whether the number is the terminal's
        own or its linked item's (the link search's shape and range) and
        what the terminal's map field must be."""
        program = self.program
        textfile = next((c for c in program.usecode.classes() if program.name(c) == BY_NUMBER[0]), None)
        out = defaultdict(list)
        for (class_id, function), fn in program.functions.items():
            for k, i in enumerate(fn.ins):
                if NAMES[i.op] != 'CALL' or i.operands[0] != textfile or i.operands[1] not in BY_NUMBER[1]:
                    continue
                if class_id == textfile:
                    continue  # 21h passes its number on to 23h
                low = program.start_of(fn, k - 1, 2)
                high = program.start_of(fn, low, 2) if low is not None else None
                number = program.render(fn, high, low) if high is not None else None
                assigned = re.fullmatch(r'local (-\d+)', number or '')
                if assigned:
                    number = program.assigned(fn, k, int(assigned.group(1))) or number
                source = {'class': class_id, 'number': number}
                found = re.fullmatch(r'Item::qualityHigh\((this|&local (-?\d+))\)', number or '')
                if not found:
                    source['unknown'] = True
                elif found.group(2):
                    source['link'] = self.link_search(fn, int(found.group(2)))
                tests = [j for j, t in enumerate(fn.ins) if NAMES[t.op] == 'JUMP_IF_FALSE'
                         and program.render(fn, program.start_of(fn, j, 2) or 0, j) == 'Item::map(this) != 0']
                if tests:
                    j = tests[0]
                    source['mapField'] = 'set' if j < k < fn.target(j) else 'clear'
                out[program.place(class_id, function)].append(source)
        return out

    def link_search(self, fn, local):
        """The area search whose item goes to local: the shape its script
        tests (INT shape, SHAPE, EQUAL) and its range."""
        program = self.program
        k = next(j for j, i in enumerate(fn.ins) if NAMES[i.op] == 'LOOP' and i.operands[0] == local)
        around = program.start_of(fn, k, 2)
        start = program.start_of(fn, around, 2)
        text = program.render(fn, start, around) or ''
        size = eval(text) if re.fullmatch(r'[\d\s*+-]+', text) else None
        shape, j = None, start - 1
        tokens = []
        while j >= 0 and NAMES[fn.ins[j].op] in ('LOOP_SCRIPT', 'PUSH_WORD') + DEBUG:
            tokens.append(fn.ins[j])
            j -= 1
        script = [t.operands[0] for t in tokens if NAMES[t.op] == 'LOOP_SCRIPT']
        words = [t.operands[0] for t in tokens if NAMES[t.op] == 'PUSH_WORD']
        if ord('@') in script and ord('=') in script and len(words) == 1:
            shape = words[0]
        return {'shape': shape, 'range': size}

    def items(self, shape):
        return self.by_shape.get(shape, [])

    def of(self, text, paths):
        """The places a terminal text is shown, and how it is picked."""
        program = self.program
        callers = set()
        for path in paths:
            for step, after in zip(path, path[1:]):
                if step.startswith(f'returned by {BY_NUMBER[0]}:') and after.startswith('in '):
                    callers.add(after[3:])
        numbers = numbers_of(text['conditions'])
        if callers and numbers:
            places = []
            for caller in sorted(callers):
                for source in self.sources.get(caller, []):
                    for number in numbers:
                        places += [place | {'number': number} for place in self.terminals(source, number)]
            unique = {json.dumps(place, sort_keys=True): place for place in places}
            return {'picked': 'by its text number: the high byte of the quality of a terminal or its linked item',
                    'numbers': numbers, 'by': sorted(callers), 'places': list(unique.values())}
        if callers:
            return {'picked': 'by the text functions without a number (their default)', 'by': sorted(callers),
                    'places': []}
        own = self.items(text['class'])
        if own:
            return {'picked': f'an item of class {text["className"]}', 'places': [place_of(item, text['className'])
                                                                               for item in own]}
        return {'picked': 'not by a map item', 'places': []}

    def terminals(self, source, number):
        name = self.program.name(source['class'])
        out = []
        for terminal in self.items(source['class']):
            if source.get('mapField') == 'set' and not terminal['mapField']:
                continue
            if source.get('mapField') == 'clear' and terminal['mapField']:
                continue
            if 'link' not in source:
                if terminal['quality'] >> 8 == number:
                    out.append(place_of(terminal, name))
                continue
            link = source['link']
            for item in self.by_map_shape.get((terminal['map'], link['shape']), []):
                if (abs(item['x'] - terminal['x']) <= link['range']
                        and abs(item['y'] - terminal['y']) <= link['range']
                        and (item['quality'] & 0xff == terminal['quality'] & 0xff or not terminal['quality'] & 0xff)
                        and item['quality'] >> 8 == number):
                    out.append(place_of(terminal, name) | {'link': place_of(item, None)})
        return out


def place_of(item, class_name):
    out = {key: item[key] for key in ('map', 'x', 'y', 'z', 'shape', 'quality')}
    if class_name:
        out['className'] = class_name
    return out


def kind_of(uses):
    """The literal's use: terminal (showComputerText), hidden
    (Item::nothing), movie (a movie's name), compare, or the first other."""
    names = {use for use, _ in uses}
    if 'intrinsic showComputerText' in names:
        return 'terminal'
    if any('Movie' in use or 'movie' in use for use in names):
        return 'movie'
    if 'intrinsic Item::nothing' in names:
        return 'hidden'
    if names == {'compared'}:
        return 'compare'
    if not names:
        return 'unused'
    return 'other'


def extract(path):
    program = Program(path)
    literals = []
    chains = {}
    for fn in program.functions.values():
        for k, i in enumerate(fn.ins):
            if NAMES[i.op] == 'PUSH_STRING':
                literals.append((fn, k, i))
            elif NAMES[i.op] == 'CONCAT':
                parts, _ = program.pieces(fn, k + 1)
                for part in parts:
                    if 'text' in part and len(parts) > len(chains.get(part['text'], [])):
                        chains[part['text']] = parts
    by_text = defaultdict(list)
    for fn, k, i in literals:
        by_text[i.operands[0]].append(text_id(fn, i))
    texts = []
    for fn, k, i in literals:
        text = i.operands[0]
        raw = text.encode('latin-1')
        uses = program.trace(fn, k + 1, 0, 2, set(), ())
        kind = kind_of(uses)
        ordered = sorted(uses, key=lambda u: (u[0] != 'intrinsic showComputerText', len(u[1]), u))
        lines = text.split('^')
        trimmed = re.sub(r' +\^', '^', text).rstrip(' ')
        entry = {
            'id': text_id(fn, i),
            'class': fn.class_id,
            'className': program.name(fn.class_id),
            'function': fn.function,
            'event': fn.function < 32,
            'offset': i.offset,
            'fileOffset': fn.file_offset + i.offset,
            'length': len(raw),
            'hash': f'{fnv1a64(raw):016x}',
            'text': text,
            'trimmed': trimmed,
            'lineBreaks': text.count('^'),
            'pauses': text.count('*'),
            'longestLine': max(len(line) for line in lines),
            'padded': trimmed != text,
            'sameText': [other for other in by_text[text] if other != text_id(fn, i)],
            'use': kind,
            'uses': sorted({use for use, _ in uses}),
            'path': path_of(*ordered[0]) if ordered else [],
        }
        entry['conditions'] = program.conditions(fn, k)
        case = case_of(entry['conditions'])
        if case:
            entry['case'] = case
        entry['_paths'] = [path for _, path in uses]
        if text_id(fn, i) in chains:
            parts = chains[text_id(fn, i)]
            entry['pieces'] = parts
            entry['piece'] = next(n for n, part in enumerate(parts) if part.get('text') == text_id(fn, i))
        texts.append(entry)
    texts.sort(key=lambda t: (t['class'], t['function'], t['offset']))
    # The terminal's layout: the literal's own, and the joined text's (its
    # numbers at their widest) when it is joined with texts and numbers only.
    by_id = {t['id']: t for t in texts}
    for t in texts:
        if t['use'] != 'terminal':
            continue
        t['display'] = 'terminal'
        t['layout'] = layout_of(t['text'])
        if 'pieces' in t and all('text' in part or 'number' in part for part in t['pieces']):
            joined = ''.join(by_id[part['text']]['text'] if 'text' in part else NUMBER for part in t['pieces'])
            t['joinedLayout'] = layout_of(joined) | {'numbersAt': len(NUMBER)}
    places = Places(program, overlay.origin_path('STATIC/FIXED.DAT', Path(path).resolve().parents[1]))
    for t in texts:
        paths = t.pop('_paths')
        if t['use'] == 'terminal':
            t['shown'] = places.of(t, paths)
    data = Path(path).read_bytes()
    kinds = Counter(t['use'] for t in texts)
    summary = {
        'literals': len(texts),
        'uniqueTexts': len(by_text),
        'byUse': dict(sorted(kinds.items())),
        'uniqueByUse': {kind: len({t['text'] for t in texts if t['use'] == kind}) for kind in sorted(kinds)},
        'bytesByUse': {kind: sum(len(text.encode('latin-1')) for text in {t['text'] for t in texts if t['use'] == kind})
                       for kind in sorted(kinds)},
        'joined': sum(1 for t in texts if 'pieces' in t),
        'terminalClasses': dict(Counter(t['className'] for t in texts if t['use'] == 'terminal').most_common()),
        'terminalsPlaced': sum(1 for t in texts if t['use'] == 'terminal' and t['shown']['places']),
        'byNumber': {place: sources for place, sources in places.sources.items()},
    }
    return {
        'game': overlay.GAME,
        'displays': {'terminal': TERMINAL | {'waits': WAITS[overlay.GAME]}},
        'source': {'file': str(Path(path).resolve().relative_to(ROOT)) if Path(path).resolve().is_relative_to(ROOT)
                   else str(path), 'size': len(data), 'hash': f'{fnv1a64(data):016x}'},
        'maps': {'file': str(Path(places.file).resolve().relative_to(ROOT)) if Path(places.file).resolve()
                 .is_relative_to(ROOT) else str(places.file), 'hash': getattr(places, 'hash', None),
                 'items': sum(len(items) for items in places.by_map.values())},
        'tool': 'port/tools/usecode_text.py',
        'summary': summary,
        'texts': texts,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--flx', type=Path, default=uc.FLX)
    ap.add_argument('-o', '--output', type=Path, default=ROOT / 'build/text' / f'{overlay.GAME}_usecode_text.json')
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    result = extract(args.flx)
    if args.check:
        problems = [t for t in result['texts']
                    if any(use.startswith('unknown') for use in t['uses']) or (t['use'] == 'unused' and t['id'] not in UNREAD)]
        for t in problems:
            print(f'{t["id"]} {t["className"]} {t["text"][:40]!r}: {t["use"]} {t["uses"]}')
        for place, sources in result['summary']['byNumber'].items():
            for source in sources:
                if source.get('unknown') or ('link' in source and None in source['link'].values()):
                    problems.append(source)
                    print(f'{place}: a text number the tool cannot follow: {source}')
        print(f'usecode_text: {result["summary"]["literals"]} literals, by use {result["summary"]["byUse"]}, '
              f'{len(problems)} problems')
        sys.exit(1 if problems else 0)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=1) + '\n')
    summary = result['summary']
    print(f'{args.output}: {summary["literals"]} literals ({summary["uniqueTexts"]} texts), by use {summary["byUse"]}')


if __name__ == '__main__':
    main()
