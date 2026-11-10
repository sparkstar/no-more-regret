/* The port's display. See video.h.
 *
 * The VGA screen is 64K at A000h; the SVGA screen of VESA mode 101h is
 * svgaMemory, which view32.cpp draws on flat and whose window position it
 * keeps itself. The modes are the four VESA modes the game knows (111h's
 * 16-bit colours for the hi-colour movies), mode 13h and the text. The palette is 256 colours of 6 bits. The vertical
 * retrace comes at the mode's refresh rate in virtual time.
 *
 * With REGRET_SCREEN set, the screen is written as a PNG there when the
 * program ends, however it ends. */
#include "video.h"
#include "gamememory.h"
#include "pcstate.h"
#include "screen.h"
#include "pc.h"
#include "png.h"
#include "supervga.h"

#include <iterator>
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <vector>

/* DOS starts in 80x25 text. */
VideoState video = {3, 16};

namespace {

/* VESA 111h, 640x480 in 65536 colours: the PC's BIOS has it whichever game
 * runs. No Regret's movies set it; No Remorse's supervga.h does not name
 * it. */
const u16 VESA_HICOLOR = 0x111;

u8 vgaMemory[0x10000];
u8 svgaFramebuffer[SVGA_FRAMEBUFFER];

/* The palette: 256 colours of 6 bits. */
u8 colours[256][3];

/* The wide screen (screen.h): its mode, its width, the sides the game drew
 * last (their width, 0 for none) and the picture made of them and the
 * screen. */
std::atomic<u8> wideMode{0};
std::atomic<u16> wideWidth{WIDE_DEFAULT};
std::vector<u8> sides;
u16 sidesWidth = 0;
std::vector<u8> widePicture;

void writeScreenAtExit()
{
    if (const char *path = std::getenv("REGRET_SCREEN")) {
        writeScreen(path);
    }
}

/* The PNG at the end. */
[[maybe_unused]] const bool STARTED = [] {
    std::atexit(writeScreenAtExit);
    return true;
}();

struct ModeSize {
    u16 mode, width, height;
};
const ModeSize VESA_MODES[] = {
    {VESA_640X480, 640, 480}, {VESA_800X600, 800, 600}, {VESA_1024X768, 1024, 768}, {VESA_HICOLOR, 640, 480}};

const ModeSize *findVesaMode(u16 mode)
{
    for (const ModeSize &m : VESA_MODES) {
        if (m.mode == mode) {
            return &m;
        }
    }
    return nullptr;
}

/* A frame of the refresh in timer ticks, and its vertical retrace, two of
 * the lines: 525 lines at 60 Hz in the VESA modes, 449 at 70 Hz in the
 * VGA's. */
void refresh(std::uint64_t &frame, std::uint64_t &retrace)
{
    const bool vga = video.mode < 0x100;
    frame = PIT_HZ / (vga ? 70 : 60);
    retrace = frame * 2 / (vga ? 449 : 525);
}

}

void setTextMode(bool fiftyRows)
{
    video.mode = 3;
    video.fontHeight = fiftyRows ? 8 : 16;
}

u8 *screenVgaMemory()
{
    const SystemSide systemSide;
    return vgaMemory;
}

u8 *screenSvgaMemory()
{
    const SystemSide systemSide;
    return svgaFramebuffer;
}

u16 screenMode()
{
    const SystemSide systemSide;
    return video.mode;
}

u16 screenFontHeight()
{
    const SystemSide systemSide;
    return video.fontHeight;
}

/* The text has the 16-line font after a mode is set; the VESA modes clear
 * their memory whatever keep says, as VESA's BIOS did. */
bool screenSetMode(u16 mode, bool keep)
{
    const SystemSide systemSide;
    if (mode == 3 || mode == 0x13) {
        video.mode = mode;
        if (mode == 3) {
            video.fontHeight = 16;
        }
        if (!keep && mode == 0x13) {
            std::memset(vgaMemory, 0, sizeof(vgaMemory));
        }
        return true;
    }
    if (findVesaMode(mode) == nullptr) {
        return false;
    }
    video.mode = mode;
    std::memset(svgaFramebuffer, 0, sizeof(svgaFramebuffer));
    return true;
}

bool screenModeSize(u16 mode, u16 *width, u16 *height)
{
    const SystemSide systemSide;
    const ModeSize *size = findVesaMode(mode);
    if (size == nullptr) {
        return false;
    }
    *width = size->width;
    *height = size->height;
    return true;
}

void screenSetColors(u8 first, u16 count, const u8 *rgb)
{
    const SystemSide systemSide;
    for (u16 i = 0; i < count; i++) {
        for (int c = 0; c < 3; c++) {
            colours[static_cast<u8>(first + i)][c] = *rgb++ & 0x3f;
        }
    }
}

void screenGetColors(u8 first, u16 count, u8 *rgb)
{
    const SystemSide systemSide;
    for (u16 i = 0; i < count; i++) {
        for (int c = 0; c < 3; c++) {
            *rgb++ = colours[static_cast<u8>(first + i)][c];
        }
    }
}

namespace {

/* Whether the retrace is on at time. */
bool inRetrace(std::uint64_t time)
{
    std::uint64_t frame;
    std::uint64_t retrace;
    refresh(frame, retrace);
    return time % frame < retrace;
}

/* A look at the retrace, a tick of the interval timer later. */
bool lookAtRetrace()
{
    passTime(pcTime() + 1);
    return inRetrace(pcTime());
}

}

void screenWaitRetrace()
{
    const SystemSide systemSide;
    while (lookAtRetrace()) {
        ;
    }
    while (!lookAtRetrace()) {
        ;
    }
}

bool viewScreen(ScreenView &view)
{
    view.bits = 8;
    if (video.mode == 0x13) {
        view.pixels = vgaMemory;
        view.width = 320;
        view.height = 200;
    } else if (video.mode == VESA_640X480) {
        view.pixels = svgaFramebuffer;
        view.width = SCREEN_WIDTH;
        view.height = SCREEN_HEIGHT;
    } else if (video.mode == VESA_HICOLOR) {
        view.pixels = svgaFramebuffer;
        view.width = SCREEN_WIDTH;
        view.height = SCREEN_HEIGHT;
        view.bits = 16;
    } else {
        return false;
    }
    view.pitch = static_cast<u16>(view.width * view.bits / 8);
    for (int i = 0; i < 256; i++) {
        for (int c = 0; c < 3; c++) {
            const u8 level = colours[i][c];
            view.palette[(i * 3) + c] = static_cast<u8>(level << 2 | level >> 4);
        }
    }
    return true;
}

void screenSetWide(u8 mode)
{
    const SystemSide systemSide;
    wideMode = mode <= 2 ? mode : 0;
}

u8 screenWide()
{
    const SystemSide systemSide;
    return wideMode;
}

u16 screenWideWidth()
{
    const SystemSide systemSide;
    return wideMode != 0U ? wideWidth.load() : static_cast<u16>(SCREEN_WIDTH);
}

u16 wideFrameWidth()
{
    return wideMode != 0U ? wideWidth.load() : static_cast<u16>(SCREEN_WIDTH);
}

u8 wideModeNow()
{
    return wideMode;
}

void setWideWidth(u16 width)
{
    wideWidth = std::clamp<u16>(static_cast<u16>(width & ~1U), SCREEN_WIDTH, WIDE_MOST);
}

void screenShowSides(const u8 *pixels, u16 width)
{
    const SystemSide systemSide;
    if (pixels == nullptr || width <= SCREEN_WIDTH) {
        sidesWidth = 0;
        return;
    }
    const std::size_t side = (width - 640U) / 2;
    sides.resize(static_cast<std::size_t>(width) * SCREEN_HEIGHT);
    for (std::size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const std::size_t row = y * width;
        std::memcpy(sides.data() + row, pixels + row, side);
        std::memcpy(sides.data() + row + side + SCREEN_WIDTH, pixels + row + side + SCREEN_WIDTH, side);
    }
    sidesWidth = width;
}

bool viewWideScreen(ScreenView &view)
{
    if (!viewScreen(view)) {
        return false;
    }
    if (wideMode == 0U) {
        return true;
    }
    const u16 width = wideWidth;
    view.frameWidth = width;
    if (video.mode != VESA_640X480 || sidesWidth != width) {
        return true;
    }
    const std::size_t side = (width - 640U) / 2;
    widePicture.resize(static_cast<std::size_t>(width) * SCREEN_HEIGHT);
    for (std::size_t y = 0; y < SCREEN_HEIGHT; y++) {
        const std::size_t row = y * width;
        std::memcpy(widePicture.data() + row, sides.data() + row, side);
        std::memcpy(widePicture.data() + row + side, svgaFramebuffer + (y * SCREEN_WIDTH), SCREEN_WIDTH);
        std::memcpy(widePicture.data() + row + side + SCREEN_WIDTH, sides.data() + row + side + SCREEN_WIDTH, side);
    }
    view.pixels = widePicture.data();
    view.width = width;
    view.pitch = width;
    view.sides = static_cast<u16>(side);
    view.dimmed = wideMode == 2;
    return true;
}

bool writeScreen(const std::string &path)
{
    ScreenView view;
    if (!viewWideScreen(view)) {
        return false;
    }
    if (view.bits == 8) {
        return writePng(path, view.pixels, view.width, view.height, view.pitch, view.palette);
    }
    std::vector<u8> rgb;
    rgb.reserve(static_cast<size_t>(view.width) * view.height * 3);
    for (u16 y = 0; y < view.height; y++) {
        for (u16 x = 0; x < view.width; x++) {
            const u32 c = viewColour(view, x, y);
            rgb.insert(rgb.end(), {static_cast<u8>(c >> 16), static_cast<u8>(c >> 8), static_cast<u8>(c)});
        }
    }
    return writePngRgb(path, rgb.data(), view.width, view.height);
}

VideoMemory videoMemoryState()
{
    VideoMemory state{video, {}, {}, {}};
    state.vga.assign(std::begin(vgaMemory), std::end(vgaMemory));
    state.svga.assign(std::begin(svgaFramebuffer), std::end(svgaFramebuffer));
    const auto *colour = &colours[0][0];
    state.colours.assign(colour, colour + sizeof colours);
    return state;
}

void putVideoMemoryState(const VideoMemory &state)
{
    video = state.mode;
    std::copy(state.vga.begin(), state.vga.end(), std::begin(vgaMemory));
    std::copy(state.svga.begin(), state.svga.end(), std::begin(svgaFramebuffer));
    std::copy(state.colours.begin(), state.colours.end(), &colours[0][0]);
}

VideoPlaces videoPlaces()
{
    return {vgaMemory, sizeof vgaMemory, svgaFramebuffer, sizeof svgaFramebuffer, &colours[0][0], sizeof colours};
}

VideoState videoState()
{
    return video;
}

void putVideoState(const VideoState &state)
{
    video = state;
}
