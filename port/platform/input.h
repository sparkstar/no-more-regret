/* The game's input: the keyboard, the mouse and the gamepad as the game
 * takes them. The events come at their times in the clock's time (clock.h):
 * to the handlers the game gives while it gives them, else to the
 * keyboard's own keeping (its shift state and buffer of keys, what the BIOS
 * did). The system layer gives it (port/system's keyboard.cpp, mouse.cpp
 * and window.cpp). Like the clock's ticks, they come while the game does not
 * hold them (clock.h). */
#ifndef PORT_PLATFORM_INPUT_H
#define PORT_PLATFORM_INPUT_H
#include <cstddef>
#include <cstdint>

/* The keyboard: the scan codes of set 1 (E0h before an extended key, its
 * make code, the make code with 80h when it goes up). */

/* The game's handler of the scan codes, or 0: the keyboard keeps them
 * itself (the BIOS's INT 9: the shift state and the buffer of keys). */
void inputKeys(void (*handler)(std::uint8_t scan));
/* The scan codes to the handler on and off (the original's mask of IRQ 1):
 * while off they wait in the keyboard. */
void inputKeysOn();
void inputKeysOff();
/* What the keyboard itself does with a scan code, for the handler to pass
 * a key on (the kernel's shift keys, Scroll Lock). */
void inputKeyboardScan(std::uint8_t scan);
/* The shift state the keyboard keeps (the BIOS's word at 0040:0017: the
 * shifts, control, alt and the locks, then the keys held), and the game's
 * own of it. */
std::uint16_t inputShiftFlags();
void inputSetShiftFlags(std::uint16_t state);
/* The shift state as INT 16h AH=12h gives it: AL the flags, AH the left and
 * right control and alt keys held, the locks held and SysRq. */
std::uint16_t inputShiftState();
/* The repeat of the key held down: its rate (0-31) and delay (0-3), as
 * INT 16h AX=0305h sets them. */
void inputTypematic(std::uint8_t rate, std::uint8_t delay);
/* Whether the keyboard's buffer has a key (conio's kbhit), and the next key
 * from it (getch): its character, or 0 and its scan code at the next call.
 * inputKey waits for one. */
bool inputKeyWaiting();
int inputKey();

/* The mouse: the pointer in the driver's coordinates (the game's 0-1278
 * across, twice the screen's pixels), its limits and the buttons (bits
 * 0-2: left, right, middle). */

/* The mouse driver reset (INT 33h AX=0, the system object's start): the
 * pointer in the middle of the virtual screen of 640x200, no handler. */
void inputMouseReset();
/* The game's handler of the mouse's events (the bits of what happened:
 * moved, then each button down and up), from a reset driver: the pointer
 * at 0,0 of the virtual screen of 640x200, and the game's speed (3 mickeys
 * across, 16 down, for 8 pixels). */
void inputMouseStart(void (*handler)(std::uint16_t events, std::uint16_t buttons, std::uint16_t x, std::uint16_t y));
/* The handler off; the mouse's events go nowhere. */
void inputMouseStop();
/* The pointer's place, inside its limits. */
void inputMousePlace(std::int16_t x, std::int16_t y);
/* The limits; the smaller of the two is the low one. */
void inputMouseXRange(std::int16_t low, std::int16_t high);
void inputMouseYRange(std::int16_t low, std::int16_t high);

/* The joystick on the PC's game port: its stick across and down (-32768 at
 * the left or top to 32767) and four buttons (bits 0-3, 1 held) as they are
 * now; false without one. The port's PC has none: the gamepad is an input
 * of the controls' bindings (the devices below). */
bool inputJoystick(std::int16_t &x, std::int16_t &y, std::uint8_t &buttons);

/* The inputs past the keyboard's and the mouse driver's: the mouse's middle
 * and side buttons and its wheel, the gamepad's buttons and axes, as they
 * came. They come at their times on the keys' line, after the scan codes
 * waiting there, one at a time, to the game's handler; without one they are
 * dropped. */
enum class DeviceKind : std::uint8_t {
    MOUSE_BUTTON, /* which: DEVICE_MOUSE_*; value 1 down, 0 up */
    WHEEL,        /* value: the notches, up above 0 */
    PAD_BUTTON,   /* which: DEVICE_PAD_*; value 1 down, 0 up */
    PAD_AXIS,     /* which: DEVICE_AXIS_*; value -32768 to 32767 (a
                   * trigger 0 to 32767), a stick's up and left below 0 */
    PAD_GONE      /* the gamepad taken away */
};
struct DeviceEvent {
    DeviceKind kind;
    std::uint8_t which;
    std::int16_t value;
    bool operator==(const DeviceEvent &) const = default;
};
const std::uint8_t DEVICE_MOUSE_MIDDLE = 0;
const std::uint8_t DEVICE_MOUSE_X1 = 1;
const std::uint8_t DEVICE_MOUSE_X2 = 2;
/* The standard gamepad's buttons (SDL's, the places of an Xbox pad's),
 * the face's south, east, west and north first. */
const std::uint8_t DEVICE_PAD_SOUTH = 0;
const std::uint8_t DEVICE_PAD_EAST = 1;
const std::uint8_t DEVICE_PAD_WEST = 2;
const std::uint8_t DEVICE_PAD_NORTH = 3;
const std::uint8_t DEVICE_PAD_BACK = 4;
const std::uint8_t DEVICE_PAD_GUIDE = 5;
const std::uint8_t DEVICE_PAD_START = 6;
const std::uint8_t DEVICE_PAD_LEFT_STICK = 7;
const std::uint8_t DEVICE_PAD_RIGHT_STICK = 8;
const std::uint8_t DEVICE_PAD_LEFT_SHOULDER = 9;
const std::uint8_t DEVICE_PAD_RIGHT_SHOULDER = 10;
const std::uint8_t DEVICE_PAD_DPAD_UP = 11;
const std::uint8_t DEVICE_PAD_DPAD_DOWN = 12;
const std::uint8_t DEVICE_PAD_DPAD_LEFT = 13;
const std::uint8_t DEVICE_PAD_DPAD_RIGHT = 14;
const std::uint8_t DEVICE_PAD_MISC = 15;
const std::uint8_t DEVICE_PAD_RIGHT_PADDLE1 = 16;
const std::uint8_t DEVICE_PAD_LEFT_PADDLE1 = 17;
const std::uint8_t DEVICE_PAD_RIGHT_PADDLE2 = 18;
const std::uint8_t DEVICE_PAD_LEFT_PADDLE2 = 19;
const std::uint8_t DEVICE_PAD_TOUCHPAD = 20;
const std::uint8_t DEVICE_PAD_BUTTONS = 21;
const std::uint8_t DEVICE_AXIS_LEFT_X = 0;
const std::uint8_t DEVICE_AXIS_LEFT_Y = 1;
const std::uint8_t DEVICE_AXIS_RIGHT_X = 2;
const std::uint8_t DEVICE_AXIS_RIGHT_Y = 3;
const std::uint8_t DEVICE_AXIS_LEFT_TRIGGER = 4;
const std::uint8_t DEVICE_AXIS_RIGHT_TRIGGER = 5;
const std::uint8_t DEVICE_AXES = 6;
/* The game's handler of the devices' events, or 0. */
void inputDevices(void (*handler)(const DeviceEvent &event));

/* The port's own actions, which the window takes before the game: the
 * rewind while its key is held, the state's save and load, the full screen
 * on or off, a watched replay taken over, the fast forward while its key
 * is held and the pause on or off. The game gives their keys from its
 * bindings (bindings.h's host actions) as they change; a key is the PC's
 * scan code, E0h before it or not, and the modifiers held with it (1
 * Shift, 2 Ctrl, 4 Alt), these and no others; the Pause key, which sends
 * no scan code of its own (E1h 1Dh 45h), is 45h with E0h (bindings.h's
 * "pause"). */
enum class HostAction : std::uint8_t { REWIND, SAVE_STATE, LOAD_STATE, FULLSCREEN, TAKE_OVER, FAST_FORWARD, PAUSE };
const std::size_t HOST_ACTIONS = 7;
/* Whether the host action is one of the port's debug features (host.h's
 * hostDebugFeatures), all but the full screen: without them the window
 * does not take its inputs, which go to the game. */
inline bool hostActionIsDebug(HostAction action)
{
    return action != HostAction::FULLSCREEN;
}
struct HostKey {
    std::uint8_t scan;
    bool extended;
    std::uint8_t modifiers;
    /* Or a button of the other devices, without modifiers: the mouse's
     * middle and side buttons, the gamepad's (a DeviceEvent's kind and
     * which), which the window takes before the game then. */
    bool device = false;
    DeviceKind kind = DeviceKind::MOUSE_BUTTON;
    std::uint8_t which = 0;
};
void inputHostKeys(HostAction action, const HostKey *keys, std::size_t count);
#endif
