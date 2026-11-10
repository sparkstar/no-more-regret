/* A run of the other game (restart.h). */
#include "restart.h"
#include "gamefolder.h"
#include "profile.h"
#include "report.h"

#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void restartAs(const std::string &game, char *const *argv)
{
    const char *asked = std::getenv("CRUSADER_GAME");
    if (asked != nullptr && game == asked) {
        /* This run was told the game already and plays another: it plays
         * one game only. */
        portRefuse("%s plays %s only, not %s", argv[0], PROFILE_NAME, game.c_str());
    }
    /* The command line's paths are in the directory it started in. */
    leaveGameFolder();
#ifdef _WIN32
    _putenv_s("CRUSADER_GAME", game.c_str());
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    STARTUPINFOW startup = {};
    startup.cb = sizeof startup;
    PROCESS_INFORMATION process = {};
    if (CreateProcessW(path, GetCommandLineW(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup, &process) == 0) {
        portEnd(3, "%s cannot start again for %s", argv[0], game.c_str());
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(process.hProcess, &code);
    std::exit(static_cast<int>(code));
#else
    setenv("CRUSADER_GAME", game.c_str(), 1);
#if defined(__APPLE__)
    /* macOS has no /proc: the program's file by its path. */
    const std::string program = programFile().string();
#else
    const std::string program = "/proc/self/exe";
#endif
    execv(program.c_str(), argv);
    portEnd(3, "%s cannot start again for %s: %s", argv[0], game.c_str(), std::strerror(errno));
#endif
}
