# Debug features

English | [한국어](DEBUG_FEATURES.ko.md)

What the port is fixed and tested with: the rewind, a saved state, the fast forward, the pause, a play's recording and
replay, and a replay's picture and sound captured.

## Turning them on

- They are off by default. `--debug-feature` on the command line, or `debug_feature=true` in the game folder's
  `enhanced.cfg`, turns them on, as the run starts and for the whole run.
- Without them the command line options below end the run before it starts, and the keys below go to the game.
- With them the keys below are on the key settings screen too, to be changed there. While one of the features works,
  the window's top left shows it and the frame the run is at.

## Keys

| Key | What it does |
|---|---|
| F11 (held) | Rewind, four times the game's speed; the game goes on where the key is let go |
| F12 / Shift+F12 | Save / load the state: one, in memory, gone when the run ends |
| F6 (held) | Fast forward |
| Pause | Pause on and off; no game time passes while paused |
| Ctrl+F11 | Take over a recording watched with `--watch` where it is, and play on |

## Recording and replay

A recording is a directory: what every frame (a sixtieth of a second) gave the game (`inputs.tsv`), and the saves and
settings files as they were when the recording began (`files/`). The same input gives the same play, so a replay plays
the recording's play again. A replay writes into a copy of `files/`, so the game folder's saves and settings are not
touched.

| Option | What it does |
|---|---|
| `--record DIR` | Records the play with a window into DIR. A rewind or a state's load while recording takes the recording back too |
| `--replay DIR` | Replays a recording without a window, as fast as it can; says so if the game's state comes to differ from the recording's |
| `--watch` | Shows `--replay`'s recording in the window at its own pace; at its end, or taken over with Ctrl+F11, the play is the player's |
| `--continue DIR` | Replays the recording fast to its end and plays on from there; with `--record`, records on |
| `--capture FILE` | Writes the replay's picture and sound as NUT (640x480, wider with the wide screen; 60 frames a second, 44100 Hz stereo); `-` is the standard output |
| `--from TIME` | `--watch` goes fast to it and shows the replay from there; `--capture` writes from there |
| `--to TIME` | Ends the replay there; `--continue` plays on from there |
| `--keep-files DIR` | Copies the files the replay wrote (saves and so on) into DIR as it ends |

A time is a number of frames or `[h:]m:ss[.ss]`. `--record` with `--replay` records what the replay gives as a new
recording, and the player's input after a take-over.

```sh
./crusader-port --debug-feature --record ~/rec/run1
./crusader-port --debug-feature --replay ~/rec/run1 --watch --from 12:30
./crusader-port --debug-feature --replay ~/rec/run1 --capture - | ffmpeg -f nut -i - run1.mp4
```

`port/tools/replays.py` replays several recordings at once and compares their states with those they hold
([`port/tools/README.md`](../port/tools/README.md)).
