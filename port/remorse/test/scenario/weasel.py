#!/usr/bin/env python3
"""No Remorse's weasel's shop (WEZZY.C).
In a fresh install (port/tools/install.py, without its movies) the game
starts in mission 2 (-warp 2), whose start has a dead guard (D_GUARD, item
5078) with a credits card of 17 in it: the script uses him (as a double
click searches him), and the card is the Silencer's. The cheat (JASSICA16)
and F10 twice (the first wakes the Silencer, whom the mission starts with
no hit points; the second gives 30,000 credits and the cheat's weapons,
which fill his weapon slots), then the cheat once more to turn it off (with
it the shop sells level 0's wares for nothing). The script's jump takes him
to the base (map 40) as a teleport pad ends a mission (startMapJump: the
equipment goes with him), where he uses the shop's panel (SPANEL, item 783:
its trigger has the booth, WEA_BOOT, show the shop of the mission).

In the shop: Enter buys the BA-40 for 525, which the shop refuses (his
weapon slots are full: its face stays as it was); Tab shows the ammunition; Enter twice buys two
BA-41 clips for 25 each; Escape asks whether to buy them, Y says yes and
delivers them, N to "anything else?" closes the shop. A save after it must
hold the card with 29,950 credits, the three clips (his one and the two)
and the weasel's visit (object 91). The refused buy must leave the shop's
face as it was, each other step change the screen; the game must be on the screen at
the end, and two runs must give the same screens, states and save.

usage: weasel.py REMORSE [--cd CD] [--keep DIR]
"""
import argparse, hashlib, os, struct, subprocess, sys, tempfile, zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]

ITEMS = 10240
# The item arrays of a save's object 0 (itemcach.cpp): x (a contained item's
# container), y and shape words, z and frame bytes, flags and quality words.
X, SHAPE, FLAGS, QUALITY = 0, 5 * ITEMS, 8 * ITEMS, 10 * ITEMS
ITEM_CONTAINED = 0x8
SHAPE_CREDITS, SHAPE_BA_41_CLIP = 0x4ed, 0x33d
CREDITS = 30000 - 2 * 25
CLIPS = 3
# The shop's face on the screen (its lights and the world behind blink).
FACE = (240, 60, 400, 420)
# The cheat's scan codes: J A S S I C A 1 6.
CHEAT = (0x24, 0x1e, 0x1f, 0x1f, 0x17, 0x2e, 0x1e, 0x02, 0x07)


def press(tick, code, extended=False):
    """A key's make and break codes (extended: E0h before each)."""
    prefix = 'e0 ' if extended else ''
    return f'{tick} key {prefix}{code:02x}\n{tick + 6} key {prefix}{code | 0x80:02x}\n'


def cheat(tick):
    """JASSICA16 from tick, and Enter to its message."""
    text = ''.join(f'{tick + 8 * k} key {code:02x}\n{tick + 8 * k + 3} key {code | 0x80:02x}\n'
                   for k, code in enumerate(CHEAT))
    return text + press(tick + 112, 0x1c)


# Ticks are sixtieths of a second; mission 2 is up at 900. The cheat's
# message closes 112 after its first key; the jump has the base up some 270
# later and the shop takes keys some 150 after the panel's use.
SHOP = 1668 + 150
SCRIPT = ('900 use 5078\n' + cheat(960) + press(1132, 0x44) + press(1172, 0x44) + cheat(1232) + '1400 jump 40\n'
          + '1668 use 783\n'
          + f'{SHOP} screen {{out}}/shop.png\n' + press(SHOP + 10, 0x1c) + f'{SHOP + 40} screen {{out}}/refused.png\n'
          + press(SHOP + 50, 0x0f) + f'{SHOP + 80} screen {{out}}/ammunition.png\n'
          + press(SHOP + 90, 0x1c) + f'{SHOP + 120} screen {{out}}/bought1.png\n'
          + press(SHOP + 130, 0x1c) + f'{SHOP + 160} screen {{out}}/bought2.png\n'
          + press(SHOP + 170, 0x01) + f'{SHOP + 200} screen {{out}}/confirm.png\n'
          + press(SHOP + 270, 0x15) + f'{SHOP + 300} screen {{out}}/delivered.png\n'
          + press(SHOP + 370, 0x31) + f'{SHOP + 460} screen {{out}}/closed.png\n'
          + f'{SHOP + 500} save 1\n{SHOP + 501} quit\n')
SCREENS = ('shop', 'refused', 'ammunition', 'bought1', 'bought2', 'confirm', 'delivered', 'closed')


def run(remorse, cd, work, name):
    """The screens', the state's and the save's bytes of a run in a fresh
    install."""
    target = work / name
    install = target / 'install'
    subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(cd),
                    '--no-movies'], check=True, capture_output=True, env=dict(os.environ, CRUSADER_GAME='noremorse'))
    script = target / 'weasel.script'
    script.write_text(SCRIPT.format(out=target))
    env = dict(os.environ, REGRET_SCRIPT=str(script), CRUSADER_CD=str(cd), REGRET_STATE=str(target / 'state.txt'))
    env.pop('REGRET_SCREEN', None)
    env.pop('REGRET_AUDIO', None)
    result = subprocess.run([str(remorse), '-warp', '2'], cwd=install, env=env, capture_output=True,
                            encoding='latin-1', timeout=600)
    if result.returncode != 0:
        sys.exit(f'remorse ended with {result.returncode}:\n{result.stdout[-2000:]}{result.stderr[-2000:]}')
    folder = next(p for p in install.iterdir() if p.name.upper() == 'SAVEGAME')
    save = next(p for p in folder.iterdir() if p.name.upper() == 'SAVEGAME.2').read_bytes()
    screens = {n: (target / f'{n}.png').read_bytes() for n in SCREENS}
    return screens, (target / 'state.txt').read_text(), save


def face(png):
    """The rows of the shop's face (FACE) in the port's PNG (filter 0 rows)."""
    at, data, width, sample = 8, b'', 0, 1
    while at < len(png):
        size = struct.unpack_from('>I', png, at)[0]
        kind, body = png[at + 4:at + 8], png[at + 8:at + 8 + size]
        if kind == b'IHDR':
            width = struct.unpack_from('>I', body)[0]
            sample = 3 if body[9] == 2 else 1
        elif kind == b'IDAT':
            data += body
        at += 12 + size
    raw = zlib.decompress(data)
    row = width * sample + 1
    left, top, right, bottom = FACE
    return [raw[y * row + 1 + left * sample:y * row + 1 + right * sample] for y in range(top, bottom)]


def save_objects(data):
    """{number: bytes} of a save's objects (its table at 80h, the count at
    54h), the empty ones left out."""
    count = struct.unpack_from('<I', data, 0x54)[0]
    out = {}
    for i in range(count):
        offset, size = struct.unpack_from('<II', data, 0x80 + 8 * i)
        if size:
            out[i] = data[offset:offset + size]
    return out


def carried(save):
    """{shape: quality} of what the Silencer (item 1) holds, and the weasel's
    visit (object 91)."""
    objects = save_objects(save)
    items = objects[0]
    word = lambda offset, i: struct.unpack_from('<H', items, offset + 2 * i)[0]
    held = {}
    for i in range(1, ITEMS):
        if word(SHAPE, i) and word(X, i) == 1 and word(FLAGS, i) & ITEM_CONTAINED:
            held[word(SHAPE, i)] = word(QUALITY, i)
    return held, objects.get(91, b'\0')[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('remorse', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', ROOT / 'cd/noremorse')))
    ap.add_argument('--keep', type=Path, help='keep the first run\'s screens, state and save here')
    args = ap.parse_args()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH remorse_weasel: {what}')

    with tempfile.TemporaryDirectory(prefix='port-remorse-weasel') as work:
        work = Path(work)
        first, first_state, first_save = run(args.remorse.resolve(), args.cd.resolve(), work, 'first')
        second, second_state, second_save = run(args.remorse.resolve(), args.cd.resolve(), work, 'second')
        if args.keep:
            args.keep.mkdir(parents=True, exist_ok=True)
            for name, data in first.items():
                (args.keep / f'{name}.png').write_bytes(data)
            (args.keep / 'state.txt').write_text(first_state)
            (args.keep / 'savegame.2').write_bytes(first_save)
    for name in SCREENS:
        check(first[name] == second[name], f'{name}.png differs between two runs')
    check(first_state == second_state, 'the states differ between two runs')
    check(first_save == second_save, 'the saves differ between two runs')
    check(face(first['refused']) == face(first['shop']), 'the refused buy changed the shop')
    steps = ('refused', 'ammunition', 'bought1', 'bought2', 'confirm', 'delivered', 'closed')
    for before, after in zip(steps, steps[1:]):
        check(first[before] != first[after], f'{after}.png is {before}.png')
    held, visited = carried(first_save)
    check(held.get(SHAPE_CREDITS) == CREDITS, f'the card has {held.get(SHAPE_CREDITS)} credits, not {CREDITS}')
    check(held.get(SHAPE_BA_41_CLIP) == CLIPS, f'{held.get(SHAPE_BA_41_CLIP)} BA-41 clips, not {CLIPS}')
    check(visited == 1, 'the weasel was not visited (object 91)')
    for name in SCREENS:
        print(f'{name}.png {hashlib.sha256(first[name]).hexdigest()[:16]}')
    print(f'state {hashlib.sha256(first_state.encode()).hexdigest()[:16]}, save '
          f'{hashlib.sha256(first_save).hexdigest()[:16]}: credits {held.get(SHAPE_CREDITS)}, '
          f'clips {held.get(SHAPE_BA_41_CLIP)}')
    print(f'remorse_weasel: {len(SCREENS)} screens, {failures} mismatches')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
