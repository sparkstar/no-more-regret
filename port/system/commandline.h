/* The port's own options on the command line, which come out of it before the game reads the rest:
 *     --debug-feature        the port's debug features for the run (host.h's
 *                            hostDebugFeatures), as debug_feature=true in the
 *                            game folder's enhanced.cfg gives them: the rewind,
 *                            the fast forward, the states, the pause, a
 *                            replay's take-over, and the options below but
 *                            --game and --version, which without them end
 *                            the run
 *
 *     --record <directory>   the run, which has a window, recorded into
 *                            the directory, made if need be (replay.h);
 *                            with --replay, what the replay gives
 *
 *     --replay <directory>   a recorded run again, without a window, as
 *                            fast as the host goes
 *     --watch                the replay in a window at its pace: the
 *                            rewind and the states go along it, and at its
 *                            end (--to's) or when taken over (Ctrl+F11) the
 *                            window's input goes on, recorded with --record
 *
 *     --continue <directory> a recorded run gone on with: the replay
 *                            watched from its end (--to's), where the
 *                            window's input takes it over, recorded with
 *                            --record
 *     --capture <file>       the replay's picture and sound as NUT
 *                            (capture.h); - the standard output
 *     --from <time>          the capture from that time on; a watched
 *                            replay shown from then, as fast as it can go
 *                            before (replay.h's watchReplay)
 *     --to <time>            the replay up to that time (watched, the
 *                            frame it is taken over at)
 *     --keep-files <directory>  the replay's player's files at its end
 *                            (its sandbox's: a save it made), copied there
 *                            (a recording from a save)
 *     --game <regret|remorse>  the game the run plays (else the recording's,
 *                            else the last one played; restart.h)
 *     --version              the port's name and version on the standard
 *                            output, and nothing else (portVersionAsked)
 * A time is a frame, a sixtieth of a second, or [h:]m:ss[.ss]. A mistake
 * ends the run with what is wrong. The rest is the game's (the original's
 * command line: -warp, -egg, -skill and the like): a recording keeps it in
 * its run.tsv and a replay gives the game the recording's (replay.h); given
 * with --replay, it must be the recording's. */
#ifndef PORT_COMMANDLINE_H
#define PORT_COMMANDLINE_H
#include <string>
#include <vector>

/* --version anywhere on the command line: the port's name and version
 * (portversion.h) printed, and true; main then ends before anything else. */
bool portVersionAsked(int argc, char **argv);
void portCommandLine(int &argc, char **&argv);

/* Whether enhanced.cfg's text turns the debug features on: a line
 * debug_feature=true (or 1), its key in any case, spaces round it. */
[[nodiscard]] bool debugFeatureLine(const std::string &text);
/* The first of the options that are debug features (--record, --replay,
 * --continue, --watch, --capture, --from, --to, --keep-files) among the
 * command line's words, "" for none. */
[[nodiscard]] std::string debugOption(const std::vector<std::string> &words);
#endif
