/* Code118 (file name unknown; No Regret's 13a8, No Remorse's 1400; the
 * addresses are No Regret's), the parts the game uses: the joystick driver
 * and its settings in crusader.cfg. */
#ifndef PORT_JOYDRV_H
#define PORT_JOYDRV_H
#include <array>
#include "common.h"
#include "kprocess.h"
#include "rect.h"

/* Reads the joystick when there is one (13a8:0ae4). */
void pollJoystick();
/* 1 when a joystick answers, which it also sets joystickReady to; it says
 * which kind to the text screen (13a8:0cc0). */
u8 detectJoystick();
/* The stick's last reading (pollJoystick): the times of its two axes. */
extern i16 &stickX;
extern i16 &stickY;
/* The stick's direction (STICK_*, -1 in the middle) from its last reading
 * against the calibration's corners (13a8:0d1b). */
i16 stickDirection();
/* Sets the joystick up again from its saved settings when one is set up
 * (13a8:0000). */
void resetJoystick();
/* With a joystick in crusader.cfg's control: finds it, calibrates it when it
 * is not calibrated yet and writes crusader.cfg (13a8:0a93); No Remorse's
 * calibrates it each time. */
void setupJoystick();
/* Calibrates the joystick: with gump, in a dialog when a joystick is set up;
 * else in text mode when it is not calibrated yet (13a8:0b17). No Remorse's
 * takes no argument and calibrates in text mode each time (1400:0b13). */
void calibrateJoystick(u8 gump = 0);
extern u8 &joystickReady; /* 1480:6985: a joystick is set up */
/* The bits of the four buttons in joystickState. */
constexpr int JOYSTICK_BUTTONS = 0xf;
/* The stick's directions (JoystickDriver::direction), clockwise from up. The
 * centre is -1 in stickDirection's table and FFh in the byte. */
constexpr int STICK_UP = 0;
constexpr int STICK_UP_RIGHT = 1;
constexpr int STICK_RIGHT = 2;
constexpr int STICK_DOWN_RIGHT = 3;
constexpr int STICK_DOWN = 4;
constexpr int STICK_DOWN_LEFT = 5;
constexpr int STICK_LEFT = 6;
constexpr int STICK_UP_LEFT = 7;
constexpr int STICK_CENTRED = 0xff;
extern u16 &joystickState; /* 1480:697f: its buttons in the low bits */
/* crusader.cfg's control: 0 none, 1 a joystick, 2 a Gravis GamePad
 * (1480:6983). */
extern i16 &joystickType;
/* 1 until the joystick is calibrated; crusader.cfg's joystick line clears it
 * (1480:697a; No Regret's). */
extern u8 &joystickUncalibrated;
/* The actions of button1-button4 and of two buttons together (button1_2,
 * button1_3, button1_4, button2_3, button2_4, button3_4); -1 for none
 * (1480:6986). The numbers are each game's (JOY_KEYS and the others,
 * joydrv_game.h): No Regret's 0 fire, 1 jump, 2-4 Ctrl, Alt and Shift held,
 * 5-9 the keys u, g, s, m and e, 10 and 11 w and o; No Remorse's 0 fire, 1-3
 * Ctrl, Alt and Shift held, 4-8 the keys u, g, s, m and e, 9 q, w, i or o as
 * the stick turns left, right, up or down. crusader.cfg keeps the first ten;
 * nothing reads the rest. */
extern std::array<i16, 16> &joystickActions;
/* The calibration: the stick points left or up below joystickLow and right or
 * down above joystickHigh, each a third of the range in (joystickThird) from
 * the corners (BSS 1480:96b6). */
extern Point &joystickLow;
extern Point &joystickHigh;
extern Point &joystickThird;
/* Declared in joydrv.cpp in the original; here so that the saved games'
 * schema reaches it. */
class JoystickDriver : public Process {
public:
    explicit JoystickDriver(Loading loading) : Process(loading) {}
    u8 direction; /* STICK_* */
    u8 lastDirection;
    u8 spaceHeld; /* fire is down */
    u8 unknown35, unknown36;
    u8 up, down, left, right;
    u8 wasUp, wasDown, wasLeft, wasRight;
    u8 button1, button2, button3, button4;
    u16 lastButtons;
    u16 lastArrow; /* the scan code of the arrow held */
    i16 lastAction;

    JoystickDriver();
    void run() override;
    void cleanup() override;
    void load(File *file) override;
    void reset();
    /* Releases fire and the arrow held. */
    void releaseKeys();
};

#include "joydrv_game.h"
#endif
