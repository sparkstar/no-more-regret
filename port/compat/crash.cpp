/* Fatal errors and unexpected crashes leave a report beside the game (or in
 * REGRET_CRASH_DIR). A signal handler writes its essential fields with
 * async-signal-safe calls before attempting a best-effort stack trace. */
#include "crash.h"
#include "portversion.h"

#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <initializer_list>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <csignal>
#else
#include <cerrno>
#include <csignal>
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

char reportPath[4096];
char fallbackPath[4096];
/* reportPath or fallbackPath once a report is in it (crashReportPath). */
const char *writtenPath = nullptr;
char executablePath[4096] = "port";
char programName[64] = "port";
char programTitle[128] = "Crusader";
char reportTime[32];
std::once_flag pathOnce;
std::atomic_flag reporting = ATOMIC_FLAG_INIT;

void makePath(char *out, const std::filesystem::path &directory, const char *name)
{
    const std::string path = (directory / name).string();
    if (path.size() < sizeof(reportPath)) {
        std::memcpy(out, path.c_str(), path.size() + 1);
    }
}

void preparePaths()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
    unsigned long pid = GetCurrentProcessId();
#else
    localtime_r(&now, &local);
    const unsigned long pid = static_cast<unsigned long>(getpid());
#endif
    std::strftime(reportTime, sizeof(reportTime), "%Y-%m-%d %H:%M:%S", &local);
    char name[160];
    std::snprintf(name, sizeof(name), "%s-crash-%04d%02d%02d-%02d%02d%02d-%lu.log", programName, local.tm_year + 1900,
                  local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, pid);

    std::error_code error;
    std::filesystem::path directory;
    if (const char *configured = std::getenv("REGRET_CRASH_DIR"); (configured != nullptr) && (*configured != 0)) {
        directory = std::filesystem::absolute(configured, error);
        if (!error) {
            std::filesystem::create_directories(directory, error);
        }
    } else {
        directory = std::filesystem::current_path(error);
    }
    if (!error) {
        makePath(reportPath, directory, name);
    }
    error.clear();
    directory = std::filesystem::temp_directory_path(error);
    if (!error) {
        makePath(fallbackPath, directory, name);
    }
}

const char *reportFile(std::FILE *&file)
{
    for (const char *path : {reportPath, fallbackPath}) {
        if ((*path != 0) && ((file = std::fopen(path, "wb")) != nullptr)) {
            return path;
        }
    }
    return nullptr;
}

void writeReason(std::FILE *file, const char *reason)
{
    std::fputs("Reason: ", file);
    for (const char *at = reason; *at != 0; at++) {
        std::fputc(*at == '\r' || *at == '\n' ? ' ' : *at, file);
    }
    std::fputc('\n', file);
}

void writeCrashTime(std::FILE *file)
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char when[32];
    std::strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", &local);
    std::fprintf(file, "Crash time: %s\n", when);
}

#ifdef _WIN32
const char *exceptionName(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        return "access violation";
    case EXCEPTION_IN_PAGE_ERROR:
        return "memory page read error";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "integer division by zero";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "floating-point division by zero";
    case EXCEPTION_STACK_OVERFLOW:
        return "stack overflow";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "illegal instruction";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "array bounds exceeded";
    default:
        return "unhandled exception";
    }
}

/* MinGW can turn a Windows fault into a C signal before the top-level
 * filter sees it. Keep the faulting context without changing whether an
 * exception is handled; the signal handler uses it only if the run ends. */
struct WindowsFault {
    EXCEPTION_RECORD record;
    CONTEXT context;
    bool ready;
};
thread_local WindowsFault lastFault;

LONG WINAPI rememberFault(EXCEPTION_POINTERS *exception)
{
    switch (exception->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        lastFault.record = *exception->ExceptionRecord;
        lastFault.record.ExceptionRecord = nullptr;
        lastFault.context = *exception->ContextRecord;
        lastFault.ready = true;
        break;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

bool signalMatchesFault(int signal, DWORD code)
{
    if (signal == SIGSEGV) {
        return code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR ||
               code == EXCEPTION_STACK_OVERFLOW;
    }
    if (signal == SIGILL) {
        return code == EXCEPTION_ILLEGAL_INSTRUCTION;
    }
    if (signal == SIGFPE) {
        return code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_FLT_DIVIDE_BY_ZERO;
    }
    return false;
}

void miniDump(const char *logPath, EXCEPTION_POINTERS *exception)
{
    char path[sizeof(reportPath)];
    std::snprintf(path, sizeof(path), "%s", logPath);
    char *suffix = std::strrchr(path, '.');
    if (!suffix || std::strlen(suffix) != 4) {
        return;
    }
    std::memcpy(suffix, ".dmp", 5);
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), exception, FALSE};
    BOOL written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal,
                                     exception ? &info : nullptr, nullptr, nullptr);
    CloseHandle(file);
    if (!written) {
        DeleteFileA(path);
    } else {
        std::fprintf(stderr, "%s: minidump: %s\n", programName, path);
    }
}

LONG WINAPI windowsException(EXCEPTION_POINTERS *exception)
{
    char reason[160];
    std::snprintf(
        reason, sizeof(reason), "Windows %s (0x%08lX) at %p", exceptionName(exception->ExceptionRecord->ExceptionCode),
        (unsigned long)exception->ExceptionRecord->ExceptionCode, exception->ExceptionRecord->ExceptionAddress);
    if (!reporting.test_and_set(std::memory_order_relaxed)) {
        std::FILE *file = nullptr;
        const char *path = reportFile(file);
        if (path) {
            std::fprintf(file, "%s crash report\nVersion: %s %s\nStarted: %s\nExecutable: %s\n", programTitle,
                         PORT_NAME, portVersion(), reportTime, executablePath);
            writeCrashTime(file);
            writeReason(file, reason);
            std::fclose(file);
            writtenPath = path;
            miniDump(path, exception);
            std::fprintf(stderr, "%s: crash report: %s\n", programName, path);
        } else {
            std::fprintf(stderr, "%s: %s (crash report could not be written)\n", programName, reason);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void windowsSignal(int signal)
{
    const char *reason = signal == SIGABRT   ? "SIGABRT: abort"
                         : signal == SIGSEGV ? "SIGSEGV: invalid memory access"
                         : signal == SIGILL  ? "SIGILL: illegal instruction"
                         : signal == SIGFPE  ? "SIGFPE: arithmetic error"
                                             : "fatal C signal";
    if (lastFault.ready && signalMatchesFault(signal, lastFault.record.ExceptionCode)) {
        EXCEPTION_POINTERS exception{&lastFault.record, &lastFault.context};
        lastFault.ready = false;
        windowsException(&exception);
    } else {
        crashReport(reason);
    }
    std::_Exit(128 + signal);
}

#else
/* Each thread's alternate stack for the handlers (installCrashThread). */
alignas(16) thread_local unsigned char signalStack[64 * 1024];

/* No stdio, allocation, or locks before the fault information is on disk.
 * backtrace below is best effort: libunwind is not async-signal-safe. */
void writeAll(int fd, const char *text)
{
    std::size_t size = 0;
    while (text[size] != 0) {
        size++;
    }
    while (size != 0U) {
        const ssize_t count = write(fd, text, size);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            break;
        }
        text += count;
        size -= static_cast<std::size_t>(count);
    }
}

void writeNumber(int fd, unsigned long long number, int base)
{
    char text[32];
    int used = 0;
    do {
        const unsigned digit = static_cast<unsigned>(number % base);
        text[used++] = static_cast<char>(digit < 10 ? '0' + digit : 'a' + digit - 10);
        number /= base;
    } while ((number != 0U) && used < static_cast<int>(sizeof(text)));
    for (int i = 0; i < used / 2; i++) {
        const char old = text[i];
        text[i] = text[used - 1 - i];
        text[used - 1 - i] = old;
    }
    text[used] = 0;
    writeAll(fd, text);
}

void signalCrash(int number, siginfo_t *info, void * /*unused*/)
{
    if (reporting.test_and_set(std::memory_order_relaxed)) {
        _Exit(128 + number);
    }
    int fd = -1;
    const char *path = nullptr;
    for (const char *candidate : {reportPath, fallbackPath}) {
        if ((*candidate != 0) && (fd = open(candidate, O_WRONLY | O_CREAT | O_TRUNC, 0600)) >= 0) {
            path = candidate;
            break;
        }
    }
    if (fd >= 0) {
        writeAll(fd, programTitle);
        writeAll(fd, " crash report\nVersion: ");
        writeAll(fd, PORT_NAME);
        writeAll(fd, " ");
        writeAll(fd, portVersion());
        writeAll(fd, "\nStarted: ");
        writeAll(fd, reportTime);
        writeAll(fd, "\nCrash epoch: ");
        writeNumber(fd, static_cast<unsigned long long>(std::time(nullptr)), 10);
        writeAll(fd, "\nExecutable: ");
        writeAll(fd, executablePath);
        writeAll(fd, "\nSignal: ");
        writeNumber(fd, static_cast<unsigned>(number), 10);
        switch (number) {
        case SIGSEGV:
            writeAll(fd, " (SIGSEGV: invalid memory access)");
            break;
        case SIGBUS:
            writeAll(fd, " (SIGBUS: invalid memory access)");
            break;
        case SIGILL:
            writeAll(fd, " (SIGILL: illegal instruction)");
            break;
        case SIGFPE:
            writeAll(fd, " (SIGFPE: arithmetic error)");
            break;
        case SIGABRT:
            writeAll(fd, " (SIGABRT: abort)");
            break;
        }
        writeAll(fd, "\nSignal code: ");
        if (info->si_code < 0) {
            writeAll(fd, "-");
            writeNumber(fd, static_cast<unsigned long long>(-static_cast<long long>(info->si_code)), 10);
        } else {
            writeNumber(fd, static_cast<unsigned>(info->si_code), 10);
        }
        writeAll(fd, "\nFault address: 0x");
        writeNumber(fd, static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(info->si_addr)), 16);
        writeAll(fd, "\nStack (best effort):\n");
        void *frames[64];
        const int count = backtrace(frames, 64);
        backtrace_symbols_fd(frames, count, fd);
        close(fd);
        writeAll(STDERR_FILENO, programName);
        writeAll(STDERR_FILENO, ": crash report: ");
        writeAll(STDERR_FILENO, path);
        writeAll(STDERR_FILENO, "\n");
    } else {
        writeAll(STDERR_FILENO, programName);
        writeAll(STDERR_FILENO, ": crash report could not be written\n");
    }
    /* SA_RESETHAND restored the default action; retain the OS crash status
     * and allow a core dump when the host is configured to produce one. */
    sigset_t unblocked;
    sigemptyset(&unblocked);
    sigaddset(&unblocked, number);
    sigprocmask(SIG_UNBLOCK, &unblocked, nullptr);
    raise(number);
    _Exit(128 + number);
}
#endif

void terminateCrash()
{
    const char *reason = "std::terminate";
    char detail[512];
    if (auto exception = std::current_exception()) {
        try {
            std::rethrow_exception(exception);
        } catch (const std::exception &error) {
            std::snprintf(detail, sizeof(detail), "Uncaught C++ exception: %s", error.what());
            reason = detail;
        } catch (...) {
            reason = "Uncaught non-standard C++ exception";
        }
    }
    crashReport(reason);
    std::_Exit(3);
}

} // namespace

void installCrashHandler(const char *executable, const char *name, const char *title)
{
    /* Before the paths, which have the name. */
    std::snprintf(programName, sizeof(programName), "%s", name);
    std::snprintf(programTitle, sizeof(programTitle), "%s", title);
    std::call_once(pathOnce, preparePaths);
    std::snprintf(executablePath, sizeof(executablePath), "%s", (executable != nullptr) ? executable : name);
    std::set_terminate(terminateCrash);
#ifdef _WIN32
    SetUnhandledExceptionFilter(windowsException);
    AddVectoredExceptionHandler(1, rememberFault);
    std::signal(SIGABRT, windowsSignal);
    std::signal(SIGSEGV, windowsSignal);
    std::signal(SIGILL, windowsSignal);
    std::signal(SIGFPE, windowsSignal);
#else
    installCrashThread();
    struct sigaction action{};
    action.sa_sigaction = signalCrash;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    for (const int number : {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT}) {
        sigaction(number, &action, nullptr);
    }
#endif
}

void installCrashThread()
{
#ifndef _WIN32
    stack_t stack{};
    stack.ss_sp = signalStack;
    stack.ss_size = sizeof(signalStack);
    sigaltstack(&stack, nullptr);
#endif
}

void crashReport(const char *reason)
{
    std::call_once(pathOnce, preparePaths);
    if (reporting.test_and_set(std::memory_order_relaxed)) {
        return;
    }
    std::FILE *file = nullptr;
    const char *path = reportFile(file);
    if (path == nullptr) {
        std::fprintf(stderr, "%s: crash report could not be written: %s\n", programName, reason);
        return;
    }
    std::fprintf(file, "%s crash report\nVersion: %s %s\nStarted: %s\nExecutable: %s\n", programTitle, PORT_NAME,
                 portVersion(), reportTime, executablePath);
    writeCrashTime(file);
    writeReason(file, reason);
#ifdef _WIN32
    std::fprintf(file, "Stack addresses:\n");
    void *frames[64];
    USHORT count = CaptureStackBackTrace(0, 64, frames, nullptr);
    for (USHORT at = 0; at < count; at++) {
        std::fprintf(file, "%p\n", frames[at]);
    }
#else
    std::fprintf(file, "Stack:\n");
    std::fflush(file);
    void *frames[64];
    const int count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, fileno(file));
#endif
    std::fclose(file);
    writtenPath = path;
    std::fprintf(stderr, "%s: crash report: %s\n", programName, path);
#ifdef _WIN32
    miniDump(path, nullptr);
#endif
}

const char *crashReportPath()
{
    return writtenPath;
}

void fatalError(int status, const char *format, ...)
{
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    std::fflush(stdout);
    std::fprintf(stderr, "%s: %s\n", programName, message);
    crashReport(message);
    std::exit(status);
}
