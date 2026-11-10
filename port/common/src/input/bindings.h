/* The bindings of the game's controls: which inputs (the keyboard's keys, the
 * mouse's buttons, wheel and motion, the gamepad's buttons and sticks) give
 * which action.
 *
 * Every action has its original's inputs (each game's bindings_game.cpp), the
 * first of which is the action's own: the code that acts on the controls (the
 * controller, control.cpp, and the world's gump, world.cpp) receives an action
 * as one of its original keys and finds the action from it (controlAction,
 * worldAction), so that it branches on the actions; what it does then is the
 * original's. The bindings that enhanced.cfg gives (bind_<name>=<input>
 * <input>..., enhanced.h) take the
 * place of an action's inputs: a key the player presses is handed on as the
 * key of the action it is bound to (controlKey for the controller, worldCode
 * for the world's gump), a key that was an action's and is bound to none no
 * longer comes, and every other key comes as it is. The keys the game makes
 * itself (the mouse's, the joystick's) are actions' keys already and do not go
 * through the bindings. Without bindings in enhanced.cfg every input gives
 * what it gave in the original.
 *
 * The modes are actions held down: run (Shift; Caps Lock turns it round),
 * the combat step (Alt) and the roll (Ctrl), the original's modifier keys.
 * The combinations stay the game's (the combat step with a turn is a side
 * step, control.cpp): a key the controller receives carries the modes held
 * as the modifier bits of its shift state, Ctrl also for an action whose
 * original inputs hold it (ctrl+r).
 *
 * An input written in enhanced.cfg:
 * - a key of the controller's actions: [shift+][ctrl+][alt+]<key>[@numlock |
 *   @nonumlock], the modifiers it needs held, and for the keypad's keys the
 *   Num Lock state it needs; <key> is a name of the PC keyboard's keys
 *   (bindings.cpp's keyNames: a, 1, f1, space, kp8, up, delete, lshift ...).
 * - a code of the world's actions (the gumps' key codes, newgump.h): a
 *   printable character (u, U, ?), esc, enter, tab, backspace, space, or
 *   [shift+|ctrl+|alt+]<key> for a key without a character or with a
 *   modifier (f4, alt+x, ctrl+d).
 * - a mode: shift, ctrl or alt (either key of the two), capslock, numlock or
 *   scrolllock (the lock's state), or a key held.
 * - in all of them, an input of the other devices (Device, deviceName):
 *   mouse_left, pad_a, lstick_up ... (a mode not the wheel nor the mouse's
 *   motion, which are not held).
 *
 * The other devices: their inputs give an action in the
 * world, or in the world's selection of items (the selection's actions),
 * each with its own bindings. An action the original's mouse did comes from
 * the mouse's inputs in the mouse's way (world.cpp: the fire, the walk, the
 * turn), and a stick's turn by its lean (devices.cpp); the others come as
 * their actions' keys (to the controller) or codes (to the world's gump),
 * and the run's lock goes round at each press of an input that is not a
 * lock. */
#ifndef PORT_BINDINGS_H
#define PORT_BINDINGS_H
#include "common.h"
#include "keyboard.h"

#include <cstddef>
#include <string>
#include <vector>

/* The actions of both games; each game's bindings_game.cpp names the ones it
 * has. */
enum class Action : u8 {
    NONE,
    /* The modes. */
    RUN,
    RUN_LOCK,
    COMBAT_STEP,
    ROLL,
    /* The controller's moves (Controller::move): the grey keys, the keypad's
     * eight directions without Num Lock and its keys with Num Lock. */
    MOVE_FORWARD,
    MOVE_BACK,
    TURN_LEFT,
    TURN_RIGHT,
    ADVANCE,
    RETREAT,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    ROLL_LEFT,
    ROLL_RIGHT,
    /* No Regret's roll forward, which the roll's key with the key ahead
     * makes, on an input of its own (none by default). */
    ROLL_FORWARD,
    JUMP_FORWARD,
    KNEEL,
    MOVE_NORTHWEST,
    MOVE_SOUTHEAST,
    MOVE_SOUTHWEST,
    MOVE_NORTHEAST,
    MOVE_WEST,
    MOVE_NORTH,
    MOVE_SOUTH,
    MOVE_EAST,
    NUMPAD_FORWARD,
    NUMPAD_BACK,
    NUMPAD_TURN_LEFT,
    NUMPAD_TURN_RIGHT,
    NUMPAD_SLIDE_LEFT,
    NUMPAD_SLIDE_RIGHT,
    NUMPAD_ROLL_LEFT,
    NUMPAD_ROLL_RIGHT,
    /* The controller's other keys (handleKey, Controller::event). */
    FIRE,
    JUMP,
    /* The weapon readied, or put away in the combat stance: the original's
     * double click of the right button (world.cpp's readyWeapon), which no
     * key had; the controller gets it with the key pressed. */
    READY_WEAPON,
    KEYPAD_USE,
    KEYPAD_NEXT_WEAPON,
    KEYPAD_NEXT_ITEM,
    KEYPAD_SELECT,
    CAMERA,
    RETURN_CONTROL,
    MUSIC_LOUDER,
    MUSIC_QUIETER,
    RETICLE_STYLE,
    SELECT_WEAPON_1,
    SELECT_WEAPON_2,
    SELECT_WEAPON_3,
    SELECT_WEAPON_4,
    SELECT_WEAPON_5,
    SELECT_WEAPON_6,
    SELECT_WEAPON_7,
    SELECT_WEAPON_8,
    SELECT_WEAPON_9,
    SELECT_WEAPON_10,
    SELECT_WEAPON_11,
    SELECT_WEAPON_12,
    /* The world's gump's keys (WorldGump::handleKeyboard). */
    QUIT,
    CANCEL,
    MENU,
    HELP,
    OPTIONS,
    QUICK_LOAD,
    QUICK_SAVE,
    LOAD,
    SAVE,
    DETONATE,
    USE_ENERGY_CUBE,
    USE_MEDKIT,
    USE_VIR_IMAGER,
    USE_MINE_DETECTOR,
    USE_RADIATION_SHIELD,
    TARGETING_RETICLE,
    DATALINK,
    DROP_WEAPON,
    SHOW_POSITION,
    CALIBRATE_JOYSTICK,
    NEXT_SONG,
    VERSION,
    PREVIOUS_WEAPON,
    NEXT_WEAPON,
    USE_ITEM,
    PREVIOUS_ITEM,
    NEXT_ITEM,
    SELECT_ITEMS,
    GRAB_ITEMS,
    /* The keys of the world's gump while it selects items
     * (WorldGump::handleKeyboard, selectItems). */
    SELECTION_PREVIOUS,
    SELECTION_NEXT,
    SELECTION_CHOOSE,
    SELECTION_USE,
    SELECTION_CANCEL,
    /* The port's own (platform/input.h's host actions), which the window
     * takes before the game: the rewind held, the state's save and load,
     * the full screen, a watched replay taken over, the fast forward held,
     * the pause on or off. */
    REWIND,
    SAVE_STATE,
    LOAD_STATE,
    FULLSCREEN,
    TAKE_OVER,
    FAST_FORWARD,
    PAUSE,
    COUNT
};

/* The inputs of the other devices: the mouse's left and right buttons and
 * their double clicks as the world's gump gets them, its middle and side
 * buttons, wheel and motion across as the window sends them, the gamepad's
 * buttons (input.h's order: SDL's standard gamepad, an Xbox pad's places),
 * triggers and sticks (a stick's four directions, the one it leans most). */
enum class Device : u8 {
    NONE,
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_LEFT_DOUBLE,
    MOUSE_RIGHT_DOUBLE,
    MOUSE_MIDDLE,
    MOUSE_X1,
    MOUSE_X2,
    WHEEL_UP,
    WHEEL_DOWN,
    MOUSE_MOVE_LEFT,
    MOUSE_MOVE_RIGHT,
    PAD_A,
    PAD_B,
    PAD_X,
    PAD_Y,
    PAD_BACK,
    PAD_GUIDE,
    PAD_START,
    PAD_LS,
    PAD_RS,
    PAD_LB,
    PAD_RB,
    PAD_UP,
    PAD_DOWN,
    PAD_LEFT,
    PAD_RIGHT,
    PAD_MISC,
    PAD_PADDLE1,
    PAD_PADDLE2,
    PAD_PADDLE3,
    PAD_PADDLE4,
    PAD_TOUCHPAD,
    PAD_LT,
    PAD_RT,
    LSTICK_UP,
    LSTICK_DOWN,
    LSTICK_LEFT,
    LSTICK_RIGHT,
    RSTICK_UP,
    RSTICK_DOWN,
    RSTICK_LEFT,
    RSTICK_RIGHT,
    COUNT
};
/* A device's input's name in enhanced.cfg; nullptr for NONE. */
const char *deviceName(Device device);

/* An action's inputs in a game, as enhanced.cfg writes them. */
struct ActionDefault {
    Action action;
    const char *inputs;
};

/* The keys' presets: every action's inputs the original's, or the modern
 * layout's (enhanced.cfg's keys=, before its bind_ lines). */
enum class KeyPreset : u8 { ORIGINAL, MODERN };
/* Every action's inputs to the preset's; the bind_ lines after it change
 * single actions. */
void setKeyPreset(KeyPreset chosen);
KeyPreset keyPreset();
/* A preset's name in enhanced.cfg (original, modern), and the preset of a
 * name: false for one it does not know. */
const char *keyPresetName(KeyPreset named);
bool keyPresetNamed(const char *name, KeyPreset *named);

/* An action's name in enhanced.cfg (bind_<name>); nullptr for NONE. */
const char *actionName(Action action);
/* The action of a name, Action::NONE for none. */
Action actionNamed(const char *name);
/* Whether the game has the action (its bindings_game.cpp names it). */
bool gameHasAction(Action action);
/* Whether the action is one of the port's debug features (host.h's
 * hostDebugFeatures): the host actions but the full screen. Without them
 * the window does not take their inputs and the key settings do not show
 * them. */
[[nodiscard]] bool debugAction(Action action);

/* Sets an action's inputs from enhanced.cfg's text; false (and the inputs
 * left as they were) for an action the game has not or a text it cannot
 * read (more than 16 inputs too). */
bool bindAction(Action action, const char *text);
/* An action's inputs as enhanced.cfg writes them. */
std::string boundText(Action action);
/* Every action's inputs back to the preset's. */
void resetBindings();
/* The bindings made from the game's tables, the host actions' keys given
 * to the window (platform/input.h), before enhanced.cfg changes them. */
void loadBindings();

/* The bindings' lines of the port's enhanced.cfg (enhanced.h): the keys'
 * preset (keys=), whether the mouse rolls (mouse_roll=1 or 0: mouseRoll)
 * and every action the game has with its inputs (bind_<name>=), in the
 * order of GAME_ACTION_DEFAULTS, a line each ending in CR LF. */
[[nodiscard]] std::string bindingsText();
/* The bindings set from such a text, line by line: keys= sets the preset
 * (its bind_ lines come after it), mouse_roll= whether the mouse rolls (a
 * text without it leaves it: on by default), bind_ an action's inputs;
 * comments (# or /) and other keys are passed over. Returns the lines of either kind
 * the game cannot read, which it leaves. */
[[nodiscard]] std::vector<std::string> readBindingsText(const std::string &text);

/* A key the player pressed, for the controller: the key of the action it is
 * bound to (action), or the key as it is (Action::NONE); false when it is
 * an action's key bound to nothing, which does not come. The key's shift
 * state carries the modes held (and the Ctrl of an action that needs it)
 * as its modifier bits. */
struct ControlKey {
    Action action;
    KeyEvent key;
};
bool controlKey(const KeyEvent &pressed, ControlKey *result);
/* The action of a key the controller receives (an action's key, or a key of
 * the game's own: the mouse's, the joystick's), Action::NONE for one that
 * is no action's. */
Action controlAction(const KeyEvent *key);
/* Whether a key the controller receives comes with a mode held (run,
 * run_lock, combat_step, roll): its modifier bits (Shift, Ctrl, Alt) and
 * Caps Lock, as controlKey and modeShift set them. */
bool keyMode(const KeyEvent *key, Action mode);
/* The shift state with the modes held as its modifier bits, for the keys
 * the game makes from the mouse and the joystick (KeyEvent::shift). */
u16 modeShift(u16 shift);
/* Whether the mouse's motion and the sticks roll the NPC and step it aside
 * as they turn it with the roll's and the combat step's modes held (Ctrl
 * and Alt), as the original's mouse (the default), or only turn it: the key
 * settings' MOUSE ROLL, a setting the player chose (platform/gamestate.h's
 * gameSetting), enhanced.cfg's mouse_roll= (bindingsText). */
void setMouseRoll(bool rolls);
[[nodiscard]] bool mouseRoll();
/* The shift state of a turn's key that the mouse's motion or a stick makes:
 * with all the modes held (modeShift), or, when the mouse does not roll
 * (mouseRoll), without the roll's and the combat step's, which would make
 * the turn a roll or a step aside (control.cpp chooseMove). */
u16 deviceTurnShift(u16 shift);
/* How long an action's keys have been held: the most repeats of any of them
 * (keyRepeats), 0 when none is down. */
u16 actionHeld(Action action);

/* A key code the world's gump gets from a key the player pressed
 * (GumpDispatcher): the code of the action it is bound to, or the code as it
 * is; false when it is an action's code bound to nothing. */
bool worldCode(i16 code, i16 *result);
/* The action of a key code the world's gump receives, Action::NONE for one
 * that is no action's. */
Action worldAction(i16 code);
/* The same for the world's gump while it selects items: a key code the
 * selection's bindings give it (the dispatcher's), and the selection's
 * action of a code. */
bool selectionCode(i16 code, i16 *result);
Action selectionAction(i16 code);

/* The action an input of the other devices gives in the world, or in the
 * selection of items (selecting): the controller's, the world's or the
 * selection's (not a mode's), Action::NONE for none. */
Action deviceAction(Device device, bool selecting);
/* Whether the input is one of the run's lock's, which goes round at its
 * press (the lock's state turned round, runLocked). */
bool runLockInput(Device device);
void toggleRunLock();
/* The key of an action for the controller as an input of the other devices
 * gives it: the action's original key (the first key of its inputs), with
 * the modes held and the modifiers the key needs as its shift state (the
 * kernel's keyShift); an action without a key (READY_WEAPON) has scan code
 * 0. */
KeyEvent actionKey(Action action, u8 pressed, u16 repeat);
/* The key code of a world's or a selection's action: its original's first. */
i16 actionCode(Action action);
/* Whether the action comes to the controller (a key), to the world's gump
 * (a code), or is a mode's. */
bool controlActionOf(Action action);
bool codeActionOf(Action action);

/* The key settings (keysgump.h): inputs added to an action's, from a text
 * of enhanced.cfg's inputs: one the action has already stays once, and an
 * action that gives the same with it loses it (one of the same domain; for
 * a key of the controller's, the window's too; for an input of the other
 * devices, one of the world's, the selection's or the modes' as
 * deviceAction gives them). False, and nothing changed, for a text that is
 * not of the action's domain, or when the action would have more than 16
 * (the key settings are a setting of bytes). clearInputs leaves the action
 * none. */
bool addInputs(Action action, const char *text);
void clearInputs(Action action);
/* The key settings' wait for an action's next input. While it waits, the
 * keys pressed and let go (the dispatcher's, captureKey) and the other
 * devices' inputs (devices.cpp's and the gump's mouse buttons,
 * captureDevice) come to it and not to the game, and the window gives the
 * host actions' keys to the game as well (they are taken from it). The
 * first input of the action's domain ends the wait and is added (addInputs);
 * Esc ends it with the action's inputs cleared; a modifier let go with no
 * key between is the input itself (a mode's shift, ctrl or alt; the
 * controller's and the window's lshift ...; none for a code); the inputs of
 * another domain are passed over. stopCapture ends it with nothing. */
void startCapture(Action action);
void stopCapture();
/* The action waited for, Action::NONE when none. */
Action capturingAction();
/* A key or an input of the other devices the wait may take: true when it
 * did (the game does not get it). */
bool captureKey(const KeyEvent &key);
bool captureDevice(Device device);
/* The inputs of an action's domain a key pressed with the shift state gives,
 * as enhanced.cfg writes them: the controller's and the window's key with
 * the modifiers held as its prefixes; a mode's key alone; for the world's
 * and the selection's, the code the dispatcher makes of the key (keyCode),
 * and of a character without Ctrl or Alt both its cases ("u U", ", <").
 * Empty for a key without a name. */
[[nodiscard]] std::string keyInputText(Action action, u8 scan, u8 extended, u16 shift);

/* The game's actions' original inputs and the modern layout's
 * (bindings_game.h). */
#include "bindings_game.h"
#endif
