/* Crash reports for the playable port. No game state is changed by reporting. */
#ifndef PORT_CRASH_H
#define PORT_CRASH_H

/* The handlers of the program's faults; name is the program's (its
 * messages' prefix and its reports' file names), title the game's (the
 * reports' head), from the game's profile (profile.h). Before it, the
 * messages and reports say "port". */
void installCrashHandler(const char *executable, const char *name, const char *title);
/* The handlers' own stack for the calling thread, which a thread other than
 * the program's first needs before it can fault: POSIX gives each thread
 * its alternate signal stack (a stack overflow is reported on it). Nothing
 * on Windows, whose handlers are the process's. */
void installCrashThread();
void crashReport(const char *reason);
/* The report the run wrote (crashReport, a fault's), nullptr before one or
 * when it could not be written. */
const char *crashReportPath();
[[noreturn, gnu::format(printf, 2, 3)]] void fatalError(int status, const char *format, ...);

#endif
