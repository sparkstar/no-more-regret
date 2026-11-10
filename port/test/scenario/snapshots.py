#!/usr/bin/env python3
"""Memory snapshots (port/system/snapshot.h) with the game of
CRUSADER_GAME: milestone 2's input written as a recording (replay.py's),
replayed once for its checks (checks.tsv, each 60th pass), then again with
a snapshot taken at a frame and put back some frames later
(PORT_SNAPSHOT_TEST), from frames in the start menu, in the menus, walking
in and fighting: each replay must go on as recorded to its end, its states
the first one's, the snapshot put back; and again with a rewind
(PORT_REWIND_TEST, port/system/rewind.h) of some frames from frames as far
on, each frame of them put back from the rewind's ring.

usage: snapshots.py PROGRAM [--cd CD]
"""
import argparse, os, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import overlay  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
import replay  # noqa: E402

# The frames the snapshots are taken at (the last ones before the end) and
# how many frames on each is put back.
FRAMES = (1, 250, 400, 700, 850)
BACK = 60
# The rewinds: at which frame, how many frames back.
REWINDS = ((250, 60), (700, 300), (900, 850))


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
            print(f'MISMATCH snapshots: {what}')

    with tempfile.TemporaryDirectory(prefix='port-snapshots') as work:
        work = Path(work)
        install = work / 'install'
        subprocess.run([sys.executable, str(ROOT / 'port/tools/install.py'), str(install), '--cd', str(args.cd),
                        '--sound', '--no-movies'], check=True, capture_output=True)
        directory = work / 'recording'
        end = replay.recording(directory, install)
        first, code, error, _ = replay.replay(args.program, args.cd, install, directory, work / 'first.txt')
        check(code == 0 and f'({end + 1} frames)' in error, f'the first replay: {code}, {error[-500:]}')
        checks = ['\t'.join(line.split()) for line in first.splitlines() if int(line.split()[0]) % 60 == 0]
        (directory / 'checks.tsv').write_text('# pass\tstate\n' + '\n'.join(checks) + '\n')
        frames = [frame for frame in FRAMES if frame + BACK < end] + [end - BACK - 1]
        for frame in frames:
            os.environ['PORT_SNAPSHOT_TEST'] = f'{frame}:{BACK}'
            _, code, error, _ = replay.replay(args.program, args.cd, install, directory, work / f'at{frame}.txt')
            check(code == 0 and f'({end + 1} frames)' in error, f'at {frame}: {code}, {error[-500:]}')
            check('snapshot: put back' in error, f'at {frame}: no snapshot put back: {error[-500:]}')
            check('not as recorded' not in error, f'at {frame}: {error[-800:]}')
        os.environ.pop('PORT_SNAPSHOT_TEST', None)
        rewinds = [(frame, back) for frame, back in REWINDS if frame < end]
        for frame, back in rewinds:
            os.environ['PORT_REWIND_TEST'] = f'{frame}:{back}'
            _, code, error, _ = replay.replay(args.program, args.cd, install, directory, work / f'back{frame}.txt')
            check(code == 0 and f'({end + 1} frames)' in error, f'rewind at {frame}: {code}, {error[-500:]}')
            check(f'rewind: {back} frames back at frame {frame}' in error, f'rewind at {frame}: {error[-500:]}')
            check('not as recorded' not in error, f'rewind at {frame}: {error[-800:]}')
        os.environ.pop('PORT_REWIND_TEST', None)
        print(f'snapshots: {len(frames)} snapshots put back {BACK} frames on, at {", ".join(map(str, frames))}; '
              f'rewinds of {", ".join(f"{back} frames at {frame}" for frame, back in rewinds)}')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
