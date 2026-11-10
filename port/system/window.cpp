/* The port's window with SDL3 (window.h). A run without a script has one.
 * The window is the largest whole multiple of 640x480 the desktop holds.
 * Closing it does what the game gave host.h's hostCloseAction (the run's end
 * at once, system.cpp); so do the signals SDL turns into the same event
 * (SIGINT, SIGTERM).
 *
 * The run's two threads (window.h): the program's first, the window thread,
 * keeps SDL's events, the drawing and the showing, which the host may hold
 * (a covered window's showing, the window moved); the game thread runs the
 * game, its virtual time and its sound, and alone touches the game's state.
 * They hand each other two things under locks: the window's input, which the
 * game takes at its next frame, and a copy of the screen at each frame,
 * which the window shows when it can (the last one; one it had no time for
 * is not shown). The game thread wakes the window thread with an event of
 * SDL's, and asks it the calls of SDL's that only the program's first thread
 * may make (windowCall): the window's opening, a message box, the audio's
 * start, the end. The window thread takes a call after the events before it,
 * in their order. The run ends on the game thread (the game's own end): the
 * exit's handler has the window thread close SDL and stop, and the game
 * thread goes on with the exit.
 *
 * Each frame draws the screen the mode shows (video.h) in the DAC's
 * colours, on a 4:3 monitor as the original's: VESA's 640x480 as it is,
 * the 320x200 of mode 13h stretched to 320x240. The picture is the largest
 * whole multiple of that which the window holds, in its middle, black
 * around it; a window smaller than one keeps the 4:3 shape. The 200 rows
 * of mode 13h come to a height that is no whole multiple of them, which
 * SDL's pixel-art scaling keeps even. In a text mode the window is black:
 * the text goes to the standard output (console.cpp).
 *
 * The keys go to the keyboard (pc.h's keyboardScan) as the scan codes of
 * set 1 a PC keyboard sends, E0h before the extended ones, at the time of
 * the game's next frame: a key down its make code, up its break code
 * (| 80h). SDL's own repeats are dropped: the keyboard repeats
 * (keyboard.cpp). Pause sends its six bytes going down and nothing going up
 * (with Control, Break's E0 46 E0 C6); Print Screen its four bytes, or
 * E0 37 with Shift or Control and 54h (SysRq) with Alt. The keyboard's
 * other shift codes around the extended keys (E0 2A with Num Lock or
 * Shift), which the game drops, are not sent.
 *
 * The mouse is the window's while it is captured: SDL's relative mode
 * hides the host's cursor (the game draws its own) and keeps the mouse in
 * the window. Its motion goes to the driver in mickeys, a unit of SDL's
 * relative motion a mickey, which the driver moves the cursor by at the
 * game's speed (mousedrv.h); the motion of a frame is one event of the
 * mouse, and a button going down or up one more with the motion before it.
 * The window captures the mouse when it opens; Control+F10 frees it, and a
 * click in the window captures it again, a click the game does not get.
 *
 * The other devices' events go to the keyboard as they are (input.h's
 * DeviceEvent, pc.h's keyboardDevice): the mouse's middle and side buttons
 * and its wheel in whole notches while it is captured (the middle button
 * also to the driver), and the gamepad's buttons and axes. The window thread
 * keeps the gamepads: it opens the first one there, and when that one goes,
 * the next; the axis's motion of a frame is its last place.
 *
 * The sound's output bytes go to an SDL audio stream of the output's format,
 * from the game thread. The output's units are samples, not frames: the
 * bytes of a time can end inside a frame, and SDL drops a put of part of
 * one, so the part waits for the rest. The samples come as the virtual time
 * passes, a sixtieth of a second at once when a frame has waited for the
 * wall clock, and later by what the host's sleep oversleeps; the device
 * takes a buffer at its own times and plays silence when the stream has not
 * got it. So the stream starts with a lead of silence, the device's buffer
 * and two sixtieths, and keeps near it: each frame sets the speed the device
 * plays at, a little faster while the stream holds more (the device's clock
 * is slower than the wall clock), a little slower while it holds less. A
 * host that stood still gets the device's silence meanwhile and the samples
 * of the time at once when it goes on, as the picture skips it: what comes
 * beyond twice the lead is dropped, and the sound keeps with the picture. */
#include "window.h"
#include "gamememory.h"
#include "crash.h"
#include "mousedrv.h"
#include "pc.h"
#include "pcstate.h"
#include "replay.h"
#include "report.h"
#include "host.h"
#include "input.h"
#include "video.h"
#include "portversion.h"
#include "profile.h"
#include "screen.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

/* The run's threads: none yet (no window, or before and after the game
 * thread), the window thread keeping the window while the game thread
 * runs, or the window thread stopped at the run's end, SDL closed. */
enum class Loop { NONE, RUNNING, PARKED };
std::atomic<Loop> loop{Loop::NONE};
std::thread::id windowThread;
/* The event of SDL's the game thread wakes the window thread with: a frame
 * to show, a call to make (windowCall), the game's end. */
Uint32 wakeEvent = 0;
enum WakeCode : Sint32 { WAKE_FRAME, WAKE_CALL, WAKE_END };

/* A call the window thread makes for the game thread, which waits for it
 * to be done. */
struct WindowThreadCall {
    const std::function<void()> *function;
    bool done;
};
std::mutex callLock;
std::condition_variable callDone;

/* A picture of the screen, which the game thread hands the window thread:
 * the mode's view (video.h) over a copy of its pixels; no picture in a text
 * mode. */
/* What the window shows over the frame's top left: the run's state, the
 * first of the rewind, the pause, the fast forward, a watched replay and
 * the recorder, and the frame the run is at. */
struct Status {
    enum Kind : u8 { NONE, RECORD, REPLAY, FAST_FORWARD, PAUSE, REWIND } kind = NONE;
    std::uint64_t frame = 0;
};
struct Frame {
    bool picture = false;
    ScreenView view{};
    std::vector<u8> pixels;
    Status status;
};
/* The last picture the game thread has handed, not yet shown while fresh,
 * and whether its wake is on the way. */
std::mutex frameLock;
Frame handed;
bool fresh = false;
std::atomic<bool> frameWoken{false};

/* The input the window thread hands the game thread for its next frame, in
 * the order of the window's events: a scan code to the keyboard, the
 * mouse's relative motion, its buttons now (bits 0-2: left, right, middle),
 * the mouse captured or freed (the motion not yet sent is dropped), the
 * window closed, an event of the other devices. */
struct Input {
    enum Kind { SCAN, MOTION, BUTTONS, CAPTURE, CLOSE, DEVICE } kind;
    u8 scan = 0;
    float x = 0;
    float y = 0;
    u16 buttons = 0;
    DeviceEvent device{};
};
std::mutex inputLock;
std::vector<Input> inputs;
/* The keys the window has given the game down and not up (the window
 * thread's, under inputLock), and the keys the game thread has taken so
 * (takeInput's), with the E0h before the next scan code taken. */
KeysHeld keysGiven;
KeysHeld keysTaken;
bool takenAfterE0 = false;

/* The host actions' keys (platform/input.h), which the game thread gives
 * and the window thread reads; the rewind's key held, the state asked for,
 * and the keys whose going up the game does not get (host actions'). */
std::mutex hostLock;
std::vector<HostKey> hostKeys[HOST_ACTIONS];
std::atomic<bool> rewindHeld{false};
std::atomic<bool> fastHeld{false};
std::atomic<bool> paused{false};
std::atomic<int> stateAsked{0};
std::atomic<bool> takeOverAsked{false};
/* The window over the whole screen, as the window thread last made or
 * found it (its host's events). */
std::atomic<bool> fullscreenNow{false};
/* What holds a host action's input down: a key by its SDL scan code, or a
 * device's button from DEVICE_SOURCE on (the mouse's at it, the
 * gamepad's 32 on); the rewind's and the fast forward's, while held. */
constexpr int NO_SOURCE = -1;
constexpr int DEVICE_SOURCE = SDL_SCANCODE_COUNT;
constexpr int PAD_SOURCE = DEVICE_SOURCE + 32;
int rewindSource = NO_SOURCE;
int fastSource = NO_SOURCE;
bool hostDown[PAD_SOURCE + 32] = {};

/* Whether the wide screen was on (video.h's wideModeNow) when the window
 * last took a shape for it. */
bool wideShaped = false;

/* The window thread's: the window, its renderer, the screen in the
 * window's colours, the size of the mode's, the picture it shows; the
 * mouse captured or free, its buttons down, and the key or button that
 * captured or freed it, whose going up the game does not get either; the
 * window closed, and when. */
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_Texture *texture = nullptr;
std::vector<Uint32> colours;
Frame shown;
bool captured = false;
/* What the title says after the game's name (windowTitleNote). */
std::string titleNote;
u16 buttons = 0;
SDL_Scancode keptKey = SDL_SCANCODE_UNKNOWN;
Uint8 keptButton = 0;
/* The mouse's middle and side buttons down as the game has them (bits of
 * DEVICE_MOUSE_*). */
u8 deviceButtons = 0;
bool closing = false;
std::chrono::steady_clock::time_point closedAt;
/* The gamepad the window thread has open, the first there. */
SDL_Gamepad *gamepad = nullptr;
SDL_JoystickID gamepadId = 0;
/* A game thread that comes to no frame in this time after the window is
 * closed (one that hangs) does not hold the run: it ends at once. */
const std::chrono::seconds CLOSE_WAIT{3};

/* The game thread's: the window opened, the mouse's motion not yet sent and
 * its buttons, the window closed, the picture being made and the input
 * being taken. */
bool opened = false;
float motionX = 0, motionY = 0;
u16 mouseButtons = 0;
bool closeAsked = false;
Frame building;
std::vector<Input> taken;
/* The game's end, which closing the window calls (host.h). */
void (*closeAction)() = nullptr;
/* The full screen as the game has it (host.h), and what a change of the
 * window's does. */
bool fullscreen = false;
void (*fullscreenAction)() = nullptr;

[[noreturn]] void failure(const char *what)
{
    portEnd(3, "%s: %s", what, SDL_GetError());
}

/* Wakes the window thread with an event of SDL's; a queue that is full (a
 * window thread its host holds) takes it when it has room again. */
void wake(Sint32 code, void *data)
{
    SDL_Event event{};
    event.type = wakeEvent;
    event.user.code = code;
    event.user.data1 = data;
    while (!SDL_PushEvent(&event)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

/* The window thread's turn to the game thread's input. */
void give(const Input &input)
{
    const std::lock_guard<std::mutex> hold(inputLock);
    inputs.push_back(input);
}

/* The window thread's: an event of the other devices; an axis's place
 * takes the place of the one before it when nothing came between. */
void giveDevice(const DeviceEvent &device)
{
    const std::lock_guard<std::mutex> hold(inputLock);
    if (device.kind == DeviceKind::PAD_AXIS && !inputs.empty() && inputs.back().kind == Input::DEVICE &&
        inputs.back().device.kind == DeviceKind::PAD_AXIS && inputs.back().device.which == device.which) {
        inputs.back().device.value = device.value;
        return;
    }
    inputs.push_back({.kind = Input::DEVICE, .device = device});
}

/* SDL closed, on the window thread. */
void closeWindow()
{
    if (gamepad != nullptr) {
        SDL_CloseGamepad(gamepad);
    }
    gamepad = nullptr;
    gamepadId = 0;
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }
    texture = nullptr;
    renderer = nullptr;
    window = nullptr;
    SDL_Quit();
}

/* The run's end (std::atexit), on the thread that ends it: SDL closed on the
 * window thread. Ended on the game thread, the window thread then stops
 * (Loop::PARKED) and the game thread goes on with the exit. The first of
 * its registrations does it: windowMain's, or the window's, which comes
 * after the handlers its graphics library set up with the renderer and so
 * runs before them. */
bool windowEnded = false;
void endWindow()
{
    if (windowEnded) {
        return;
    }
    windowEnded = true;
    if (loop == Loop::RUNNING && std::this_thread::get_id() != windowThread) {
        windowCall([] {
            closeWindow();
            loop = Loop::PARKED;
        });
        return;
    }
    closeWindow();
}

/* The window thread's: the first gamepad there, when none is open. */
void openGamepad()
{
    if (gamepad != nullptr) {
        return;
    }
    int count = 0;
    SDL_JoystickID *pads = SDL_GetGamepads(&count);
    for (int i = 0; (pads != nullptr) && i < count && (gamepad == nullptr); i++) {
        gamepad = SDL_OpenGamepad(pads[i]);
        gamepadId = (gamepad != nullptr) ? pads[i] : 0;
    }
    SDL_free(pads);
}

/* The window thread's: the gamepad open taken away (its buttons and axes
 * let go), and the next one there opened. */
void gamepadGone()
{
    SDL_CloseGamepad(gamepad);
    gamepad = nullptr;
    gamepadId = 0;
    giveDevice({DeviceKind::PAD_GONE, 0, 0});
    openGamepad();
}

void capture(bool on);
void showTitle();

/* The window thread's: the wide screen's width the shape of the window's
 * picture, width x height pixels (over the whole screen, its display's),
 * for 480 rows. */
void followPicture(int width, int height)
{
    if (width > 0 && height > 0) {
        setWideWidth(static_cast<u16>(
            std::min(std::lround(static_cast<double>(SCREEN_HEIGHT) * width / height), static_cast<long>(WIDE_MOST))));
    }
}

/* The wide screen's width for 480 rows of a display mode's shape. */
int wideWidthOf(const SDL_DisplayMode *mode)
{
    if ((mode == nullptr) || mode->h <= 0) {
        return 0;
    }
    return std::clamp(static_cast<int>(std::lround(static_cast<double>(SCREEN_HEIGHT) * mode->w / mode->h)),
                      SCREEN_WIDTH, static_cast<int>(WIDE_MOST));
}

/* The window thread's: the game's icon on the window, from GOG's
 * goggame-<id>.ico in the game's folder (the current directory,
 * gamefolder.h), its largest PNG picture; none without one (a folder made
 * for the tests). */
void showIcon()
{
    std::error_code error;
    for (std::filesystem::directory_iterator entry(".", error), end; !error && entry != end; entry.increment(error)) {
        std::u8string name = entry->path().filename().u8string();
        std::transform(name.begin(), name.end(), name.begin(),
                       [](char8_t c) { return c >= u8'A' && c <= u8'Z' ? static_cast<char8_t>(c + 32) : c; });
        if (!name.starts_with(u8"goggame-") || !name.ends_with(u8".ico")) {
            continue;
        }
        std::ifstream file(entry->path(), std::ios::binary);
        const std::vector<u8> ico((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        const std::optional<IconPicture> picture = largestIconPng(ico);
        if (picture) {
            SDL_Surface *icon = SDL_LoadPNG_IO(SDL_IOFromConstMem(&ico[picture->offset], picture->size), true);
            if (icon != nullptr) {
                SDL_SetWindowIcon(window, icon);
                SDL_DestroySurface(icon);
            }
        }
        return;
    }
}

/* The window, on the window thread, the mouse captured. */
void openWindow()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        failure("no window");
    }
    /* With the wide screen on, the window opens in the shape of the
     * display it opens on (the primary: there is no window yet), for 480
     * rows; its picture's shape sets the width as it goes
     * (followPicture). */
    wideShaped = wideModeNow() != 0U;
    int frameWidth = SCREEN_WIDTH;
    const int displayWidth = wideWidthOf(SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay()));
    if (wideShaped && displayWidth != 0) {
        frameWidth = displayWidth;
    }
    int scale = 1;
    SDL_Rect desktop;
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &desktop)) {
        while (frameWidth * (scale + 1) <= desktop.w && SCREEN_HEIGHT * (scale + 1) <= desktop.h) {
            scale++;
        }
    }
    /* Over the whole screen when the game has it so: the desktop's mode,
     * not one of the game's own. */
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    if (!SDL_CreateWindowAndRenderer(PROFILE_TITLE, frameWidth * scale, SCREEN_HEIGHT * scale, flags, &window,
                                     &renderer)) {
        failure("no window");
    }
    int pixelWidth = 0;
    int pixelHeight = 0;
    if (SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight)) {
        followPicture(pixelWidth, pixelHeight);
    }
    showIcon();
    fullscreenNow = fullscreen;
    std::atexit(endWindow);
    capture(true);
    /* Without the gamepads the window has none; those there come as SDL's
     * events of their adding. */
    SDL_InitSubSystem(SDL_INIT_GAMEPAD);
}

/* The window thread's: the mouse captured or freed. */
void capture(bool on)
{
    /* The buttons the game has down go up when the mouse is freed: their
     * going up comes to the window, not the game. */
    for (u8 which = 0; !on && which < 3; which++) {
        if ((deviceButtons & 1 << which) != 0) {
            giveDevice({DeviceKind::MOUSE_BUTTON, which, 0});
        }
    }
    if (!on) {
        deviceButtons = 0;
    }
    captured = on;
    give({.kind = Input::CAPTURE});
    SDL_SetWindowRelativeMouseMode(window, on);
    showTitle();
}

/* The title: the game's name, the port's name and version, the note and what
 * the mouse does. */
void showTitle()
{
    const std::string title = std::string(PROFILE_TITLE) + " - " + PORT_NAME + " " + portVersion() + titleNote +
                              (captured ? " - Ctrl+F10 frees the mouse" : " - click to capture the mouse");
    SDL_SetWindowTitle(window, title.c_str());
}

/* The game thread's: the motion so far, the whole mickeys of it, and the
 * buttons as an event of the mouse. */
void sendMouse()
{
    const i16 x = static_cast<i16>(motionX);
    const i16 y = static_cast<i16>(motionY);
    motionX -= x;
    motionY -= y;
    recordMotion(x, y, mouseButtons);
    mouseMotion(x, y, mouseButtons);
}

/* hostKey's for a device's button (below). */
bool hostButton(const DeviceEvent &device);

void buttonEvent(const SDL_MouseButtonEvent &event)
{
    if (!captured || event.button == keptButton) {
        if (event.down && !captured) {
            capture(true);
            keptButton = event.button;
        } else if (!event.down && event.button == keptButton) {
            keptButton = 0;
        }
        return;
    }
    if (event.button == SDL_BUTTON_MIDDLE || event.button == SDL_BUTTON_X1 || event.button == SDL_BUTTON_X2) {
        u8 which = DEVICE_MOUSE_X2;
        if (event.button == SDL_BUTTON_MIDDLE) {
            which = DEVICE_MOUSE_MIDDLE;
        } else if (event.button == SDL_BUTTON_X1) {
            which = DEVICE_MOUSE_X1;
        }
        const DeviceEvent button{DeviceKind::MOUSE_BUTTON, which, static_cast<i16>(event.down ? 1 : 0)};
        if (!hostButton(button)) {
            giveDevice(button);
            deviceButtons = event.down ? static_cast<u8>(deviceButtons | 1 << which)
                                       : static_cast<u8>(deviceButtons & ~(1 << which));
        }
    }
    u16 bit = 0;
    if (event.button == SDL_BUTTON_LEFT) {
        bit = 1;
    } else if (event.button == SDL_BUTTON_RIGHT) {
        bit = 2;
    } else if (event.button == SDL_BUTTON_MIDDLE) {
        bit = 4;
    }
    if (bit == 0U) {
        return;
    }
    buttons = event.down ? static_cast<u16>(buttons | bit) : static_cast<u16>(buttons & ~bit);
    give({.kind = Input::BUTTONS, .buttons = buttons});
}

/* The wheel's whole notches, up away from the user. */
void wheelEvent(const SDL_MouseWheelEvent &event)
{
    const Sint32 notches = event.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.integer_y : event.integer_y;
    if (notches != 0) {
        giveDevice({DeviceKind::WHEEL, 0,
                    static_cast<i16>(std::clamp(notches, static_cast<Sint32>(-32768), static_cast<Sint32>(32767)))});
    }
}

/* A key's scan code in set 1, EXTENDED for E0h before it; 0 for a key
 * the PC's keyboard has not. SDL's codes are the USB keyboard's. */
const u16 EXTENDED = 0x100;

u16 setOne(SDL_Scancode key)
{
    if (key >= SDL_SCANCODE_A && key <= SDL_SCANCODE_Z) {
        static const u8 letters[26] = {0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
                                       0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c};
        return letters[key - SDL_SCANCODE_A];
    }
    if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_0) {
        return static_cast<u16>(0x02 + (key - SDL_SCANCODE_1));
    }
    if (key >= SDL_SCANCODE_F1 && key <= SDL_SCANCODE_F10) {
        return static_cast<u16>(0x3b + (key - SDL_SCANCODE_F1));
    }
    switch (key) {
    case SDL_SCANCODE_RETURN:
        return 0x1c;
    case SDL_SCANCODE_ESCAPE:
        return 0x01;
    case SDL_SCANCODE_BACKSPACE:
        return 0x0e;
    case SDL_SCANCODE_TAB:
        return 0x0f;
    case SDL_SCANCODE_SPACE:
        return 0x39;
    case SDL_SCANCODE_MINUS:
        return 0x0c;
    case SDL_SCANCODE_EQUALS:
        return 0x0d;
    case SDL_SCANCODE_LEFTBRACKET:
        return 0x1a;
    case SDL_SCANCODE_RIGHTBRACKET:
        return 0x1b;
    case SDL_SCANCODE_BACKSLASH:
        return 0x2b;
    case SDL_SCANCODE_NONUSHASH:
        return 0x2b;
    case SDL_SCANCODE_SEMICOLON:
        return 0x27;
    case SDL_SCANCODE_APOSTROPHE:
        return 0x28;
    case SDL_SCANCODE_GRAVE:
        return 0x29;
    case SDL_SCANCODE_COMMA:
        return 0x33;
    case SDL_SCANCODE_PERIOD:
        return 0x34;
    case SDL_SCANCODE_SLASH:
        return 0x35;
    case SDL_SCANCODE_CAPSLOCK:
        return 0x3a;
    case SDL_SCANCODE_F11:
        return 0x57;
    case SDL_SCANCODE_F12:
        return 0x58;
    case SDL_SCANCODE_SCROLLLOCK:
        return 0x46;
    case SDL_SCANCODE_INSERT:
        return EXTENDED | 0x52;
    case SDL_SCANCODE_HOME:
        return EXTENDED | 0x47;
    case SDL_SCANCODE_PAGEUP:
        return EXTENDED | 0x49;
    case SDL_SCANCODE_DELETE:
        return EXTENDED | 0x53;
    case SDL_SCANCODE_END:
        return EXTENDED | 0x4f;
    case SDL_SCANCODE_PAGEDOWN:
        return EXTENDED | 0x51;
    case SDL_SCANCODE_RIGHT:
        return EXTENDED | 0x4d;
    case SDL_SCANCODE_LEFT:
        return EXTENDED | 0x4b;
    case SDL_SCANCODE_DOWN:
        return EXTENDED | 0x50;
    case SDL_SCANCODE_UP:
        return EXTENDED | 0x48;
    case SDL_SCANCODE_NUMLOCKCLEAR:
        return 0x45;
    case SDL_SCANCODE_KP_DIVIDE:
        return EXTENDED | 0x35;
    case SDL_SCANCODE_KP_MULTIPLY:
        return 0x37;
    case SDL_SCANCODE_KP_MINUS:
        return 0x4a;
    case SDL_SCANCODE_KP_PLUS:
        return 0x4e;
    case SDL_SCANCODE_KP_ENTER:
        return EXTENDED | 0x1c;
    case SDL_SCANCODE_KP_1:
        return 0x4f;
    case SDL_SCANCODE_KP_2:
        return 0x50;
    case SDL_SCANCODE_KP_3:
        return 0x51;
    case SDL_SCANCODE_KP_4:
        return 0x4b;
    case SDL_SCANCODE_KP_5:
        return 0x4c;
    case SDL_SCANCODE_KP_6:
        return 0x4d;
    case SDL_SCANCODE_KP_7:
        return 0x47;
    case SDL_SCANCODE_KP_8:
        return 0x48;
    case SDL_SCANCODE_KP_9:
        return 0x49;
    case SDL_SCANCODE_KP_0:
        return 0x52;
    case SDL_SCANCODE_KP_PERIOD:
        return 0x53;
    case SDL_SCANCODE_NONUSBACKSLASH:
        return 0x56;
    case SDL_SCANCODE_APPLICATION:
        return EXTENDED | 0x5d;
    case SDL_SCANCODE_LCTRL:
        return 0x1d;
    case SDL_SCANCODE_LSHIFT:
        return 0x2a;
    case SDL_SCANCODE_LALT:
        return 0x38;
    case SDL_SCANCODE_LGUI:
        return EXTENDED | 0x5b;
    case SDL_SCANCODE_RCTRL:
        return EXTENDED | 0x1d;
    case SDL_SCANCODE_RSHIFT:
        return 0x36;
    case SDL_SCANCODE_RALT:
        return EXTENDED | 0x38;
    case SDL_SCANCODE_RGUI:
        return EXTENDED | 0x5c;
    default:
        return 0;
    }
}

void send(std::initializer_list<u8> bytes)
{
    for (const u8 byte : bytes) {
        give({.kind = Input::SCAN, .scan = byte});
    }
}

/* Whether the window takes the host action's inputs: the debug
 * features' only with them (host.h's hostDebugFeatures); else they go to
 * the game. */
bool hostActionTaken(HostAction action)
{
    return !hostActionIsDebug(action) || hostDebugFeatures();
}

/* A host action's input pressed (from source): the rewind and the fast
 * forward held until it goes up, the pause turned on or off, the full
 * screen, a take over or a state asked for. */
void hostActionDown(HostAction action, int source)
{
    hostDown[source] = true;
    if (action == HostAction::REWIND) {
        rewindSource = source;
        rewindHeld = true;
    } else if (action == HostAction::FAST_FORWARD) {
        fastSource = source;
        fastHeld = true;
    } else if (action == HostAction::PAUSE) {
        paused = !paused;
    } else if (action == HostAction::FULLSCREEN) {
        fullscreenNow = !fullscreenNow;
        SDL_SetWindowFullscreen(window, fullscreenNow);
    } else if (action == HostAction::TAKE_OVER) {
        takeOverAsked = true;
    } else {
        stateAsked = action == HostAction::SAVE_STATE ? 1 : 2;
    }
}

/* A host action's input let go: whether it was one. */
bool hostInputUp(int source)
{
    if (!hostDown[source]) {
        return false;
    }
    hostDown[source] = false;
    if (source == rewindSource) {
        rewindHeld = false;
        rewindSource = NO_SOURCE;
    }
    if (source == fastSource) {
        fastHeld = false;
        fastSource = NO_SOURCE;
    }
    return true;
}

/* A key of a host action (platform/input.h) taken from the game, its going
 * up too. The Pause key is 45h with E0h. */
bool hostKey(const SDL_KeyboardEvent &key, bool down)
{
    if (!down) {
        return hostInputUp(key.scancode);
    }
    const u16 code = key.scancode == SDL_SCANCODE_PAUSE ? static_cast<u16>(EXTENDED | 0x45) : setOne(key.scancode);
    if (code == 0U) {
        return false;
    }
    const u8 modifiers =
        static_cast<u8>((((key.mod & SDL_KMOD_SHIFT) != 0U) ? 1 : 0) | (((key.mod & SDL_KMOD_CTRL) != 0U) ? 2 : 0) |
                        (((key.mod & SDL_KMOD_ALT) != 0U) ? 4 : 0));
    const std::lock_guard<std::mutex> hold(hostLock);
    for (std::size_t action = 0; action < HOST_ACTIONS; action++) {
        if (!hostActionTaken(static_cast<HostAction>(action))) {
            continue;
        }
        for (const HostKey &host : hostKeys[action]) {
            if (host.device || host.scan != (code & 0x7f) || host.extended != ((code & EXTENDED) != 0) ||
                host.modifiers != modifiers) {
                continue;
            }
            hostActionDown(static_cast<HostAction>(action), key.scancode);
            return true;
        }
    }
    return false;
}

/* A device's button of a host action (the mouse's middle and side
 * buttons, the gamepad's) taken from the game, its going up too. */
bool hostButton(const DeviceEvent &device)
{
    if ((device.kind != DeviceKind::MOUSE_BUTTON && device.kind != DeviceKind::PAD_BUTTON) || device.which >= 32) {
        return false;
    }
    const int source = (device.kind == DeviceKind::MOUSE_BUTTON ? DEVICE_SOURCE : PAD_SOURCE) + device.which;
    if (device.value == 0) {
        return hostInputUp(source);
    }
    const std::lock_guard<std::mutex> hold(hostLock);
    for (std::size_t action = 0; action < HOST_ACTIONS; action++) {
        if (!hostActionTaken(static_cast<HostAction>(action))) {
            continue;
        }
        for (const HostKey &host : hostKeys[action]) {
            if (host.device && host.kind == device.kind && host.which == device.which) {
                hostActionDown(static_cast<HostAction>(action), source);
                return true;
            }
        }
    }
    return false;
}

void keyEvent(const SDL_KeyboardEvent &key)
{
    if (key.repeat) {
        return;
    }
    const bool down = key.down;
    if (down && key.scancode == SDL_SCANCODE_F10 && ((key.mod & SDL_KMOD_CTRL) != 0U) && captured) {
        if (buttons != 0U) {
            buttons = 0;
            give({.kind = Input::BUTTONS, .buttons = buttons});
        }
        capture(false);
        keptKey = key.scancode;
        return;
    }
    if (!down && key.scancode == keptKey) {
        keptKey = SDL_SCANCODE_UNKNOWN;
        return;
    }
    if (hostKey(key, down)) {
        return;
    }
    if (key.scancode == SDL_SCANCODE_PAUSE) {
        if (down && ((key.mod & SDL_KMOD_CTRL) != 0U)) {
            send({0xe0, 0x46, 0xe0, 0xc6});
        } else if (down) {
            send({0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5});
        }
        return;
    }
    if (key.scancode == SDL_SCANCODE_PRINTSCREEN) {
        if ((key.mod & SDL_KMOD_ALT) != 0U) {
            send({static_cast<u8>(down ? 0x54 : 0xd4)});
        } else if ((key.mod & (SDL_KMOD_SHIFT | SDL_KMOD_CTRL)) != 0U) {
            send({0xe0, static_cast<u8>(down ? 0x37 : 0xb7)});
        } else if (down) {
            send({0xe0, 0x2a, 0xe0, 0x37});
        } else {
            send({0xe0, 0xb7, 0xe0, 0xaa});
        }
        return;
    }
    const u16 code = setOne(key.scancode);
    if (code == 0U) {
        return;
    }
    {
        const std::lock_guard<std::mutex> hold(inputLock);
        keysGiven[(code & 0x7f) + (((code & EXTENDED) != 0) ? 128 : 0)] = down;
    }
    if ((code & EXTENDED) != 0) {
        send({0xe0});
    }
    send({static_cast<u8>(down ? code : code | 0x80)});
}

/* The game thread's: the run's state now (Status). */
Status statusNow()
{
    Status status;
    status.frame = timerState().windowFrames;
    if (rewindHeld) {
        status.kind = Status::REWIND;
    } else if (paused) {
        status.kind = Status::PAUSE;
    } else if (clockSpeed() > 1) {
        status.kind = Status::FAST_FORWARD;
    } else if (replaying() && watching()) {
        status.kind = Status::REPLAY;
    } else if (recording()) {
        status.kind = Status::RECORD;
    }
    return status;
}

/* The game thread's: the screen the mode shows into frame, its pixels
 * copied, and the run's state. */
void copyScreen(Frame &frame)
{
    frame.status = statusNow();
    ScreenView view;
    frame.picture = viewWideScreen(view);
    if (!frame.picture) {
        return;
    }
    const std::size_t size = static_cast<std::size_t>(view.pitch) * view.height;
    frame.pixels.resize(size);
    std::memcpy(frame.pixels.data(), view.pixels, size);
    frame.view = view;
    /* The copy's, where it is drawn. */
    frame.view.pixels = nullptr;
}

/* The game thread's: the screen handed to the window thread, which is woken
 * unless a wake is on its way. */
void handFrame()
{
    copyScreen(building);
    {
        const std::lock_guard<std::mutex> hold(frameLock);
        std::swap(building, handed);
        fresh = true;
    }
    if (!frameWoken.exchange(true)) {
        wake(WAKE_FRAME, nullptr);
    }
}

/* The window thread's: the last picture handed, when there is a new one;
 * whether there was. */
bool takeFrame()
{
    const std::lock_guard<std::mutex> hold(frameLock);
    if (!fresh) {
        return false;
    }
    std::swap(handed, shown);
    fresh = false;
    return true;
}

/* The screen into the texture, which follows the mode's size: the
 * palette's colours, or the 16-bit mode's own. */
bool screenTexture(const ScreenView &view)
{
    float width;
    float height;
    if ((texture == nullptr) || !SDL_GetTextureSize(texture, &width, &height) || width != view.width ||
        height != view.height) {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
        texture =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, view.width, view.height);
        if (texture == nullptr) {
            failure("no picture");
        }
    }
    colours.resize(static_cast<size_t>(view.width) * view.height);
    if (view.bits == 16) {
        for (u16 y = 0; y < view.height; y++) {
            for (u16 x = 0; x < view.width; x++) {
                colours[(static_cast<size_t>(y) * view.width) + x] = viewColour(view, x, y);
            }
        }
    } else {
        Uint32 palette[256];
        /* The wide screen's sides dimmed: each colour at half. */
        Uint32 dimmed[256];
        for (int i = 0; i < 256; i++) {
            palette[i] = static_cast<Uint32>(view.palette[i * 3]) << 16 | view.palette[(i * 3) + 1] << 8 |
                         view.palette[(i * 3) + 2];
            dimmed[i] = (palette[i] >> 1) & 0x7f7f7f;
        }
        const int left = view.dimmed ? view.sides : 0;
        const int right = view.dimmed ? view.width - view.sides : view.width;
        for (int y = 0; y < view.height; y++) {
            const u8 *row = view.pixels + (static_cast<size_t>(y) * view.pitch);
            Uint32 *out = &colours[static_cast<size_t>(y) * view.width];
            for (int x = 0; x < view.width; x++) {
                out[x] = x < left || x >= right ? dimmed[row[x]] : palette[row[x]];
            }
        }
    }
    return SDL_UpdateTexture(texture, nullptr, colours.data(), view.width * static_cast<int>(sizeof(Uint32)));
}

/* The frame's place in an output of width x height: the box of
 * frameWidth x 480 as large as it fits, in the middle (the full screen's
 * height, not a whole multiple's). */
SDL_FRect framePlace(int width, int height, int frameWidth)
{
    int w = width;
    int h = width * SCREEN_HEIGHT / frameWidth;
    if (h > height) {
        w = height * frameWidth / SCREEN_HEIGHT;
        h = height;
    }
    return {static_cast<float>((width - w) / 2), static_cast<float>((height - h) / 2), static_cast<float>(w),
            static_cast<float>(h)};
}

/* The picture's place in the frame: all of it with the wide screen's sides,
 * else 4:3 as high as the frame, in its middle. */
SDL_FRect picturePlace(const SDL_FRect &frame, const ScreenView &view)
{
    if (view.sides != 0U) {
        return frame;
    }
    const int h = static_cast<int>(frame.h);
    const int w = h * 4 / 3;
    return {frame.x + static_cast<float>((static_cast<int>(frame.w) - w) / 2), frame.y, static_cast<float>(w),
            static_cast<float>(h)};
}

/* The window thread's: a window (not over the screen, not maximized) of
 * the shape of the wide screen turned on, its display's, or of 4:3 turned
 * off, as high as it was. Only the turn: the window's own shape after it
 * is the player's, which the width follows (followPicture). */
void keepFrameShape()
{
    const bool wide = wideModeNow() != 0U;
    if (wide == wideShaped) {
        return;
    }
    wideShaped = wide;
    if ((SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED)) != 0U) {
        return;
    }
    int width = 0;
    int height = 0;
    if (!SDL_GetWindowSize(window, &width, &height) || height <= 0) {
        return;
    }
    width = height * 4 / 3;
    const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(window));
    if (wide && (mode != nullptr) && mode->h > 0) {
        width = std::min(height * mode->w / mode->h, height * static_cast<int>(WIDE_MOST) / SCREEN_HEIGHT);
    }
    SDL_SetWindowSize(window, width, height);
}

/* The window thread's: a triangle of three corners in a colour. */
void fillTriangle(SDL_FPoint a, SDL_FPoint b, SDL_FPoint c, SDL_FColor colour)
{
    const SDL_Vertex corners[3] = {{a, colour, {0, 0}}, {b, colour, {0, 0}}, {c, colour, {0, 0}}};
    SDL_RenderGeometry(renderer, nullptr, corners, 3, nullptr, 0);
}

/* The window thread's: the run's state over the frame's top left (the
 * frame's place in the window): its sign, an icon size high (the
 * recorder's red dot, the replay's play, the fast forward's two plays,
 * the pause's two bars, the rewind's two plays back), and under it the frame and its time in
 * the replay's form (replay.h's replayTimeText, which --from and --to
 * take), on a dark box; nothing in the run's plain state. */
void drawStatus(const Status &status, const SDL_FRect &frame)
{
    if (status.kind == Status::NONE) {
        return;
    }
    /* The window's pixels of a pixel of the 480 rows, the debug font's
     * scale (whole, that its 8x8 glyphs stay sharp; small: 1 up to a frame
     * 1280 rows high) and its lines; the icon a line high. */
    const float unit = frame.h / static_cast<float>(SCREEN_HEIGHT);
    const float scale = std::max(1.0F, std::floor(unit * 0.75F));
    const float glyph = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale;
    const float pad = 2 * scale;
    const float icon = glyph;
    const std::string frameText = std::to_string(status.frame);
    const std::string timeText = replayTimeText(status.frame);
    const float textWidth = static_cast<float>(std::max(frameText.size(), timeText.size())) * glyph;
    const float x = frame.x + (4 * unit);
    const float y = frame.y + (4 * unit);
    const SDL_FRect box{x, y, std::max(textWidth, 2 * icon) + (2 * pad), icon + (2 * glyph) + (4 * pad)};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, &box);
    const float left = x + pad;
    const float top = y + pad;
    const SDL_FColor white{0.95F, 0.95F, 0.95F, 1};
    const SDL_FColor red{0.92F, 0.2F, 0.2F, 1};
    switch (status.kind) {
    case Status::RECORD: {
        const int sides = 24;
        const SDL_FPoint middle{left + (icon / 2), top + (icon / 2)};
        for (int k = 0; k < sides; k++) {
            const float a = 2 * 3.14159265F * static_cast<float>(k) / sides;
            const float b = 2 * 3.14159265F * static_cast<float>(k + 1) / sides;
            fillTriangle(middle, {middle.x + (std::cos(a) * icon / 2), middle.y + (std::sin(a) * icon / 2)},
                         {middle.x + (std::cos(b) * icon / 2), middle.y + (std::sin(b) * icon / 2)}, red);
        }
        break;
    }
    case Status::REPLAY:
        fillTriangle({left, top}, {left, top + icon}, {left + icon, top + (icon / 2)}, white);
        break;
    case Status::FAST_FORWARD:
        for (int k = 0; k < 2; k++) {
            const float from = left + (static_cast<float>(k) * icon * 0.75F);
            fillTriangle({from, top}, {from, top + icon}, {from + (icon * 0.75F), top + (icon / 2)}, white);
        }
        break;
    case Status::REWIND:
        for (int k = 0; k < 2; k++) {
            const float to = left + (static_cast<float>(k + 1) * icon * 0.75F);
            fillTriangle({to, top}, {to, top + icon}, {to - (icon * 0.75F), top + (icon / 2)}, white);
        }
        break;
    case Status::PAUSE: {
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, SDL_ALPHA_OPAQUE);
        const SDL_FRect bars[2] = {{left + (icon * 0.1F), top, icon * 0.3F, icon},
                                   {left + (icon * 0.6F), top, icon * 0.3F, icon}};
        SDL_RenderFillRects(renderer, bars, 2);
        break;
    }
    case Status::NONE:
        break;
    }
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, SDL_ALPHA_OPAQUE);
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_RenderDebugText(renderer, left / scale, (top + icon + pad) / scale, frameText.c_str());
    SDL_RenderDebugText(renderer, left / scale, (top + icon + glyph + (2 * pad)) / scale, timeText.c_str());
    SDL_SetRenderScale(renderer, 1, 1);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

/* The window thread's: the picture shown, not yet on the screen, and the
 * run's state over it unless without. */
void drawScreen(bool withStatus)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    int width;
    int height;
    if (!shown.picture || !SDL_GetCurrentRenderOutputSize(renderer, &width, &height)) {
        return;
    }
    ScreenView view = shown.view;
    view.pixels = shown.pixels.data();
    keepFrameShape();
    if (!screenTexture(view)) {
        failure("no picture");
    }
    const SDL_FRect place = picturePlace(framePlace(width, height, view.frameWidth), view);
    /* A whole multiple both ways each pixel's own; else (200 rows as 240,
     * a size between) SDL's scaling for pixel art, sharp but even. */
    const int w = static_cast<int>(place.w);
    const int h = static_cast<int>(place.h);
    const bool whole = w % view.width == 0 && h % view.height == 0 && w / view.width == h / view.height;
    SDL_SetTextureScaleMode(texture, whole ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_PIXELART);
    SDL_RenderTexture(renderer, texture, nullptr, &place);
    if (withStatus) {
        drawStatus(shown.status, framePlace(width, height, view.frameWidth));
    }
}

/* The game thread's: the input the window has handed since the last frame,
 * to the keyboard and the mouse's driver. While a watched replay gives the
 * input (replay.h) the window's goes nowhere but its close. */
void takeInput()
{
    taken.clear();
    {
        const std::lock_guard<std::mutex> hold(inputLock);
        taken.swap(inputs);
    }
    if (replaying()) {
        closeAsked = closeAsked || std::any_of(taken.begin(), taken.end(),
                                               [](const Input &input) { return input.kind == Input::CLOSE; });
        return;
    }
    for (const Input &input : taken) {
        switch (input.kind) {
        case Input::SCAN:
            recordScan(input.scan);
            keyboardScan(input.scan);
            if (input.scan == 0xe0) {
                takenAfterE0 = true;
            } else {
                keysTaken[(input.scan & 0x7f) + (takenAfterE0 ? 128 : 0)] = (input.scan & 0x80) == 0;
                takenAfterE0 = false;
            }
            break;
        case Input::MOTION:
            motionX += input.x;
            motionY += input.y;
            break;
        case Input::BUTTONS:
            mouseButtons = input.buttons;
            sendMouse();
            break;
        case Input::CAPTURE:
            motionX = motionY = 0;
            break;
        case Input::CLOSE:
            closeAsked = true;
            break;
        case Input::DEVICE:
            recordDevice(input.device);
            keyboardDevice(input.device);
            break;
        }
    }
    if (((static_cast<i16>(motionX)) != 0) || ((static_cast<i16>(motionY)) != 0)) {
        sendMouse();
    }
}

/* The window's audio (windowAudio): SDL's stream, the bytes of a frame of
 * the output's samples and of a second of them, the part of a frame the last
 * bytes ended in, the lead, the level the stream holds smoothed over the
 * frames, and the speed it plays at. */
SDL_AudioStream *audio = nullptr;
bool audioFailed = false;
u32 audioFrame = 4;
u32 audioSecond = 0;
std::vector<u8> audioPart;
/* While the clock goes faster (timer.cpp's fast forward), one sample of so
 * many goes out, so that the sound keeps the wall clock's pace: where in
 * the so many the next is. */
std::uint64_t audioSkipped = 0;
std::vector<u8> audioThinned;
u32 audioLead = 0;
double audioLevel = 0;
float audioSpeed = 1;

/* The stream of 16-bit stereo samples (the 8-bit output's made so,
 * windowAudio: the output's depth may change, asylum.h's ASS_SetDepth) on
 * the device, with the lead of silence: the device's buffer and two
 * sixtieths of a second. The driver is the one REGRET_AUDIO_DRIVER names,
 * else SDL's choice (its SDL_AUDIO_DRIVER, else the first that works). The
 * audio starts on the window thread; the stream is the game thread's. */
bool openAudio(u32 rate)
{
    const char *driver = std::getenv("REGRET_AUDIO_DRIVER");
    if (driver != nullptr) {
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, driver);
    }
    bool started = false;
    std::string error;
    windowCall([&started, &error] {
        started = SDL_InitSubSystem(SDL_INIT_AUDIO);
        if (!started) {
            error = SDL_GetError();
        }
    });
    const SDL_AudioSpec spec{SDL_AUDIO_S16LE, 2, static_cast<int>(rate)};
    if (started) {
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (audio == nullptr) {
            error = SDL_GetError();
        }
    }
    if (audio == nullptr) {
        std::fprintf(stderr, "%s: no sound (%s)\n", PROFILE_NAME, error.c_str());
        audioFailed = true;
        return false;
    }
    audioFrame = 4;
    audioSecond = rate * audioFrame;
    SDL_AudioSpec device;
    int frames = 0;
    double buffer = 0.05;
    if (SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(audio), &device, &frames) && device.freq > 0 && frames > 0) {
        buffer = static_cast<double>(frames) / device.freq;
    }
    audioLead = static_cast<u32>((buffer + 2.0 / 60) * rate) * audioFrame;
    audioLevel = audioLead;
    std::vector<u8> silence(audioLead, 0);
    SDL_PutAudioStreamData(audio, silence.data(), static_cast<int>(silence.size()));
    SDL_ResumeAudioStreamDevice(audio);
    return true;
}

/* A frame's look at the audio: the level smoothed over half a second of
 * frames, and the speed that brings it to the lead, faster by 0.05 of what
 * it holds over it in seconds, slower by as much under, at most 0.3%. */
void keepAudioLead()
{
    if (audio == nullptr) {
        return;
    }
    audioLevel += (SDL_GetAudioStreamQueued(audio) - audioLevel) / 30;
    const double over = (audioLevel - audioLead) / audioSecond;
    audioSpeed = static_cast<float>(1 + std::clamp(over * 0.05, -0.003, 0.003));
    SDL_SetAudioStreamFrequencyRatio(audio, audioSpeed);
}

/* The window thread's: the picture of the screen read back from the
 * renderer, the last handed. */
bool readPicture(std::vector<u8> &rgb, int &width, int &height)
{
    takeFrame();
    drawScreen(false);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, nullptr);
    if (surface == nullptr) {
        return false;
    }
    SDL_Surface *packed = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGB24);
    SDL_DestroySurface(surface);
    if (packed == nullptr) {
        return false;
    }
    width = packed->w;
    height = packed->h;
    rgb.resize(static_cast<size_t>(width) * height * 3);
    for (int y = 0; y < height; y++) {
        std::copy_n(static_cast<const u8 *>(packed->pixels) + (static_cast<size_t>(y) * packed->pitch),
                    static_cast<size_t>(width) * 3, &rgb[static_cast<size_t>(y) * width * 3]);
    }
    SDL_DestroySurface(packed);
    SDL_RenderPresent(renderer);
    return true;
}

/* The window thread's: a call of the game thread's made, and the game thread
 * woken. It is woken under the lock: a game thread that ends the run
 * (endWindow) goes on with the exit, which destroys the lock and the
 * condition, only after the window thread has let go of both. */
void runCall(void *data)
{
    WindowThreadCall *call = static_cast<WindowThreadCall *>(data);
    (*call->function)();
    const std::lock_guard<std::mutex> hold(callLock);
    call->done = true;
    callDone.notify_all();
}

/* The window thread's: an event of SDL's; true at the game's end. A picture
 * to show again (the window uncovered or resized) sets redraw. */
bool windowEvent(const SDL_Event &event, bool &redraw)
{
    if (event.type == wakeEvent) {
        if (event.user.code == WAKE_FRAME) {
            frameWoken = false;
        } else if (event.user.code == WAKE_CALL) {
            runCall(event.user.data1);
        } else if (event.user.code == WAKE_END) {
            return true;
        }
        return false;
    }
    switch (event.type) {
    case SDL_EVENT_QUIT:
        give({.kind = Input::CLOSE});
        if ((closeAction != nullptr) && !closing) {
            closing = true;
            closedAt = std::chrono::steady_clock::now();
        }
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        if (window != nullptr) {
            keyEvent(event.key);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (captured) {
            give({.kind = Input::MOTION, .x = event.motion.xrel, .y = event.motion.yrel});
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (window != nullptr) {
            buttonEvent(event.button);
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        if (captured) {
            wheelEvent(event.wheel);
        }
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        openGamepad();
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if ((gamepad != nullptr) && event.gdevice.which == gamepadId) {
            gamepadGone();
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if ((gamepad != nullptr) && event.gbutton.which == gamepadId && event.gbutton.button < DEVICE_PAD_BUTTONS) {
            const DeviceEvent button{DeviceKind::PAD_BUTTON, event.gbutton.button,
                                     static_cast<i16>(event.gbutton.down ? 1 : 0)};
            if (!hostButton(button)) {
                giveDevice(button);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if ((gamepad != nullptr) && event.gaxis.which == gamepadId && event.gaxis.axis < DEVICE_AXES) {
            giveDevice({DeviceKind::PAD_AXIS, event.gaxis.axis, event.gaxis.value});
        }
        break;
    case SDL_EVENT_WINDOW_EXPOSED:
        redraw = true;
        break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        followPicture(event.window.data1, event.window.data2);
        redraw = true;
        break;
    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        fullscreenNow = event.type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN;
        break;
    }
    return false;
}

/* The window thread stopped at the run's end, which the game thread goes
 * on with: it touches nothing again. */
[[noreturn]] void park()
{
    for (;;) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
}

/* The window thread's loop until the game's end: the events, and the last
 * picture shown when there is a new one. */
void keepWindow()
{
    for (;;) {
        bool redraw = false;
        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, 100)) {
            do {
                if (windowEvent(event, redraw)) {
                    return;
                }
                if (loop == Loop::PARKED) {
                    park();
                }
            } while (SDL_PollEvent(&event));
        }
        if (closing && std::chrono::steady_clock::now() - closedAt > CLOSE_WAIT) {
            std::fprintf(stderr, "%s: the game did not end when the window closed\n", PROFILE_NAME);
            windowExit(1);
        }
        if ((window != nullptr) && (takeFrame() || redraw)) {
            drawScreen(true);
            SDL_RenderPresent(renderer);
        }
    }
}

/* The game thread's stack: 8 MiB, the program's first thread's on Linux,
 * on every host. A thread of the system's default size would have less on
 * some (512 KiB on macOS); the game's usecode nests its processes deeply
 * (a process's code runs the next one's inside it). */
const Sint64 GAME_STACK = 8 * 1024 * 1024;

/* The game thread's run. */
struct GameRun {
    int (*game)(int, char **);
    int argc;
    char **argv;
    int code;
};

int SDLCALL runGame(void *data)
{
    installCrashThread();
    GameRun *run = static_cast<GameRun *>(data);
    run->code = run->game(run->argc, run->argv);
    wake(WAKE_END, nullptr);
    return 0;
}

}

int windowMain(int (*game)(int, char **), int argc, char **argv)
{
    if (!windowRun()) {
        return game(argc, argv);
    }
    if (!SDL_Init(SDL_INIT_EVENTS)) {
        failure("no events");
    }
    wakeEvent = SDL_RegisterEvents(1);
    if (wakeEvent == 0U) {
        failure("no events");
    }
    windowThread = std::this_thread::get_id();
    std::atexit(endWindow);
    GameRun run{game, argc, argv, 0};
    loop = Loop::RUNNING;
    const SDL_PropertiesID properties = SDL_CreateProperties();
    SDL_SetPointerProperty(properties, SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER,
                           reinterpret_cast<void *>(runGame));
    SDL_SetStringProperty(properties, SDL_PROP_THREAD_CREATE_NAME_STRING, "game");
    SDL_SetPointerProperty(properties, SDL_PROP_THREAD_CREATE_USERDATA_POINTER, &run);
    SDL_SetNumberProperty(properties, SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER, GAME_STACK);
    SDL_Thread *thread = SDL_CreateThreadWithProperties(properties);
    SDL_DestroyProperties(properties);
    if (thread == nullptr) {
        loop = Loop::NONE;
        failure("no game thread");
    }
    keepWindow();
    SDL_WaitThread(thread, nullptr);
    loop = Loop::NONE;
    return run.code;
}

void hostCloseAction(void (*close)())
{
    const SystemSide systemSide;
    closeAction = close;
}

void hostSetFullscreen(bool on)
{
    const SystemSide systemSide;
    fullscreen = on;
    if (opened) {
        windowCall([on] {
            fullscreenNow = on;
            SDL_SetWindowFullscreen(window, on);
        });
    }
}

bool hostFullscreen()
{
    const SystemSide systemSide;
    return fullscreen;
}

void hostFullscreenAction(void (*changed)())
{
    const SystemSide systemSide;
    fullscreenAction = changed;
}

bool windowRun()
{
    return (std::getenv("REGRET_SCRIPT") == nullptr) && (!replaying() || replayWatched());
}

u16 displayWideWidth()
{
    int width = 0;
    windowCall([&width] {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            return;
        }
        int count = 0;
        if (SDL_DisplayID *displays = SDL_GetDisplays(&count); displays != nullptr) {
            for (int k = 0; k < count; k++) {
                width = std::max(width, wideWidthOf(SDL_GetDesktopDisplayMode(displays[k])));
            }
            SDL_free(displays);
        }
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    });
    return static_cast<u16>(width);
}

namespace {

/* A frame of the window (window.h): its input, and with picture its
 * picture, the sound's lead kept. */
bool frameOfWindow(bool picture)
{
    if (loop != Loop::RUNNING) {
        return false;
    }
    if (!opened) {
        windowCall(openWindow);
        opened = true;
    }
    if (picture) {
        keepAudioLead();
    }
    takeInput();
    if (picture) {
        handFrame();
    }
    if (closeAsked && (closeAction != nullptr)) {
        recordClose();
        const GameSide gameSide;
        closeAction();
    }
    if (fullscreenNow != fullscreen) {
        fullscreen = fullscreenNow;
        if (fullscreenAction != nullptr) {
            const GameSide gameSide;
            fullscreenAction();
        }
    }
    return true;
}

}

bool windowFrame()
{
    return frameOfWindow(true);
}

bool windowFrameUnshown()
{
    return frameOfWindow(false);
}

bool windowPicture(std::vector<u8> &rgb, int &width, int &height)
{
    if (!opened) {
        return false;
    }
    handFrame();
    bool read = false;
    windowCall([&read, &rgb, &width, &height] { read = readPicture(rgb, width, height); });
    return read;
}

/* The sound's output to SDL3's audio (window.h). */
void windowAudio(const u8 *bytes, u32 size, u32 rate, bool sixteen)
{
    /* None from a watched replay before it is shown (replay.h). */
    if (audioFailed || !windowRun() || (replaying() && !watching()) || (size == 0U) ||
        ((audio == nullptr) && !openAudio(rate))) {
        return;
    }
    if (sixteen) {
        audioPart.insert(audioPart.end(), bytes, bytes + size);
    } else {
        /* An 8-bit sample's offset from 80h, the high byte of a 16-bit one. */
        for (u32 k = 0; k < size; k++) {
            audioPart.push_back(0);
            audioPart.push_back(static_cast<u8>(bytes[k] ^ 0x80));
        }
    }
    const u32 whole = static_cast<u32>(audioPart.size()) / audioFrame * audioFrame;
    const std::uint64_t speed = clockSpeed();
    const u8 *out = audioPart.data();
    u32 outBytes = whole;
    if (speed > 1) {
        audioThinned.clear();
        for (u32 at = 0; at < whole; at += audioFrame) {
            if (audioSkipped == 0) {
                audioThinned.insert(audioThinned.end(), audioPart.begin() + at, audioPart.begin() + at + audioFrame);
            }
            audioSkipped = (audioSkipped + 1) % speed;
        }
        out = audioThinned.data();
        outBytes = static_cast<u32>(audioThinned.size());
    }
    const u32 held = static_cast<u32>(SDL_GetAudioStreamQueued(audio));
    const u32 room = held < 2 * audioLead ? (2 * audioLead - held) / audioFrame * audioFrame : 0;
    const u32 put = std::min(outBytes, room);
    if (put != 0U) {
        SDL_PutAudioStreamData(audio, out, static_cast<int>(put));
    }
    audioPart.erase(audioPart.begin(), audioPart.begin() + whole);
}

bool windowAudioState(WindowAudio &audioState)
{
    if (audio == nullptr) {
        return false;
    }
    audioState = {static_cast<u32>(SDL_GetAudioStreamQueued(audio)), audioLead, audioSpeed};
    return true;
}

/* The message of the run's end (report.h), over the window when it is
 * open. The box needs the mouse the window has captured. */
void windowMessage(const char *text)
{
    windowCall([text] {
        if (window && captured) {
            capture(false);
        }
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PROFILE_TITLE, text, window);
    });
}

void windowWarning(const char *text)
{
    windowCall([text] {
        if (window && captured) {
            capture(false);
        }
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, PROFILE_TITLE, text, window);
    });
}

std::optional<IconPicture> largestIconPng(const std::vector<u8> &ico)
{
    /* The icon file's head (0, type 1, the count) and a 16-byte entry a
     * picture: its width (0 for 256) at 0, its size at 8, its place at 12. */
    constexpr std::size_t HEAD = 6;
    constexpr std::size_t ENTRY = 16;
    constexpr std::array<u8, 8> PNG_SIGNATURE = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (ico.size() < HEAD || ico[0] != 0 || ico[1] != 0 || ico[2] != 1 || ico[3] != 0) {
        return std::nullopt;
    }
    const auto word = [&ico](std::size_t at) { return static_cast<std::size_t>(ico[at] | ico[at + 1] << 8); };
    const auto dword = [&word](std::size_t at) { return word(at) | word(at + 2) << 16; };
    const std::size_t count = word(4);
    std::optional<IconPicture> largest;
    std::size_t largestWidth = 0;
    for (std::size_t k = 0; k < count && HEAD + ((k + 1) * ENTRY) <= ico.size(); k++) {
        const std::size_t entry = HEAD + (k * ENTRY);
        const std::size_t width = ico[entry] == 0 ? 256 : ico[entry];
        const std::size_t size = dword(entry + 8);
        const std::size_t offset = dword(entry + 12);
        const bool png =
            offset <= ico.size() && size >= PNG_SIGNATURE.size() && size <= ico.size() - offset &&
            std::equal(PNG_SIGNATURE.begin(), PNG_SIGNATURE.end(), ico.begin() + static_cast<std::ptrdiff_t>(offset));
        if (png && width > largestWidth) {
            largest = IconPicture{.offset = offset, .size = size};
            largestWidth = width;
        }
    }
    return largest;
}

void windowTitleNote(const std::string &note)
{
    windowCall([&note] {
        titleNote = note;
        if (window != nullptr) {
            showTitle();
        }
    });
}

void windowCall(const std::function<void()> &function)
{
    if (loop != Loop::RUNNING || std::this_thread::get_id() == windowThread) {
        if (loop != Loop::PARKED) {
            function();
        }
        return;
    }
    WindowThreadCall call{&function, false};
    wake(WAKE_CALL, &call);
    std::unique_lock<std::mutex> hold(callLock);
    callDone.wait(hold, [&call] { return call.done; });
}

void windowExit(int code)
{
    if (loop == Loop::RUNNING && std::this_thread::get_id() == windowThread) {
        std::fflush(nullptr);
        std::_Exit(code);
    }
    std::exit(code);
}

void inputHostKeys(HostAction action, const HostKey *keys, std::size_t count)
{
    const SystemSide systemSide;
    const std::lock_guard<std::mutex> hold(hostLock);
    hostKeys[static_cast<std::size_t>(action)].assign(keys, keys + count);
}

bool windowRewinding()
{
    return rewindHeld;
}

bool windowFastForwarding()
{
    return fastHeld;
}

bool windowPaused()
{
    return paused;
}

bool windowPauseFrame()
{
    handFrame();
    const std::lock_guard<std::mutex> hold(inputLock);
    return std::none_of(inputs.begin(), inputs.end(), [](const Input &input) { return input.kind == Input::CLOSE; });
}

int windowStateAsked()
{
    return stateAsked.exchange(0);
}

bool windowTakeOverAsked()
{
    return takeOverAsked.exchange(false);
}

KeysHeld windowKeysTaken()
{
    return keysTaken;
}

bool windowRewindFrame()
{
    keepAudioLead();
    handFrame();
    const std::lock_guard<std::mutex> hold(inputLock);
    return std::none_of(inputs.begin(), inputs.end(), [](const Input &input) { return input.kind == Input::CLOSE; });
}

void windowRewindEnd(const KeysHeld &held)
{
    const std::lock_guard<std::mutex> hold(inputLock);
    /* The buttons down now: the last that came, else the last the game has. */
    u16 down = mouseButtons;
    for (const Input &input : inputs) {
        if (input.kind == Input::BUTTONS) {
            down = input.buttons;
        }
    }
    std::erase_if(inputs,
                  [](const Input &input) { return input.kind != Input::CLOSE && input.kind != Input::CAPTURE; });
    for (std::size_t key = 0; key < keysGiven.size(); key++) {
        if (held[key] == keysGiven[key]) {
            continue;
        }
        if (key >= 128) {
            inputs.push_back({.kind = Input::SCAN, .scan = 0xe0});
        }
        const u8 scan = static_cast<u8>(key & 0x7f);
        inputs.push_back({.kind = Input::SCAN, .scan = static_cast<u8>(keysGiven[key] ? scan : scan | 0x80)});
    }
    inputs.push_back({.kind = Input::BUTTONS, .buttons = down});
    keysTaken = held;
    takenAfterE0 = false;
}
