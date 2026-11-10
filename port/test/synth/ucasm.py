"""A usecode assembler: INTER.C's tokens by the names of inter.cpp's enum
(OP_*), their operands as tools/usecode.py's disassembler reads them."""
import shlex, struct, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
import usecode  # noqa: E402

TOKENS = {name: number for number, name in usecode.token_names().items()}
PACK = {'b': '<b', 'B': '<B', 'w': '<h', 'W': '<H', 'l': '<i'}
BITS = {'b': 8, 'w': 16}


def operand(kind, text):
    """An operand's bytes; a signed kind takes its unsigned form too."""
    value = int(text, 0)
    if kind in BITS:
        half = 1 << (BITS[kind] - 1)
        value = (value + half) % (2 * half) - half
    return struct.pack(PACK[kind], value)


def words_of(line):
    """A line's words: ';' outside quotes starts a comment, a quoted string
    is one word."""
    quoted = False
    for at, char in enumerate(line):
        if char == '"':
            quoted = not quoted
        elif char == ';' and not quoted:
            line = line[:at]
            break
    return shlex.split(line)


def assemble(text):
    """A function's code from lines of TOKEN OPERAND... (';' starts a
    comment; 'name:' is a label, which a jump's operand names; a string
    operand is quoted), ended by the 7Ah every function ends with."""
    lines = [words for words in (words_of(line) for line in text.strip().splitlines()) if words]
    labels = {}
    for _ in range(2):
        code, found = bytearray(), {}
        for words in lines:
            if words[0].endswith(':'):
                found[words[0][:-1]] = len(code)
                continue
            token = TOKENS[words[0]]
            kinds, operands = usecode.OPERANDS[token], words[1:]
            if len(operands) != len(kinds):
                raise ValueError(f'{words[0]} takes {len(kinds)} operands: {" ".join(words)}')
            code.append(token)
            for kind, text in zip(kinds, operands):
                if kind == 'j':
                    code += struct.pack('<h', labels.get(text, 0) - (len(code) + 2))
                elif kind == 's':
                    # a length, the bytes and a 0
                    data = text.encode('latin-1')
                    code += struct.pack('<H', len(data)) + data + b'\0'
                elif kind == 'n':
                    # a name of 9 bytes
                    code += text.encode('latin-1')[:8].ljust(9, b'\0')
                else:
                    code += operand(kind, text)
        labels = found
    return bytes(code) + bytes([usecode.END])
