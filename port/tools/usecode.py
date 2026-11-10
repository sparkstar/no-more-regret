#!/usr/bin/env python3
"""The usecode disassembler: EUSECODE.FLX's classes and their functions as
INTER.C's tokens.

The usecode file is a FLEX (UNITFILE.C): object 1 is a ClassInfo of 13
bytes a class (the events it has functions for, its name in 9 bytes), and
class n is object n+2. A class's unit starts with a ClassHeader of 20 bytes
(the number of functions, a long, codeBase, 8 bytes) and a FunctionEntry of
6 bytes (size, offset) for each of the functions+32; a function's code is
size bytes at offset+codeBase-1 of the unit (UPROCESS.C's ClassTable::code),
none when the offset's low word is 0. Functions 0-31 are the events'.

The tokens' names come from inter.cpp's enum (OP_*), their
operands from what UsecodeThread::run reads after each: b a signed byte, B
an unsigned one, w a signed word, W an unsigned one, l a long, j a word the
thread jumps by from the next token, s a word n and a string of n bytes and
a 0, n a name of 9 bytes. Every function starts with OP_ENTER and ends with
OP_RETURN and a byte 7Ah, which the interpreter never reaches (its default
case); No Remorse's has the names of its locals between them (locals()). OP_CALL_INTRINSIC's number is an index into USECODE\\UNKCOFF.DAT,
whose C functions the game's tables/intrinsics.tsv names, or a table of the
original's addresses (--symbols: name, segment and offset a line) by their
far addresses.

intrinsics checks the calls of intrinsics against the game's
tables/intrinsics.tsv: each passes at least the bytes its function takes
(more are left unread, as by the original's C call, and listed) and reads no
more of a result than the original's function sets.

loops checks that each loop's end drops as many bytes as BCC gave its search
(and the local's word), which the port puts on the stack in BCC's layout.

pointers sees where the usecode keeps pointers, which the port's design of
them leans on: no function OP_CALL reaches suspends, so a thread stops only
in its top function and no return address is ever on a stopped stack; the
only pointer a function reads from its locals is local 6, its arguments'
pointer (the one the original mends on a load); none goes into a long local,
the flags or a list; OP_SPAWN's comes from local 6, a local's or a global's
address or 0.

coverage counts what traces of runs (REGRET_USECODE_TRACE, platform/trace.h;
tools/replays.py --trace) ran of the usecode: the classes and functions (a
process's start, U, and OP_CALL, C), the events sent (E) and the intrinsics
called (I), of those the file has and calls; with --missing, what never ran.

usage: usecode.py [--flx F] [--symbols T] check | pointers | intrinsics | loops | list | stats
                   | dis CLASS [FUNCTION] | coverage [--missing] TRACE...
(CLASS a name or a number in hex, FUNCTION in hex)
"""
import argparse, re, signal, struct, sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
FLX = overlay.origin_path('USECODE/EUSECODE.FLX')
UNKCOFF = overlay.origin_path('USECODE/UNKCOFF.DAT')
INTER = overlay.find('inter.cpp')
INTRINSICS = overlay.TABLES / 'intrinsics.tsv'

OPERANDS = {
    0x00: 'b', 0x01: 'b', 0x02: 'b', 0x03: 'bb', 0x08: '', 0x09: 'bbb', 0x0a: 'b', 0x0b: 'w', 0x0c: 'l', 0x0d: 's',
    0x0e: 'bb', 0x0f: 'bW', 0x10: 'l', 0x11: 'WW', 0x12: '', 0x13: '', 0x14: '', 0x15: '', 0x16: '', 0x17: '',
    0x18: 'b', 0x19: 'b', 0x1a: 'b', 0x1b: 'b', 0x1c: '', 0x1d: '', 0x1e: '', 0x1f: '', 0x20: '', 0x21: '',
    0x22: '', 0x23: '', 0x24: '', 0x25: '', 0x26: '', 0x27: 'b', 0x28: '', 0x29: '', 0x2a: '', 0x2b: '', 0x2c: '',
    0x2d: '', 0x2e: '', 0x2f: '', 0x30: '', 0x31: '', 0x32: '', 0x33: '', 0x34: '', 0x35: '', 0x36: '', 0x37: '',
    0x38: 'bb', 0x39: '', 0x3a: '', 0x3b: '', 0x3c: '', 0x3d: '', 0x3e: 'b', 0x3f: 'b', 0x40: 'b', 0x41: 'b',
    0x42: 'bb', 0x43: 'b', 0x44: 'bb', 0x45: 'bb', 0x4b: 'b', 0x4c: 'b', 0x4d: 'b', 0x4e: 'Wb', 0x4f: 'Wb',
    0x50: '', 0x51: 'j', 0x52: 'j', 0x53: '', 0x54: 'bb', 0x55: '', 0x56: '', 0x57: 'bbWW', 0x58: 'WWWbb',
    0x59: '', 0x5a: 'b', 0x5b: 'W', 0x5c: 'Wn', 0x5d: '', 0x5e: '', 0x5f: '', 0x60: '', 0x61: '', 0x62: 'b',
    0x63: 'b', 0x64: 'b', 0x65: 'b', 0x66: 'b', 0x67: 'b', 0x68: '', 0x69: 'b', 0x6a: '', 0x6b: '', 0x6c: 'bb',
    0x6d: '', 0x6e: 'b', 0x6f: 'b', 0x70: 'bbb', 0x73: '', 0x74: 'B', 0x75: 'bbj', 0x76: 'bbj', 0x77: '', 0x78: '',
    0x79: 'W',
}
OP_ENTER, OP_RETURN, END = 0x5a, 0x50, 0x7a
SIZES = {'b': 1, 'B': 1, 'w': 2, 'W': 2, 'l': 4, 'j': 2, 'n': 9}


def token_names():
    """INTER.C's names of the tokens, from the port's source."""
    text = INTER.read_text()
    names = {int(value, 16): name for name, value in re.findall(r'\bOP_(\w+)\s*=\s*0x([0-9a-fA-F]+)', text)}
    if set(names) != set(OPERANDS):
        sys.exit(f'inter.cpp\'s tokens and the operand table differ: {sorted(set(names) ^ set(OPERANDS))}')
    return names


def flex_objects(data):
    """A FLEX's objects: (offset, size) by index, the empty ones None."""
    count = struct.unpack_from('<I', data, 0x54)[0]
    objects = []
    for i in range(count):
        offset, size = struct.unpack_from('<II', data, 0x80+8*i)
        objects.append((offset, size) if size else None)
    return objects


def locals_table(code, at):
    """The table of a function's locals that No Remorse's usecode has after
    its last OP_RETURN, from at to the 7Ah that ends the function, or None
    when the bytes are not one: a count, then for each a byte (1 for the
    first of a function's), its type ('i', '$', 'b', 'l', 's', 'd', 'z'),
    its offset from BP, a byte (0, 2 or 5) and its name with a 0. The
    interpreter never reads it."""
    if at >= len(code):
        return None
    out, pos = [], at + 1
    for _ in range(code[at]):
        end = code.find(b'\0', pos + 4)
        if pos + 4 > len(code) or end < 0:
            return None
        out.append((code[pos], chr(code[pos + 1]), struct.unpack_from('<b', code, pos + 2)[0], code[pos + 3],
                    code[pos + 4:end].decode('latin-1')))
        pos = end + 1
    return out if pos == len(code) - 1 and code[pos] == END else None


class Instruction:
    def __init__(self, offset, op, operands, size):
        self.offset, self.op, self.operands, self.size = offset, op, operands, size


def decode(code, start=0):
    """The instructions of a function's code; raises ValueError where it
    does not decode."""
    out = []
    at = start
    while at < len(code):
        op = code[at]
        if op == END and at == len(code)-1 and out and out[-1].op == OP_RETURN:
            break
        if op not in OPERANDS:
            raise ValueError(f'unknown token {op:02X} at {at:04X}')
        pos = at+1
        operands = []
        for kind in OPERANDS[op]:
            if kind == 's':
                length = struct.unpack_from('<H', code, pos)[0]
                text = code[pos+2:pos+2+length]
                if code[pos+2+length] != 0:
                    raise ValueError(f'string without its 0 at {at:04X}')
                operands.append(text.decode('latin-1'))
                pos += 2+length+1
                continue
            size = SIZES[kind]
            if pos+size > len(code):
                raise ValueError(f'{op:02X} at {at:04X} runs past the end')
            if kind == 'b':
                value = struct.unpack_from('<b', code, pos)[0]
            elif kind == 'B':
                value = code[pos]
            elif kind in 'wj':
                value = struct.unpack_from('<h', code, pos)[0]
            elif kind == 'W':
                value = struct.unpack_from('<H', code, pos)[0]
            elif kind == 'l':
                value = struct.unpack_from('<i', code, pos)[0]
            else:
                value = code[pos:pos+9].split(b'\0')[0].decode('latin-1')
            operands.append(value)
            pos += size
        out.append(Instruction(at, op, operands, pos-at))
        at = pos
        if op == OP_RETURN and locals_table(code, at) is not None:
            break
    return out


class Usecode:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        self.objects = flex_objects(self.data)
        offset, size = self.objects[1]
        info = self.data[offset:offset+size]
        self.events, self.names = [], []
        for i in range(size//13):
            self.events.append(struct.unpack_from('<I', info, 13*i)[0])
            self.names.append(info[13*i+4:13*i+13].split(b'\0')[0].decode('latin-1'))

    def unit(self, class_id):
        entry = self.objects[class_id+2] if class_id+2 < len(self.objects) else None
        if not entry:
            return None
        return self.data[entry[0]:entry[0]+entry[1]]

    def classes(self):
        return [c for c in range(len(self.names)) if self.unit(c)]

    def functions(self, class_id):
        """{function: code} of a class."""
        unit = self.unit(class_id)
        count, _, code_base = struct.unpack_from('<III', unit, 0)
        out = {}
        for f in range(count+32):
            size, offset = struct.unpack_from('<HI', unit, 20+6*f)
            if size and offset & 0xffff:
                start = offset+code_base-1
                out[f] = unit[start:start+size]
        return out

    def find_class(self, text):
        """A class by its name, else by its number in hex."""
        for c, name in enumerate(self.names):
            if name and name.upper() == text.upper() and self.unit(c):
                return c
        try:
            return int(text, 16)
        except ValueError:
            sys.exit(f'no class {text}')


def intrinsic_names(unkcoff=UNKCOFF, symbols=None):
    """The C function of each intrinsic number: the game's table of them
    (tables/intrinsics.tsv), or, given a table of the original's addresses,
    by its far address in the map's segments (NE segment 1000h+(n-1)*8)."""
    if symbols is None:
        rows = (line.split('\t') for line in INTRINSICS.read_text().splitlines()[1:])
        return {int(number, 16): name for number, name, *_ in rows}
    if not symbols.exists():
        return {}
    by_address = {}
    for line in symbols.read_text().splitlines():
        name, segment, offset, *_ = line.split('\t')
        # A module's first function over the module's label (DISTANCE:CODE).
        if ':' not in name or (int(segment, 16), int(offset, 16)) not in by_address:
            by_address[(int(segment, 16), int(offset, 16))] = name
    data = unkcoff.read_bytes()
    names = {}
    for i in range(len(data)//4):
        offset, segment = struct.unpack_from('<HH', data, 4*i)
        names[i] = by_address.get((0x1000+(segment-1)*8, offset), f'?{segment:04X}:{offset:04X}')
    return names


def text_of(ins, names, intrinsics):
    ops = ins.operands
    name = names[ins.op]
    if ins.op == 0x0f:
        return f'{name} {ops[0]} {ops[1]:03X} {intrinsics.get(ops[1], "")}'.rstrip()
    if ins.op == 0x11:
        return f'{name} class {ops[0]:04X} function {ops[1]:04X}'
    if ins.op == 0x57:
        return f'{name} {ops[0]} {ops[1]} class {ops[2]:04X} function {ops[3]:04X}'
    if OPERANDS[ins.op].endswith('j'):
        head = ' '.join(str(o) for o in ops[:-1])
        return f'{name} {head} -> {ins.offset+ins.size+ops[-1]:04X}'.replace('  ', ' ')
    if ins.op == 0x0d:
        return f'{name} {ops[0]!r}'
    return ' '.join([name]+[f'{o:X}' if isinstance(o, int) and OPERANDS[ins.op][k] in 'WB' else str(o)
                            for k, o in enumerate(ops)])


def check(usecode, names):
    """Every function decodes to its end, starts with OP_ENTER and ends
    with OP_RETURN and 7Ah."""
    problems, functions, counts = [], 0, Counter()
    for c in usecode.classes():
        for f, code in usecode.functions(c).items():
            functions += 1
            try:
                ins = decode(code)
            except ValueError as error:
                problems.append(f'class {c:04X} function {f:04X}: {error}')
                continue
            if not ins or ins[0].op != OP_ENTER or ins[-1].op != OP_RETURN or code[-1] != END:
                problems.append(f'class {c:04X} function {f:04X}: not OP_ENTER ... OP_RETURN 7A')
            for i in ins:
                counts[i.op] += 1
    return len(usecode.classes()), functions, counts, problems


def check_intrinsics(usecode, table=INTRINSICS):
    """The calls of intrinsics against their table: the
    argument bytes each call passes and the width the usecode reads of what
    it returns (OP_PUSH_RETURNED_BYTE, _WORD, _LONG before another intrinsic,
    a usecode function (which leaves its own), OP_SPAWN or
    OP_POP_RETURNED_WORD sets it, or a jump or the end). The problems (fewer
    bytes than the function takes, a read wider than what the original sets),
    the calls with more bytes than it takes (the original's C calls leave the
    rest unread), and the count of calls."""
    rows = [line.split('\t') for line in table.read_text().splitlines()[1:]]
    arguments = {int(r[0], 16): int(r[3]) for r in rows if r[3] != '-'}
    returns = {int(r[0], 16): int(r[4]) for r in rows if r[4] != '-'}
    names = {int(r[0], 16): r[1] for r in rows}
    reads = {0x5d: 1, 0x5e: 2, 0x5f: 4}
    problems, wider, extra, calls = [], [], [], 0
    for c in usecode.classes():
        for f, code in usecode.functions(c).items():
            ins = decode(code)
            for k, i in enumerate(ins):
                if i.op != 0x0f:
                    continue
                calls += 1
                size, number = i.operands
                where = f'{c:04X}:{f:04X}@{i.offset:04X}'
                if number not in arguments:
                    problems.append(f'{where}: intrinsic {number:03X} is not in the table')
                    continue
                if size < arguments[number]:
                    problems.append(f'{where}: {size} bytes to {names[number]}, which takes {arguments[number]}')
                elif size > arguments[number]:
                    extra.append(f'{where}: {size} bytes to {names[number]}, which takes {arguments[number]}')
                for later in ins[k+1:]:
                    if later.op in reads:
                        if reads[later.op] > returns[number]:
                            wider.append(f'{where}: reads {reads[later.op]} bytes of {names[number]}, which returns '
                                         f'{returns[number]}')
                        break
                    if later.op in (0x0f, 0x11, 0x57, 0x12, 0x50, 0x51, 0x52):
                        break
    return problems, wider, extra, calls


# The events, by the Item and Npc functions that send them (callEvent's
# callers: item.cpp, egg.cpp, npc.cpp); 13 has none.
EVENTS = {0: 'look', 1: 'use', 2: 'anim', 3: 'activity', 4: 'cacheIn', 5: 'hit', 6: 'gotHit', 7: 'hatch',
          8: 'schedule', 9: 'release', 10: 'equip', 11: 'unequip', 12: 'combine', 14: 'calledFromAnim',
          15: 'enterFastArea', 16: 'leaveFastArea', 17: 'cast', 18: 'justMoved', 19: 'avatarStoleSomething',
          20: 'unknownEvent14', 21: 'unhatch'}


def demangled(name):
    """A Borland name without its arguments: @Item@frame$qv, Item::frame."""
    return name.lstrip('@').split('$')[0].replace('@', '::')


def coverage(usecode, traces, table=INTRINSICS):
    """What the traces ran of the usecode: the functions with code, the
    events the classes take and the intrinsics the code calls, each as
    (all, ran), and the counts of the intrinsics' calls in the code."""
    functions = {(c, f) for c in usecode.classes() for f in usecode.functions(c)}
    events = {(c, f) for c, f in functions if f < 32 and usecode.events[c] >> f & 1}
    sites = Counter()
    for c, f in functions:
        for ins in decode(usecode.functions(c)[f]):
            if ins.op == 0x0f:
                sites[ins.operands[1]] += 1
    ran_functions, ran_events, ran_intrinsics = Counter(), Counter(), Counter()
    for trace in traces:
        with open(trace, encoding='latin-1') as lines:
            for line in lines:
                words = line.split(' ', 5)
                kind = words[1]
                if kind == 'I':
                    ran_intrinsics[int(words[4], 16)] += 1
                elif kind in ('U', 'C'):
                    c, f = words[3].split(':')
                    ran_functions[(int(c, 16), int(f, 16))] += 1
                elif kind == 'E':
                    c, f = words[3].split(':')
                    ran_events[(int(c, 16), int(f, 16))] += 1
    rows = [line.split('\t') for line in table.read_text().splitlines()[1:]]
    names = {int(r[0], 16): r[1] for r in rows}
    return {'functions': (functions, ran_functions), 'events': (events, ran_events),
            'intrinsics': (set(sites), ran_intrinsics)}, sites, names


# The bytes BCC gave OP_LOOP's searches (usesearch.cpp).
SEARCH_SIZES = {2: 0x38, 3: 0x38, 4: 0x28, 5: 0x2a, 6: 0x41}


def loops(usecode):
    """OP_LOOP's searches against BCC's sizes: each loop goes on with
    OP_JUMP_IF_FALSE to its end, where OP_MOVE_TOP drops the search and
    the local's word, as many bytes as BCC gave the search. The problems
    and the count of loops."""
    problems, count = [], 0
    for c in usecode.classes():
        for f, code in usecode.functions(c).items():
            ins = decode(code)
            at = {i.offset: i for i in ins}
            for k, i in enumerate(ins):
                if i.op != 0x70:
                    continue
                count += 1
                where = f'{c:04X}:{f:04X}@{i.offset:04X}'
                kind = i.operands[2]
                if kind not in SEARCH_SIZES:
                    problems.append(f'{where}: a search of kind {kind}')
                    continue
                test = ins[k+1] if k+1 < len(ins) else None
                if not test or test.op != 0x51:
                    problems.append(f'{where}: no OP_JUMP_IF_FALSE after it')
                    continue
                end = at.get(test.offset+test.size+test.operands[0])
                if not end or end.op != 0x6e or end.operands[0] != -(SEARCH_SIZES[kind]+2):
                    problems.append(f'{where}: the end drops {end.operands[0] if end and end.op == 0x6e else "nothing"}'
                                    f', not {SEARCH_SIZES[kind]+2}')
    return problems, count


def pointers(usecode):
    """Where the usecode keeps pointers: the problems with what the port's
    design of them leans on."""
    funcs = {}
    for c in usecode.classes():
        for f, code in usecode.functions(c).items():
            funcs[(c, f)] = decode(code)
    calls = {k: {(i.operands[0], i.operands[1]) for i in ins if i.op == 0x11} for k, ins in funcs.items()}
    suspends = {k for k, ins in funcs.items() if any(i.op == 0x53 for i in ins)}
    reach = {}

    def suspending(k, seen=()):
        if k not in reach:
            reach[k] = k not in seen and (k in suspends or any(suspending(t, seen+(k,)) for t in calls.get(k, ())))
        return reach[k]
    problems = []
    for target in sorted({t for v in calls.values() for t in v}):
        if suspending(target):
            problems.append(f'{target[0]:04X}:{target[1]:04X}, which OP_CALL reaches, can suspend')
    pointer_sources = {0x4b, 0x6f, 0x79, 0x6b, 0x69}
    for (c, f), ins in funcs.items():
        where = f'{c:04X}:{f:04X}'
        for k, i in enumerate(ins):
            before = ins[k-1] if k else None
            if i.op in (0x4c, 0x4d) and not (before and before.op == 0x40 and before.operands[0] == 6):
                problems.append(f'{where}@{i.offset:04X}: an indirect access not through local 6')
            if i.op == 0x57 and not (before and (before.op == 0x40 and before.operands[0] == 6 or
                                                before.op in (0x4b, 0x79) or before.op == 0x0c and not before.operands[0])):
                problems.append(f'{where}@{i.offset:04X}: OP_SPAWN\'s pointer from elsewhere')
            if i.op == 0x02 and before and before.op in pointer_sources | {0x40}:
                if before.op != 0x40 or before.operands[0] == 6:
                    problems.append(f'{where}@{i.offset:04X}: a pointer into a local')
            if i.op == 0x4f and i.operands[1] > 2:
                problems.append(f'{where}@{i.offset:04X}: {i.operands[1]} bytes into the flags')
            if i.op == 0x0e and i.operands[0] != 2:
                problems.append(f'{where}@{i.offset:04X}: a list of {i.operands[0]}-byte elements')
            if i.op in (0x10, 0x58):
                problems.append(f'{where}@{i.offset:04X}: {i.op:02X}, which moves code')
    return len(funcs), problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--flx', type=Path, default=FLX)
    ap.add_argument('--symbols', type=Path)
    ap.add_argument('command', choices=('check', 'pointers', 'intrinsics', 'loops', 'list', 'stats', 'dis',
                                        'coverage'))
    ap.add_argument('args', nargs='*')
    ap.add_argument('--missing', action='store_true', help='coverage: list what never ran')
    args = ap.parse_intermixed_args()
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    names = token_names()
    usecode = Usecode(args.flx)
    if args.command == 'check':
        classes, functions, counts, problems = check(usecode, names)
        for p in problems[:20]:
            print('MISMATCH usecode:', p)
        print(f'usecode: {classes} classes, {functions} functions, {sum(counts.values())} tokens, '
              f'{len(problems)} not decoded')
        sys.exit(1 if problems else 0)
    if args.command == 'intrinsics':
        problems, wider, extra, calls = check_intrinsics(usecode)
        for p in problems[:20]+wider[:20]:
            print('MISMATCH usecode:', p)
        for e in extra:
            print('more bytes than taken:', e)
        print(f'usecode intrinsics: {calls} calls, {len(problems)} too few bytes, {len(wider)} reads wider than the '
              f'result, {len(extra)} with more bytes than taken')
        sys.exit(1 if problems or wider else 0)
    if args.command == 'loops':
        problems, count = loops(usecode)
        for p in problems[:20]:
            print('MISMATCH usecode:', p)
        print(f'usecode loops: {count} loops, {len(problems)} problems')
        sys.exit(1 if problems else 0)
    if args.command == 'pointers':
        count, problems = pointers(usecode)
        for p in problems[:20]:
            print('MISMATCH usecode:', p)
        print(f'usecode pointers: {count} functions, {len(problems)} problems')
        sys.exit(1 if problems else 0)
    if args.command == 'stats':
        _, _, counts, _ = check(usecode, names)
        for op in sorted(OPERANDS):
            print(f'{op:02X}\t{names[op]}\t{counts[op]}')
        return
    if args.command == 'coverage':
        parts, sites, table_names = coverage(usecode, args.args)
        functions, ran = parts['functions']
        events, sent = parts['events']
        called, calls = parts['intrinsics']
        classes = {c for c, _ in functions}
        print(f'usecode coverage {overlay.GAME}: {len(args.args)} traces')
        print(f'  classes    {len({c for c, _ in functions & set(ran)}):5} of {len(classes)} with code')
        print(f'  functions  {len(functions & set(ran)):5} of {len(functions)} ({sum(ran.values())} starts and calls)')
        print(f'  events     {len(events & set(sent)):5} of {len(events)} a class takes ({sum(sent.values())} sent)')
        print(f'  intrinsics {len(called & set(calls)):5} of {len(called)} the code calls ({sum(calls.values())} calls; '
              f'{len(table_names)} in the table)')
        # No Remorse's table has some C functions at several numbers.
        numbers = defaultdict(set)
        for number in called:
            numbers[table_names.get(number, f'?{number:03X}')].add(number)
        ran_names = {name for name, at in numbers.items() if at & set(calls)}
        print(f'  C functions{len(ran_names):5} of {len(numbers)} behind the intrinsics the code calls')
        for event in sorted({f for _, f in events}):
            takes = {e for e in events if e[1] == event}
            print(f'    event {event:2} {EVENTS.get(event, "?"):20} {len(takes & set(sent)):4} of {len(takes):4} classes')
        if args.missing:
            for name, at in sorted(numbers.items(), key=lambda item: min(item[1])):
                if name not in ran_names:
                    print(f'intrinsic {" ".join(f"{n:03X}" for n in sorted(at))} {demangled(name)}: '
                          f'{sum(sites[n] for n in at)} calls in the code, none ran')
            for c, f in sorted(functions - set(ran)):
                kind = f'event {EVENTS.get(f, f)}' if f < 32 else 'function'
                print(f'function {c:04X}:{f:04X} {usecode.names[c]} ({kind}): never ran')
        return
    if args.command == 'list':
        for c in usecode.classes():
            fs = usecode.functions(c)
            print(f'{c:04X} {usecode.names[c]:9} events {usecode.events[c]:08X} functions '
                  + ' '.join(f'{f:X}' for f in fs))
        return
    intrinsics = intrinsic_names(symbols=args.symbols)
    c = usecode.find_class(args.args[0])
    functions = usecode.functions(c)
    wanted = [int(args.args[1], 16)] if len(args.args) > 1 else sorted(functions)
    for f in wanted:
        print(f'; class {c:04X} {usecode.names[c]} function {f:04X} ({len(functions[f])} bytes)')
        for ins in decode(functions[f]):
            print(f'{ins.offset:04X}  {text_of(ins, names, intrinsics)}')


if __name__ == '__main__':
    main()
