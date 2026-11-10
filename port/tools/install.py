#!/usr/bin/env python3
"""Install the game for the port: what INSTALL.EXE's full install puts on
the hard drive, a copy of the CD's STATIC and USECODE directories, an empty
SAVEGAME and crusader.cfg, in DIR, which regret runs in with CRUSADER_CD set
to the CD.

usage: install.py DIR [--cd CD] [--force] [--sound] [--no-movies]

The game is CRUSADER_GAME's (tools/overlay.py): No Regret's by default,
No Remorse's with CRUSADER_GAME=noremorse, whose program is remorse and
whose CD may name its directories and files in lower case.

The game reads most of its static data and the usecode from the hard
drive (makePath without a drive) and opens some of it for writing, so
the files are writable copies, not links into the CD. The files the game
reads from the CD (gameDrive) stay there: the maps' fixed items, whose
copy on the hard drive would be a patch ("Using map patch file."), the
help pages and the credits, the names INSTALL.EXE has in capitals (No Remorse's help
pages are .dat files). CD defaults to the game's original in the
repository (cd/noregret/, cd/noremorse/). DIR must be new or empty unless
--force is given, which copies over what is there.

crusader.cfg has the settings INSTALL.EXE writes, in its forms (its first
line is No Remorse's), for a machine without a sound card: the sound and
the music off, which also leaves the sound effects (SOUND\\SOUND.FLX) on
the CD; the keyboard and mouse; the CD at D: and its movies in flics, which
the game needs set (with no flicpath it copies from a null pointer).
No Remorse's has no moviehicolor, which it does not read, and fullinstall,
which a full install sets: the shapes stay in memory, as No Regret's always
do, and are not swapped in from the CD.

--sound installs for the port's sound card: a Sound Blaster
16 (AMFLOAD's card 3, ASYLUM.DLL's "Sound Blaster 16/AWE32") at 220h, IRQ 7
and the 16-bit DMA 5, the sound and the music on, as INSTALL.EXE writes them
(no mixrate: the game's own is 22050), and the sound effects copied to the
hard drive, where the game opens them (the music it reads from the CD).

--no-movies points flicpath at a directory the CD does not have, so that the
game plays none of the CD's movies (FLICS) and goes on as without them: the
milestones' input is without movies.
"""
import argparse, shutil, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
import overlay  # noqa: E402

DIRECTORIES = ('STATIC', 'USECODE')
CONFIG = {
    'regret': ['// CRUSADER NO REMORSE Configuration File.', 'soundcard=0', 'sound=off', 'music=off',
               'moviehicolor=false', 'control=0', 'cdletter=D', 'flicpath=flics'],
    'remorse': ['// CRUSADER NO REMORSE Configuration File.', 'soundcard=0', 'sound=off', 'music=off', 'control=0',
                'cdletter=D', 'flicpath=flics', 'fullinstall=true'],
}[overlay.GAME]
SOUND = ['soundcard=3', 'sound=on', 'music=on', 'port=0x220', 'irq=7', 'dma=5']
CD_ONLY = {
    'regret': {'FIXED.DAT', 'HELP1.BMP', 'HELP2.BMP', 'HELP3.BMP', 'HELP4.BMP', 'HELP5.BMP', 'CRED.DAT', 'CRED.PAL',
               'CREDITS.DAT'},
    'remorse': {'FIXED.DAT', 'HELP1.DAT', 'HELP2.DAT', 'HELP3.DAT', 'CRED.DAT', 'CRED.PAL', 'CREDITS.DAT'},
}[overlay.GAME]


def entry(directory, name):
    """The entry of directory called name in any case (No Remorse's CD is in
    lower case), as named when there is none."""
    for path in directory.iterdir():
        if path.name.upper() == name.upper():
            return path
    return directory / name


def install(target, cd, sound=False, movies=True):
    target.mkdir(parents=True, exist_ok=True)
    for name in DIRECTORIES:
        # The contents alone: the CD's files are read-only, the installed
        # ones not.
        shutil.copytree(entry(cd, name), target / name, copy_function=shutil.copyfile, dirs_exist_ok=True,
                        ignore=lambda _, names: [n for n in names if n.upper() in CD_ONLY])
    (target / 'SAVEGAME').mkdir(exist_ok=True)
    config = CONFIG
    if sound:
        (target / 'SOUND').mkdir(exist_ok=True)
        shutil.copyfile(entry(entry(cd, 'SOUND'), 'SOUND.FLX'), target / 'SOUND' / 'SOUND.FLX')
        config = [line for line in CONFIG if not line.startswith(('soundcard=', 'sound=', 'music='))]
        config[1:1] = SOUND
    if not movies:
        config = [line if line != 'flicpath=flics' else 'flicpath=nomovies' for line in config]
    (target / 'crusader.cfg').write_bytes(''.join(line + '\r\n' for line in config).encode())


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('dir', type=Path)
    ap.add_argument('--cd', type=Path, default=overlay.ORIGIN)
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--sound', action='store_true', help='install for the port\'s Sound Blaster 16')
    ap.add_argument('--no-movies', action='store_true', help='play none of the CD\'s movies')
    args = ap.parse_args()
    if args.dir.exists() and any(args.dir.iterdir()) and not args.force:
        sys.exit(f'{args.dir} is not empty (--force copies over it)')
    if not entry(args.cd, 'STATIC').is_dir():
        sys.exit(f'{args.cd} has no STATIC directory')
    install(args.dir, args.cd, args.sound, not args.no_movies)
    print(f'installed in {args.dir}; run {overlay.GAME} there with CRUSADER_CD={args.cd.resolve()}')


if __name__ == '__main__':
    main()
