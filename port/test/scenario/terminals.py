#!/usr/bin/env python3
"""The terminals in other languages (the game's text in other languages):
terminals of mission 1 (-warp 1) used in a fresh install (the script's use
of the terminal's item), in English (the original's ComputerGump) and in
Korean (enhanced.cfg's language=ko: the usecode's literals translated
where OP_PUSH_STRING runs, port/lang/ko, and TerminalGump's layout with
the Hangul font). Each page is shot once the terminal has typed it (the
waits of ComputerGump: No Remorse's * waits 4.8 seconds), then Enter turns
it; for each page the terminal shows, the first 16 digits of the SHA-256
of the terminal's part of the screen (TERMINAL).

The terminals (CASES), each with the text its quality picks: No Regret's
MONITNS 1704 (text 157, a letter: Mr. Belye's VIR chip) and WALLMEW 345
(text 18: Di-Cor's passcode, a number joined into translated pieces, two
pages); No Remorse's MONITEW 5447 (text 18: DBEYER's keycard, a number and
a piece MGREENE's shares), MONITNS 4901 (text 19: MGREENE's passcodes) and
7224 (text 173: RESISTNET's welcome, its glitches and pauses).

The pages and their hashes must be those of port/<game>/test/expected/terminals.tsv,
which --write writes; Korean's must differ from English's. Without the CD
it is skipped (exit 77, which ctest counts as skipped).

usage: terminals.py PROGRAM [--cd CD] [--write] [--keep DIR]
"""
import argparse, hashlib, os, struct, subprocess, sys, tempfile, zlib
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402

SKIPPED = 77
TABLE = overlay.TESTS / 'expected/terminals.tsv'
START = 600
# (name, item, the ticks a page takes to type) by game: the item numbers of
# mission 1's terminals in a fresh install's save.
CASES = {
    'regret': [('belye', 1704, 1500), ('dicor', 345, 1500)],
    'remorse': [('dbeyer', 5447, 1500), ('mgreene', 4901, 1500), ('resist', 7224, 2400)],
}[overlay.GAME]
LANGUAGES = ('en', 'ko')
PAGES = 3
# The terminal's gump on the 640x480 screen (centred: 252x221 at 194, 129),
# and two of its pixels: the bezel (light) and the screen inside (black).
TERMINAL = (194, 129, 446, 350)
BEZEL = (210, 229)
INSIDE = (330, 229)
ENTER = ('1c', '9c')


def screen_rows(png):
    """(width, rows of RGB bytes) of the port's PNG (system/png.cpp: 8 bits,
    filter 0, its palette for an 8-bit mode's screen, RGB for 16-bit)."""
    at, data, width, height, colour, palette = 8, b'', 0, 0, 0, b''
    while at < len(png):
        size = struct.unpack_from('>I', png, at)[0]
        kind, body = png[at + 4:at + 8], png[at + 8:at + 8 + size]
        if kind == b'IHDR':
            width, height = struct.unpack_from('>II', body)
            colour = body[9]
            if body[8] != 8 or colour not in (2, 3):
                sys.exit('terminals: the screen is not an 8-bit RGB or palette PNG')
        elif kind == b'PLTE':
            palette = body
        elif kind == b'IDAT':
            data += body
        at += 12 + size
    raw = zlib.decompress(data)
    step = 3 if colour == 2 else 1
    stride = width * step + 1
    rows = []
    for y in range(height):
        if raw[y * stride] != 0:
            sys.exit('terminals: a row of the screen is filtered')
        line = raw[y * stride + 1:(y + 1) * stride]
        if colour == 3:
            line = b''.join(palette[3 * index:3 * index + 3] for index in line)
        rows.append(line)
    return width, rows


def terminal_hash(png):
    """The hash of the terminal's part of the screen; None when the terminal
    is not on it."""
    _, rows = screen_rows(png)
    pixel = lambda x, y: rows[y][3 * x:3 * x + 3]
    if sum(pixel(*BEZEL)) < 3 * 150 or sum(pixel(*INSIDE)) > 3 * 40:
        return None
    left, top, right, bottom = TERMINAL
    return hashlib.sha256(b''.join(rows[y][3 * left:3 * right] for y in range(top, bottom))).hexdigest()[:16]


def run(program, cd, work, case, language):
    """The hashes of the pages a terminal shows."""
    name, item, wait = case
    target = work / f'{name}_{language}'
    install = target / 'install'
    target.mkdir(parents=True)
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(cd), '--no-movies'],
                   check=True, capture_output=True)
    if language != 'en':
        (install / 'enhanced.cfg').write_bytes(f'language={language}\r\n'.encode())
    lines = [f'{START} use {item}']
    tick = START + wait
    for page in range(PAGES):
        lines += [f'{tick} screen {target}/page{page}.png', f'{tick + 10} key {ENTER[0]}', f'{tick + 16} key {ENTER[1]}']
        tick += wait
    lines.append(f'{tick} quit')
    script = target / 'script'
    script.write_text('\n'.join(lines) + '\n')
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd))
    for variable in ('REGRET_AUDIO', 'REGRET_SCREEN', 'REGRET_STATE', 'REGRET_FRAMES'):
        env.pop(variable, None)
    result = subprocess.run([str(program), '-warp', '1'], cwd=install, env=env, capture_output=True,
                            encoding='latin-1', timeout=1200)
    if result.returncode != 0:
        sys.exit(f'{program.name} ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    hashes = []
    for page in range(PAGES):
        digest = terminal_hash((target / f'page{page}.png').read_bytes())
        if digest is None:
            break
        hashes.append(digest)
    return hashes


def read_table():
    rows = {}
    if TABLE.exists():
        for line in TABLE.read_text().splitlines():
            if line and not line.startswith('#'):
                name, language, hashes = line.split('\t')
                rows[(name, language)] = hashes.split(',') if hashes else []
    return rows


def write_table(rows):
    header = ('# The terminals of mission 1 (port/test/scenario/terminals.py --write): for each terminal and language the\n'
              '# first 16 digits of the SHA-256 of the terminal\'s part of the screen, a page each, as the port\n'
              '# shows them.\n')
    body = ''.join(f'{name}\t{language}\t{",".join(hashes)}\n' for (name, language), hashes in sorted(rows.items()))
    TABLE.write_text(header + body)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', overlay.ORIGIN)))
    ap.add_argument('--write', action='store_true')
    ap.add_argument('--keep', type=Path)
    args = ap.parse_args()
    if not overlay.origin_path('USECODE/EUSECODE.FLX', args.cd).exists():
        print(f'terminals: no CD at {args.cd}, skipped')
        return SKIPPED
    with tempfile.TemporaryDirectory(prefix='port-terminals') as temporary:
        work = args.keep or Path(temporary)
        jobs = [(case, language) for case in CASES for language in LANGUAGES]
        with ThreadPoolExecutor(max_workers=4) as pool:
            found = dict(zip(((case[0], language) for case, language in jobs),
                             pool.map(lambda job: run(args.program.resolve(), args.cd.resolve(), work, *job), jobs)))
    if args.write:
        write_table(found)
        print(f'terminals: {TABLE} written')
        return 0
    expected = read_table()
    failures = 0
    for (name, language), hashes in sorted(found.items()):
        if not hashes:
            failures += 1
            print(f'MISMATCH terminals: {name} {language}: no terminal on the screen')
        elif expected.get((name, language)) != hashes:
            failures += 1
            print(f'MISMATCH terminals: {name} {language}: {hashes}, not {expected.get((name, language))}')
    for case in CASES:
        if found.get((case[0], 'ko')) == found.get((case[0], 'en')):
            failures += 1
            print(f'MISMATCH terminals: {case[0]}: Korean shows what English does')
    print(f'terminals: {len(found)} runs, {sum(len(h) for h in found.values())} pages, {failures} mismatches')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
