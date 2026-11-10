#!/usr/bin/env python3
"""The game's menu at both depths of the port's sound, chosen in
enhanced.cfg's sound_bits and, none chosen, crusader.cfg's DMA channel's:
the output in words at both, the DMA's the same as the choice of its depth,
8 bits near the 16-bit output's level (the pans' difference, not the 8-bit
DMA's 12 dB). sounddepth holds the mix to the original's.

usage: soundprecision.py PROGRAM
"""
import argparse, array, math, os, subprocess, sys, tempfile, wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'port/tools'))
import install, overlay  # noqa: E402


def capture(program, work, name, dma, bits):
    target = work / name
    install.install(target, overlay.ORIGIN, sound=True, movies=False)
    cfg = target / 'crusader.cfg'
    cfg.write_bytes(cfg.read_bytes().replace(b'dma=5', f'dma={dma}'.encode()))
    if bits:
        (target / 'enhanced.cfg').write_text(f'game={overlay.GAME}\nsound_bits={bits}\n')
    script = target / 'menu.script'
    script.write_text('720 quit\n')
    path = target / 'sound.wav'
    env = dict(os.environ, CRUSADER_CD=str(overlay.ORIGIN), REGRET_SCRIPT=str(script), REGRET_AUDIO=str(path))
    for key in ('REGRET_STATE', 'REGRET_SCREEN', 'PORT_SNAPSHOT_TEST', 'PORT_REWIND_TEST'):
        env.pop(key, None)
    result = subprocess.run([str(program), '--game', overlay.GAME], cwd=target, env=env,
                            capture_output=True, timeout=120)
    if result.returncode:
        raise RuntimeError(f'{name}: exit {result.returncode}: {result.stderr[-2000:].decode("latin-1")}')
    with wave.open(str(path), 'rb') as wav:
        channels, rate, width = wav.getnchannels(), wav.getframerate(), wav.getsampwidth()
        body = wav.readframes(wav.getnframes())
    if width == 1:
        samples = [(x - 128) * 256 for x in body]
    else:
        samples = array.array('h', body)
        if sys.byteorder != 'little':
            samples.byteswap()
    return (channels, rate, width), samples


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('program', type=Path)
    args = ap.parse_args()
    failures = 0

    def check(ok, what):
        nonlocal failures
        if not ok:
            failures += 1
            print(f'MISMATCH soundprecision: {what}')

    with tempfile.TemporaryDirectory(prefix='port-soundprecision') as temp:
        work = Path(temp)
        runs = {}
        for name, dma, bits in (('chosen16', 5, 16), ('chosen8', 5, 8), ('dma16', 5, 0), ('dma8', 1, 0)):
            fmt, samples = capture(args.program.resolve(), work, name, dma, bits)
            check(fmt == (2, 22050, 2) and len(samples) > 22050 * 10, f'{name}: the menu in 16-bit stereo')
            runs[name] = samples
        check(runs['dma16'] == runs['chosen16'], 'a 16-bit DMA: the 16-bit choice\'s output')
        check(runs['dma8'] == runs['chosen8'], 'an 8-bit DMA: the 8-bit choice\'s output')
        sixteen, eight = runs['chosen16'], runs['chosen8']
        check(len(sixteen) == len(eight), '8 bits: the same samples as 16')
        power16 = sum(a * a for a in sixteen)
        power8 = sum(b * b for b in eight)
        difference = 10 * math.log10(power8 / power16) if power16 and power8 else float('inf')
        check(abs(difference) < 6, f'8 bits near the 16-bit level ({difference:.2f} dB)')
        print(f'soundprecision {overlay.GAME}: 8 bits {difference:+.2f} dB against 16')
    print(f'soundprecision {overlay.GAME}: {failures} mismatches')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
