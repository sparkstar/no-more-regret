#!/usr/bin/env python3
"""The game's text in other languages: writes the C++ the executables carry
from the translators' files.

- port/common/fonts/ko.png: the Hangul font, the 2350 syllables of KS X 1001
  in their order (EUC-KR B0A1-C8FE), 64 to a row of 9x9 cells, the ink
  green; port/common/src/text/hangul_glyphs.cpp holds each syllable's rows by
  code point (hangul.h).
- port/lang/<language>/<game>/usecode.jsonl and subtitles.jsonl: a game's
  translations of its usecode's literals and of its movies' subtitles, a
  JSON object a line: "hash" (the original's FNV-1a 64-bit hash,
  port/tools/usecode_text.py's and subtitle_text.py's, 16 hex digits) and
  "text" (the translation, UTF-8; the usecode's with the original's
  controls: ^ a new row, * a pause), and the translators' own fields
  ("state", "note"); no English (the game's text is not ours to give).
  port/<game>/src/text/translations.cpp holds them by hash (translate.h).

A translation may hold printable ASCII (the game's font 6 has it) and the
font's syllables; anything else, a hash twice or a hash not of 16 hex digits
stops it. --check writes nothing and fails (exit 1) when a file differs from
what it would write (ctest text).

usage: make_text.py [--check]
"""
import argparse, json, re, struct, sys, zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from format import MISSING, binary, formatted  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / 'port'
FONT = PORT / 'common/fonts/ko.png'
GLYPHS = PORT / 'common/src/text/hangul_glyphs.cpp'
LANG = PORT / 'lang'
GAMES = {'regret': 'No Regret', 'remorse': 'No Remorse'}
# The languages of port/lang by their codes (language.h).
LANGUAGES = {'ko': 'Korean'}
# The kinds of text: the file of each and the name of its tables.
KINDS = {'usecode': 'usecode', 'subtitles': 'subtitle'}
CELL = 9
COLUMNS = 64
INK = (99, 207, 99)


def png_pixels(path):
    """(width, height, rows of (r, g, b, a)) of an 8-bit RGB or RGBA PNG
    without interlace."""
    data = Path(path).read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        sys.exit(f'{path}: not a PNG')
    at, idat, header = 8, b'', None
    while at < len(data):
        size = struct.unpack_from('>I', data, at)[0]
        kind, body = data[at + 4:at + 8], data[at + 8:at + 8 + size]
        if kind == b'IHDR':
            header = struct.unpack('>IIBBBBB', body)
        elif kind == b'IDAT':
            idat += body
        at += 12 + size
    width, height, depth, colour, _, _, interlace = header
    if depth != 8 or colour not in (2, 6) or interlace:
        sys.exit(f'{path}: an 8-bit RGB or RGBA PNG without interlace is needed')
    step = 4 if colour == 6 else 3
    raw = zlib.decompress(idat)
    stride = width * step
    rows, previous = [], bytearray(stride)
    for y in range(height):
        kind = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for x in range(stride):
            left = line[x - step] if x >= step else 0
            up = previous[x]
            corner = previous[x - step] if x >= step else 0
            if kind == 1:
                line[x] = (line[x] + left) & 0xff
            elif kind == 2:
                line[x] = (line[x] + up) & 0xff
            elif kind == 3:
                line[x] = (line[x] + (left + up) // 2) & 0xff
            elif kind == 4:
                guess = left + up - corner
                near = min((abs(guess - left), 0, left), (abs(guess - up), 1, up), (abs(guess - corner), 2, corner))[2]
                line[x] = (line[x] + near) & 0xff
        rows.append([tuple(line[x:x + 3]) + ((line[x + 3],) if step == 4 else (255,)) for x in range(0, stride, step)])
        previous = line
    return width, height, rows


def syllables():
    """The Hangul syllables of KS X 1001 in its order."""
    return [bytes([high, low]).decode('euc-kr') for high in range(0xB0, 0xC9) for low in range(0xA1, 0xFF)]


def font_glyphs():
    """{code point: [rows as 9-bit numbers, bit 8 the leftmost]} of ko.png."""
    width, height, pixels = png_pixels(FONT)
    order = syllables()
    if width != COLUMNS * CELL or height < (len(order) + COLUMNS - 1) // COLUMNS * CELL:
        sys.exit(f'{FONT}: {width}x{height}, not {COLUMNS} cells of {CELL} a row for {len(order)} syllables')
    glyphs = {}
    for k, syllable in enumerate(order):
        left, top = k % COLUMNS * CELL, k // COLUMNS * CELL
        rows = []
        for y in range(CELL):
            bits = 0
            for x in range(CELL):
                if pixels[top + y][left + x][:3] == INK:
                    bits |= 1 << (CELL - 1 - x)
            rows.append(bits)
        if not any(rows):
            sys.exit(f'{FONT}: {syllable} (cell {k}) is empty')
        glyphs[ord(syllable)] = rows
    return glyphs


def glyphs_cpp(glyphs):
    lines = [
        '/* The Hangul font\'s glyphs (hangul.h), written by port/tools/make_text.py',
        ' * from port/common/fonts/ko.png; do not edit. */',
        '#include "hangul.h"',
        '',
        'const HangulGlyph HANGUL_GLYPHS[HANGUL_GLYPH_COUNT] = {',
    ]
    for code in sorted(glyphs):
        rows = ', '.join(f'0x{row:03x}' for row in glyphs[code])
        lines.append(f'    {{0x{code:04x}, {{{rows}}}}}, /* {chr(code)} */')
    lines.append('};')
    return '\n'.join(lines) + '\n'


def translations(game, kind, glyphs):
    """{language code: [(hash, text)] by hash} of a game's files of a kind."""
    out = {}
    for code in sorted(LANGUAGES):
        path = LANG / code / game / f'{kind}.jsonl'
        if not path.exists():
            continue
        entries = {}
        for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
            if not line.strip():
                continue
            where = f'{path.relative_to(ROOT)} line {number}'
            try:
                entry = json.loads(line)
            except json.JSONDecodeError as error:
                sys.exit(f'{where}: {error}')
            key, text = entry.get('hash', ''), entry.get('text')
            if not re.fullmatch(r'[0-9a-f]{16}', key) or not isinstance(text, str):
                sys.exit(f'{where}: a "hash" of 16 hex digits and a "text" are needed')
            if key in entries:
                sys.exit(f'{where}: hash {key} twice')
            bad = sorted({ch for ch in text if not (' ' <= ch <= '~' or ord(ch) in glyphs)})
            if bad:
                sys.exit(f'{where}: no glyph for {"".join(bad)!r} (printable ASCII and the 2350 syllables of KS X 1001)')
            entries[key] = text
        out[code] = sorted((int(key, 16), text) for key, text in entries.items())
    return out


def literal(text):
    """text as a C++ string literal (the source is UTF-8)."""
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"') + '"'


def translations_cpp(game, kinds):
    """The C++ of a game's translations: {kind: {language code: entries}}."""
    lines = [
        f'/* {GAMES[game]}\'s translations of its text (translate.h), written by',
        f' * port/tools/make_text.py from port/lang/<language>/{game}/usecode.jsonl and',
        ' * subtitles.jsonl; do not edit. */',
        '#include "translate.h"',
        '',
        '#include <iterator>',
        '',
    ]
    arrays = []
    body = []
    for kind, languages in kinds.items():
        for code, entries in languages.items():
            # A constant array: upper case (clang-tidy's readability-identifier-naming).
            name = f'{LANGUAGES[code].upper()}_{KINDS[kind].upper()}'
            arrays.append((kind, code, name))
            body.append(f'/* {code}, {kind}: {len(entries)} texts, by hash. */')
            body.append(f'const Translation {name}[] = {{')
            for key, text in entries:
                body.append(f'    {{0x{key:016x}ULL, {literal(text)}}},')
            body.append('};')
            body.append('')
    if arrays:
        lines += ['namespace {', ''] + body + ['}', '']
    for kind in KINDS:
        table = f'{KINDS[kind].upper()}_TRANSLATION_TABLES'
        tables = [f'{{Language::{LANGUAGES[code].upper()}, {name}, std::size({name})}}' for k, code, name in arrays if k == kind]
        if tables:
            lines.append(f'const Translations {table}[] = {{' + ', '.join(tables) + '};')
            lines.append(f'const std::size_t {table[:-1]}_COUNT = std::size({table});')
        else:
            lines.append(f'const Translations {table}[] = {{{{Language::ENGLISH, nullptr, 0}}}};')
            lines.append(f'const std::size_t {table[:-1]}_COUNT = 0;')
    return '\n'.join(lines) + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    if binary() is None:
        # The files are in clang-format 20's format; ctest counts 77 as skipped.
        print(f'make_text: {MISSING}')
        return 77
    glyphs = font_glyphs()
    files = {GLYPHS: glyphs_cpp(glyphs)}
    for game in GAMES:
        kinds = {kind: translations(game, kind, glyphs) for kind in KINDS}
        files[PORT / game / 'src/text/translations.cpp'] = translations_cpp(game, kinds)
    stale = []
    for path, text in files.items():
        text = formatted(text, str(path))
        if path.exists() and path.read_text(encoding='utf-8') == text:
            continue
        stale.append(path)
        if not args.check:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding='utf-8')
    for path in stale:
        print(f'{path.relative_to(ROOT)}: {"differs from what make_text.py writes" if args.check else "written"}')
    if args.check and stale:
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
