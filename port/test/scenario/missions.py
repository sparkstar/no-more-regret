#!/usr/bin/env python3
"""The usecode at the missions' starts: with
the game of CRUSADER_GAME installed from its CD for the sound card without
its movies, each scenario is a recording made from keys: the original's
own arguments (-warp to a mission, -egg to a teleport egg in it; run.tsv's
arguments, port/system/replay.h), its cheat and the controlled NPC's
immortality (Ctrl+F10) at once, then the scenario's keys. Replayed without
a window with the usecode's trace (REGRET_USECODE_TRACE, platform/trace.h):
- the run ends at the recording's end, the Silencer alive (or dead in the
  scenario of his death) and the scenario's NPCs dead;
- the trace has the scenario's lines in their order (what its usecode is to
  do: an event to a class, a function of it, an intrinsic, a teleport);
- the states (checks.tsv, each 60th pass) written, a second replay is as
  recorded;
- the recording's arguments are the replay's: other game arguments on the
  command line end the run, and the replay recorded again (--record) keeps
  them.
The recordings and their traces stay in DIR with --keep (for
tools/usecode.py coverage, or to go among the player's in replays/).

usage: missions.py PROGRAM [--cd CD] [--keep DIR]
"""
import argparse, os, re, shutil, subprocess, sys, tempfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
import replay  # noqa: E402

# The cheats (control.cpp: No Regret's LOOSECANNON16, No Remorse's JASSICA16)
# and the keys' scan codes (set 1).
CHEAT = {'regret': 'loosecannon16', 'remorse': 'jassica16'}[overlay.GAME]
SCANS = dict(zip('1234567890', range(0x02, 0x0c))) | dict(zip('qwertyuiop', range(0x10, 0x1a))) | \
    dict(zip('asdfghjkl', range(0x1e, 0x27))) | dict(zip('zxcvbnm', range(0x2c, 0x33)))
ENTER, CTRL, F10 = 0x1c, 0x1d, 0x44
# The keypad's directions (NumLock off: the game's isometric ones) in map
# units: 8 north-west (-x and -y a little), 9 -y, 6 +x -y, 3 +x, 2 +x +y,
# 1 +y, 4 -x +y, 7 -x.
KEYPAD = {'8': 0x48, '9': 0x49, '6': 0x4d, '3': 0x51, '2': 0x50, '1': 0x4f, '4': 0x4b, '7': 0x47}


@dataclass
class Scenario:
    name: str
    arguments: list
    # From cheat on: the cheat typed a key each 2 frames, its notice closed,
    # Ctrl+F10 and its notice closed, 30 frames for it all; None for none.
    cheat: int
    # The keypad's keys held: (frame, key, frames).
    walks: list
    end: int
    # The trace's lines in their order, as regular expressions after the pass.
    expect: list
    # The Silencer's place at the end (x, y, z), if checked.
    place: tuple = None
    notes: str = ''
    games: tuple = field(default=('regret',))
    # Other keys pressed: (frame, scan code, frames held); E0xxh for an
    # extended key (the arrows, Insert, Page Up and the others the keypad's
    # share their codes with).
    presses: list = field(default_factory=list)
    # The Silencer alive at the end (else dead: the scenario's death).
    alive: bool = True
    # The NPCs dead at the end (the state's count, the Silencer among them),
    # if checked.
    dead: int = None


# No Regret's missions as -warp starts them: the frame the mission's egg
# (MISSn, its class) gives the Silencer back (setHeld 0), after which the
# cheat goes in; 11, 12, 15 and 16 have no map, as in the original.
# Without the immortality the Silencer dies in 1, 3, 6, 7 and 9.
REGRET_STARTS = {1: (376, '090D'), 2: (354, '090E'), 3: (290, '090F'), 4: (416, '0910'), 5: (432, '0911'),
                 6: (415, '0912'), 7: (375, '0913'), 8: (373, '0901'), 9: (416, '0915'), 10: (416, '0916'),
                 13: (471, '0917'), 14: (477, '0917')}


def pad(name, mission, egg, key, sends, place, final=None):
    """No Regret's teleport pad (TELEPAD) with the arrival egg egg on it: the
    Silencer lands there, a step (key) ends the arrival's hold (event 9) and
    the next event 6 sends him to egg sends at place (x, y, z, map)."""
    x, y, z, number = place
    return Scenario(f'regret-mission{mission}-{name}', ['-warp', str(mission), '-egg', str(egg)], cheat=100,
                    walks=[(200, key, 20)], end=600,
                    expect=[r'E \d+ 01DB:0006', r'U \d+ 01DB:0020', r'I \d+ 01DB:0020 07C ',
                            rf'I \d+ 01DB:0020 09A {sends:02X} 00 ', rf'T 1 {x} {y} {z} {number}'],
                    place=final or (x, y, z))


SCENARIOS = [
    # Each mission's start: its egg hatches (event 7: the walk in, the
    # Silencer held and given back), the map's NPCs and eggs come in, and
    # its enemies fight the immortal Silencer for ten seconds.
    *[Scenario(f'regret-mission{n}-start', ['-warp', str(n)], cheat=held + 6, walks=[], end=held + 600,
               expect=[rf'E \d+ {egg}:0007', rf'U \d+ {egg}:0007']) for n, (held, egg) in REGRET_STARTS.items()],
    # Teleport pads (TELEPAD): mission 9's start room's (quality 3Dh: egg 61
    # on the floor above), mission 2's of frame 1, mission 7's across the map
    # and mission 6's from the floor above to another (egg 18 at z 112, the
    # Silencer falls to the floor at 104).
    pad('telepad', 9, 73, '1', 61, (3918, 4330, 104, 17)),
    pad('telepad', 2, 4, '1', 2, (23090, 7402, 8, 3)),
    pad('telepad', 7, 11, '1', 12, (27119, 7279, 8, 13)),
    pad('telepad', 6, 19, '1', 18, (2175, 29823, 112, 11), final=(2175, 29823, 104)),
    # Mission 10's pad of quality 109 (6Dh): to egg 109 of map 20, the
    # mission's end (FREE's function 22h after the fade: the keycards, the
    # music and the alert off); in map 20 NPC's function 23h takes the
    # immortality away (the run ends before an enemy kills the Silencer).
    # From egg 250, south to the pad and a step west onto its middle.
    Scenario('regret-mission10-telepad-map20', ['-warp', '10', '-egg', '250'], cheat=100,
             walks=[(200, '1', 120), (325, '4', 20)], end=900,
             expect=[r'U \d+ 01DB:0020', r'I \d+ 01DB:0020 07F ', r'C \d+ 0A1D:0022',
                     r'I \d+ 01DB:0020 086 14 00 6D 00 ', r'M 20', r'T 1 3695 30671 8 20', r'I \d+ 0A0F:0023 088 '],
             place=(3695, 30671, 8)),
    # A fight: mission 1's start without the cheat,
    # milestone 2's walk (test/scenario/milestone2.py) after the
    # Silencer is given back and four shots; the enemies hit him and the
    # shots kill NPC 3 (its Attack ends, its death's animation calls it,
    # event 14), the Silencer alive at the end.
    Scenario('regret-mission1-fight', ['-warp', '1'], cheat=None,
             walks=[(400, '8', 4), (412, '8', 30), (460, '2', 4), (472, '2', 18)], end=900,
             presses=[(510 + 40 * k, 0x39, 6) for k in range(4)], dead=1,
             expect=[r'E \d+ 090D:0007', r'- \d+ \d+ 3 Attack', r'E 3 \S+:000E']),
    # The Silencer's death: mission 1's start without the
    # cheat; its enemies hit him and kill him.
    Scenario('regret-mission1-death', ['-warp', '1'], cheat=None, walks=[], end=1500, alive=False, dead=1,
             expect=[r'E \d+ 090D:0007']),
    # The eggs' race: at mission 1's start the immortal
    # Silencer stands in ONCEEGG 3254 (its trigger 5); the keypad's 6
    # (north-east), Insert and Page Up (rolls left and right), the right
    # arrow (a turn) and Page Up again roll him out of it and back in two
    # passes later. The exit's function (15h) holds the egg's class at
    # 0905h (0900h and the trigger) while its TRIGGER runs, so the entry
    # goes to MHATCHER's function 7, which wakes monster 5045 (event 10),
    # as No Remorse's race had the camera follow its egg (CAMERA, 0905h
    # there). The keys were found by random ones.
    Scenario('regret-mission1-egg-race', ['-warp', '1'], cheat=380, walks=[], end=900, dead=1,
             presses=[(440, 0x4d, 12), (463, 0xe052, 7), (489, 0xe049, 8), (502, 0x4d, 28), (543, 0x4d, 14),
                      (567, 0xe04d, 20), (587, 0xe049, 27)],
             expect=[r'E 3254 0901:0015', r'E 3254 0905:0007', r'E 5045 \S+:000A']),
]


# No Remorse's missions as -warp starts them (16 is the base, map 40): the
# frame the cheat goes in (after mission 1's and 15's eggs give the
# Silencer back, setHeld 0; the others hold him not), the mission's map
# and its egg's event the start sends (MISS1EGG's and MISS15's hatch, the
# others' cachein; mission 5's map has no egg of its class).
REMORSE_STARTS = {1: (192, 1, '090D:0007'), 2: (100, 3, '090E:0004'), 3: (100, 5, '090F:0004'),
                  4: (100, 7, '0910:0004'), 5: (100, 9, None), 6: (100, 11, '0912:0004'), 7: (100, 13, '0913:0004'),
                  8: (100, 15, '0914:0004'), 9: (100, 17, '0915:0004'), 10: (100, 19, '0916:0004'),
                  11: (100, 21, '0917:0004'), 12: (100, 23, '0918:0004'), 13: (100, 25, '0919:0004'),
                  14: (100, 27, '091A:0004'), 15: (385, 29, '091B:0007'), 16: (100, 40, '0907:0007')}
REMORSE = ('remorse',)


def start_lines(map, egg):
    """A start's trace lines: the egg's cachein as its map loads, or its
    hatch after."""
    if egg is None:
        return [rf'M {map}']
    if egg.endswith(':0004'):
        return [rf'E \d+ {egg}', rf'M {map}']
    return [rf'M {map}', rf'E \d+ {egg}']


SCENARIOS += [
    # Each mission's start and the base's: its egg's event and its map, the
    # immortal Silencer for ten seconds.
    *[Scenario(f'remorse-mission{n}-start', ['-warp', str(n)], cheat=cheat, walks=[], end=cheat + 600, games=REMORSE,
               expect=start_lines(map, egg)) for n, (cheat, map, egg) in REMORSE_STARTS.items()],
    # Mission 1's pads (TELEPAD with RTELEPAD): eggs 20 and 21 each on a pad
    # that sends to the other, 14 and 15 the same; the Silencer lands on the
    # pad's egg, a step centres him on it and it sends him.
    Scenario('remorse-mission1-telepad', ['-warp', '1', '-egg', '20'], cheat=None, walks=[(120, '1', 20)], end=600,
             games=REMORSE, expect=[r'E \d+ 01DB:0006', r'T 1 22669 15579 8 1'], place=(22669, 15579, 8)),
    Scenario('remorse-mission1-telepad-14', ['-warp', '1', '-egg', '14'], cheat=None, walks=[(120, '1', 20)],
             end=600, games=REMORSE, expect=[r'E \d+ 01DB:0006', r'T 1 23829 17207 8 1'], place=(23829, 17207, 8)),
    # Mission 1's end: warped onto its last pad (TELEPAD 4166, quality 1Eh),
    # a step sends the Silencer to the base's egg 30 (map 40, loaded).
    Scenario('remorse-mission1-endpad-base', ['-warp', '1', '15500', '18028', '104'], cheat=None,
             walks=[(100, '1', 12)], end=800, games=REMORSE,
             expect=[r'E \d+ 01DB:0006', r'T 1 0 0 0 40', r'M 40', r'T 1 15254 17038 24 40'],
             place=(15254, 17038, 24)),
    # A fight: mission 2's start, the immortal Silencer turned
    # south and firing eight times kills NPC 2 (its Attack ends, its death's
    # animation calls it, event 14) long before it would die of itself.
    Scenario('remorse-mission2-fight', ['-warp', '2'], cheat=100, walks=[(150, '1', 3)], end=700, games=REMORSE,
             presses=[(160 + 60 * k, 0x39, 6) for k in range(8)], dead=1,
             expect=[r'- \d+ \d+ 2 Attack', r'E 2 \S+:000E']),
    # The Silencer's death: mission 2's start without the
    # cheat; its enemies hit him and kill him.
    Scenario('remorse-mission2-death', ['-warp', '2'], cheat=None, walks=[], end=1000, alive=False, dead=2,
             games=REMORSE, expect=[r'M 3']),
    # The eggs' race at the base: warped to the base by TRIGEGG 1007
    # (trigger 5), the down arrow's first step takes the Silencer
    # out of its east side and the next back in three passes later. The
    # exit's function (15h) holds the egg's class at 0905h while its TRIGGER
    # runs, so the entry goes to CAMERA's function 7: the camera moves to the
    # egg and follows it (setFollowedItem 03EFh). Trigger 5 opens the door
    # west of the egg: south out of the egg and back in, its entry is
    # TRIGEGG's again and the Silencer walks through.
    Scenario('remorse-base-egg-race', ['-warp', '16', '16788', '18067', '16'], cheat=None,
             walks=[(150, '1', 120), (300, '9', 200), (530, '7', 420)], end=1050, games=REMORSE,
             presses=[(100, 0xe050, 8)], dead=0, place=(15727, 17903, 16),
             expect=[r'E 1007 0900:0015', r'E 1007 0905:0007', r'I \d+ 0905:0007 041 EF 03 ', r'E 1007 0900:0015',
                     r'E 1007 0900:0007']),
]


def cheat_inputs(frame):
    """The cheat, its notice closed, the immortality and its notice closed:
    inputs.tsv's lines from frame on."""
    lines = []
    for letter in CHEAT:
        lines += [f'{frame}\tkey\t{SCANS[letter]:02x}', f'{frame + 1}\trelease\t{SCANS[letter]:02x}']
        frame += 2
    frame += 8
    lines += [f'{frame}\tkey\t{ENTER:02x}', f'{frame + 1}\trelease\t{ENTER:02x}']
    frame += 6
    lines += [f'{frame}\tkey\t{CTRL:02x}', f'{frame + 1}\tkey\t{F10:02x}', f'{frame + 2}\trelease\t{F10:02x}',
              f'{frame + 3}\trelease\t{CTRL:02x}']
    frame += 10
    lines += [f'{frame}\tkey\t{ENTER:02x}', f'{frame + 1}\trelease\t{ENTER:02x}']
    return lines


def write_recording(scenario, directory, install):
    directory.mkdir(parents=True)
    lines = ['# frame\tinput\tvalues (' + scenario.name + ')', '0\tdate\t1996-01-01']
    if scenario.cheat is not None:
        lines += cheat_inputs(scenario.cheat)
    for frame, key, frames in scenario.walks:
        lines += [f'{frame}\tkey\t{KEYPAD[key]:02x}', f'{frame + frames}\trelease\t{KEYPAD[key]:02x}']
    for frame, scan, frames in scenario.presses:
        # An extended key's scan code has E0h above it (its prefix byte).
        prefix = ['key\te0'] if scan >> 8 == 0xe0 else []
        lines += [f'{frame}\t{word}' for word in prefix + [f'key\t{scan & 0xff:02x}']]
        lines += [f'{frame + frames}\t{word}' for word in prefix + [f'release\t{scan & 0xff:02x}']]
    head, body = lines[:2], lines[2:]
    lines = head + sorted(body, key=lambda line: int(line.split('\t')[0]))
    lines.append(f'{scenario.end}\tend')
    (directory / 'inputs.tsv').write_text('\n'.join(lines) + '\n')
    (directory / 'run.tsv').write_text(f'game\t{overlay.GAME}\narguments\t' + '\t'.join(scenario.arguments) + '\n')
    shutil.copytree(install / 'SAVEGAME', directory / 'files' / 'SAVEGAME')
    shutil.copy2(install / 'crusader.cfg', directory / 'files' / 'crusader.cfg')


def in_order(lines, patterns):
    """The first pattern no line after the previous one's matches, else None."""
    at = 0
    for pattern in patterns:
        expression = re.compile(r'^\d+ ' + pattern)
        while at < len(lines) and not expression.match(lines[at]):
            at += 1
        if at == len(lines):
            return pattern
        at += 1
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', overlay.ORIGIN)))
    ap.add_argument('--keep', type=Path, help='keep the recordings and their traces here')
    args = ap.parse_args()
    args.program = args.program.resolve()
    args.cd = args.cd.resolve()
    scenarios = [s for s in SCENARIOS if overlay.GAME in s.games]
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH missions: {what}')

    with tempfile.TemporaryDirectory(prefix='port-missions') as work:
        work = Path(work)
        install = work / 'install'
        subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(args.cd),
                        '--sound', '--no-movies'], check=True, capture_output=True)
        for scenario in scenarios:
            directory = work / scenario.name
            write_recording(scenario, directory, install)
            trace = work / f'{scenario.name}.trace'
            first, code, error, _ = replay.replay(args.program, args.cd, install, directory, work / 'first.txt',
                                                  trace=trace)
            states = [line.split() for line in first.splitlines()]
            check(code == 0 and f'({scenario.end + 1} frames)' in error,
                  f'{scenario.name}: the replay: {code}, {error[-400:]}')
            check(states and (states[-1][9] != '0') == scenario.alive,
                  f'{scenario.name}: the Silencer {"died" if scenario.alive else "lives"}: {states[-1:]}')
            check(scenario.dead is None or (states and int(states[-1][11]) == scenario.dead),
                  f'{scenario.name}: {states[-1][11] if states else "no"} NPCs dead, not {scenario.dead}')
            lines = trace.read_text(encoding='latin-1').splitlines() if trace.exists() else []
            missing = in_order(lines, scenario.expect)
            check(missing is None, f'{scenario.name}: the trace has no "{missing}" in its order')
            if scenario.place and states:
                place = tuple(int(states[-1][k]) for k in (5, 6, 7))
                check(place == scenario.place, f'{scenario.name}: the Silencer ends at {place}, not {scenario.place}')

            checks = ['\t'.join(line.split()) for line in first.splitlines() if int(line.split()[0]) % 60 == 0]
            (directory / 'checks.tsv').write_text('# pass\tstate\n' + '\n'.join(checks) + '\n')
            second, code, error, _ = replay.replay(args.program, args.cd, install, directory, work / 'second.txt')
            check(code == 0 and second == first and 'not as recorded' not in error,
                  f'{scenario.name}: the second replay: {code}, {error[-400:]}')

            # Other game arguments than the recording's end the run; the
            # replay recorded again keeps the recording's.
            _, code, error, _ = replay.replay(args.program, args.cd, install, directory, work / 'third.txt', '-skill',
                                              '2')
            check(code != 0 and "recording's arguments" in error, f'{scenario.name}: other arguments: {code}, '
                  f'{error[-300:]}')
            again = work / f'{scenario.name}-again'
            _, code, error, _ = replay.replay(args.program, args.cd, install, directory, work / 'again.txt',
                                              '--record', str(again))
            run = (again / 'run.tsv').read_text() if (again / 'run.tsv').exists() else ''
            check(code == 0 and ('arguments\t' + '\t'.join(scenario.arguments) + '\n') in run,
                  f'{scenario.name}: recorded again: {code}, run.tsv {run!r}, {error[-300:]}')
            if args.keep:
                args.keep.mkdir(parents=True, exist_ok=True)
                shutil.rmtree(args.keep / scenario.name, ignore_errors=True)
                shutil.copytree(directory, args.keep / scenario.name)
                shutil.copy2(trace, args.keep / f'{scenario.name}.trace')

    print(f'missions {overlay.GAME}: {len(scenarios)} scenarios, {failures} mismatches')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
