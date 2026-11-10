/* The port's window (port/system/window.h) with SDL's dummy driver, no
 * display needed: the picture of the screen in the window, which the frames
 * draw from the video memory and the DAC, in the window's sizes: VESA's
 * 640x480 a whole multiple, mode 13h's 320x200 on 4:3, black around, a text
 * mode black; the DAC's colours through the pixel mask. The virtual time on
 * the wall clock, waiting when ahead, catching up when behind, no more than
 * a second. SDL's keys as the keyboard's scan codes, through the BIOS.
 * SDL's relative motion as mickeys, at the game's speed, and the buttons,
 * to the game's handler; the mouse freed and captured again. The other
 * devices: the mouse's middle and side buttons and wheel, a gamepad's
 * (SDL's virtual one) buttons and axes, to the game's handler of them on
 * the keys' line. The sound on SDL's dummy audio: the lead of silence,
 * samples that end inside a frame, no more than twice the lead, the speed
 * toward it. The game's haltMessage, the end closing the window calls
 * (host.h) and the kernel's mouse handler are the test's.
 *
 * The run's two threads (window.h): the checks run on the game thread
 * (windowMain), the window on the program's first. The test's own calls of
 * SDL's that only that thread may make go through windowCall, and after
 * pushing events the test waits for the window thread to have taken them
 * (sync) before the frame that gets them. The game's time goes on while the
 * window thread is held; the window's closing comes to the game at its next
 * frame. With --exit the game thread ends the run with std::exit after a
 * frame: the window thread closes SDL and the run ends (window_exit). The
 * run is recorded: the keys' bytes, the mouse's motion, the devices'
 * events, the passes the wall clock made longer and the closing are in
 * inputs.tsv. */
#include "clock.h"
#include "commandline.h"
#include "conio.h"
#include "dos.h"
#include "host.h"
#include "screen.h"
#include "input.h"
#include "mousedrv.h"
#include "pc.h"
#include "png.h"
#include "replay.h"
#include "supervga.h"
#include "svgaprim.h"
#include "video.h"
#include "window.h"
#include "x32code.h"
#include "testenv.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <utility>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

double secondsSince(Clock::time_point start)
{
    return std::chrono::duration<double>(Clock::now() - start).count();
}

/* The virtual time since start, in seconds. */
double virtualSince(std::uint64_t start)
{
    return (double)(pcTime() - start) / PIT_HZ;
}

int checks = 0, failures = 0;

void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok) {
        failures++;
        std::printf("MISMATCH window: %s\n", what.c_str());
    }
}

/* The DAC's colour of index in 8 bits, as the window shows it. */
struct Rgb {
    u8 r, g, b;
    bool operator==(const Rgb &) const = default;
};
u8 dac[256][3];

Rgb colour(u8 index)
{
    auto wide = [](u8 level) { return (u8)(level << 2 | level >> 4); };
    return {wide(dac[index][0]), wide(dac[index][1]), wide(dac[index][2])};
}

struct Picture {
    std::vector<u8> rgb;
    int width = 0, height = 0;

    Rgb at(int x, int y) const
    {
        const u8 *p = &rgb[((size_t)y * width + x) * 3];
        return {p[0], p[1], p[2]};
    }
    /* Whether the rectangle is all black. */
    bool black(int left, int top, int right, int bottom) const
    {
        for (int y = top; y < bottom; y++) {
            for (int x = left; x < right; x++) {
                if (!(at(x, y) == Rgb{0, 0, 0})) {
                    return false;
                }
            }
        }
        return true;
    }
};

/* The window thread has taken the events pushed before. */
void sync()
{
    windowCall([] {});
}

/* The window's picture at width x height. */
Picture picture(int width, int height)
{
    int count = 0;
    windowCall([&count, width, height] {
        SDL_Window **windows = SDL_GetWindows(&count);
        if (count == 1) {
            SDL_SetWindowSize(windows[0], width, height);
            SDL_SyncWindow(windows[0]);
            /* The renderer takes the new size from the window's event. */
            SDL_PumpEvents();
        }
        SDL_free(windows);
    });
    Picture p;
    check(count == 1, "one window");
    check(windowPicture(p.rgb, p.width, p.height), "a picture");
    check(p.width == width && p.height == height,
          "the picture is the window's size: " + std::to_string(p.width) + "x" + std::to_string(p.height));
    return p;
}

struct MouseCall {
    u16 events, buttons, x, y;
};
std::vector<MouseCall> mouseCalls;

void mouseHandler(u16 events, u16 buttons, u16 x, u16 y)
{
    mouseCalls.push_back({events, buttons, x, y});
}

/* The other devices' events the game's handler (the test's) got. */
std::vector<DeviceEvent> deviceCalls;

void deviceHandler(const DeviceEvent &event)
{
    deviceCalls.push_back(event);
}

bool sawDevice(DeviceKind kind, u8 which, i16 value)
{
    for (const DeviceEvent &event : deviceCalls) {
        if (event.kind == kind && event.which == which && event.value == value) {
            return true;
        }
    }
    return false;
}

}

void haltMessage(const char *message, std::source_location place)
{
    std::printf("halt %s %u: %s\n", place.file_name(), (unsigned)place.line(), message);
    std::exit(2);
}

const char *haltFormat(const char *format, ...)
{
    return format;
}

namespace {

bool closed = false;
/* The window's changes of the full screen the game was told of. */
int fullscreenChanges = 0;

/* --exit: the game thread's exit after the window's first frame. */
int exitRun(int, char **)
{
    windowFrame();
    std::printf("window: the game thread exits\n");
    std::exit(0);
}

int checkWindow(int, char **)
{
    hostCloseAction([] { closed = true; });
    /* The run recorded (replay.h): what its frames give the game,
     * checked at the end. */
    std::filesystem::create_directories("window_recording");
    startRecording("window_recording", {});
    /* The test's virtual gamepad alone is a gamepad: not the host's. */
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT, "0x1234/0x5678");
    /* Every entry of the DAC its own colour. */
    for (int i = 0; i < 256; i++) {
        dac[i][0] = (u8)(i & 0x3f);
        dac[i][1] = (u8)(i >> 2);
        dac[i][2] = (u8)(0x3f - (i & 0x3f));
        screenSetColors((u8)i, 1, dac[i]);
    }

    /* The first frame opens the window; in the text mode of the start it
     * is black. */
    check(windowFrame(), "a window without a script");
    Picture text = picture(640, 480);
    check(text.black(0, 0, 640, 480), "a text mode is black");

    /* VESA's 640x480 twice as large: each pixel 2x2 of its colour. */
    check(screenSetMode(VESA_640X480, false), "mode 101h");
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            svgaMemory[y * 640 + x] = (u8)(x ^ y);
        }
    }
    Picture twice = picture(1280, 960);
    bool same = true;
    for (int y = 0; y < 960 && same; y++) {
        for (int x = 0; x < 1280 && same; x++) {
            same = twice.at(x, y) == colour((u8)(x / 2 ^ y / 2));
        }
    }
    check(same, "640x480 at 1280x960: each pixel 2x2");

    /* The hi-colour movies' 16-bit mode (VESA 111h): each pixel its own
     * colour of 5, 6 and 5 bits, their top bits again below. */
    check(screenSetMode(VESA_640X480_64K, false), "mode 111h");
    auto hiColour = [](u16 c) {
        u8 r = (u8)(c >> 11), g = (u8)(c >> 5 & 0x3f), b = (u8)(c & 0x1f);
        return Rgb{(u8)(r << 3 | r >> 2), (u8)(g << 2 | g >> 4), (u8)(b << 3 | b >> 2)};
    };
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            u16 c = (u16)(x * 97 + y * 31);
            svgaMemory[y * 1280 + 2 * x] = (u8)c;
            svgaMemory[y * 1280 + 2 * x + 1] = (u8)(c >> 8);
        }
    }
    Picture hi = picture(640, 480);
    same = true;
    for (int y = 0; y < 480 && same; y++) {
        for (int x = 0; x < 640 && same; x++) {
            same = hi.at(x, y) == hiColour((u16)(x * 97 + y * 31));
        }
    }
    check(same, "640x480 of 16-bit colours as they are");
    check(screenSetMode(VESA_640X480, false), "mode 101h again");
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            svgaMemory[y * 640 + x] = (u8)(x ^ y);
        }
    }

    /* A window of another shape: the 4:3 box as large as it fits, in its
     * middle, black at the sides: 800x600 at (100,0), each pixel 1.25
     * wide and high. Blocks of 8x8 pixels of a colour each, 10x10 there:
     * their middles of it. */
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            svgaMemory[y * 640 + x] = (u8)(x / 8 ^ y / 8 * 5);
        }
    }
    Picture wide = picture(1000, 600);
    same = true;
    for (int y = 0; y < 60 && same; y++) {
        for (int x = 0; x < 80 && same; x++) {
            same = wide.at(100 + x * 10 + 5, y * 10 + 5) == colour((u8)(x ^ y * 5));
        }
    }
    check(same, "640x480 at 1000x600: 800x600 at (100,0)");
    check(wide.black(0, 0, 100, 600) && wide.black(900, 0, 1000, 600) && !wide.black(100, 0, 101, 600) &&
              !wide.black(899, 0, 900, 600),
          "black at the sides");

    /* A window smaller than 640x480: 4:3 still, as wide as it is. */
    Picture small = picture(400, 400);
    check(small.black(0, 0, 400, 50) && small.black(0, 350, 400, 400), "400x400: black above and below 400x300");
    check(!small.black(0, 50, 400, 51) && !small.black(0, 349, 400, 350), "the picture from row 50 to 349");

    /* Mode 13h's 320x200 on 4:3: 1280x960 is four times 320x240, each row
     * 4.8 rows high. Rows of one colour each, then columns. */
    screenSetMode(0x13, false);
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            videoMemory[y * 320 + x] = (u8)y;
        }
    }
    Picture rows = picture(1280, 960);
    check(!rows.black(0, 0, 1280, 1) && !rows.black(0, 959, 1280, 960), "the rows fill the height");
    same = true;
    for (int y = 0; y < 200 && same; y++) {
        int middle = (int)((y + 0.5) * 4.8);
        for (int x = 0; x < 1280 && same; x += 7) {
            same = rows.at(x, middle) == colour((u8)y);
        }
    }
    check(same, "320x200 at 1280x960: each row in the middle of its 4.8");
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            videoMemory[y * 320 + x] = (u8)x;
        }
    }
    Picture columns = picture(1280, 960);
    same = true;
    for (int x = 0; x < 320 && same; x++) {
        for (int y = 0; y < 960 && same; y += 13) {
            same = columns.at(x * 4 + 1, y) == colour((u8)x) && columns.at(x * 4 + 2, y) == colour((u8)x);
        }
    }
    check(same, "each column four wide");

    /* The experimental wide screen: turned on, the window takes its
     * display's shape, as high as it was; then the width is the shape of
     * its picture, 854 for 1708x960. Mode 101h's screen with the sides the
     * game drew around it, dimmed at 2 (each colour at half); without the
     * sides, and a mode's screen of 4:3, 4:3 in the frame's middle. Blocks
     * of 8x8 pixels as above; the sides of colours of their own. */
    screenSetMode(VESA_640X480, false);
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            svgaMemory[y * 640 + x] = (u8)(x / 8 ^ y / 8 * 5);
        }
    }
    picture(640, 480);
    screenSetWide(1);
    Picture shaped;
    check(windowPicture(shaped.rgb, shaped.width, shaped.height), "a picture of the wide frame");
    int shapeWidth = 0;
    int shapeHeight = 0;
    int displayWidth = 0;
    int displayHeight = 0;
    windowCall([&] {
        int count = 0;
        SDL_Window **windows = SDL_GetWindows(&count);
        if (count == 1) {
            SDL_GetWindowSize(windows[0], &shapeWidth, &shapeHeight);
            const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(windows[0]));
            if (mode != nullptr) {
                displayWidth = mode->w;
                displayHeight = mode->h;
            }
        }
        SDL_free(windows);
    });
    check(shapeHeight == 480 && displayHeight > 0 &&
              shapeWidth == std::min(480 * displayWidth / displayHeight, (int)WIDE_MOST),
          "turned on, the window takes its display's shape: " + std::to_string(shapeWidth) + "x" +
              std::to_string(shapeHeight) + " on " + std::to_string(displayWidth) + "x" +
              std::to_string(displayHeight));
    picture(1708, 960);
    sync();
    check(screenWideWidth() == 854, "the width the picture's shape: " + std::to_string(screenWideWidth()));
    std::vector<u8> sides(854 * 480, 0);
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 107; x++) {
            sides[y * 854 + x] = 0x21;
            sides[y * 854 + 747 + x] = 0x42;
        }
    }
    screenShowSides(sides.data(), 854);
    Picture wideSides = picture(1708, 960);
    same = true;
    for (int y = 0; y < 60 && same; y++) {
        for (int x = 0; x < 80 && same; x++) {
            same = wideSides.at(214 + x * 16 + 8, y * 16 + 8) == colour((u8)(x ^ y * 5));
        }
    }
    check(same && wideSides.at(100, 500) == colour(0x21) && wideSides.at(1600, 500) == colour(0x42),
          "854x480 at 1708x960: the sides and the screen between them");
    screenSetWide(2);
    Picture dimmed = picture(1708, 960);
    auto half = [](Rgb c) { return Rgb{(u8)(c.r >> 1), (u8)(c.g >> 1), (u8)(c.b >> 1)}; };
    check(dimmed.at(100, 500) == half(colour(0x21)) && dimmed.at(1600, 500) == half(colour(0x42)) &&
              dimmed.at(214 + 8, 8) == colour(0),
          "dimmed: the sides at half, the screen as it is");
    screenShowSides(nullptr, 0);
    Picture alone = picture(1708, 960);
    check(alone.black(0, 0, 214, 960) && alone.black(1494, 0, 1708, 960) && !alone.black(214, 0, 215, 960) &&
              !alone.black(1493, 0, 1494, 960),
          "without the sides: 4:3 in the frame's middle");
    screenSetMode(0x13, false);
    Picture mode13 = picture(1708, 960);
    check(mode13.black(0, 0, 214, 960) && mode13.black(1494, 0, 1708, 960) && !mode13.black(214, 0, 1494, 960),
          "mode 13h: 4:3 in the frame's middle");
    screenSetWide(0);

    /* The time: the first pass's frame starts the wall clock. Ahead of it,
     * thirty passes take half a second; a test that falls behind it (a
     * loaded machine) goes as far as the clock. */
    clockPass();
    Clock::time_point wall = Clock::now();
    std::uint64_t start = pcTime();
    for (int i = 0; i < 30; i++) {
        clockPass();
    }
    double took = secondsSince(wall);
    check(virtualSince(start) > 0.499 && virtualSince(start) < std::max(0.501, took + 0.01),
          "thirty passes are half a second: " + std::to_string(virtualSince(start)) + ", " + std::to_string(took));
    check(took > 0.48 && took < 0.7, "and take half a second: " + std::to_string(took));
    /* The wall clock the passes' frames woke late to (a host's sleep wakes
     * late: macOS's by milliseconds, as it gathers its timers' wakes), which
     * the delay's first frames catch up on: it takes as much less. */
    const double behind = std::max(0.0, took - virtualSince(start));
    wall = Clock::now();
    clockDelay(200);
    took = secondsSince(wall);
    check(took > 0.18 - behind && took < 0.4, "clockDelay(200) takes 0.2 s: " + std::to_string(took) + ", the passes " +
                                                  std::to_string(behind) + " s late");
    /* With a window the day is the host's. */
    std::time_t host = std::time(nullptr);
    std::tm local = *std::localtime(&host);
    ClockDate date = clockDate();
    check(date.year == local.tm_year + 1900 && date.month == local.tm_mon + 1 && date.day == local.tm_mday &&
              date.weekday == local.tm_wday,
          "the host's day");
    /* Behind it, a pass goes as far as it: 0.3 s, less up to a frame by
     * which the time was ahead of it (the frames wait for the wall clock,
     * and the delay before ended between two). */
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    start = pcTime();
    clockPass();
    check(virtualSince(start) > 0.28 && virtualSince(start) < 0.45,
          "a pass 0.3 s behind: " + std::to_string(virtualSince(start)));
    /* No more than a second of it. */
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    start = pcTime();
    clockPass();
    check(virtualSince(start) > 0.99 && virtualSince(start) < 1.02,
          "a pass 1.5 s behind goes a second: " + std::to_string(virtualSince(start)));
    wall = Clock::now();
    for (int i = 0; i < 6; i++) {
        clockPass();
    }
    check(secondsSince(wall) < 0.2, "and keeps to the clock after: " + std::to_string(secondsSince(wall)));

    /* The keys: SDL's events at a frame, as set 1's scan codes to the
     * keyboard, and so to the BIOS's buffer. SDL's repeats are dropped. */
    auto key = [](SDL_Scancode code, bool down, bool repeat = false) {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.scancode = code;
        event.key.down = down;
        event.key.repeat = repeat;
        SDL_PushEvent(&event);
    };
    while (inputKeyWaiting()) {
        inputKey();
    }
    key(SDL_SCANCODE_A, true);
    key(SDL_SCANCODE_A, true, true);
    key(SDL_SCANCODE_A, false);
    key(SDL_SCANCODE_UP, true);
    key(SDL_SCANCODE_UP, false);
    key(SDL_SCANCODE_KP_ENTER, true);
    key(SDL_SCANCODE_KP_ENTER, false);
    sync();
    clockPass();
    clockPass();
    check(inputKeyWaiting() && inputKey() == 'a', "A, once");
    check(inputKeyWaiting() && inputKey() == 0 && inputKey() == 0x48, "the up arrow: E0 48");
    check(inputKeyWaiting() && inputKey() == 0 && inputKey() == 0x1c, "the keypad's Enter: E0 1C");
    check(!inputKeyWaiting(), "nothing more");

    /* A debug feature's key (the rewind's F11, as the game gives it):
     * without the debug features the window does not take it, and the
     * game gets it; with --debug-feature on the command line it holds the
     * rewind while it is down, and the game gets nothing. */
    const HostKey f11{0x57, false, 0};
    inputHostKeys(HostAction::REWIND, &f11, 1);
    key(SDL_SCANCODE_F11, true);
    sync();
    clockPass();
    check(!hostDebugFeatures() && !windowRewinding() && inputKeyWaiting(),
          "F11 without the debug features: the game's key, no rewind");
    key(SDL_SCANCODE_F11, false);
    sync();
    clockPass();
    while (inputKeyWaiting()) {
        inputKey();
    }
    {
        char program[] = "window_test";
        char debugFeature[] = "--debug-feature";
        char *words[] = {program, debugFeature, nullptr};
        int count = 2;
        char **given = words;
        portCommandLine(count, given);
        check(hostDebugFeatures() && count == 1, "--debug-feature: on, and out of the game's arguments");
    }
    key(SDL_SCANCODE_F11, true);
    sync();
    check(windowRewinding(), "F11 with them: the rewind held");
    key(SDL_SCANCODE_F11, false);
    sync();
    clockPass();
    check(!windowRewinding() && !inputKeyWaiting(), "let go, and the game got no key");

    /* The full screen: its host action's key (Alt+Enter, as the game
     * gives it) makes the window cover the screen in the desktop's mode,
     * the game told at its next frame; the picture 4:3 as high as the
     * screen, in the middle. The game's own change is not told back. */
    const HostKey altEnter{0x1c, false, 4};
    inputHostKeys(HostAction::FULLSCREEN, &altEnter, 1);
    hostFullscreenAction([] { fullscreenChanges++; });
    auto windowFlags = [] {
        SDL_WindowFlags flags = 0;
        windowCall([&flags] {
            int count = 0;
            SDL_Window **windows = SDL_GetWindows(&count);
            if (count == 1) {
                flags = SDL_GetWindowFlags(windows[0]);
            }
            SDL_free(windows);
        });
        return flags;
    };
    auto altKey = [](SDL_Scancode code, bool down) {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.scancode = code;
        event.key.mod = SDL_KMOD_LALT;
        event.key.down = down;
        SDL_PushEvent(&event);
    };
    altKey(SDL_SCANCODE_RETURN, true);
    altKey(SDL_SCANCODE_RETURN, false);
    sync();
    check((windowFlags() & SDL_WINDOW_FULLSCREEN) != 0, "Alt+Enter: the full screen");
    check(!hostFullscreen() && fullscreenChanges == 0, "the game not told before its frame");
    clockPass();
    check(hostFullscreen() && fullscreenChanges == 1, "and told at it");
    check(!inputKeyWaiting(), "the game gets no Enter");
    SDL_Rect screen{};
    windowCall([&screen] {
        int count = 0;
        SDL_Window **windows = SDL_GetWindows(&count);
        if (count == 1) {
            SDL_GetDisplayBounds(SDL_GetDisplayForWindow(windows[0]), &screen);
        }
        SDL_free(windows);
    });
    Picture full;
    check(windowPicture(full.rgb, full.width, full.height) && full.width == screen.w && full.height == screen.h,
          "the picture the screen's size: " + std::to_string(full.width) + "x" + std::to_string(full.height));
    const int fullWidth = std::min(full.width, full.height * 4 / 3);
    const int fullHeight = fullWidth * 3 / 4;
    const int left = (full.width - fullWidth) / 2;
    const int top = (full.height - fullHeight) / 2;
    check(full.black(0, 0, left, full.height) && full.black(left + fullWidth, 0, full.width, full.height) &&
              !full.black(left, top, left + 1, top + fullHeight) &&
              !full.black(left + fullWidth - 1, top, left + fullWidth, top + fullHeight) &&
              !full.black(left, top, left + fullWidth, top + 1),
          "4:3 as large as the screen holds: " + std::to_string(fullWidth) + "x" + std::to_string(fullHeight));
    hostSetFullscreen(false);
    check((windowFlags() & SDL_WINDOW_FULLSCREEN) == 0 && !hostFullscreen(), "the game's own change: a window");
    clockPass();
    check(fullscreenChanges == 1, "not told back");
    /* Held 0.7 s, the keyboard repeats it after 500 ms. */
    key(SDL_SCANCODE_SPACE, true);
    sync();
    wall = Clock::now();
    while (secondsSince(wall) < 0.7) {
        clockPass();
    }
    key(SDL_SCANCODE_SPACE, false);
    sync();
    clockPass();
    int spaces = 0;
    while (inputKeyWaiting()) {
        spaces += inputKey() == ' ';
    }
    check(spaces >= 2 && spaces <= 4, "space held 0.7 s: " + std::to_string(spaces));

    /* The mouse: SDL's relative motion at a frame as mickeys, which the
     * driver moves the cursor by at the game's speed, 3 mickeys across and
     * 16 down to 8 pixels; the rest of a pixel, and of a mickey, kept. */
    inputMouseStart(mouseHandler);
    inputMouseXRange(0, 1278);
    inputMouseYRange(0, 479);
    auto motion = [](float x, float y) {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.xrel = x;
        event.motion.yrel = y;
        SDL_PushEvent(&event);
    };
    auto button = [](Uint8 which, bool down) {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.button = which;
        event.button.down = down;
        SDL_PushEvent(&event);
    };
    auto last = [](u16 events, u16 buttons, u16 x, u16 y) {
        return !mouseCalls.empty() && mouseCalls.back().events == events && mouseCalls.back().buttons == buttons &&
               mouseCalls.back().x == x && mouseCalls.back().y == y;
    };
    mouseCalls.clear();
    motion(2, 10);
    motion(1, 6);
    sync();
    clockPass();
    check(mouseCalls.size() == 1 && last(1, 0, 8, 8), "a frame's motion, 3 and 16 mickeys: one event, 8 and 8");
    std::vector<u16> xs;
    for (int i = 0; i < 3; i++) {
        motion(1, 0);
        sync();
        clockPass();
        xs.push_back(mouseCalls.back().x);
    }
    check(xs == std::vector<u16>{10, 13, 16}, "a mickey across three times: 2, 3 and 3 pixels");
    size_t calls = mouseCalls.size();
    motion(0.5f, 0);
    sync();
    clockPass();
    check(mouseCalls.size() == calls, "half a mickey: nothing yet");
    motion(0.5f, 0);
    sync();
    clockPass();
    check(mouseCalls.size() == calls + 1 && last(1, 0, 18, 8), "the other half: a mickey");
    button(SDL_BUTTON_LEFT, true);
    button(SDL_BUTTON_LEFT, false);
    button(SDL_BUTTON_RIGHT, true);
    sync();
    clockPass();
    check(mouseCalls.size() == calls + 4 && mouseCalls[calls + 1].events == 2 && mouseCalls[calls + 1].buttons == 1 &&
              mouseCalls[calls + 2].events == 4 && mouseCalls[calls + 3].events == 8 &&
              mouseCalls[calls + 3].buttons == 2,
          "the left button down and up, the right down");
    button(SDL_BUTTON_RIGHT, false);
    motion(-5000, -5000);
    sync();
    clockPass();
    check(mouseCalls.size() == calls + 6 && mouseCalls[calls + 4].events == 0x10 && last(1, 0, 0, 0),
          "the right up, then the motion to the limits");
    /* Control+F10 frees the mouse, and the game gets neither the key nor
     * the motion; a click captures it again, which the game does not get. */
    SDL_Event f10{};
    f10.type = SDL_EVENT_KEY_DOWN;
    f10.key.scancode = SDL_SCANCODE_F10;
    f10.key.mod = SDL_KMOD_LCTRL;
    f10.key.down = true;
    SDL_PushEvent(&f10);
    f10.type = SDL_EVENT_KEY_UP;
    f10.key.down = false;
    SDL_PushEvent(&f10);
    calls = mouseCalls.size();
    motion(30, 160);
    sync();
    clockPass();
    clockPass();
    check(mouseCalls.size() == calls && !inputKeyWaiting(), "freed: no motion, no F10");
    button(SDL_BUTTON_LEFT, true);
    button(SDL_BUTTON_LEFT, false);
    sync();
    clockPass();
    check(mouseCalls.size() == calls, "the click that captures it");
    motion(3, 16);
    sync();
    clockPass();
    /* Two thirds of a pixel back from the motion to the limits are kept. */
    check(mouseCalls.size() == calls + 1 && last(1, 0, 7, 8), "captured again");

    /* The other devices: the side buttons and the wheel's notches (flipped
     * the other way) to the game's handler of them at the frame; the middle
     * button to it and to the driver; a freed mouse lets go of a side button
     * held. */
    inputDevices(deviceHandler);
    auto wheel = [](Sint32 notches, bool flipped) {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.integer_y = notches;
        event.wheel.y = (float)notches;
        event.wheel.direction = flipped ? SDL_MOUSEWHEEL_FLIPPED : SDL_MOUSEWHEEL_NORMAL;
        SDL_PushEvent(&event);
    };
    deviceCalls.clear();
    button(SDL_BUTTON_X1, true);
    wheel(2, false);
    wheel(1, true);
    sync();
    clockPass();
    check(deviceCalls.size() == 3 && sawDevice(DeviceKind::MOUSE_BUTTON, DEVICE_MOUSE_X1, 1) &&
              deviceCalls[1].kind == DeviceKind::WHEEL && deviceCalls[1].value == 2 &&
              deviceCalls[2].kind == DeviceKind::WHEEL && deviceCalls[2].value == -1,
          "the side button down, the wheel two notches up and one down");
    calls = mouseCalls.size();
    button(SDL_BUTTON_MIDDLE, true);
    button(SDL_BUTTON_MIDDLE, false);
    sync();
    clockPass();
    /* The game's mask of the driver's events (3Fh) has the middle's down
     * and not its up. */
    check(sawDevice(DeviceKind::MOUSE_BUTTON, DEVICE_MOUSE_MIDDLE, 1) &&
              sawDevice(DeviceKind::MOUSE_BUTTON, DEVICE_MOUSE_MIDDLE, 0) && mouseCalls.size() == calls + 1 &&
              mouseCalls[calls].events == 0x20,
          "the middle button to the game's handler of the devices and to the driver");
    deviceCalls.clear();
    f10.type = SDL_EVENT_KEY_DOWN;
    f10.key.down = true;
    SDL_PushEvent(&f10);
    f10.type = SDL_EVENT_KEY_UP;
    f10.key.down = false;
    SDL_PushEvent(&f10);
    sync();
    clockPass();
    check(deviceCalls.size() == 1 && sawDevice(DeviceKind::MOUSE_BUTTON, DEVICE_MOUSE_X1, 0),
          "freed: the side button held goes up");
    button(SDL_BUTTON_LEFT, true);
    button(SDL_BUTTON_LEFT, false);
    sync();
    clockPass();

    /* The sound card's bytes go to SDL's audio, which
     * opens on the driver REGRET_AUDIO_DRIVER names, the dummy one here
     * (main): the sound stays silent. Its device takes a buffer, then waits
     * a hundred times the buffer's time (2.3 s), in which the stream holds
     * what it is given. */
    SDL_SetHint(SDL_HINT_AUDIO_DUMMY_TIMESCALE, "100");
    WindowAudio audio;
    check(!windowAudioState(audio), "no audio before the sound");
    std::vector<u8> sound(22050 * 4);
    for (std::size_t k = 0; k < sound.size(); k++) {
        sound[k] = (u8)(k * 7 + 1);
    }
    windowAudio(sound.data(), 2, 22050, true);
    const char *driver = SDL_GetCurrentAudioDriver();
    check((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) && driver && std::string(driver) == "dummy",
          std::string("the audio on the dummy driver: ") + (driver ? driver : "none"));
    /* The lead: the device's buffer and two sixtieths of a second, whole
     * frames, silence the device starts on. */
    const u32 SIXTIETHS = 22050 * 2 / 60 * 4;
    bool open = windowAudioState(audio);
    check(open && audio.lead > SIXTIETHS && audio.lead % 4 == 0 && audio.held <= audio.lead,
          "the lead of silence: " + std::to_string(audio.lead) + " bytes, " + std::to_string(audio.held) + " held");
    Clock::time_point opened = Clock::now();
    while (windowAudioState(audio) && audio.held == audio.lead && secondsSince(opened) < 5) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    u32 held = audio.held;
    check(held < audio.lead && held >= SIXTIETHS - 64, "the device took its buffer: " + std::to_string(held) + " held");
    /* Less than the lead: slower. */
    for (int i = 0; i < 10; i++) {
        clockPass();
    }
    windowAudioState(audio);
    check(audio.speed < 1 && audio.speed >= 0.997f, "slower under the lead: " + std::to_string(audio.speed));
    /* Samples in odd counts, the first's half frame and the rest: whole
     * frames, all of them. */
    u32 given = 2;
    for (u32 count = 1; given < audio.lead; count = count % 9 + 2) {
        u32 size = std::min(count * 2, audio.lead - given);
        windowAudio(sound.data() + given, size, 22050, true);
        given += size;
    }
    windowAudioState(audio);
    check(audio.held == held + audio.lead, "samples that end inside a frame: " + std::to_string(audio.held) +
                                               " held, " + std::to_string(held + audio.lead) + " expected");
    /* 8-bit samples (the depth changed, asylum.h's ASS_SetDepth): the
     * stream's 16-bit ones, twice the bytes. */
    const u32 sixteenHeld = audio.held;
    windowAudio(sound.data(), 8, 22050, false);
    windowAudioState(audio);
    check(audio.held == sixteenHeld + 16, "8-bit samples as 16-bit: " + std::to_string(audio.held) + " held, " +
                                              std::to_string(sixteenHeld + 16) + " expected");
    /* A second at once: up to twice the lead, the rest dropped. */
    windowAudio(sound.data(), (u32)sound.size(), 22050, true);
    windowAudioState(audio);
    check(audio.held == 2 * audio.lead, "no more than twice the lead: " + std::to_string(audio.held));
    /* More than the lead: faster. */
    for (int i = 0; i < 40; i++) {
        clockPass();
    }
    windowAudioState(audio);
    check(audio.speed > 1 && audio.speed <= 1.003f, "faster over the lead: " + std::to_string(audio.speed));

    /* A gamepad (SDL's virtual one): the window thread opens it, and its
     * buttons and axes go to the game's handler of the other devices at the
     * frame, an axis's last place of the frame; taken away, the game hears
     * so. The game port has no joystick. */
    std::int16_t padX = 0, padY = 0;
    std::uint8_t padButtons = 0;
    check(!inputJoystick(padX, padY, padButtons), "no joystick on the game port");
    deviceCalls.clear();
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.vendor_id = 0x1234;
    desc.product_id = 0x5678;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    SDL_JoystickID padId = SDL_AttachVirtualJoystick(&desc);
    SDL_Joystick *pad = SDL_OpenJoystick(padId);
    check(padId && pad, std::string("a virtual gamepad: ") + SDL_GetError());
    /* The window thread takes the adding, then SDL's events of the pad's
     * state after it: two syncs. */
    auto padSync = [] {
        sync();
        sync();
        clockPass();
    };
    padSync();
    SDL_SetJoystickVirtualAxis(pad, SDL_GAMEPAD_AXIS_LEFTX, -20000);
    SDL_SetJoystickVirtualAxis(pad, SDL_GAMEPAD_AXIS_LEFTY, 12345);
    SDL_SetJoystickVirtualButton(pad, SDL_GAMEPAD_BUTTON_EAST, true);
    SDL_SetJoystickVirtualButton(pad, SDL_GAMEPAD_BUTTON_NORTH, true);
    padSync();
    check(sawDevice(DeviceKind::PAD_AXIS, DEVICE_AXIS_LEFT_X, -20000) &&
              sawDevice(DeviceKind::PAD_AXIS, DEVICE_AXIS_LEFT_Y, 12345) &&
              sawDevice(DeviceKind::PAD_BUTTON, DEVICE_PAD_EAST, 1) &&
              sawDevice(DeviceKind::PAD_BUTTON, DEVICE_PAD_NORTH, 1),
          "the stick and the east and north buttons");
    deviceCalls.clear();
    SDL_SetJoystickVirtualButton(pad, SDL_GAMEPAD_BUTTON_EAST, false);
    SDL_SetJoystickVirtualButton(pad, SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, true);
    SDL_SetJoystickVirtualAxis(pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
    padSync();
    check(sawDevice(DeviceKind::PAD_BUTTON, DEVICE_PAD_EAST, 0) &&
              sawDevice(DeviceKind::PAD_BUTTON, DEVICE_PAD_LEFT_PADDLE1, 1) &&
              sawDevice(DeviceKind::PAD_AXIS, DEVICE_AXIS_RIGHT_TRIGGER, 32767),
          "the east button up, a paddle and the right trigger");
    deviceCalls.clear();
    for (int i = 1; i <= 3; i++) {
        SDL_SetJoystickVirtualAxis(pad, SDL_GAMEPAD_AXIS_RIGHTX, (Sint16)(i * 10000));
        windowCall([] { SDL_UpdateJoysticks(); });
    }
    padSync();
    int rightX = 0;
    for (const DeviceEvent &event : deviceCalls) {
        rightX += event.kind == DeviceKind::PAD_AXIS && event.which == DEVICE_AXIS_RIGHT_X;
    }
    check(rightX == 1 && sawDevice(DeviceKind::PAD_AXIS, DEVICE_AXIS_RIGHT_X, 30000),
          "an axis's motion of a frame: its last place, " + std::to_string(rightX) + " events");
    deviceCalls.clear();
    SDL_CloseJoystick(pad);
    SDL_DetachVirtualJoystick(padId);
    padSync();
    check(sawDevice(DeviceKind::PAD_GONE, 0, 0), "the gamepad taken away");
    inputDevices(nullptr);

    /* The window thread held (as its host holds it while the window moves,
     * or a covered window's showing): the game's frames and time go on,
     * thirty of them in half a second, and the window shows the last
     * picture when it is free. */
    std::thread hold([] { windowCall([] { std::this_thread::sleep_for(std::chrono::milliseconds(700)); }); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    wall = Clock::now();
    start = pcTime();
    for (int i = 0; i < 30; i++) {
        clockPass();
    }
    took = secondsSince(wall);
    check(virtualSince(start) > 0.49 && took < 0.65,
          "the window thread held: thirty frames in " + std::to_string(took) + " s");
    hold.join();

    /* The window closed: the game's end (the test's) at its next frame. */
    SDL_Event quit{};
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
    sync();
    check(!closed, "the window closed: nothing before the frame");
    clockPass();
    check(closed, "and the game's end at it");

    /* The window's icon (largestIconPng): of an icon file's pictures the
     * largest PNG one, past a smaller PNG and a bitmap, which SDL reads; none
     * in a file cut short or that is no icon file. */
    const std::vector<u8> iconGrey(48 * 48 * 3, 0x80);
    const std::vector<u8> iconSmall = pngBytesRgb(iconGrey.data(), 16, 16);
    const std::vector<u8> iconLarge = pngBytesRgb(iconGrey.data(), 48, 48);
    const std::vector<u8> iconBitmap(40, 0);
    const std::vector<std::pair<u8, const std::vector<u8> *>> iconPictures = {
        {16, &iconSmall}, {32, &iconBitmap}, {48, &iconLarge}};
    std::vector<u8> iconFile = {0, 0, 1, 0, 3, 0};
    std::size_t iconAt = 6 + (3 * 16);
    for (const auto &[width, bytes] : iconPictures) {
        iconFile.insert(iconFile.end(), {width, width, 0, 0, 1, 0, 32, 0});
        for (const std::size_t value : {bytes->size(), iconAt}) {
            iconFile.insert(iconFile.end(), {static_cast<u8>(value), static_cast<u8>(value >> 8),
                                             static_cast<u8>(value >> 16), static_cast<u8>(value >> 24)});
        }
        iconAt += bytes->size();
    }
    for (const auto &[width, bytes] : iconPictures) {
        iconFile.insert(iconFile.end(), bytes->begin(), bytes->end());
    }
    const std::optional<IconPicture> iconPicture = largestIconPng(iconFile);
    check(iconPicture && iconPicture->offset == 6 + (3 * 16) + iconSmall.size() + iconBitmap.size() &&
              iconPicture->size == iconLarge.size(),
          "the icon: the largest PNG picture");
    SDL_Surface *iconSurface =
        iconPicture ? SDL_LoadPNG_IO(SDL_IOFromConstMem(&iconFile[iconPicture->offset], iconPicture->size), true)
                    : nullptr;
    check(iconSurface != nullptr && iconSurface->w == 48 && iconSurface->h == 48, "the icon's picture read by SDL");
    SDL_DestroySurface(iconSurface);
    iconFile.resize(iconFile.size() - 1);
    check(largestIconPng(iconFile) && largestIconPng(iconFile)->size == iconSmall.size(),
          "the icon cut short: its largest whole PNG picture");
    check(!largestIconPng({0, 0, 2, 0, 0, 0}), "no icon file: none");

    /* The recording: inputs.tsv's lines, written out at the next frame. */
    clockPass();
    std::ifstream recorded("window_recording/inputs.tsv");
    std::string line, inputs;
    while (std::getline(recorded, line)) {
        inputs += line.substr(line.find('\t') + 1) + "\n";
    }
    for (const char *wanted : {"key\t1e\nrelease\t1e\nkey\te0\nkey\t48\nkey\te0\nrelease\t48\n", "motion\t3\t16\t0\n",
                               "axis\trightx\t30000\n", "padgone\n", "close\n", "pass\t"}) {
        check(inputs.find(wanted) != std::string::npos,
              std::string("recorded: ") + wanted + " (" + std::to_string(inputs.size()) + " bytes)");
    }

    std::printf("window: %d checks, %d mismatches\n", checks, failures);
    return failures ? 1 : 0;
}

}

int main(int argc, char *argv[])
{
    /* SDL's dummy driver, which needs neither a display nor OpenGL (the
     * offscreen driver makes its windows with EGL, which macOS has not),
     * and SDL's own renderer, which draws on the window's surface. */
    setenv("SDL_VIDEO_DRIVER", "dummy", 0);
    setenv("SDL_RENDER_DRIVER", "software", 0);
    /* No sound from the tests: a run's own choice is SDL's driver, which
     * plays on the desktop's device (window.h). */
    setenv("REGRET_AUDIO_DRIVER", "dummy", 1);
    unsetenv("REGRET_SCRIPT");
    if (argc > 1 && std::string(argv[1]) == "--exit") {
        return windowMain(exitRun, argc, argv);
    }
    return windowMain(checkWindow, argc, argv);
}
