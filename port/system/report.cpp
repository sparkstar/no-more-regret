/* See report.h. */
#include "report.h"
#include "gamememory.h"
#include "host.h"
#include "window.h"
#include "crash.h"
#include "profile.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

namespace {

/* The text at the end of regret.log, after the time. */
void logLine(const char *text)
{
    const std::string name = std::string(PROFILE_NAME) + ".log";
    if (std::FILE *log = std::fopen(name.c_str(), "a")) {
        char when[32];
        const std::time_t now = std::time(nullptr);
        std::strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        std::fprintf(log, "%s %s\n", when, text);
        std::fclose(log);
    }
}

}

std::string endMessageText(const char *reportPath)
{
    std::string text = "The game stopped because of an error.";
    if (reportPath != nullptr && *reportPath != 0) {
        text += "\n\nA report of it is in\n";
        text += reportPath;
    }
    return text;
}

void hostEndMessage(const char *text)
{
    const SystemSide systemSide;
    /* Once, from either of the run's threads (window.h): the game thread's
     * halt, the window thread's failure. */
    static std::atomic<bool> said{false};

    if (!windowRun() || said.exchange(true)) {
        return;
    }
    logLine(text);
    windowMessage(endMessageText(crashReportPath()).c_str());
}

void hostNotice(const char *text)
{
    const SystemSide systemSide;
    char line[512];
    std::snprintf(line, sizeof(line), "%s: %s", PROFILE_NAME, text);
    std::fflush(stdout);
    std::fprintf(stderr, "%s\n", line);
    std::fflush(stderr);
    if (windowRun()) {
        logLine(line);
    }
}

void portRefuse(const char *format, ...)
{
    char text[512];
    std::snprintf(text, sizeof(text), "%s: ", PROFILE_NAME);
    const std::size_t prefix = std::strlen(text);
    std::va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(text + prefix, sizeof(text) - prefix, format, arguments);
    va_end(arguments);
    std::fflush(stdout);
    std::fprintf(stderr, "%s\n", text);
    std::fflush(stderr);
    windowExit(3);
}

void portEnd(int code, const char *format, ...)
{
    char text[512];
    std::snprintf(text, sizeof(text), "%s: ", PROFILE_NAME);
    const std::size_t prefix = std::strlen(text);
    std::va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(text + prefix, sizeof(text) - prefix, format, arguments);
    va_end(arguments);
    std::fflush(stdout);
    std::fprintf(stderr, "%s\n", text);
    std::fflush(stderr);
    crashReport(text);
    hostEndMessage(text);
    windowExit(code);
}
