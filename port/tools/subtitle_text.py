#!/usr/bin/env python3
"""The movies' subtitles (the game's text in other languages): every line
of the CD's subtitle files as JSON for the translation's tools, read as
the port reads them (common/src/movie/subtitles.cpp): No Regret's
FLICS/*.IFF, the lines of the English chunk (ENGL: a frame, a length and
the text, CP437); No Remorse's flics/*.txt (an "@frame N" marker, the line
after it its text when it is no marker and not blank; a file without text
shows none).

Each line: its movie (the file's name), its number in the file (the cue
the player counts), its frame and the next line's (when it gives way), the
text as UTF-8, its FNV-1a 64-bit hash (the translations' key, as the port
hashes the line it read) and its width in the game's subtitle font (No
Regret's font 3, 8 pixels a character; No Remorse's font 4, 7 and a
spacing of 1) against the strip's (640, 600); the same text elsewhere.

The output holds the game's English text, which is not ours to give: it
goes to build/text (outside the repository) by default.

usage: subtitle_text.py [--cd CD] [-o OUT]
"""
import argparse, json, re, struct, sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
# The game's subtitle font and its strip (<game>/src/movie/movie_game.h's
# SubtitleStyle) and the characters' advance (cafont.cpp's charSpacings).
STYLE = {'regret': {'font': 3, 'spacing': 0, 'width': 640, 'files': 'IFF'},
         'remorse': {'font': 4, 'spacing': 1, 'width': 600, 'files': 'TXT'}}[overlay.GAME]


def fnv1a64(data):
    value = 0xcbf29ce484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return value


def iff_lines(data, language=b'ENGL'):
    """[(frame, text)] of an IFF file's language chunk (readIffSubtitles):
    the first place its letters are, the chunk's size big-endian, lines up
    to its end or a negative frame."""
    at = data.find(language)
    if at < 0 or at + 8 > len(data):
        return []
    end = min(len(data), at + 8 + struct.unpack_from('>I', data, at + 4)[0])
    at += 8
    out = []
    while at + 5 <= end:
        frame = struct.unpack_from('<i', data, at)[0]
        if frame < 0:
            break
        length = data[at + 4]
        raw = data[at + 5:at + 5 + length] if at + 5 + length <= len(data) else b''
        # The port reads the line as a C string: up to its first 0.
        out.append((frame, raw.split(b'\0')[0].decode('cp437')))
        at += 5 + length
    return out


def text_lines(data):
    """[(cue, frame, text)] of a TXT file (readTextSubtitles): the markers'
    frames (from the eighth character, atoi) and the line after each when it
    is no marker and not blank; none when no marker has text."""
    lines = data.replace(b'\r', b'').split(b'\n')
    if lines and not lines[-1]:
        lines.pop()
    markers = []
    for i, line in enumerate(lines):
        if not line.startswith(b'@'):
            continue
        found = re.match(rb'\s*([+-]?\d+)', line[7:]) if len(line) > 7 else None
        frame = int(found.group(1)) if found else 0
        text = ''
        if i + 1 < len(lines) and not lines[i + 1].startswith(b'@') and lines[i + 1].strip(b' \t'):
            text = lines[i + 1].split(b'\0')[0].decode('cp437')
        markers.append((frame, text))
    cues = [(k, frame, text) for k, (frame, text) in enumerate(markers) if frame >= 0]
    return cues if any(text for _, _, text in cues) else []


def glyph_widths():
    """{character code: width} of the game's subtitle font (fonts.flx)."""
    data = overlay.origin_path('STATIC/FONTS.FLX').read_bytes()
    count = struct.unpack_from('<I', data, 0x54)[0]
    offset, size = struct.unpack_from('<II', data, 0x80 + 8 * STYLE['font'])
    shape = data[offset:offset + size]
    frames = struct.unpack_from('<h', shape, 4)[0]
    widths = {}
    for k in range(max(frames, 0)):
        at, length = struct.unpack_from('<Ii', shape, 6 + 8 * k)
        at = (at & 0x7fffffff) + (8 if at & 0x80000000 else 0)
        if length > 0 and at + 0x14 <= len(shape):
            widths[k] = struct.unpack_from('<i', shape, at + 4)[0]
    assert count > STYLE['font']
    return widths


def width_of(text, widths):
    """The text's width in the font (GumpFont::textWidth: each glyph's width
    and the spacing); a character CP437 has not counts as none."""
    total = 0
    for ch in text:
        try:
            code = ch.encode('cp437')[0]
        except UnicodeEncodeError:
            continue
        total += widths.get(code, 0) + STYLE['spacing']
    return total


def extract(cd):
    flics = overlay.origin_path('FLICS', cd)
    widths = glyph_widths()
    cues = []
    for path in sorted(flics.iterdir()):
        if path.suffix.upper() != '.' + STYLE['files']:
            continue
        data = path.read_bytes()
        if STYLE['files'] == 'IFF':
            lines = [(k, frame, text) for k, (frame, text) in enumerate(iff_lines(data))]
            others = [language.decode() for language in (b'GERM', b'FREN') if iff_lines(data, language)]
        else:
            lines = text_lines(data)
            others = []
        for n, (cue, frame, text) in enumerate(lines):
            raw = text.encode('utf-8')
            cues.append({
                'movie': path.stem,
                'cue': cue,
                'frame': frame,
                'end': lines[n + 1][1] if n + 1 < len(lines) else None,
                'text': text,
                'hash': f'{fnv1a64(raw):016x}',
                'width': width_of(text, widths),
                'otherLanguages': others,
            })
    by_hash = defaultdict(list)
    for cue in cues:
        by_hash[cue['hash']].append(f'{cue["movie"]}:{cue["cue"]}')
    for cue in cues:
        cue['sameText'] = [place for place in by_hash[cue['hash']] if place != f'{cue["movie"]}:{cue["cue"]}']
    shown = [cue for cue in cues if cue['text'].strip()]
    return {
        'game': overlay.GAME,
        'source': str(flics.resolve().relative_to(ROOT)) if flics.resolve().is_relative_to(ROOT) else str(flics),
        'tool': 'port/tools/subtitle_text.py',
        'style': {'font': STYLE['font'], 'stripWidth': STYLE['width']},
        'summary': {
            'movies': len({cue['movie'] for cue in cues}),
            'cues': len(cues),
            'shown': len(shown),
            'uniqueTexts': len({cue['hash'] for cue in shown}),
            'widest': max((cue['width'] for cue in shown), default=0),
        },
        'cues': cues,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('--cd', type=Path, default=overlay.ORIGIN)
    ap.add_argument('-o', '--output', type=Path, default=ROOT / 'build/text' / f'{overlay.GAME}_subtitles.json')
    args = ap.parse_args()
    result = extract(args.cd)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=1) + '\n')
    summary = result['summary']
    print(f'{args.output}: {summary["movies"]} movies, {summary["cues"]} lines ({summary["uniqueTexts"]} texts), '
          f'widest {summary["widest"]} of {STYLE["width"]}')


if __name__ == '__main__':
    sys.exit(main())
