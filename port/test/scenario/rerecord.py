#!/usr/bin/env python3
"""Rerecording (port/system/replay.h, rewind.h, snapshot.h) with the game of CRUSADER_GAME. The recording SRC is milestone 2's
input (replay.py's) with a quick save (F5, Y, Enter) and the music's volume
one louder (its key) after the walk; replayed once for its states
(checks.tsv, each 60th pass). Then it is replayed and recorded (--replay SRC
--record DST), the recording what the replay gave:
- as it goes: DST is SRC's inputs line for line, the replay's states its
  checks, run.tsv's rerecords 0;
- rewound past the quick save and the volume's key (PORT_REWIND_TEST): the
  player's settings and files go back with the game's state (they had
  changed), the recorder's lines after the frame are cut and recorded again
  as the replay goes on: DST is SRC's inputs, rerecords 1;
- a state taken before them and loaded after (PORT_SNAPSHOT_TEST): the same.
Each DST replayed is as recorded.
Then SRC is gone on with (--continue)
on an Xvfb of the test's own, where there is one (Linux, not a Windows
build): refused at its end (the script's quit is the game's own end), and
taken over at its last frame with a lead of 30 frames (PORT_WATCH_LEAD),
so that it goes fast without drawing, draws the whole screen again and
keeps the ring in the lead, and is recorded (--record CONT) until the
window closes (SIGTERM) right after: CONT is SRC's inputs up to the
take-over, the replay was as recorded, and CONT replayed is as recorded,
its last screen the window's (the screen drawn again whole at the lead's
start: without it No Regret's is not). A build without a window (the
sanitizers') refuses the take-over, as it must.

usage: rerecord.py PROGRAM [--cd CD]
"""
import argparse, os, shutil, signal, subprocess, sys, tempfile, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
import replay  # noqa: E402

# The quick save and the volume's key, after the walk (milestone 2's last
# walking key is at 790, its first shot at 850): F5, Y to the question, Enter
# to the notice; the music one louder (No Regret's key needs Ctrl).
QUICK_SAVE = [(800, 'key', 0x3f), (804, 'release', 0x3f), (812, 'key', 0x15), (816, 'release', 0x15),
              (830, 'key', 0x1c), (834, 'release', 0x1c)]
LOUDER = [(840, 'key', 0x1d), (842, 'key', 0x0d), (846, 'release', 0x0d), (848, 'release', 0x1d)]
# The rewind from after them to before them, and the state taken before and
# loaded after.
REWIND = (860, 80)
STATE = (790, 70)
# The lead of the continued replay (--continue): what it draws and keeps on
# the ring before the take-over, short enough that No Regret's screen is
# not whole at the take-over unless it was drawn again whole (No Remorse's
# scene draws all of it anew by then); the window closed this long after.
LEAD = 30
AFTER = 0.2


def lines(path):
    """A recording's lines without comments and blank lines, each its words."""
    words = [line.split('#')[0].split() for line in path.read_text().splitlines()]
    return [w for w in words if w]


def run_value(directory, key):
    for words in lines(directory / 'run.tsv'):
        if len(words) == 2 and words[0] == key:
            return words[1]
    return None


def source_recording(directory, install):
    """Milestone 2's recording with the quick save and the volume's key, its
    lines in the frames' order (as a recorder writes them)."""
    end = replay.recording(directory, install)
    added = QUICK_SAVE + ([(840, 'key', 0x1d)] if overlay.GAME == 'regret' else []) + \
        [line for line in LOUDER if line[2] != 0x1d] + \
        ([(848, 'release', 0x1d)] if overlay.GAME == 'regret' else [])
    text = (directory / 'inputs.tsv').read_text().splitlines()
    header = [line for line in text if line.startswith('#')]
    body = [line for line in text if line and not line.startswith('#')]
    body += [f'{frame}\t{verb}\t{code & 0x7f:02x}' for frame, verb, code in added]
    body.sort(key=lambda line: int(line.split()[0]))
    (directory / 'inputs.tsv').write_text('\n'.join(header + body) + '\n')
    return end


def window_display():
    """An Xvfb of the test's own and its display, None where there is none."""
    if not sys.platform.startswith('linux') or shutil.which('Xvfb') is None:
        return None, None
    read, write = os.pipe()
    xvfb = subprocess.Popen(['Xvfb', '-displayfd', str(write), '-screen', '0', '1024x768x24', '-nolisten', 'tcp'],
                            pass_fds=(write,), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    os.close(write)
    with os.fdopen(read) as displays:
        number = displays.readline().strip()
    if not number:
        xvfb.kill()
        xvfb.wait()
        return None, None
    return xvfb, f':{number}'


def continued(program, cd, install, source, recorded, display, *options):
    """--continue SOURCE --record RECORDED in a window on display, closed
    (SIGTERM) AFTER seconds after the take-over: its return code and standard
    error."""
    env = dict(os.environ, CRUSADER_CD=str(cd), DISPLAY=display, REGRET_AUDIO_DRIVER='dummy',
               PORT_WATCH_LEAD=str(LEAD), REGRET_SCREEN=str(recorded.with_suffix('.png')))
    for name in ('REGRET_SCRIPT', 'REGRET_AUDIO', 'REGRET_STATE', 'WAYLAND_DISPLAY'):
        env.pop(name, None)
    error_path = recorded.with_suffix('.txt')
    with open(error_path, 'w') as error:
        game = subprocess.Popen([str(program), '--debug-feature', '--continue', str(source), '--record',
                                 str(recorded), *options],
                                cwd=install, env=env, stdout=subprocess.DEVNULL, stderr=error)
        deadline = time.monotonic() + 300
        while game.poll() is None and time.monotonic() < deadline and 'taken over' not in error_path.read_text(
                encoding='latin-1'):
            time.sleep(0.2)
        if game.poll() is None:
            time.sleep(AFTER)
            game.send_signal(signal.SIGTERM)
        try:
            code = game.wait(timeout=60)
        except subprocess.TimeoutExpired:
            game.kill()
            code = game.wait()
    return code, error_path.read_text(encoding='latin-1')


def continue_checks(args, install, source, end, display, work, check):
    """--continue's checks (the description's); what the summary says of them."""
    code, error = continued(args.program, args.cd, install, source, work / 'refused', display)
    check(code != 0 and "ends without the window's close" in error and not (work / 'refused').exists(),
          f'continued to the game\'s own end: {code}, {error[-500:]}')
    cont = work / 'continued'
    code, error = continued(args.program, args.cd, install, source, cont, display, '--to', str(end))
    if 'need a window' in error:
        check(code != 0 and not cont.exists(), f'continued without a window: {code}, {error[-500:]}')
        return ', continued only to the refusals (a build without a window)'
    taken = end - 1
    check('replay: shown from' in error and f'(frame {taken})' in error and 'replay: taken over at' in error
          and 'not as recorded' not in error, f'continued: {code}, {error[-800:]}')
    if not (cont / 'inputs.tsv').exists():
        check(False, 'continued: no recording')
        return ''
    before = [words for words in lines(source / 'inputs.tsv') if int(words[0]) <= taken and words[1] != 'end']
    check([words for words in lines(cont / 'inputs.tsv') if int(words[0]) <= taken] == before,
          'continued: the recording before the take-over is not the source\'s')
    check(lines(cont / 'inputs.tsv')[-1][1] == 'end' and lines(cont / 'inputs.tsv')[-2][1] == 'close',
          'continued: the recording does not end at the window\'s close')
    env = dict(os.environ, CRUSADER_CD=str(args.cd), REGRET_SCREEN=str(work / 'again.png'))
    for name in ('REGRET_SCRIPT', 'REGRET_AUDIO', 'REGRET_STATE', 'DISPLAY', 'WAYLAND_DISPLAY'):
        env.pop(name, None)
    again = subprocess.run([str(args.program), '--debug-feature', '--replay', str(cont)], cwd=install, env=env,
                           capture_output=True, encoding='latin-1', timeout=600)
    check(again.returncode == 0 and 'not as recorded' not in again.stderr,
          f'continued: the recording replayed: {again.returncode}, {again.stderr[-500:]}')
    check((work / 'again.png').read_bytes() == cont.with_suffix('.png').read_bytes(),
          'continued: the window\'s last screen is not the replay\'s')
    return f', continued from {taken} with a lead of {LEAD}'


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
            print(f'MISMATCH rerecord: {what}')

    with tempfile.TemporaryDirectory(prefix='port-rerecord') as work:
        work = Path(work)
        install = work / 'install'
        subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(args.cd),
                        '--sound', '--no-movies'], check=True, capture_output=True)
        source = work / 'source'
        end = source_recording(source, install)
        first, code, error, _ = replay.replay(args.program, args.cd, install, source, work / 'first.txt')
        check(code == 0 and f'({end + 1} frames)' in error, f'the first replay: {code}, {error[-500:]}')
        checks = ['\t'.join(line.split()) for line in first.splitlines() if int(line.split()[0]) % 60 == 0]
        (source / 'checks.tsv').write_text('# pass\tstate\n' + '\n'.join(checks) + '\n')

        def recorded_from_replay(name, rerecords, said, **env):
            recorded = work / name
            os.environ.update(env)
            try:
                _, code, error, _ = replay.replay(args.program, args.cd, install, source, work / f'{name}.txt',
                                                  '--record', str(recorded))
            finally:
                for key in env:
                    os.environ.pop(key, None)
            check(code == 0 and f'({end + 1} frames)' in error and 'not as recorded' not in error,
                  f'{name}: the replay recorded: {code}, {error[-800:]}')
            for line in said:
                check(line in error, f'{name}: no "{line}": {error[-800:]}')
            check(lines(recorded / 'inputs.tsv') == lines(source / 'inputs.tsv'),
                  f'{name}: the recording is not the replay\'s inputs')
            check(lines(recorded / 'checks.tsv') == lines(source / 'checks.tsv'),
                  f'{name}: the recording\'s checks are not the replay\'s')
            check(run_value(recorded, 'rerecords') == str(rerecords) and run_value(recorded, 'game') == overlay.GAME,
                  f'{name}: run.tsv: {(recorded / "run.tsv").read_text()!r}')
            check((recorded / 'files' / 'crusader.cfg').exists(), f'{name}: the recording without the replay\'s files')
            _, code, error, _ = replay.replay(args.program, args.cd, install, recorded, work / f'{name}-again.txt')
            check(code == 0 and f'({end + 1} frames)' in error and 'not as recorded' not in error,
                  f'{name}: the recording replayed: {code}, {error[-500:]}')

        recorded_from_replay('recorded', 0, [])
        at, back = REWIND
        recorded_from_replay('rewound', 1, [f'rewind: {back} frames back at frame {at}',
                                            "rewind: the player's settings and files as they were, changed since"],
                             PORT_REWIND_TEST=f'{at}:{back}')
        at, frames = STATE
        recorded_from_replay('state', 1, [f'snapshot: put back at frame {at + frames}, the player\'s settings and '
                                          'files as they were, changed since'],
                             PORT_SNAPSHOT_TEST=f'{at}:{frames}')
        said = ''
        # A Windows build: its program, or the script a cross build runs it
        # by under wine (test/setup.cmake's program_run, <name>-run).
        if str(args.program).endswith(('.exe', '-run')):
            said = ', not continued (a Windows build)'
        else:
            xvfb, display = window_display()
            if xvfb is None:
                said = ', not continued (no Xvfb)'
            else:
                try:
                    said = continue_checks(args, install, source, end, display, work, check)
                finally:
                    xvfb.terminate()
                    xvfb.wait()
        print(f'rerecord: {end + 1} frames recorded from the replay, rewound {REWIND[1]} frames at {REWIND[0]}, '
              f'a state of {STATE[0]} loaded at {STATE[0] + STATE[1]}{said}')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
