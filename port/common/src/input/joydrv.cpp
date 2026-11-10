/* Code118 (file name unknown; No Regret's 13a8:0000-1183, No Remorse's
 * 1400:0000-0d76; the addresses are No Regret's), the functions both games
 * have the same: the driver process's start, end, loading and keys' release,
 * reading and finding the stick and its direction (the original's static
 * functions among them), setting up and calibrating the stick with No Regret's
 * calibration gump (joydrv_game.h), the driver's run with each game's
 * actions. See joydrv.h.
 *
 * The driver process turns the stick into arrow keys for the controller
 * and, when the top modal gump takes keys, into keys for it; the buttons do
 * the actions of joystickActions (fire, held shift states, keys of the game's
 * gumps; joydrv.h). */
#include "gamestate.h"
#include "joydrv.h"
#include "options.h"
#include "gumpshap.h"
#include "dialog.h"
#include "bindings.h"
#include "control.h"
#include "crusys.h"
#include "dispatch.h"
#include "joystick.h"
#include "kernel.h"
#include "keyboard.h"
#include "world.h"
#include "host.h"
#include "waits.h"
#include "screen.h"

constexpr int JOYSTICK_TYPE = 0x262;

i16 &unknown6981 = gameGlobal<i16>(2); /* not read */

i16 &joystickType = gameGlobal<i16>(0);

namespace {
JoystickDriver *&joystickDriver = gameGlobal<JoystickDriver *>(nullptr);
}
i16 &stickX = gameGlobal<i16>(0);
i16 &stickY = gameGlobal<i16>(0);
u16 &joystickState = gameGlobal<u16>(0);

u8 &joystickReady = gameGlobal<u8>(0);
namespace {
/* The direction of each ninth of the stick's range, by row and column. */
i16 directions[9] = {STICK_UP_LEFT, STICK_UP,        STICK_UP_RIGHT, STICK_LEFT,      -1,
                     STICK_RIGHT,   STICK_DOWN_LEFT, STICK_DOWN,     STICK_DOWN_RIGHT};
}

Point &joystickLow = gameSetting<Point>();
Point &joystickHigh = gameSetting<Point>();
Point &joystickThird = gameSetting<Point>();

namespace {
void readJoystick();
}

void resetJoystick()
{
    if (joystickType != 0) {
        if (joystickDriver == nullptr) {
            new JoystickDriver;
        } else {
            joystickDriver->reset();
        }
    }
}

JoystickDriver::JoystickDriver()
{
    nameProcess(pid, "JoystickDriver");
    setType(JOYSTICK_TYPE);
    makeSystem();
    reset();
    joystickDriver = this;
}

void JoystickDriver::reset()
{
    direction = lastDirection = lastAction = -1;
    button1 = button2 = button3 = button4 = 0;
    up = down = left = right = 0;
    wasUp = wasDown = wasLeft = wasRight = 0;
    spaceHeld = 0;
    unknown35 = unknown36 = 0;
    lastArrow = 0;
}

void JoystickDriver::load(File *file)
{
    Process::load(file);
    joystickDriver = this;
}

void JoystickDriver::cleanup()
{
    Process::cleanup();
    joystickDriver = nullptr;
}

void JoystickDriver::releaseKeys()
{
    KeyEvent key;

    key.shift = modeShift(keyShift);
    key.time = 0;
    key.pressed = 1;
    key.extended = 0;
    key.repeat = 1;
    /* The fire button's release (No Remorse's original sent it pressed
     * again). */
    if (spaceHeld != 0U) {
        key.scan = SCAN_SPACE;
        key.pressed = 0;
        controller->syntheticKey(&key);
        spaceHeld = 0;
    }
    key.pressed = 0;
    key.scan = lastArrow;
    controller->syntheticKey(&key);
}

void pollJoystick()
{
    if (joystickReady != 0U) {
        timerPeriod = timerDivisor;
        readJoystick();
    }
}

namespace {
void readJoystick()
{
    sampleJoystick();
    stickX = joystickFirstX;
    stickY = joystickFirstY;
    joystickState = joystickButtons;
}
}

u8 detectJoystick()
{
    if (joystickType == 0) {
        return 0;
    }
    readJoystick();
    if (joystickAxes != 0U) {
        debugPrintf(0xff, "\n\r\n\r\n\r");
        if (joystickType == 1) {
            debugPrintf(0xff, "Joystick detected in GamePort #1.\n\r");
        } else {
            debugPrintf(0xff, "Gravis GamePad detected in GamePort #1.\n\r");
        }
        joystickReady = 1;
    } else {
        joystickReady = 0;
    }
    return joystickReady;
}

i16 stickDirection()
{
    const i16 x = stickX / 2;
    const i16 y = stickY / 2;
    i16 ninth;

    if (x <= joystickLow.x) {
        ninth = 0;
    } else if (x >= joystickHigh.x) {
        ninth = 2;
    } else {
        ninth = 1;
    }
    if (y <= joystickLow.y) {
        ninth += 0;
    } else if (y >= joystickHigh.y) {
        ninth += 6;
    } else {
        ninth += 3;
    }
    return directions[ninth];
}

/* The actions of joystickActions (each game's, joydrv_game.h): fire, Ctrl,
 * Alt or Shift held, and from JOY_KEYS on the keys u, g, s, m and e for the
 * top modal gump; then No Regret's w and o, again each 14h ticks while held
 * (JOY_REPEATED_KEYS), or No Remorse's q, w, i or o as the stick turns left,
 * right, up or down. No Regret's action 1 jumps (JOY_JUMPS). */
constexpr int JOY_FIRE = 0;

ASSERT_SIZE(JoystickDriver, 0x49);

std::array<i16, 16> &joystickActions = gameGlobal<std::array<i16, 16>>(JOYSTICK_ACTIONS);

void JoystickDriver::run()
{
    if (joystickReady == 0U) {
        return;
    }
    WorldGump *top = static_cast<WorldGump *>(modals[modalTop]);
    bool sendKeys;
    KeyEvent key;
    u16 buttons;
    i16 action;

    pollJoystick();
    lastDirection = direction;
    wasUp = up;
    wasDown = down;
    wasLeft = left;
    wasRight = right;
    direction = stickDirection();
    up = static_cast<u8>(direction == STICK_UP_LEFT || direction == STICK_UP || direction == STICK_UP_RIGHT);
    down = static_cast<u8>(direction == STICK_DOWN_LEFT || direction == STICK_DOWN || direction == STICK_DOWN_RIGHT);
    right = static_cast<u8>(direction == STICK_UP_RIGHT || direction == STICK_RIGHT || direction == STICK_DOWN_RIGHT);
    left = static_cast<u8>(direction == STICK_UP_LEFT || direction == STICK_LEFT || direction == STICK_DOWN_LEFT);
    buttons = joystickState & JOYSTICK_BUTTONS;
    button1 = static_cast<u8>((buttons & 1) != 0);
    button2 = static_cast<u8>((buttons & 2) != 0);
    button3 = static_cast<u8>((buttons & 8) != 0);
    button4 = static_cast<u8>((buttons & 4) != 0);
    if (worldActive == 0U) {
        return;
    }
    if (((top->modes & WORLD_SELECTING) != 0) && (joystickType != 0)) {
        if ((buttons != 0U) && lastButtons == buttons) {
            setTimeout(4);
            return;
        }
        if (direction == STICK_RIGHT && lastDirection != STICK_RIGHT) {
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, '>')));
        }
        if (direction == STICK_LEFT && lastDirection != STICK_LEFT) {
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, '<')));
        }
        if (button1 != 0U) {
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, '\r')));
            setTimeout(0x1e);
        }
        if ((button2 != 0U) || (button3 != 0U) || (button4 != 0U)) {
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, ASCII_ESCAPE)));
            setTimeout(0x1e);
        }
    } else if (joystickType != 0) {
        lastButtons = buttons;
        action = -1;
        sendKeys = false;
        if ((button1 != 0U) && (button2 != 0U)) {
            action = joystickActions[4];
        } else if ((button1 != 0U) && (button3 != 0U)) {
            action = joystickActions[5];
        } else if ((button1 != 0U) && (button4 != 0U)) {
            action = joystickActions[6];
        } else if ((button2 != 0U) && (button3 != 0U)) {
            action = joystickActions[7];
        } else if ((button2 != 0U) && (button4 != 0U)) {
            action = joystickActions[8];
        } else if ((button3 != 0U) && (button4 != 0U)) {
            action = joystickActions[9];
        } else if (button1 != 0U) {
            action = joystickActions[0];
        } else if (button2 != 0U) {
            action = joystickActions[1];
        } else if (button3 != 0U) {
            action = joystickActions[2];
        } else if (button4 != 0U) {
            action = joystickActions[3];
        }
        if (action != JOY_KEYS + 5 && (!JOY_REPEATED_KEYS || action != JOY_KEYS + 6) && action >= 0 &&
            lastAction >= JOY_KEYS) {
            setTimeout(0x1e);
            return;
        }
        /* The actions' numbers are each game's (joydrv_game.h): an if chain,
         * not case labels, so that they can be values at run time. */
        if (action == JOY_KEYS) {
            releaseKeys();
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'u')));
        } else if (action == JOY_KEYS + 1) {
            releaseKeys();
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'g')));
        } else if (action == JOY_KEYS + 2) {
            releaseKeys();
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 's')));
        } else if (action == JOY_KEYS + 3) {
            releaseKeys();
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'm')));
        } else if (action == JOY_KEYS + 4) {
            releaseKeys();
            top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'e')));
        } else if (action == JOY_KEYS + 5) {
            releaseKeys();
            if (JOY_REPEATED_KEYS) {
                top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'w')));
                setTimeout(0x14);
                lastAction = action;
                return;
            }
            if (direction == STICK_LEFT && lastDirection != STICK_LEFT) {
                top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'q')));
            }
            if (direction == STICK_RIGHT && lastDirection != STICK_RIGHT) {
                top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'w')));
            }
            if (direction == STICK_UP && lastDirection != STICK_UP) {
                top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'i')));
            }
            if (direction == STICK_DOWN && lastDirection != STICK_DOWN) {
                top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'o')));
            }
        } else if (action == JOY_KEYS + 6) {
            if (JOY_REPEATED_KEYS) {
                releaseKeys();
                top->handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, 'o')));
                setTimeout(0x14);
                lastAction = action;
                return;
            }
            sendKeys = true;
        } else {
            sendKeys = true;
        }
        lastAction = action;
        if (sendKeys) {
            key.shift = modeShift(keyShift);
            key.time = 0;
            key.pressed = 1;
            key.extended = 1;
            key.repeat = 1;
            if (action == JOY_FIRE) {
                key.scan = SCAN_SPACE;
                controller->syntheticKey(&key);
                spaceHeld = 1;
            } else if (action == JOY_CONTROL) {
                if (JOY_SHIFTS_TAKE_CONTROL) {
                    if (controlledNpcNumber() != 1) {
                        setControlledNpcNumber(1);
                    }
                }
                key.shift |= KEY_LEFT_CONTROL;
            } else if (action == JOY_ALT) {
                if (JOY_SHIFTS_TAKE_CONTROL) {
                    if (controlledNpcNumber() != 1) {
                        setControlledNpcNumber(1);
                    }
                    setControlledNpcNumber(1);
                }
                key.shift |= KEY_LEFT_ALT;
            } else if (action == JOY_SHIFT) {
                key.shift |= KEY_LEFT_SHIFT;
            } else if (JOY_JUMPS && action == JOY_JUMP) {
                key.scan = SCAN_J;
                controller->syntheticKey(&key);
            }
            /* The fire button's release. No Remorse's original sent it
             * pressed, so the controller fired again instead of stopping and
             * a PL-1's automatic fire went on; the arrows after it are
             * presses again there (ARROWS_PRESSED_AFTER_FIRE), No Regret's
             * sends them released. */
            if ((spaceHeld != 0U) && (action != 0)) {
                key.scan = SCAN_SPACE;
                key.pressed = 0;
                controller->syntheticKey(&key);
                if (ARROWS_PRESSED_AFTER_FIRE) {
                    key.pressed = 1;
                }
                spaceHeld = 0;
            }
            if (direction == STICK_CENTRED && lastDirection != STICK_CENTRED) {
                key.pressed = 0;
                key.scan = lastArrow;
                controller->syntheticKey(&key);
            } else if (lastDirection != STICK_CENTRED) {
                key.repeat++;
            }
            if ((up != 0U) && left == 0 && right == 0) {
                key.scan = SCAN_UP;
                lastArrow = key.scan;
                controller->syntheticKey(&key);
            }
            if ((down != 0U) && left == 0 && right == 0) {
                key.scan = SCAN_DOWN;
                lastArrow = key.scan;
                controller->syntheticKey(&key);
            }
            if (left != 0U) {
                key.scan = SCAN_LEFT;
                lastArrow = key.scan;
                controller->syntheticKey(&key);
            }
            if (right != 0U) {
                key.scan = SCAN_RIGHT;
                lastArrow = key.scan;
                controller->syntheticKey(&key);
            }
        }
    }
    setTimeout(1);
}

/* No Regret's: calibrates the stick in a dialog, step 1 the upper left, 2
 * the lower right. */
class CalibrationGump : public DialogGump {
public:
    i16 step;
    Point low, high, third;

    CalibrationGump();
    void execute(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
};
ASSERT_SIZE(CalibrationGump, 0x55);

/* 1 until the joystick is calibrated (joydrv.h). */
u8 &joystickUncalibrated = gameSetting<u8>(1);

void setupJoystick()
{
    if (joystickType != 0) {
        if (detectJoystick() != 0U) {
            new JoystickDriver;
            calibrateJoystick(0);
            if (waitOpened()) {
                giveRest([]() {
                    if (CALIBRATE_ONCE) {
                        writeOptions();
                    }
                    return false;
                });
                return;
            }
            if (CALIBRATE_ONCE) {
                writeOptions();
            }
            return;
        }
        debugPrintf(0xff, "Error: 'JoystickDriver::init() - No joystick found!'\n\r");
        debugPrintf(0xff, "[Press any key to continue.]\n\r");
        /* The keys come as the time passes (clock.h), a wait of the
         * steps. */
        openWaitWhile([]() { return pendingKeys == 0U; }, []() { return false; });
    }
}

/* Where the calibration's loops are (the port's): calibrateJoystick's and
 * CalibrationGump::execute's loops are a wait of the steps, a turn of the
 * loop it is at a pass (each reads the joystick, whose time passes then).
 * The buttons let go, the first corner pressed (a key gives up), the buttons
 * let go, the second corner, the buttons let go; one calibration runs at a
 * time. */
enum class CalibrationLoop : u8 { LET_GO_FIRST, PRESS_LOW, LET_GO_LOW, PRESS_HIGH, LET_GO_HIGH, DONE };
namespace {
CalibrationLoop &calibration = gameGlobal<CalibrationLoop>(CalibrationLoop::DONE);
}

void calibrateJoystick(u8 gump)
{
    if (CALIBRATE_ONCE) {
        if (gump != 0U) {
            if (joystickReady != 0U) {
                openModalGump(new CalibrationGump, static_cast<u32>(-1), [](s32 /*result*/) {});
            }
            return;
        }
        if (joystickUncalibrated == 0U) {
            return;
        }
    }
    pollJoystick();
    calibration = CalibrationLoop::LET_GO_FIRST;
    openLoopFrame([]() { return calibration != CalibrationLoop::DONE; },
                  []() {
                      for (;;) {
                          switch (calibration) {
                          case CalibrationLoop::LET_GO_FIRST:
                              if ((joystickState & JOYSTICK_BUTTONS) != 0) {
                                  pollJoystick();
                                  return;
                              }
                              debugPrintf(0xff, "Please move the joystick to the upper left and hit a button.\n\r");
                              flushKeys();
                              gumpKeys.clear();
                              calibration = CalibrationLoop::PRESS_LOW;
                              break;
                          case CalibrationLoop::PRESS_LOW:
                              if ((joystickState & JOYSTICK_BUTTONS) == 0) {
                                  if (pendingKeys != 0U) {
                                      flushKeys();
                                      gumpKeys.clear();
                                      joystickReady = 0;
                                      calibration = CalibrationLoop::DONE;
                                      return;
                                  }
                                  pollJoystick();
                                  hostPrintText("X=%d Y=%d         \r", stickX / 2, stickY / 2);
                                  return;
                              }
                              pollJoystick();
                              joystickLow.x = stickX / 2;
                              joystickLow.y = stickY / 2;
                              calibration = CalibrationLoop::LET_GO_LOW;
                              break;
                          case CalibrationLoop::LET_GO_LOW:
                              if ((joystickState & JOYSTICK_BUTTONS) != 0) {
                                  pollJoystick();
                                  return;
                              }
                              debugPrintf(0xff, "Please move the joystick to the lower right and hit a button.\n\r");
                              calibration = CalibrationLoop::PRESS_HIGH;
                              break;
                          case CalibrationLoop::PRESS_HIGH:
                              if ((joystickState & JOYSTICK_BUTTONS) == 0) {
                                  if (pendingKeys != 0U) {
                                      flushKeys();
                                      gumpKeys.clear();
                                      joystickReady = 0;
                                      calibration = CalibrationLoop::DONE;
                                      return;
                                  }
                                  pollJoystick();
                                  hostPrintText("X=%d Y=%d         \r", stickX / 2, stickY / 2);
                                  return;
                              }
                              debugPrintf(0xff, "                 \r");
                              pollJoystick();
                              joystickHigh.x = stickX / 2;
                              joystickHigh.y = stickY / 2;
                              calibration = CalibrationLoop::LET_GO_HIGH;
                              break;
                          case CalibrationLoop::LET_GO_HIGH:
                              if ((joystickState & JOYSTICK_BUTTONS) != 0) {
                                  pollJoystick();
                                  return;
                              }
                              joystickThird.x = static_cast<i16>(joystickHigh.x - joystickLow.x) / 3;
                              joystickThird.y = static_cast<i16>(joystickHigh.y - joystickLow.y) / 3;
                              joystickLow.x += joystickThird.x;
                              joystickLow.y += joystickThird.y;
                              joystickHigh.x -= joystickThird.x;
                              joystickHigh.y -= joystickThird.y;
                              calibration = CalibrationLoop::DONE;
                              return;
                          case CalibrationLoop::DONE:
                              return;
                          }
                      }
                  },
                  nullptr);
}

CalibrationGump::CalibrationGump() : DialogGump(NewGumpId(), nullptr, Rect(0, 0, 400, 100), guiColors->face)
{
    freezeWorld(1);
    step = 0;
    eventMask |= EVENT_EXECUTE;
    bounds.moveTo(static_cast<i16>(SCREEN_WIDTH - RECT_WIDTH(bounds)) >> 1,
                  static_cast<i16>(SCREEN_HEIGHT - RECT_HEIGHT(bounds)) >> 2);
}

void CalibrationGump::execute(NewGumpEvent * /*unused*/)
{
    pollJoystick();
    calibration = CalibrationLoop::LET_GO_FIRST;
    /* The loops as calibrateJoystick's, the gump's steps painted and its
     * corners kept. */
    openLoopFrame([]() { return calibration != CalibrationLoop::DONE; },
                  [this]() {
                      for (;;) {
                          switch (calibration) {
                          case CalibrationLoop::LET_GO_FIRST:
                              if ((joystickState & JOYSTICK_BUTTONS) != 0) {
                                  pollJoystick();
                                  return;
                              }
                              step = 1;
                              paint(1);
                              flushKeys();
                              gumpKeys.clear();
                              calibration = CalibrationLoop::PRESS_LOW;
                              break;
                          case CalibrationLoop::PRESS_LOW:
                              if ((joystickState & JOYSTICK_BUTTONS) == 0) {
                                  if (pendingKeys != 0U) {
                                      flushKeys();
                                      gumpKeys.clear();
                                      close();
                                      thawWorld(1);
                                      calibration = CalibrationLoop::DONE;
                                      return;
                                  }
                                  pollJoystick();
                                  paint(1);
                                  return;
                              }
                              pollJoystick();
                              low.x = stickX / 2;
                              low.y = stickY / 2;
                              calibration = CalibrationLoop::LET_GO_LOW;
                              break;
                          case CalibrationLoop::LET_GO_LOW:
                              if ((joystickState & JOYSTICK_BUTTONS) != 0) {
                                  pollJoystick();
                                  return;
                              }
                              step = 2;
                              paint(1);
                              calibration = CalibrationLoop::PRESS_HIGH;
                              break;
                          case CalibrationLoop::PRESS_HIGH:
                              if ((joystickState & JOYSTICK_BUTTONS) == 0) {
                                  if (pendingKeys != 0U) {
                                      flushKeys();
                                      gumpKeys.clear();
                                      close();
                                      thawWorld(1);
                                      calibration = CalibrationLoop::DONE;
                                      return;
                                  }
                                  pollJoystick();
                                  paint(1);
                                  return;
                              }
                              pollJoystick();
                              high.x = stickX / 2;
                              high.y = stickY / 2;
                              calibration = CalibrationLoop::LET_GO_HIGH;
                              break;
                          case CalibrationLoop::LET_GO_HIGH:
                              if ((joystickState & JOYSTICK_BUTTONS) != 0) {
                                  pollJoystick();
                                  return;
                              }
                              third.x = static_cast<i16>(high.x - low.x) / 3;
                              third.y = static_cast<i16>(high.y - low.y) / 3;
                              low.x += third.x;
                              low.y += third.y;
                              high.x -= third.x;
                              high.y -= third.y;
                              joystickLow = low;
                              joystickHigh = high;
                              joystickThird = third;
                              writeOptions();
                              close();
                              thawWorld(1);
                              calibration = CalibrationLoop::DONE;
                              return;
                          case CalibrationLoop::DONE:
                              return;
                          }
                      }
                  },
                  nullptr);
}

void CalibrationGump::paintSelf(i16 x, i16 y)
{
    DialogGump::paintSelf(x, y);
    print(10, 14, "JOYSTICK CALIBRATION");
    print(10, 70, "[PRESS ANY KEY TO ABORT]");
    switch (step) {
    case 1:
        print(10, 34, "Please move the joystick to the upper left and hit a button.");
        break;
    case 2:
        /* The text screen's message, line end and all. */
        print(10, 34, "Please move the joystick to the lower right and hit a button.\n\r");
        break;
    }
    print(10, 50, "X=%d Y=%d", stickX / 2, stickY / 2);
}
