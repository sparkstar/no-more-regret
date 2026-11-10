# The games' CD files

The tests that run the games' own data read each game's files here. Nothing of them is in the repository; the two
folders are kept empty.

| Folder | Game | Version |
|---|---|---|
| `noregret/` | Crusader: No Regret | 1.06 |
| `noremorse/` | Crusader: No Remorse | 1.21 |

Each folder holds the game's CD as its root has it (`FLICS`, `STATIC`, `USECODE`, `SOUND` and the rest) with the
installed game's files of that version over it: the executable (`REGRET.EXE` or `CRUSADER.EXE`), by which CMake
knows the files are there, `ASYLUM.DLL` (and No Regret's `XANDRV32.DLL`), and `STATIC`, `USECODE` and `SOUND`. The
names may be in either case. Links to the files work as well.

GOG's releases keep the CD as an image, `GAME.GOG` in the game's folder (ISO 9660), and the installed files beside
it. Unpack the image into the folder, then copy the installed files over it:

```sh
7z x -ocd/noregret ~/games/no-regret/GAME.GOG
cp -r ~/games/no-regret/{REGRET.EXE,ASYLUM.DLL,XANDRV32.DLL,STATIC,USECODE,SOUND} cd/noregret/

7z x -ocd/noremorse ~/games/no-remorse/GAME.GOG
cp -r ~/games/no-remorse/{CRUSADER.EXE,ASYLUM.DLL,STATIC,USECODE,SOUND} cd/noremorse/
```

Copy them from a fresh install: the game opens some of its `STATIC` files for writing as it plays. The CD image's own
`CRUSADER.EXE` and `USECODE` are of an older version than No Remorse 1.21's, which is why the installed ones go over
them.

CMake looks for the executable when it configures and runs again when a folder's files change. Without it, the
tests of that game's data are left out; the others still run.
