/* The rewind and the state's save and load, between two of the game's steps
 * (end.h's playGame calls hostFrame after each in which a frame passed).
 *
 * At each such frame the game's memory (its arena, its two static places)
 * and the screen's are compared with a copy of them at the frame before,
 * and what differs goes, as it was then, onto a ring (256 MB at most, the
 * oldest dropped), with what is small enough to keep whole: the virtual
 * PC's state (pcstate.h), the keys the game had down, and the sound that
 * came in the frame. While the rewind's key is held, four frames each
 * sixtieth of a second are put back from the ring (four times as fast as
 * the game went), the screen shown and their sound played backwards, quiet and
 * thinned to its time; at the key's going up the game goes on
 * from there, the input of the rewind dropped and the keys and buttons
 * brought to those down now (window.h).
 *
 * The state's save is a snapshot (snapshot.h) kept in memory, one; its load
 * puts it back and starts the ring anew.
 *
 * While the run follows a recording (replay.h) the ring keeps also the
 * player's settings and files (the files each frame wrote, as they were
 * before) and where the run is in the recordings: a rewind goes back in the
 * replay, and cuts what the recorder has after the frame and counts a
 * rerecord, as a state's load does with the state's lines (snapshot.h). A
 * recording is of one line of time: the one the run went on. The ring and the
 * state start anew when the run follows a recording no more (a replay watched
 * to its end) or does now. A replay watched from a later frame keeps the ring
 * from its lead only (replay.h's watchReplay); as the game draws again there,
 * it draws the whole screen anew first (host.h's hostRepaintAction).
 *
 * The replay's test, PORT_REWIND_TEST=<frame>:<frames>, rewinds that many
 * frames at that frame, so that the replay goes on as recorded only if the
 * ring held every frame, and says whether the player's settings and files
 * are as they were then. */
#ifndef PORT_REWIND_H
#define PORT_REWIND_H

/* playGame's after a step in which a frame passed: the snapshots' test
 * (snapshot.h's snapshotFrame), the state asked for, the rewind or the
 * frame onto the ring. */
void hostFrame();
#endif
