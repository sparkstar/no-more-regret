/* What the port cannot go on from (a saved game it cannot take, an
 * intrinsic it does not have): its name, then the run's end (report.h's
 * portEnd: the crash report has it). */
#ifndef PORT_UNIMPLEMENTED_H
#define PORT_UNIMPLEMENTED_H

[[noreturn]] void portUnimplemented(const char *name);

#endif
