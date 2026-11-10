/* The script's input events (script.h), in the order they come. */
#include "script.h"
#include "passdump.h"
#include "gamememory.h"
#include "host.h"
#include "mousedrv.h"
#include "pc.h"
#include "report.h"
#include "video.h"
#include "profile.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

std::deque<InputEvent> events;
/* The loads and saves that have come, for the pass's end. */
std::vector<InputEvent> actions;

[[noreturn]] void mistake(const std::string &path, int line, const std::string &what)
{
    portRefuse("%s line %d: %s", path.c_str(), line, what.c_str());
}

/* The devices' names in a script (script.h), in the order of input.h's
 * numbers. */
const char *const PAD_BUTTONS[DEVICE_PAD_BUTTONS] = {
    "a",  "b",    "x",    "y",     "back", "guide",   "start",   "ls",      "rs",      "lb",      "rb",
    "up", "down", "left", "right", "misc", "paddle1", "paddle2", "paddle3", "paddle4", "touchpad"};
const char *const PAD_AXES[DEVICE_AXES] = {"leftx", "lefty", "rightx", "righty", "lt", "rt"};
const char *const MOUSE_BUTTONS[3] = {"middle", "x1", "x2"};

/* The index of name in names, or -1. */
int named(const std::string &name, const char *const *names, int count)
{
    for (int i = 0; i < count; i++) {
        if (name == names[i]) {
            return i;
        }
    }
    return -1;
}

/* A device's event of a script's line after its verb. */
bool readDevice(const std::string &verb, std::istringstream &line, DeviceEvent *device)
{
    std::string name;
    int value = 0;
    if (verb == "padgone") {
        *device = {DeviceKind::PAD_GONE, 0, 0};
        return true;
    }
    if (verb == "wheel") {
        if (!(line >> std::dec >> value) || value < -32768 || value > 32767 || (value == 0)) {
            return false;
        }
        *device = {DeviceKind::WHEEL, 0, static_cast<i16>(value)};
        return true;
    }
    if (!(line >> name >> std::dec >> value)) {
        return false;
    }
    if (verb == "axis") {
        const int axis = named(name, PAD_AXES, DEVICE_AXES);
        if (axis < 0 || value < -32768 || value > 32767) {
            return false;
        }
        *device = {DeviceKind::PAD_AXIS, static_cast<u8>(axis), static_cast<i16>(value)};
        return true;
    }
    if (value != 0 && value != 1) {
        return false;
    }
    if (verb == "pad") {
        const int button = named(name, PAD_BUTTONS, DEVICE_PAD_BUTTONS);
        if (button < 0) {
            return false;
        }
        *device = {DeviceKind::PAD_BUTTON, static_cast<u8>(button), static_cast<i16>(value)};
        return true;
    }
    const int button = named(name, MOUSE_BUTTONS, 3);
    if (button < 0) {
        return false;
    }
    *device = {DeviceKind::MOUSE_BUTTON, static_cast<u8>(button), static_cast<i16>(value)};
    return true;
}

/* REGRET_SCRIPT's script, once. */
void readScriptOnce()
{
    static bool read = false;
    if (read) {
        return;
    }
    read = true;
    if (const char *path = std::getenv("REGRET_SCRIPT")) {
        readScript(path);
    }
}

}

void addInput(const InputEvent &event)
{
    auto at = std::upper_bound(events.begin(), events.end(), event.time,
                               [](std::uint64_t time, const InputEvent &e) { return time < e.time; });
    events.insert(at, event);
}

void readScript(const std::string &path)
{
    std::ifstream in(path);
    if (!in) {
        mistake(path, 0, "cannot be read");
    }
    std::string text;
    for (int number = 1; std::getline(in, text); number++) {
        text = text.substr(0, text.find('#'));
        std::istringstream line(text);
        std::uint64_t ticks;
        std::string verb;
        if (!(line >> ticks)) {
            continue;
        }
        if (!(line >> verb)) {
            mistake(path, number, "no event");
        }
        InputEvent event{ticks * PIT_HZ / 60, InputEvent::SCAN, 0, 0, 0, 0, {}, 0, {}};
        if (verb == "key" || verb == "release") {
            unsigned scan;
            int count = 0;
            while (line >> std::hex >> scan) {
                if (scan > 0xff) {
                    mistake(path, number, "a scan code is a byte");
                }
                event.scan = static_cast<u8>(verb == "release" ? scan | 0x80 : scan);
                addInput(event);
                count++;
            }
            if (count == 0) {
                mistake(path, number, "no scan code");
            }
            continue;
        }
        if (verb == "mouse") {
            event.kind = InputEvent::MOUSE;
            if (!(line >> std::dec >> event.x >> event.y)) {
                mistake(path, number, "mouse x y [buttons]");
            }
            line >> event.buttons;
        } else if (verb == "screen") {
            event.kind = InputEvent::SCREEN;
            if (!(line >> event.path)) {
                mistake(path, number, "screen file.png");
            }
        } else if (verb == "pad" || verb == "axis" || verb == "padgone" || verb == "mousebutton" || verb == "wheel") {
            event.kind = InputEvent::DEVICE;
            if (!readDevice(verb, line, &event.device)) {
                mistake(path, number,
                        verb + ": pad <button> <1|0>, axis <axis> <value>, padgone, "
                               "mousebutton <middle|x1|x2> <1|0>, wheel <notches>");
            }
        } else if (verb == "quit") {
            event.kind = InputEvent::QUIT;
        } else if (verb == "load" || verb == "save") {
            event.kind = verb == "load" ? InputEvent::LOAD : InputEvent::SAVE;
            if (!(line >> std::dec >> event.slot) || event.slot < 0 || event.slot > 12) {
                mistake(path, number, verb + " <slot>, 0 to 12");
            }
        } else if (verb == "use") {
            event.kind = InputEvent::USE;
            if (!(line >> std::dec >> event.slot) || event.slot <= 0) {
                mistake(path, number, "use <item>");
            }
        } else if (verb == "jump") {
            event.kind = InputEvent::JUMP;
            if (!(line >> std::dec >> event.slot) || event.slot < 0) {
                mistake(path, number, "jump <map>");
            }
        } else if (verb == "movie") {
            event.kind = InputEvent::MOVIE;
            if (!(line >> event.path)) {
                mistake(path, number, "movie <name> [<form> [<x> <y>]]");
            }
            event.slot = 0;
            if (line >> std::dec >> event.slot) {
                line >> event.x >> event.y;
            }
        } else {
            mistake(path, number, "unknown event " + verb);
        }
        addInput(event);
    }
}

std::vector<std::string> deviceWords(const DeviceEvent &device)
{
    switch (device.kind) {
    case DeviceKind::MOUSE_BUTTON:
        return {"mousebutton", MOUSE_BUTTONS[device.which % 3], std::to_string(device.value)};
    case DeviceKind::WHEEL:
        return {"wheel", std::to_string(device.value)};
    case DeviceKind::PAD_BUTTON:
        return {"pad", PAD_BUTTONS[device.which % DEVICE_PAD_BUTTONS], std::to_string(device.value)};
    case DeviceKind::PAD_AXIS:
        return {"axis", PAD_AXES[device.which % DEVICE_AXES], std::to_string(device.value)};
    case DeviceKind::PAD_GONE:
        break;
    }
    return {"padgone"};
}

bool readDeviceWords(const std::string &verb, const std::string &rest, DeviceEvent *device)
{
    if (verb != "pad" && verb != "axis" && verb != "padgone" && verb != "mousebutton" && verb != "wheel") {
        return false;
    }
    std::istringstream line(rest);
    if (!readDevice(verb, line, device)) {
        return false;
    }
    std::string more;
    return !(line >> more);
}

std::uint64_t nextInputTime()
{
    readScriptOnce();
    return std::min(events.empty() ? ~static_cast<std::uint64_t>(0) : events.front().time, keyboardRepeatTime());
}

void takeInput(std::uint64_t time)
{
    readScriptOnce();
    while (!events.empty() && events.front().time <= time) {
        const InputEvent event = events.front();
        events.pop_front();
        switch (event.kind) {
        case InputEvent::SCAN:
            keyboardScan(event.scan);
            break;
        case InputEvent::MOUSE:
            mouseInput(event.x, event.y, event.buttons);
            break;
        case InputEvent::DEVICE:
            keyboardDevice(event.device);
            break;
        case InputEvent::SCREEN:
            if (!writeScreen(event.path)) {
                std::fflush(stdout);
                std::fprintf(stderr, "%s: no screen to write to %s\n", PROFILE_NAME, event.path.c_str());
            }
            break;
        case InputEvent::QUIT:
            hostEnd(0);
        case InputEvent::LOAD:
        case InputEvent::SAVE:
        case InputEvent::USE:
        case InputEvent::JUMP:
        case InputEvent::MOVIE:
            actions.push_back(event);
            break;
        }
    }
    keyboardRepeat(time);
}

namespace {
/* The game's functions (host.h), which savegame.cpp gives. */
void (*scriptLoad)(i16 slot) = nullptr;
void (*scriptSave)(i16 slot) = nullptr;
bool (*scriptReady)() = nullptr;
void (*scriptUse)(i16 item) = nullptr;
void (*scriptJump)(i16 map) = nullptr;
void (*scriptMovie)(const char *name, i16 form, i16 x, i16 y) = nullptr;

/* The game's function for an action of the script, nullptr when it gives
 * none. */
void (*actionFunction(InputEvent::Kind kind))(i16)
{
    switch (kind) {
    case InputEvent::LOAD:
        return scriptLoad;
    case InputEvent::SAVE:
        return scriptSave;
    case InputEvent::USE:
        return scriptUse;
    default:
        return scriptJump;
    }
}

const char *actionName(InputEvent::Kind kind)
{
    switch (kind) {
    case InputEvent::LOAD:
        return "load";
    case InputEvent::SAVE:
        return "save";
    case InputEvent::USE:
        return "use";
    default:
        return "jump";
    }
}
}

void hostScriptActions(void (*load)(i16 slot), void (*save)(i16 slot), void (*use)(i16 item))
{
    const SystemSide systemSide;
    scriptLoad = load;
    scriptSave = save;
    scriptUse = use;
}

void hostScriptReady(bool (*ready)())
{
    const SystemSide systemSide;
    scriptReady = ready;
}

void hostScriptJump(void (*jump)(i16 map))
{
    const SystemSide systemSide;
    scriptJump = jump;
}

void hostScriptMovie(void (*play)(const char *name, i16 form, i16 x, i16 y))
{
    const SystemSide systemSide;
    scriptMovie = play;
}

void runScriptActions()
{
    /* The actions of this pass, each taken out before the game runs it, as
     * the game may let frames pass in it; the movie's name in a buffer of
     * its own. */
    for (std::size_t count = actions.size(); count > 0 && !actions.empty(); count--) {
        const InputEvent::Kind kind = actions.front().kind;
        /* A load or a save waits for a pass's end outside the game's calls. */
        if ((kind == InputEvent::LOAD || kind == InputEvent::SAVE) && (scriptReady != nullptr)) {
            const GameSide gameSide;
            if (!scriptReady()) {
                return;
            }
        }
        const i16 slot = actions.front().slot;
        const i16 x = actions.front().x;
        const i16 y = actions.front().y;
        char name[64] = {};
        actions.front().path.copy(name, sizeof name - 1);
        if (kind == InputEvent::MOVIE) {
            beginMovieFrames(actions.front().path, slot, x, y);
        }
        actions.erase(actions.begin());
        if (kind == InputEvent::MOVIE) {
            if (scriptMovie == nullptr) {
                portRefuse("the script's movie without the game");
            }
            const GameSide gameSide;
            scriptMovie(name, slot, x, y);
            continue;
        }
        void (*function)(i16) = actionFunction(kind);
        if (function == nullptr) {
            portRefuse("the script's %s without the game", actionName(kind));
        }
        /* The pass dump's record of it: a load, a save or another. */
        char tag = 'X';
        if (kind == InputEvent::LOAD) {
            tag = 'L';
        } else if (kind == InputEvent::SAVE) {
            tag = 'V';
        }
        passDumpAction(tag, slot);
        const GameSide gameSide;
        function(slot);
    }
}
