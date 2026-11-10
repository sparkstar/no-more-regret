/* No Regret's controls as the original binds them (bindings.h): the
 * controller's keys (control.cpp: Controller::move, handleKey,
 * Controller::event) and the world's gump's (world.cpp:
 * WorldGump::handleKeyboard), in the order enhanced.cfg writes them; the
 * mouse's buttons and motion as the original's world's gump has them
 * (WorldGump::handleLeftButton, handleRightButton, handleMouseMotion), and
 * for the inputs it had not (the wheel, the gamepad) the port's. The
 * cheats' and the
 * debugger's keys are not bound. */
#include "bindings.h"

#include <iterator>

const ActionDefault GAME_ACTION_DEFAULTS[] = {
    {Action::RUN, "shift"},
    {Action::RUN_LOCK, "capslock pad_ls"},
    {Action::COMBAT_STEP, "alt pad_lt"},
    {Action::ROLL, "ctrl pad_b"},
    {Action::MOVE_FORWARD, "up mouse_right lstick_up"},
    {Action::MOVE_BACK, "down lstick_down"},
    {Action::TURN_LEFT, "left mouse_move_left rstick_left"},
    {Action::TURN_RIGHT, "right mouse_move_right rstick_right"},
    {Action::ADVANCE, "home"},
    {Action::RETREAT, "end"},
    {Action::SLIDE_LEFT, "delete lstick_left"},
    {Action::SLIDE_RIGHT, "pagedown lstick_right"},
    {Action::ROLL_LEFT, "insert"},
    {Action::ROLL_RIGHT, "pageup"},
    /* The port's: the roll forward on an input of its own, none by default
     * (the roll's key with the key ahead makes it too). */
    {Action::ROLL_FORWARD, ""},
    {Action::JUMP_FORWARD, "kpslash"},
    {Action::KNEEL, "kp5 pad_down"},
    {Action::MOVE_NORTHWEST, "kp8@nonumlock"},
    {Action::MOVE_SOUTHEAST, "kp2@nonumlock"},
    {Action::MOVE_SOUTHWEST, "kp4@nonumlock"},
    {Action::MOVE_NORTHEAST, "kp6@nonumlock"},
    {Action::MOVE_WEST, "kp7@nonumlock"},
    {Action::MOVE_NORTH, "kp9@nonumlock"},
    {Action::MOVE_SOUTH, "kp1@nonumlock"},
    {Action::MOVE_EAST, "kp3@nonumlock"},
    {Action::NUMPAD_FORWARD, "kp8@numlock"},
    {Action::NUMPAD_BACK, "kp2@numlock"},
    {Action::NUMPAD_TURN_LEFT, "kp4@numlock"},
    {Action::NUMPAD_TURN_RIGHT, "kp6@numlock"},
    {Action::NUMPAD_SLIDE_LEFT, "kp7@numlock"},
    {Action::NUMPAD_SLIDE_RIGHT, "kp9@numlock"},
    {Action::NUMPAD_ROLL_LEFT, "kp1@numlock"},
    {Action::NUMPAD_ROLL_RIGHT, "kp3@numlock"},
    {Action::FIRE, "space kp0 mouse_left mouse_left_double pad_rt"},
    {Action::JUMP, "j tab pad_a"},
    {Action::READY_WEAPON, "mouse_right_double pad_y"},
    {Action::KEYPAD_USE, "kpdot"},
    {Action::KEYPAD_NEXT_WEAPON, "kpstar"},
    {Action::KEYPAD_NEXT_ITEM, "kpminus"},
    {Action::KEYPAD_SELECT, "kpplus"},
    {Action::CAMERA, "z"},
    {Action::RETURN_CONTROL, "esc"},
    {Action::MUSIC_LOUDER, "ctrl+equals"},
    {Action::MUSIC_QUIETER, "ctrl+minus"},
    {Action::RETICLE_STYLE, "ctrl+r"},
    {Action::SELECT_WEAPON_1, "1"},
    {Action::SELECT_WEAPON_2, "2"},
    {Action::SELECT_WEAPON_3, "3"},
    {Action::SELECT_WEAPON_4, "4"},
    {Action::SELECT_WEAPON_5, "5"},
    {Action::SELECT_WEAPON_6, "6"},
    {Action::SELECT_WEAPON_7, "7"},
    {Action::SELECT_WEAPON_8, "8"},
    {Action::SELECT_WEAPON_9, "9"},
    {Action::SELECT_WEAPON_10, "0"},
    {Action::SELECT_WEAPON_11, "minus"},
    {Action::SELECT_WEAPON_12, "equals"},
    {Action::QUIT, "alt+x"},
    {Action::CANCEL, "esc ctrl+esc alt+esc"},
    {Action::MENU, "f1 alt+f1 ctrl+f1 pad_start"},
    {Action::HELP, "f3"},
    {Action::OPTIONS, "f2 pad_back"},
    {Action::QUICK_LOAD, "f4"},
    {Action::QUICK_SAVE, "f5"},
    {Action::LOAD, "f8"},
    {Action::SAVE, "f9"},
    {Action::DETONATE, "b B"},
    {Action::USE_ENERGY_CUBE, "e E"},
    {Action::USE_MEDKIT, "m M pad_up"},
    {Action::USE_VIR_IMAGER, "v V"},
    {Action::USE_MINE_DETECTOR, "x X"},
    {Action::USE_RADIATION_SHIELD, "r R"},
    {Action::DATALINK, "ctrl+d"},
    {Action::DROP_WEAPON, "ctrl+l"},
    {Action::CALIBRATE_JOYSTICK, "alt+c"},
    {Action::NEXT_SONG, "ctrl+m"},
    {Action::VERSION, "ctrl+v"},
    {Action::PREVIOUS_WEAPON, "q Q wheel_up pad_lb"},
    {Action::NEXT_WEAPON, "w W wheel_down pad_rb"},
    {Action::USE_ITEM, "u U pad_x"},
    {Action::PREVIOUS_ITEM, "i I pad_left"},
    {Action::NEXT_ITEM, "o O pad_right"},
    {Action::SELECT_ITEMS, "s S"},
    {Action::GRAB_ITEMS, "g G"},
    {Action::SELECTION_PREVIOUS, ", < wheel_up pad_left lstick_left"},
    {Action::SELECTION_NEXT, ". > s S wheel_down pad_right lstick_right"},
    {Action::SELECTION_CHOOSE, "enter space insert pad_a"},
    {Action::SELECTION_USE, "u U pad_x"},
    {Action::SELECTION_CANCEL, "esc up left right down pad_b"},
    /* The port's own: keys the game has not. */
    {Action::REWIND, "f11"},
    {Action::SAVE_STATE, "f12"},
    {Action::LOAD_STATE, "shift+f12"},
    {Action::FULLSCREEN, "alt+enter"},
    {Action::TAKE_OVER, "ctrl+f11"},
    {Action::FAST_FORWARD, "f6"},
    {Action::PAUSE, "pause"},
};
const std::size_t GAME_ACTION_DEFAULT_COUNT = std::size(GAME_ACTION_DEFAULTS);

/* The modern layout: W, A, S and D move and slide, Ctrl with A and D rolls,
 * the mouse turns as the original's (with Ctrl it rolls unless the key
 * settings' MOUSE ROLL is off: bindings.h mouseRoll), the mouse fires and
 * its right button readies the weapon, Space jumps, C kneels, E selects the
 * items about (and chooses among them), F grabs them, G uses the item held,
 * H the medical kit, T the energy cube, Y the radiation shield, R readies
 * the weapon, Tab the datalink (Ctrl+D rolls); the joystick's calibration
 * (Alt+C, which C's kneeling would come with) is bound to nothing: the game
 * port has no joystick (input.h). The other actions keep the original's
 * inputs. */
const ActionDefault GAME_MODERN_BINDINGS[] = {
    {Action::MOVE_FORWARD, "up w lstick_up"},
    {Action::MOVE_BACK, "down s lstick_down"},
    {Action::SLIDE_LEFT, "delete a lstick_left"},
    {Action::SLIDE_RIGHT, "pagedown d lstick_right"},
    {Action::ROLL_LEFT, "insert ctrl+a"},
    {Action::ROLL_RIGHT, "pageup ctrl+d"},
    {Action::KNEEL, "kp5 c pad_down"},
    {Action::FIRE, "kp0 mouse_left mouse_left_double pad_rt"},
    {Action::JUMP, "j space pad_a"},
    {Action::READY_WEAPON, "r mouse_right mouse_right_double pad_y"},
    {Action::DATALINK, "tab"},
    {Action::CALIBRATE_JOYSTICK, ""},
    {Action::SELECT_ITEMS, "e E"},
    {Action::GRAB_ITEMS, "f F"},
    {Action::USE_ITEM, "u U g G pad_x"},
    {Action::USE_MEDKIT, "m M h H pad_up"},
    {Action::USE_ENERGY_CUBE, "t T"},
    {Action::USE_RADIATION_SHIELD, "y Y"},
    {Action::PREVIOUS_WEAPON, "q Q wheel_up pad_lb"},
    {Action::NEXT_WEAPON, "wheel_down pad_rb"},
    {Action::PREVIOUS_ITEM, "i I mouse_x1 pad_left"},
    {Action::NEXT_ITEM, "o O mouse_x2 pad_right"},
    {Action::SELECTION_NEXT, ". > wheel_down pad_right lstick_right"},
    {Action::SELECTION_CHOOSE, "enter insert e pad_a"},
    {Action::SELECTION_USE, "u U g G pad_x"},
};
const std::size_t GAME_MODERN_BINDING_COUNT = std::size(GAME_MODERN_BINDINGS);
