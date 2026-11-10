/* The virtual PC's state as a value: what each part of the system layer's
 * PC keeps that the game's run goes on from (the clock and its ticks, the
 * interrupt controller, the keyboard and its controller, the mouse's
 * driver, the sound's output, the screen's memory and colours, the game's
 * open files), so that a copy is the PC at that frame and putting it back
 * puts the PC back. With the game's memory (gamememory.h) it is a snapshot
 * of the run.
 *
 * Not in it, as the host's: the wall clock the window's run keeps up with
 * (put back, it starts again from the time put back), the window, the WAV
 * and capture files, and the input the script or the replay gives, which
 * the snapshot's user puts back or not. A file the game wrote keeps what it
 * wrote: the files put back are the open ones, at their places. */
#ifndef PORT_PCSTATE_H
#define PORT_PCSTATE_H
#include "common.h"
#include "filestate.h"
#include "input.h"
#include "mousedrv.h"
#include "video.h"

#include <array>
#include <cstdint>
#include <deque>
#include <vector>

/* timer.cpp's. */
struct TimerState {
    std::uint64_t now;
    u32 divisor;
    std::uint64_t nextTick;
    void (*tickHandler)();
    std::uint64_t frames;
    std::uint64_t windowFrames;
    std::uint64_t nextWindowFrame;
};
[[nodiscard]] TimerState timerState();
void putTimerState(const TimerState &state);

/* interrupts.cpp's. */
struct InterruptState {
    bool interruptFlag;
    std::vector<bool> savedFlags;
    std::array<u8, 2> masks;
    std::array<bool, 16> requested;
    std::array<bool, 16> inService;
    bool operator==(const InterruptState &) const = default;
};
[[nodiscard]] InterruptState interruptState();
void putInterruptState(const InterruptState &state);

/* keyboard.cpp's. */
struct KeyboardState {
    std::deque<u8> sent;
    bool arriving;
    u8 arrivingScan;
    u8 typematicRate;
    u8 typematicDelay;
    u8 repeatCode;
    bool repeatExtended;
    std::uint64_t nextRepeat;
    bool afterE0;
    int pauseBytes;
    int pendingScan;
    void (*keyHandler)(u8 scan);
    std::deque<DeviceEvent> devices;
    bool deviceArriving;
    DeviceEvent arrivingDevice;
    void (*deviceHandler)(const DeviceEvent &event);
    u8 flags;
    u8 held;
    u8 more;
    std::deque<u16> stored;
    bool operator==(const KeyboardState &) const = default;
};
[[nodiscard]] KeyboardState keyboardState();
void putKeyboardState(const KeyboardState &state);

/* mouse.cpp's; its waiting events as bytes (their struct is the file's). */
struct MouseState {
    MouseDriver driver;
    void (*handler)(u16 events, u16 buttons, u16 x, u16 y);
    u16 handlerMask;
    u16 speedX;
    u16 speedY;
    s32 leftX;
    s32 leftY;
    bool present;
    void (*oldHandler)(u16 events, u16 buttons, u16 x, u16 y);
    u16 oldMask;
    std::vector<u8> pending;
    bool operator==(const MouseState &) const = default;
};
[[nodiscard]] MouseState mouseState();
void putMouseState(const MouseState &state);

/* audio.cpp's. */
struct AudioState {
    bool playing;
    bool sixteen;
    const u8 *ring;
    u32 units;
    u32 rate;
    std::uint64_t start;
    std::uint64_t taken;
    std::uint64_t stopped;
};
[[nodiscard]] AudioState audioState();
void putAudioState(const AudioState &state);

/* video.cpp's: the mode, the VGA's and the SVGA's memory, the colours. */
struct VideoMemory {
    VideoState mode;
    std::vector<u8> vga;
    std::vector<u8> svga;
    std::vector<u8> colours;
};
[[nodiscard]] VideoMemory videoMemoryState();
void putVideoMemoryState(const VideoMemory &state);

/* video.cpp's screen memory in place, which a rewind compares and writes
 * there (rewind.h), and the mode and colours beside it. */
struct VideoPlaces {
    u8 *vga;
    std::size_t vgaBytes;
    u8 *svga;
    std::size_t svgaBytes;
    u8 *colours;
    std::size_t colourBytes;
};
[[nodiscard]] VideoPlaces videoPlaces();
[[nodiscard]] VideoState videoState();
void putVideoState(const VideoState &state);

/* audio.cpp's tap of the output (a rewind's, rewind.h): while it is on, the
 * bytes the output gives the window are kept too, until taken. */
void audioTap(bool on);
[[nodiscard]] std::vector<u8> takeAudioTap();

/* All of them. */
struct PcState {
    TimerState timer;
    InterruptState interrupts;
    KeyboardState keyboard;
    MouseState mouse;
    AudioState audio;
    VideoMemory video;
    FilesState files;
};
[[nodiscard]] PcState pcState();
void putPcState(const PcState &state);
#endif
