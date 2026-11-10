# The originals' bugs fixed

English | [한국어](BUGFIXES.ko.md)

The port fixes the originals' bugs that are certain: No Regret 1.06's `REGRET.EXE` and No Remorse 1.21's
`CRUSADER.EXE`. This is the list of those fixed so far. Where the port fixes one, a comment in the code says what the
original did; the file is given with each bug (under `port/common/src`). The bugs known and not fixed yet are in the
README ([Known bugs](../README.md#known-bugs-not-fixed-yet)).

How each bug is known:

- **Seen**: in play, in the port's tests, or in the original run in DOSBox.
- **Code**: read from the original's code. The symptom is what that code does; it was not reproduced in the original.

## Summary

| # | Game | Area | Bug | Known |
|---|---|---|---|---|
| 1 | Both | Sound | Playing a sound again right after it ended lost an effects channel | Seen |
| 2 | Both | Settings | Without `CRUSADER.CFG` or its `flicpath` line, the movies' path was read from a null pointer | Seen |
| 3 | Both | Saves | Damaged saves, or saves of another version, were read as they were | Seen |
| 4 | Both | Combat | Whether an NPC close to its target fired was decided by a value left on the stack | Seen |
| 5 | No Remorse | Movies | Five movies stopped before their end to wait for a key | Seen |
| 6 | No Remorse | Movies | The last frames of every movie were not shown | Seen |
| 7 | No Remorse | Subtitles | The subtitles fell further behind as a movie went on | Seen |
| 8 | No Remorse | Subtitles | Two subtitle lines were missing | Seen |
| 9 | No Remorse | Inventory | "Next item" did not reach the pack's last slot | Code |
| 10 | No Remorse | Keys | Keys pressed during a cutscene came in after it | Code |
| 11 | No Remorse | Weapons | While another NPC was controlled, "previous weapon" changed the Silencer's weapon | Code |
| 12 | No Remorse | Joystick | Letting go of the fire button sent one more press | Code |
| 13 | No Remorse | Combat | A small shield sparked at a null pointer for a hit without a place | Code |
| 14 | No Remorse | Combat | The EM-4's hit sound was picked from four entries of a table of two | Code |
| 15 | No Remorse | Combat | The range checks did not check that the target was still there | Code |
| 16 | No Remorse | Music | A song number past the table read past it | Code |
| 17 | No Remorse | Fades | A fade's end resumed the colour cycling without checking it was there | Code |
| 18 | No Remorse | Colour cycling | Its end cleared another module's pointer | Code |
| 19 | No Remorse | Sound | The sound channels' table was not set up | Code |
| 20 | No Remorse | Files | A missing data file's message did not name the file | Seen |
| 21 | Both | Saves | After a load, the Silencer's height did not match his stance | Code |
| 22 | No Regret | Fades | A fade or a jump while another fade ran was dropped | Code |
| 23 | No Regret | Credits | After the credits, the main menu's music did not come back | Code |
| 24 | No Regret | Sprites | A sprite number out of range was used as it was | Code |
| 25 | Both | Messages | The message for a bad gump id had its arguments in the wrong order | Code |
| 26 | Both | Saves | After a load, the usecode processes' names pointed at other text | Code |
| 27 | Both | Memory | Each map loaded left the targets' table behind | Code |
| 28 | No Regret | Memory | Each map loaded left the shapes' preload table behind | Code |
| 29 | No Remorse | Memory | The sound archive's names were not freed | Code |
| 30 | Both | Memory | A map change left the pickup display's shapes behind | Code |
| 31 | Both | Memory | A fade freed by a load or the game's end left its palettes behind | Code |
| 32 | Both | Memory | A viewport on a borrowed buffer lost the rows it built | Code |
| 33 | Both | Pictures | A BMP file opened by path could fail at random | Code |
| 34 | No Remorse | Sound | The sound cache did not check that its archive was there | Code |

1 to 23 can show in play. 24 to 34 have no visible effect: what goes wrong is a message, memory never freed, or code
no caller reaches.

## Bugs that show in play

### 1. Playing a sound again right after it ended lost an effects channel (both games)

- Why: `restartSound` left the "done" mark on a sound that had just ended, and the next `updateChannels` emptied the
  channel while the mixer still played the sound. When the sound cache then freed the sound's slot, it did not find the
  channel to stop it, and the channel read the slot's next sound: past its block when that sound was shorter (the
  original faults, the port halted), a stray piece of sound when longer.
- Seen: a long recording halted on it after 683 minutes.
- Now: freeing a slot also stops the effects that read it. The channels' bookkeeping is kept as the original's, so
  what the game reads of "is it playing" is the same. The sound module also no longer halts in the eight places where
  the original faulted: a read past a block is silence, past a table 0, a division error 0.
- Code: `sound/sndcache.cpp`, `sound/asylum.cpp`. Tests: `soundchain`, `soundtrouble`.

### 2. Without `CRUSADER.CFG` or its `flicpath` line, the movies' path was read from a null pointer (both games)

- Why: only the `flicpath=` line of `CRUSADER.CFG` gives the movies' path. Without the file or the line the path is a
  null pointer, which the movies' `strcat` copies from.
- Seen: the original, run in DOSBox without the file, reaches the menu and the first mission but plays no movie, and
  does not write the file again. The port crashed at the first movie. A player who deletes the file to reset the
  settings meets it.
- Now: without the file the port reads what GOG's installer writes; with the file but no `flicpath` line, that line
  alone is GOG's. As in the original, the file is not written.
- Code: `world/options.cpp`, `<game>/src/world/options_game.h`. Tests: `options`, `remorse_options`.

### 3. Damaged saves, or saves of another version, were read as they were (both games)

- Why: loading first ends the game being played and then reads the file, trusting the classes and sizes of the
  processes saved in it.
- Seen: the port halted at a class or size it did not know, the game being played already gone. What the original does
  with such bytes was not checked.
- Now: the file is checked before anything is loaded: its objects lie within the file, the kernel's block has the
  original's size and mark, the tables' sizes fit, and every process saved is of a known class and size. Otherwise the
  original's message box says "ERROR: Unable to load game. The saved game is damaged or of another version.", and the
  game goes back to the menu or to the game being played. Every save at hand passes the check (72 of them). A save
  holding a movie player's process, which the games never write, still halts.
- Code: `world/savegame.cpp`, `kernel/kernel.cpp`. Tests: `savecheck`, `remorse_savecheck`.

### 4. Whether an NPC close to its target fired was decided by a value left on the stack (both games)

- Why: `fireDistance` checks whether a shot fired now reaches the target. When the shot's shape does not fit at the
  weapon's muzzle and the target is what is there, it returned the distance to a point that call never worked out.
- Seen: when an NPC stood so close that the Silencer was at its muzzle, whether it fired or walked away depended on
  the stack: in the original on DOS's stack at that moment, in the port on the compiler and even the run.
- Now: a target at the muzzle counts as reached, since the shot hits at once, so the NPC fires. In two synthetic
  scenes of No Remorse the original's stack gave "not reached" at such a moment, and the port differs from it there.
- Code: `game/crusader.cpp`. Test: the synthetic scene `point-blank`.

### 5. Five movies stopped before their end to wait for a key (No Remorse)

- Why: the movie player read the file in pieces of 4 KB and never took the last piece, shorter than that, while it
  waited for that piece's data.
- Seen: mva09, mva10, mva11a, mva15a and z01 stopped on a still picture just before their end, waiting for Esc, Space
  or Enter.
- Now: the movies are read to the end of the file, and the five play to their end by themselves.
- Code: `movie/avifile.cpp`.

### 6. The last frames of every movie were not shown (No Remorse)

- Why: the original's playing loop ended with the last 4 frames of a movie with sound left, and 2 of one without.
- Now: every frame is shown. Where the sound ends before the frames, silence fills the rest.
- Code: `movie/movie.cpp`.

### 7. The subtitles fell further behind as a movie went on (No Remorse)

- Why: the subtitle file's frame marks were counted down, and each line stayed one frame more, so the k-th line came
  k frames late.
- Seen: by the 30th line, about two seconds behind the speech.
- Now: each line appears at the frame its file gives.
- Code: `movie/subtitles.cpp`.

### 8. Two subtitle lines were missing (No Remorse)

- Why: a subtitle line was taken only when it began with a letter or a full stop.
- Seen: one line of 03d (frames 122 to 182) and one of 14a (frames 244 to 304) were not shown.
- Now: those lines are read too.
- Code: `movie/subtitles.cpp`.

### 9. "Next item" did not reach the pack's last slot (No Remorse)

- Why: the pack is slots 11 to 25, but "next item" went back to 11 from 24.
- The item in slot 25 could be chosen with "previous item" alone, and with an item in slot 25 only, "next item" chose
  nothing.
- Now: both games use the same function, which goes to the pack's last slot.
- Code: `world/world.cpp`.

### 10. Keys pressed during a cutscene came in after it (No Remorse)

- Why: when usecode ended the stasis of a cutscene or a talk, No Remorse cleared the flag but not the keys pressed
  meanwhile (No Regret's cleared them).
- The keys came in at once as the stasis ended, and the Silencer moved or fired by himself.
- Now: the keys are cleared in both games.
- Code: `world/world.cpp`.

### 11. While another NPC was controlled, "previous weapon" changed the Silencer's weapon (No Remorse)

- Why: "next weapon" did nothing while the NPC controlled was not the Silencer, but "previous weapon" had no such
  check.
- While a robot was controlled, "previous weapon" changed the Silencer's weapon.
- Now: No Remorse blocks both. No Regret blocks neither, as its original.
- Code: `world/world.cpp`.

### 12. Letting go of the joystick's fire button sent one more press (No Remorse)

- Why: letting go of the fire button sent the fire key to the controller still marked as pressed.
- One more shot was fired, and the controller never saw the button let go, so firing stayed held: the PL-1's auto
  fire could go on.
- Now: it is sent as let go. The port's gamepads come through its own input bindings, so this DOS joystick driver is
  not used in play.
- Code: `input/joydrv.cpp`.

### 13. A small shield sparked at a null pointer for a hit without a place (No Remorse)

- Why: a hit takes the place where it hits, but usecode can deal one without a place. No Regret then uses the NPC's
  place; No Remorse made the small shield's spark at a place read from a null pointer.
- An NPC with a small shield hit that way sparked somewhere else.
- Now: the hit is at the NPC's place, as in No Regret.
- Code: `game/crusader.cpp`.

### 14. The EM-4's hit sound was picked from four entries of a table of two (No Remorse)

- Why: the EM-4's hit sound was picked at random from 4 entries of its table of 2 sounds. The other two are the next
  table's: the shape numbers of the small blasts.
- Half of the EM-4's hits asked for a shape number as a sound: no sound, or a wrong one.
- Now: it picks from 2, as the other fire type that uses the table does. The random numbers drawn are the same.
- Code: `game/explode.cpp`.

### 15. The range checks did not check that the target was still there (No Remorse)

- Why: the intrinsics that check whether a target is within range, and whether it is in sight, read its place and
  shape without checking it.
- A target gone, or none, was measured at a deleted item's place or item 0's: an NPC could find a target in range that
  was not there.
- Now: they give 0 for such a target, as in No Regret.
- Code: `game/crusader.cpp`.

### 16. A song number past the table read past it (No Remorse)

- Why: a song's path was looked up without checking the song number. The songs' names have 21 entries and the files
  to swap 20, so even the last number read one entry past the second table.
- A number past the table used the memory after it as a file name.
- Now: such a song is none, and the music stops.
- Code: `sound/music.cpp`.

### 17. A fade's end resumed the colour cycling without checking it was there (No Remorse)

- Why: a fade that was to resume the colour cycling did so at its end without checking that the colour cycling's
  process was there; everywhere else checks it.
- Before that process starts, or after it ends, the original wrote through a null pointer: on DOS it overwrote a byte
  at the start of the data segment.
- Now: the cycling resumes only when its process is there.
- Code: `graph/fade.cpp`.

### 18. The colour cycling's end cleared another module's pointer (No Remorse)

- Why: the colour cycling's process cleared, as it ended, the pointer to the standard interrupt's handler instead of
  its own.
- A process that then subscribed to the standard interrupt used a null pointer. After a load the handler registered
  again, so it mostly did not show.
- Now: it is not cleared, as in No Regret.
- Code: `graph/cycle.cpp`.

### 19. The sound channels' table was not set up (No Remorse)

- Why: the sound manager reset its channels before it marked itself started, and the reset works only once started,
  so the new table of channels was left as the memory held it. At the end it was the other way round.
- Until each channel was first used, its handle, priority and flags were whatever the memory held, which could mislead
  the search for a channel by its handle or for a free one.
- Now: as in No Regret, it is marked started first and then reset; at the end, reset and then marked stopped.
- Code: `sound/sndmgr.cpp`.

### 20. A missing data file's message did not name the file (No Remorse)

- Why: the message for a game data file that could not be opened took the file's name, which is empty until the file
  is open.
- Seen: in the port while it followed the original.
- Now: the message names the path looked for, as in No Regret.
- Code: `file/fastflex.cpp`.

### 21. After a load, the Silencer's height did not match his stance (both games)

- Why: the Silencer's height is not in the save; his next animation sets it. After a load No Regret set it for a
  kneeling Silencer alone, and No Remorse not at all.
- A game saved in another stance than the one at the load kept the old stance's height until he moved: the shots hit
  him, and things hid him, as if he still stood or knelt.
- Now: he kneels or stands up as loaded.
- Code: `world/savegame.cpp`.

### 22. A fade or a jump while another fade ran was dropped (No Regret)

- Why: a new fade, while another fade ran, was left stopped, and jumps such as one to black were dropped.
- Such fades and jumps did not happen. It is rare, since usecode waits for each fade to end (in 37 of 38 places).
- Now: both games do it as No Remorse: a fade or a jump takes effect while another fade runs.
- Code: `graph/fade.cpp`.

### 23. After the credits, the main menu's music did not come back (No Regret)

- Why: the credits paused the menu's music as they began, and paused it again as they ended. No Remorse's stopped the
  music.
- Now: in both games the music goes on after the credits.
- Code: `gump/credits.cpp`.

## Bugs with no visible effect

### 24. A sprite number out of range was used as it was (No Regret)

- Why: at a sprite number out of range, No Regret broke into a debugger and then went on with that number: without a
  debugger, it read and wrote past the sprites' table.
- Now: the game halts there in both games, as No Remorse's does.
- Code: `display/spmgr.cpp`.

### 25. The message for a bad gump id had its arguments in the wrong order (both games)

- Why: the halt for a bad gump id passed its message as the file name and the id as the line.
- Now: the halt gives the id, at the place of the call.
- Code: `gump/newgump.cpp`.

### 26. After a load, the usecode processes' names pointed at other text (both games)

- Why: a load took every name in the save for a string of the program. A usecode process's name is its class's name,
  which lies in the usecode's file, so after a load it pointed at other text of the program.
- Only the debug output shows these names.
- Now: after a load, a usecode process takes its class's name again.
- Code: `kernel/kernel.cpp`, `world/savefields.h`.

### 27. Each map loaded left the targets' table behind (both games)

- Why: each new game and map loaded made a new targets' table (200 bytes) without freeing the one before.
- Now: the one before is freed.
- Code: `item/tracker.cpp`.

### 28. Each map loaded left the shapes' preload table behind (No Regret)

- Why: each map loaded made a table of the shapes it uses (2 KB) and never freed it.
- Now: it is freed.
- Code: `item/preload.cpp`.

### 29. The sound archive's names were not freed (No Remorse)

- Why: the archive of the sounds (`sound.flx`) made a table of names, which its end did not free: once, as the game
  ended.
- Now: it is freed.
- Code: `file/tagflex.cpp`.

### 30. A map change left the pickup display's shapes behind (both games)

- Why: a map change cleared the table of the pickup display's lines without freeing the shapes of the lines shown.
- Now: the shapes are freed first.
- Code: `display/display.cpp`.

### 31. A fade freed by a load or the game's end left its palettes behind (both games)

- Why: as a load began, or the game ended, the original freed the processes still running without their ends, and a
  fade's two palettes were freed by its end.
- Now: a fade frees them however it is freed.
- Code: `graph/fade.h`.

### 32. A viewport on a borrowed buffer lost the rows it built (both games)

- Why: a viewport drawing into another's buffer built a table of its rows, which it never freed.
- Now: it frees them.
- Code: `graph/vport.cpp`.

### 33. A BMP file opened by path could fail at random (both games)

- Why: opening a BMP file by its path did not first clear the file's pointer, and opening takes a pointer that is not
  clear for a file already open. With what the memory held, the open could fail. Neither game opens one that way.
- Now: the pointer is cleared first.
- Code: `graph/bmp.cpp`.

### 34. The sound cache did not check that its archive was there (No Remorse)

- Why: tidying the sound cache walked its blocks without checking that the sounds' archive was there. It is only
  done after the sound effects have started, when the archive is always there.
- Now: it does nothing without the archive, as in No Regret.
- Code: `sound/sndcache.cpp`.

## Not bugs, made the same in both games

Where the two games did the same thing in different ways, both now do it one way. What changed for No Remorse:

| What | No Remorse's original | Now, in both games |
|---|---|---|
| Eggs | Hatched for the Silencer, even while another NPC was controlled | Hatch for the NPC controlled |
| Music | The song playing, asked for again, began from the start | Goes on (the songs that take turns, m16a to m16c, go to the next) |
| Targets | The list of targets was made again only when an NPC teleported | Also when the map changes |
| A save's or a load's end | The teleport was cleared by the world's thaw | The save and the load clear the teleport and the stasis themselves, and the keys pressed |
| Start | The mouse driver was not reset | It is reset |

And one for No Regret: a shape button took a click anywhere in its rectangle, and now takes it only on the pixels
drawn, as in No Remorse.

## Found and kept as the originals

These were found too, but kept: they cannot be told apart from the game's design, or they have no effect.

| Game | What | Why it is kept |
|---|---|---|
| Both | A hit with no direction (at the same x and y as the object) pushes the object by values read past the table of the 16 directions, so it flies far north or south | The originals' behaviour. The port reads the same values the originals read, so that every build plays the same |
| Both | When checking whether a shape fits, the stand-in shape 31Ah gets its own solidity, not the shape's | The same in both games; the usecode may rely on it |
| Both | The NPC types' table is never freed, the condition being the wrong way round | Called only in the original's cleanup; the port frees the table at its end |
| No Remorse | A shot's hit passes the shot, not the NPC who fired it, so the code by which an NPC hit by the player stops and fires back never runs | Hard to tell from the game's design |
| No Remorse | An explosion's sound comes from the NPC controlled, not from the explosion's place | The same |
| No Remorse | A kneeling attacker walks after a target too high for it | The same |
| No Remorse | An NPC in a feint attack cannot change to another activity in combat | The same |
| No Remorse | After a kneeling attack, an NPC that goes to stand or patrol keeps its weapon drawn | The same |
| No Remorse | An NPC teleporting within its map empties the list of targets and makes it again | The same |
| No Remorse | The reticle shows while the Silencer is held (by a healer, a charger or an egg's teleport); No Regret hides it | A small difference of each game's own, kept as each original |
| No Regret | On the tick the joystick's fire button is let go, the directions are sent as let go too | Not reached in the port's play (see 12) |
