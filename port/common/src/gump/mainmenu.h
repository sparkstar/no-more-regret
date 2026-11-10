/* Code120 (file name unknown): the main menu, the options gump
 * and the difficulty dialog. */
#ifndef PORT_MAINMENU_H
#define PORT_MAINMENU_H
#include "button.h"
#include "dialog.h"
#include "gumpshap.h"
#include "newgump.h"
#include "palette.h"
#include "mainmenu_game.h"

/* The main menu's entries (its buttons' codes); runModalGump returns the
 * one that closed it: a new game, a game loaded or saved, quit (in No
 * Regret also Alt-X) or MAINMENU_RESUME (Escape). The start menu refuses save and
 * resume, so MAPJUMP.C's branch for a save never runs. */
constexpr int MAINMENU_NEW_GAME = 0;
constexpr int MAINMENU_LOAD = 1;
constexpr int MAINMENU_SAVE = 2;
constexpr int MAINMENU_OPTIONS = 3;
constexpr int MAINMENU_CREDITS = 4;
constexpr int MAINMENU_QUIT = 5;
constexpr int MAINMENU_RESUME = 6;

/* The main menu. */
class MainMenuGump : public NewGump {
public:
    /* The key daemon's rate and repeatDelay, set back when the menu closes. */
    u16 keyRate;
    u16 keyRepeatDelay;
    u16 unknown38;   /* written, never read */
    Palette palette; /* misc.pal */

    MainMenuGump();
    ~MainMenuGump() override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    /* Runs entry choice of the menu: 0 a new game (after the difficulty), 1
     * load, 2 save, 3 the options, 4 the credits; 5 and 6 (Escape, only in a
     * game) close the menu with that command. */
    void choose(i16 choice);
    /* choose's for MAINMENU_QUIT and a new game's level chosen (the port's,
     * which the level's choice goes on to after its gump has closed): tells
     * the menu's caller the choice and closes. */
    void leave(i16 choice);
};
ASSERT_SIZE(MainMenuGump, 0x41);

/* The options gump: pairs of buttons for the
 * animations, the frame skip, the movies (No Regret's colours, No Remorse's
 * size, flicVideoMode), the blasts and No Remorse's oversampling of the
 * sound, sliders for the volumes and the mouse speed. Closing it writes
 * crusader.cfg. */
class OptionsGump : public NewGump {
public:
    Palette palette; /* static\misc.pal */
    /* The volumes as steps of the two level tables, 0-22. */
    s32 soundStep;
    s32 musicStep;
    u8 frameSkipChanged; /* 1 once the frame skip changed; never read */

    /* UNCALLED: nothing calls it. */
    OptionsGump();
    ~OptionsGump() override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    /* The frame drawn around the option the keys move to (0 when none). */
    Rect focusRect(i16 focus);
    /* Carries out the button with instance id. */
    void apply(i16 optionId);
};
ASSERT_SIZE(OptionsGump, 0x44);

/* crusader.cfg's moviehicolor sets it with hiColorMovies; nothing else reads
 * it (No Regret only). */
extern i16 &movieHiColor;
/* 1 while Code7 runs the main menu before a game: the menu offers neither
 * saving nor going back to a game. */
extern u8 &atStartMenu;

/* The gumps the menus make (common/src/gump/mainmenu.cpp). */

/* One of a pair of buttons (instances n and n^1): choosing it releases the
 * other. Frame 0 shows it chosen (vtable 1480:6e74). */
class OptionButton : public NewGump, protected GumpShape {
public:
    u8 chosen;

    /* UNCALLED: nothing calls it. */
    OptionButton(u8 code, u8 on, NewGump *parentGump, GumpShape look);
    void handleLeftButton(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    void choose();
};
ASSERT_SIZE(OptionButton, 0x3a);

/* The difficulty dialog over static\diff.pal; its command is the level, 1-4,
 * or 0 (vtable 1480:6de4). */
class DifficultyGump : public DialogGump {
public:
    DifficultyGump();
    ~DifficultyGump() override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    /* Not draggable. */
    void handleLeftButton(NewGumpEvent * /*event*/) override {}
};
ASSERT_SIZE(DifficultyGump, 0x47);
/* The level the difficulty dialog's keys stand on, 1-4; its buttons and
 * the dialog's keys set it. */
extern i16 &difficultyChoice;

/* A level of the difficulty dialog: frames 0 and 1 side by side, framed in
 * DIFFICULTY_CHOSEN_COLOUR when chosen (vtable 1480:6d44). No Regret's is
 * framed in EEh when not, and plays its sound when chosen
 * (mainmenu_game.h). */
class DifficultyButton : public ButtonGump, protected GumpShape {
public:
    i16 *choice;
    /* No Regret's: the sound it plays when chosen. */
    i16 sound;

    DifficultyButton(u8 code, NewGump *parentGump, u16 buttonShape, i16 *selection, i16 buttonSound);
    /* No Remorse's, without a sound. */
    /* UNCALLED: nothing calls it. */
    DifficultyButton(u8 code, NewGump *parentGump, u16 buttonShape, i16 *selection);
    void paintSelf(i16 x, i16 y) override;
    void showPressed() override;
    void showReleased() override;
};
/* BCC's: No Regret 40h, No Remorse 3Eh (without sound). */
ASSERT_SIZE(DifficultyButton, DIFFICULTY_BUTTON_SIZE);

/* An entry of the main menu; *choice is the entry the keys stand on
 * (vtable 1480:6d94). */
class MenuButton : public ButtonGump, protected GumpShape {
public:
    i16 *choice;

    MenuButton(u8 code, NewGump *parentGump, u16 buttonShape, i16 *selection);
    void paintSelf(i16 x, i16 y) override;
    void showPressed() override;
    void showReleased() override;
};
ASSERT_SIZE(MenuButton, 0x3e);

/* An arrow of the options' sliders (vtable 1480:6cf4). */
class ArrowButton : public AutoRepeatButtonGump, protected GumpShape {
public:
    ArrowButton(u8 code, NewGump *parentGump, u16 buttonShape);
    void paintSelf(i16 x, i16 y) override;
    void showPressed() override;
    void showReleased() override;
};
ASSERT_SIZE(ArrowButton, 0x3e);

#endif
