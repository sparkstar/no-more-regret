#!/usr/bin/env python3
"""The replay and the capture (port/system/replay.h) with the game of CRUSADER_GAME, installed from its CD
for the sound card without its movies. Milestone 2's input written as a
recording (inputs.tsv, a frame each tick; the install's crusader.cfg and
SAVEGAME as its files) and replayed without a window:
- the run is milestone 2's: the Silencer walks, the NPCs are hit and one dies;
- the first replay writes the usecode's trace (REGRET_USECODE_TRACE,
  platform/trace.h): lines of each kind the run has, in the order of the
  passes, which tools/usecode.py coverage counts;
- a second replay, without the trace, comes to the same states, and with
  the first one's (checks.tsv, each 60th pass) says nothing; a state changed
  in checks.tsv is told, once;
- the second replay's capture on the standard output is NUT that ffprobe
  reads: 640x480 rgb24 and 44100 a second in two channels, a picture and
  735 samples a frame from --from to the end (skipped without ffprobe);
  with the wide screen on, the wide screen's width for 480 rows;
- --to ends the replay at its frame; a mistake in inputs.tsv ends it with
  the line;
- without --debug-feature the replay ends the run, without a crash
  report, and debug_feature=true in the game folder's enhanced.cfg gives
  it as well.

usage: replay.py PROGRAM [--cd CD]
"""
import argparse, json, os, shutil, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
import milestone2  # noqa: E402

# The frame the capture starts at, in the mission after the walk in.
FROM = 700


def recording(directory, install):
    """Milestone 2's keys as a recording, its end at the script's quit."""
    lines = ['# frame\tinput\tvalues (milestone 2\'s keys)', '0\tdate\t1996-01-01']
    end = None
    for line in milestone2.SCRIPT.splitlines():
        words = line.split()
        if words[1] == 'key':
            for byte in words[2:]:
                code = int(byte, 16)
                lines.append(f'{words[0]}\t' + (f'release\t{code & 0x7f:02x}' if code & 0x80 else f'key\t{code:02x}'))
        elif words[1] == 'quit':
            end = int(words[0])
    lines.append(f'{end}\tend')
    directory.mkdir()
    (directory / 'inputs.tsv').write_text('\n'.join(lines) + '\n')
    (directory / 'run.tsv').write_text(f'game\t{overlay.GAME}\n')
    shutil.copytree(install / 'SAVEGAME', directory / 'files' / 'SAVEGAME')
    shutil.copy2(install / 'crusader.cfg', directory / 'files' / 'crusader.cfg')
    return end


def replay(program, cd, install, directory, state, *options, capture=None, trace=None, debug=True):
    """A replay's states and standard error; with capture, the standard
    output to it as its standard input; with trace, the usecode's trace to
    that file; without debug, no --debug-feature (a replay is one)."""
    env = dict(os.environ, CRUSADER_CD=str(cd), REGRET_STATE=str(state))
    for name in ('REGRET_SCRIPT', 'REGRET_SCREEN', 'REGRET_AUDIO', 'REGRET_USECODE_TRACE', 'DISPLAY', 'WAYLAND_DISPLAY'):
        env.pop(name, None)
    if trace:
        env['REGRET_USECODE_TRACE'] = str(trace)
    command = [str(program), *(['--debug-feature'] if debug else []), '--replay', str(directory), *options]
    if capture:
        game = subprocess.Popen(command + ['--capture', '-'], cwd=install, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        reader = subprocess.run(capture, stdin=game.stdout, capture_output=True, encoding='latin-1', timeout=600)
        game.stdout.close()
        error = game.stderr.read().decode('latin-1')
        code = game.wait(timeout=600)
        result = (code, error, reader.stdout)
    else:
        run = subprocess.run(command, cwd=install, env=env, capture_output=True, encoding='latin-1', timeout=600)
        result = (run.returncode, run.stderr, None)
    return (state.read_text() if state.exists() else ''), *result


def check_trace(trace, states, check):
    """The trace's lines: each kind milestone 2's run has (the map's load,
    the camera's item, the Silencer's teleport into it, events, processes
    made, started and killed, calls and intrinsics), the passes in order up
    to the last state's, and the coverage of them."""
    lines = trace.read_text(encoding='latin-1').splitlines() if trace.exists() else []
    passes = [int(line.split()[0]) for line in lines]
    kinds = {line.split()[1] for line in lines}
    last = int(states.splitlines()[-1].split()[0]) if states else 0
    check(kinds >= set('M F T E + U C I -'.split()) and passes == sorted(passes) and passes and passes[-1] <= last + 1,
          f'the trace: {len(lines)} lines, kinds {sorted(kinds)}, passes {passes[:1]}..{passes[-1:]} to {last}')
    counted = subprocess.run([sys.executable, str(ROOT / 'port/tools/usecode.py'), 'coverage', str(trace)],
                             capture_output=True, encoding='latin-1')
    ran = [int(line.split()[1]) for line in counted.stdout.splitlines() if line.startswith('  functions ')]
    check(counted.returncode == 0 and ran and ran[0] > 0, f'the coverage: {counted.stdout[-300:]}{counted.stderr[-300:]}')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    ap.add_argument('--cd', type=Path, default=Path(os.environ.get('CRUSADER_ORIGIN', overlay.ORIGIN)))
    args = ap.parse_args()
    args.program = args.program.resolve()
    args.cd = args.cd.resolve()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH replay: {what}')

    with tempfile.TemporaryDirectory(prefix='port-replay') as work:
        work = Path(work)
        install = work / 'install'
        subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(args.cd),
                        '--sound', '--no-movies'], check=True, capture_output=True)
        directory = work / 'recording'
        end = recording(directory, install)

        trace = work / 'trace.txt'
        first, code, error, _ = replay(args.program, args.cd, install, directory, work / 'first.txt', trace=trace)
        check(code == 0 and f'({end + 1} frames)' in error, f'the first replay: {code}, {error[-500:]}')
        check_trace(trace, first, check)
        s = milestone2.states(first)
        passes = sorted(s)
        if passes:
            at = lambda p: s[max(k for k in passes if k <= p)]
            start, walked, last = at(690), at(800), s[passes[-1]]
            check((walked[5], walked[6]) != (start[5], start[6]), f'the Silencer did not move: {start[5:7]}')
            check(last[11] > walked[11], 'no NPC died')
        checks = ['\t'.join(line.split()) for line in first.splitlines() if int(line.split()[0]) % 60 == 0]
        (directory / 'checks.tsv').write_text('# pass\tstate\n' + '\n'.join(checks) + '\n')

        probe = shutil.which('ffprobe')
        reader = [probe, '-v', 'error', '-count_frames', '-show_entries',
                  'stream=codec_name,width,height,pix_fmt,sample_rate,channels,nb_read_frames', '-of', 'json',
                  '-i', '-'] if probe else None
        second, code, error, probed = replay(args.program, args.cd, install, directory, work / 'second.txt',
                                             '--from', str(FROM), capture=reader)
        check(code == 0 and second == first, f'the second replay: {code}, {error[-500:]}')
        check('not as recorded' not in error, f'the second replay is told it differs: {error[-500:]}')
        if reader:
            streams = json.loads(probed or '{}').get('streams', [])
            frames = str(end + 1 - FROM)
            video = {'codec_name': 'rawvideo', 'width': 640, 'height': 480, 'pix_fmt': 'rgb24', 'nb_read_frames': frames}
            audio = {'codec_name': 'pcm_s16le', 'sample_rate': '44100', 'channels': 2, 'nb_read_frames': frames}
            check(len(streams) == 2 and all(streams[0].get(k) == v for k, v in video.items()) and
                  all(streams[1].get(k) == v for k, v in audio.items()), f'the capture: {streams}')
            # With the wide screen on, the frame is the wide screen's: the
            # display's shape, 854 for 480 rows without one (the tests'),
            # another on a host whose display the run finds.
            wide = work / 'wide'
            shutil.copytree(directory, wide)
            (wide / 'files' / 'enhanced.cfg').write_bytes(b'widescreen=1\r\n')
            _, code, error, probed = replay(args.program, args.cd, install, wide, work / 'wide.txt', '--from',
                                            str(FROM), '--to', str(FROM + 60), capture=reader)
            streams = json.loads(probed or '{}').get('streams', [])
            width = int(streams[0].get('width', 0)) if streams else 0
            check(code == 0 and width % 2 == 0 and 640 < width <= 1536 and streams[0].get('height') == 480 and
                  streams[0].get('nb_read_frames') == '60', f'the wide capture: {code}, {streams}, {error[-300:]}')
        else:
            print('replay: no ffprobe, the capture not read')

        changed = [line.split('\t') for line in checks]
        changed[5][2] = '00000000'
        (directory / 'checks.tsv').write_text('\n'.join('\t'.join(words) for words in changed) + '\n')
        third, code, error, _ = replay(args.program, args.cd, install, directory, work / 'third.txt', '--to', '0:12')
        check(code == 0 and error.count('not as recorded after pass') == 1 and
              f'after pass {changed[5][0]} ' in error, f'a changed state is not told once: {error[-800:]}')
        check(third.splitlines() and int(third.splitlines()[-1].split()[0]) < int(first.splitlines()[-1].split()[0])
              and '(720 frames)' in error, f'--to 0:12 does not end at frame 720: {error[-300:]}')

        # A replay is a debug feature: without --debug-feature it ends the
        # run before the game starts; debug_feature=true in the game
        # folder's enhanced.cfg gives it as well.
        reports = set(install.glob('*-crash-*.log'))
        _, code, error, _ = replay(args.program, args.cd, install, directory, work / 'gated.txt', debug=False)
        check(code == 3 and '--replay is a debug feature' in error, f'without the debug features: {code}, '
              f'{error[-300:]}')
        # A mistake in what the run is given is no crash: no report.
        check(set(install.glob('*-crash-*.log')) == reports and 'crash report' not in error,
              f'the refusal wrote a crash report: {error[-300:]}')
        settings = install / 'enhanced.cfg'
        kept = settings.read_bytes() if settings.exists() else None
        settings.write_bytes((kept or b'') + b'debug_feature=true\r\n')
        try:
            _, code, error, _ = replay(args.program, args.cd, install, directory, work / 'configured.txt', '--to',
                                       '0:12', debug=False)
        finally:
            if kept is None:
                settings.unlink()
            else:
                settings.write_bytes(kept)
        check(code == 0 and '(720 frames)' in error, f'debug_feature=true in enhanced.cfg: {code}, {error[-300:]}')

        (directory / 'inputs.tsv').write_text((directory / 'inputs.tsv').read_text() + '900\tkey\tzz\n')
        _, code, error, _ = replay(args.program, args.cd, install, directory, work / 'fourth.txt')
        check(code != 0 and 'inputs.tsv line' in error and 'key <byte>' in error, f'a mistake: {code}, {error[-300:]}')

    print(f'replay {overlay.GAME}: {failures} mismatches')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
