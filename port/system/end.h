/* The run's end (platform/host.h's hostEnd): the game's run returns
 * from main, not from the middle of the game's calls. */
#ifndef PORT_END_H
#define PORT_END_H
/* The game's run: start, then step after step until the run ends
 * (hostEnd), then release, the game's release of what it holds; the run's
 * exit code. Each step returns at a pass of the game's time, its waits
 * kept as state; after a step in which a frame of the virtual PC passed,
 * frame runs (rewind.h's hostFrame), with none of the game's calls under
 * it. */
int playGame(int (*start)(int, char **), void (*step)(), void (*release)(), void (*frame)(), int argc, char **argv);
/* Between two of the game's steps (playGame's frame): none of the game's
 * calls is running. */
[[nodiscard]] bool betweenSteps();
/* The run's end has begun (hostEnd): what follows, the unwinding and the
 * release, is no more of the run's frames (timer.cpp's windowFrameNow). */
[[nodiscard]] bool runEnding();
#endif
