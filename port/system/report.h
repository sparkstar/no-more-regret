/* How a run says why it ends. The game's halts (halt.cpp) print on the
 * standard output and the port's own ends on the standard error in any run,
 * and the crash report (compat/crash.h) has why; a run with a window has no
 * console to see them (a Windows program's closes with it, one started from
 * the desktop has none), so the why goes to the end of regret.log in the
 * current directory (the install's), with the time, and a message box over
 * the window says only that the game stopped and where the report is
 * (endMessageText): what went wrong is for the report. Script runs (the
 * tests) and the build without a window leave the box and the log out.
 * hostEndMessage (host.h) is the game's way in; hostNotice puts what a run
 * passes over and goes on from on the standard error and, with a window, in
 * regret.log. A mistake in what the run was given (the command line, a
 * recording, a script, a tool's variable) is no crash: portRefuse says it
 * on the standard error alone. */
#ifndef PORT_REPORT_H
#define PORT_REPORT_H
#include <string>

/* The port's own end: "regret: " and the text on the standard error, after
 * what the standard output holds; the crash report and hostEndMessage with
 * it; then the exit code. */
[[noreturn, gnu::format(printf, 2, 3)]] void portEnd(int code, const char *format, ...);
/* The end of a run given something it cannot take: "regret: " and the text
 * on the standard error, then the exit code 3; no crash report, box or
 * log. */
[[noreturn, gnu::format(printf, 1, 2)]] void portRefuse(const char *format, ...);
/* The message box's text at the end: the game stopped, and the report's
 * place when there is one (nullptr: none). */
[[nodiscard]] std::string endMessageText(const char *reportPath);
#endif
