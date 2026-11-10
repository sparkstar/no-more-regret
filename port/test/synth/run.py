#!/usr/bin/env python3
"""The synthetic scenes: for each, the data
is made (world.py), the port replays the scene's recording twice, each to
its end with no frame inside one of the game's waits and each test's
footprint in its records (scenes.Scene's ran; a test whose work no record
keeps is counted apart), the two the same states at every pass; with
--expected, the same as the results kept for the scene (the hashes
of the states and of the usecode's trace, and every 60th state line;
--update writes them); with --oracle, the original follows the port's run
in the pass oracle with its whole state compared at each pass, to the
scene's end. The original's unkcoff.dat (its intrinsics' addresses) is made
from the table of the original's addresses (--symbols, by default the
oracle's build's symbols_<game>.tsv), not taken from the player's files;
without that table it exits 77 (skipped), as the port alone is the run
without --oracle.

usage: run.py PROGRAM --game regret|remorse [--expected FILE [--update]] [--oracle PASS_ORACLE] [--symbols TSV]
              [--scene NAME] [--keep DIR] [--keep-failed DIR] [--jobs N]
(the scenes run at once, N of them; half the processors by default)
"""
import argparse, concurrent.futures, hashlib, json, os, re, shutil, subprocess, sys, tempfile, time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import docs  # noqa: E402
import scenes  # noqa: E402
import world  # noqa: E402

# The oracle stops a few passes before a recording's end, where the port
# quits.
END_MARGIN = 5
# The kept results' checks, a pass apart (where a run parts from them).
CHECK_PASSES = 60
# ctest's count of a test skipped (SKIP_RETURN_CODE).
SKIPPED = 77


def replay(program, install, cd, recording, work, name):
    """The port's replay: its return code, standard error, states (a line
    a pass, system/statelog.cpp), usecode trace and movies' frames (a line
    a frame shown, system/movieframes.cpp), named name in work."""
    env = {k: v for k, v in os.environ.items() if k not in ('DISPLAY', 'WAYLAND_DISPLAY', 'REGRET_SCRIPT')}
    states, trace, frames = work / f'{name}.states', work / f'{name}.trace', work / f'{name}.frames'
    env.update(CRUSADER_CD=str(cd), REGRET_STATE=str(states), REGRET_USECODE_TRACE=str(trace),
               REGRET_FRAMES=str(frames), REGRET_AUDIO_DRIVER='dummy')
    run = subprocess.run([str(program), '--debug-feature', '--replay', str(recording)], cwd=install, env=env,
                         capture_output=True, encoding='latin-1', timeout=600)
    read = lambda path: path.read_text(encoding='latin-1') if path.exists() else ''
    return run.returncode, run.stderr, read(states), read(trace), read(frames)


# The scenes' documents (docs.py), read once: what each test shows.
DOCUMENTS = docs.documents(scenes.all_scenes())[0]


def explained(scene, tests):
    """Lines of what the scene's tests show, as its document says (the
    first four)."""
    lines = [f'  {test}: {docs.describe(scene.name, test, docs=DOCUMENTS)}' for test in tests[:4]]
    return ''.join(f'\n{line}' for line in lines)


def replayed(scene, code, error, record):
    """What is wrong with a replay of scene, '' for nothing, and how many of
    its tests no record shows: it must end well at its end with no frame
    inside one of the game's waits (as tools/replays.py asks of the
    player's recordings) and the footprint of each test in its records
    (scenes.Scene's ran)."""
    if code != 0 or f'({scene.frames + 1} frames)' not in error:
        return f'return {code}, {error[-600:]}', 0
    inside = re.search(r"(\d+) inside the game's waits", error)
    if inside is None or inside.group(1) != '0':
        return f"{inside.group(1) if inside else '?'} frames inside the game's waits", 0
    ran = scene.ran(record, scene.tests)
    if len(ran) != len(scene.tests):
        return f'{len(ran)} footprints for {len(scene.tests)} tests', 0
    missing = [test for test, seen in zip(scene.tests, ran) if seen is False]
    unseen = sum(seen is None for seen in ran)
    if missing:
        shown = ', '.join(missing[:6]) + (', ...' if len(missing) > 6 else '')
        return f'{len(missing)} of {len(scene.tests)} tests left no footprint: {shown}' + explained(scene, missing), unseen
    return '', unseen


def results(states, trace, frames=''):
    """A scene's results to keep: the hashes of its states and of its
    usecode's trace, and where they part: every CHECK_PASSES-th state line
    and a short digest of the trace's lines of each CHECK_PASSES passes;
    for a scene that plays movies, the hash of their frames and a short one
    of each movie's."""
    blocks = []
    for line in trace.splitlines():
        block = (int(line.split(' ', 1)[0]) - 1) // CHECK_PASSES
        while len(blocks) <= block:
            blocks.append([])
        blocks[block].append(line)
    return {'states': hashlib.sha256(states.encode('latin-1')).hexdigest(),
            'trace': hashlib.sha256(trace.encode('latin-1')).hexdigest(),
            'checks': states.splitlines()[CHECK_PASSES - 1::CHECK_PASSES],
            'trace_checks': [hashlib.sha256('\n'.join(lines).encode('latin-1')).hexdigest()[:8] for lines in blocks],
            **({'frames': hashlib.sha256(frames.encode('latin-1')).hexdigest(),
                'frame_checks': [hashlib.sha256('\n'.join(movie).encode('latin-1')).hexdigest()[:8]
                                 for movie in scenes.movies_of(frames)]} if frames else {})}


def compared(kept, found, where, explain=lambda tests: ''):
    """What differs between the results kept and those found, '' for
    nothing; where(low, high) names the tests of passes low to high (None
    when it cannot tell), explain(tests) says what they show."""
    if kept is None:
        return 'no results kept for the scene (run.py --update)'
    def passes(k):
        """Passes k's text, and what their tests show."""
        low, high = CHECK_PASSES * k + 1, CHECK_PASSES * (k + 1)
        named = where(low, high)
        if named is None:
            return f'passes {low}-{high}', ''
        if not named:
            return f'passes {low}-{high} (before or after its tests)', ''
        shown = ', '.join(named[:4]) + (', ...' if len(named) > 4 else '')
        return f'passes {low}-{high} (tests: {shown})', explain(named)
    if kept['states'] != found['states']:
        for k, (old, new) in enumerate(zip(kept['checks'], found['checks'])):
            if old != new:
                text, said = passes(k)
                return f'the states are not the kept ones, first in {text}: {new} (kept {old}){said}'
        return 'the states are not the kept ones'
    if kept['trace'] != found['trace']:
        for k, (old, new) in enumerate(zip(kept.get('trace_checks', []), found['trace_checks'])):
            if old != new:
                text, said = passes(k)
                return f"the usecode's trace is not the kept one, first in {text}{said}"
        return "the usecode's trace is not the kept one"
    if kept.get('frames') != found.get('frames'):
        for k, (old, new) in enumerate(zip(kept.get('frame_checks', []), found.get('frame_checks', []))):
            if old != new:
                return f"the movies' frames are not the kept ones, first in movie {k + 1}"
        return "the movies' frames are not the kept ones"
    return ''


def follow(oracle, program, install, cd, recording, work, passes):
    """The original following the port's run: whether it went to the end,
    and what the oracle said at its stop."""
    original, port = work / 'original', work / 'port'
    shutil.copytree(install, original)
    shutil.copytree(install, port)
    # The oracle has the port replay the recording (--replay), a debug
    # feature: on by the port's game folder's enhanced.cfg, which a port
    # without them passes over.
    settings = port / 'enhanced.cfg'
    settings.write_bytes((settings.read_bytes() if settings.exists() else b'') + b'debug_feature=true\r\n')
    env = {k: v for k, v in os.environ.items() if k not in ('DISPLAY', 'WAYLAND_DISPLAY')}
    env['REGRET_AUDIO_DRIVER'] = 'dummy'
    run = subprocess.run([str(oracle), '--install', str(original), '--cd', str(cd), '--port', str(program),
                          '--port-install', str(port), '--replay', str(recording), '--passes', str(passes)],
                         env=env, capture_output=True, encoding='latin-1', timeout=1800)
    lines = run.stdout.splitlines()
    stop = [i for i, line in enumerate(lines) if line.startswith('stop ')]
    said = '\n'.join(lines[stop[0]:stop[0] + 6]) if stop else (run.stdout + run.stderr)[-2000:]
    # The port's end is the scene's too: the frames inside the game's waits
    # (a gump up, a save) have no pass, so a scene may end before passes.
    followed = bool(stop) and (lines[stop[0]].endswith(f'after {passes} passes: passes done') or
                               "the port's run ended" in lines[stop[0]])
    return followed, said


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--game', choices=('regret', 'remorse'), required=True)
    ap.add_argument('--oracle', type=Path)
    ap.add_argument('--symbols', type=Path,
                    help="the original's addresses (symbols_<game>.tsv; the oracle's build's by default)")
    ap.add_argument('--scene', default='')
    ap.add_argument('--keep', type=Path, help="keep each scene's data and runs here")
    ap.add_argument('--keep-failed', type=Path, help='copies a failed scene\'s data and runs there')
    ap.add_argument('--expected', type=Path, help='the results kept for the scenes (a JSON file)')
    ap.add_argument('--update', action='store_true', help='writes the results found into --expected')
    ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2))
    args = ap.parse_args()
    program = args.program.resolve()
    symbols = args.symbols or (args.oracle.resolve().parents[1] / f'symbols_{args.game}.tsv' if args.oracle else None)
    if args.oracle and not (symbols and symbols.exists() and symbols.stat().st_size):
        # The original's run asked for and no table of its addresses (a
        # tree without the recovered sources): skipped, not passed by the
        # port alone, which the test without --oracle runs.
        print(f"synth: skipped: no table of the original's addresses ({symbols})")
        return SKIPPED
    unkcoff = world.unkcoff(args.game, symbols) if args.oracle else None
    oracle = args.oracle.resolve() if args.oracle else None
    failures = 0
    kept = json.loads(args.expected.read_text()) if args.expected and args.expected.exists() else {}
    found_all = {}
    with tempfile.TemporaryDirectory(prefix='port-synth') as temporary:
        base = args.keep.resolve() if args.keep else Path(temporary)
        chosen = [(name, scene) for name, make in scenes.all_scenes().items() if args.scene in name
                  for scene in [make(args.game)] if scene is not None]
        # the longest first, so that the last to end is a short one
        order = sorted(range(len(chosen)), key=lambda k: -chosen[k][1].frames)

        def one(named):
            """A scene: the port's run, then the original's; its report and
            whether it failed."""
            name, scene = named
            work = base / f'{args.game}-{name}'
            shutil.rmtree(work, ignore_errors=True)
            install, cd, recording = world.make(work, args.game, scene, unkcoff if oracle else None)
            started = time.monotonic()
            code, error, states, trace, frames = replay(program, install, cd, recording, work, 'first')
            record = scenes.Record(states, trace, frames)
            wrong, unseen = replayed(scene, code, error, record)
            # Twice the same: the port is deterministic over a recording.
            if not wrong:
                code2, error2, states2, trace2, frames2 = replay(program, install, cd, recording, work, 'second')
                wrong = replayed(scene, code2, error2, scenes.Record(states2, trace2, frames2))[0] or (
                    '' if states2 == states else 'the second replay\'s states are not the first\'s') or (
                    '' if frames2 == frames else 'the second replay\'s movie frames are not the first\'s')
            if not wrong:
                found_all[name] = results(states, trace, frames)
                if args.expected and not args.update:
                    wrong = compared(kept.get(name), found_all[name],
                                     lambda low, high: scenes.tests_at(scene, record, low, high),
                                     lambda tests: explained(scene, tests))
            seconds = time.monotonic() - started
            ok = not wrong
            if wrong and args.keep_failed:
                shutil.copytree(work, args.keep_failed / work.name, dirs_exist_ok=True)
            unshown = f' ({unseen} shown by no record)' if unseen else ''
            report = (f'synth {args.game} {name}: {len(scene.tests)} tests{unshown}, the port ran them twice the same '
                      f'({seconds:.1f} s)' if ok else f'MISMATCH synth {args.game} {name}: {wrong}')
            if oracle and ok and scene.oracle:
                started = time.monotonic()
                ok, said = follow(oracle, program, install, cd, recording, work,
                                  scene.followed_to or scene.frames - END_MARGIN)
                seconds = time.monotonic() - started
                report += (f'\nsynth {args.game} {name}: the original {"followed" if ok else "differs"} '
                           f'({seconds:.1f} s)' + ('' if ok else f'\n{said}'))
            return report, not ok

        # The scenes are apart from each other: at once.
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
            done = dict(zip(order, pool.map(one, [chosen[k] for k in order])))
        for k in range(len(chosen)):
            report, failed = done[k]
            print(report)
            failures += failed
        # A port-only scene of the port's own setting runs as its base, which
        # the original follows (Scene.same_as).
        for name, scene in chosen:
            if not scene.same_as or name not in found_all:
                continue
            if scene.same_as not in found_all:
                print(f'synth {args.game} {name}: not compared with {scene.same_as}, which did not run')
            elif all(found_all[name][key] == found_all[scene.same_as][key] for key in ('states', 'trace')):
                print(f'synth {args.game} {name}: the same records as {scene.same_as}')
            else:
                print(f'MISMATCH synth {args.game} {name}: its records are not those of {scene.same_as}')
                failures += 1
    if args.update and args.expected:
        kept.update(found_all)
        args.expected.write_text(json.dumps(kept, indent=1, sort_keys=True) + '\n')
        print(f'synth: the results of {len(found_all)} scenes written to {args.expected}')
    if not oracle:
        print('synth: the port alone (no original to follow)')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
