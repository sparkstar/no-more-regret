/* Code120 (file name unknown; No Regret's 13b8:0000-33f1, No Remorse's
 * 13c8:0000-3148; the addresses are No Regret's), the functions both games
 * have the same, their values each game's mainmenu_game.h (the options gump,
 * its fifth pair and big video by XAN_MOVIE_OPTIONS). See mainmenu.h. */
#include "mainmenu.h"
#include "gamestate.h"
#include "mainmenu_game.h"
#include "asylum.h"
#include "basecam.h"
#include "button.h"
#include "control.h"
#include "credits.h"
#include "cycle.h"
#include "dialog.h"
#include "dispatch.h"
#include "dsfxman.h"
#include "dtable.h"
#include "fade.h"
#include "filespec.h"
#include "gamefile.h"
#include "gametime.h"
#include "gumpshap.h"
#include "kdaemon.h"
#include "kernel.h"
#include "loadsave.h"
#include "music.h"
#include "npc.h"
#include "options.h"
#include "settings.h"
#include "priority.h"
#include "shape.h"
#include "shapebtn.h"
#include "u9mouse.h"
#include "viewport.h"
#include "world.h"
#include "movie.h"
#include "flicplay.h"
#include "supervga.h"
#include "options_game.h"
#include "sounds.h"
#include "screen.h"

/* The open main menu; written, never read. */
MainMenuGump *&mainMenu = gameGlobal<MainMenuGump *>(nullptr);

namespace {
[[maybe_unused]] u8 unknown6cd1 = 1; /* nothing reads or writes it */
/* The entry the keys stand on. */
i16 &menuChoice = gameGlobal<i16>(0);
}

MainMenuGump::MainMenuGump() : NewGump(NewGumpId(), nullptr, 0)
{
    flushKeys();
    keyRate = keyDaemon->rate;
    keyRepeatDelay = repeatDelay;
    keyDaemon->setRate(10);
    repeatDelay = 20;
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    unknown38 = 0;
    eventMask |= EVENT_LEFT_BUTTON | EVENT_MOTION | EVENT_COMMAND;
    bounds.set(0, 0, drawViewport->right() - drawViewport->left(), drawViewport->bottom() - drawViewport->top());
    addAcceptedArguments(-1, 0);
    mainMenu = this;
    killItemProcesses(0, FADE_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    palette.load(makePath(nullptr, staticDirectory, "misc.pal", nullptr), 0);
    const i16 x = static_cast<i16>(SCREEN_WIDTH - 638) >> 1;
    const i16 y = static_cast<i16>(SCREEN_HEIGHT - 240) >> 1;
    MenuButton *button = new MenuButton(MAINMENU_NEW_GAME, this, 0x3a, &menuChoice);

    button->bounds.moveTo(x + 45, y + 50);
    button = new MenuButton(MAINMENU_LOAD, this, 0x3b, &menuChoice);
    button->bounds.moveTo(x + 45, y + 101);
    button = new MenuButton(MAINMENU_SAVE, this, 0x3c, &menuChoice);
    button->bounds.moveTo(x + 45, y + 151);
    button = new MenuButton(MAINMENU_OPTIONS, this, 0x3d, &menuChoice);
    button->bounds.moveTo(x + 446, y + 58);
    button = new MenuButton(MAINMENU_CREDITS, this, 0x3e, &menuChoice);
    button->bounds.moveTo(x + 489, y + 151);
    button = new MenuButton(MAINMENU_QUIT, this, 0x3f, &menuChoice);
    button->bounds.moveTo(x + 550, y + 198);
    pushSong(0x12);
    palette.apply();
    paint(1);
    gameMouse->resetShape(MENU_POINTER);
    gameMouse->show();
    gumpKeys.clear();
    if (MAIN_MENU_PLAYS_SOUND) {
        playEffect(MAIN_MENU_SOUND);
    }
}

/* The four quarters centred, MAIN_MENU_HEIGHT high (No Regret's 330, No
 * Remorse's 240). */
void MainMenuGump::paintSelf(i16 /*x*/, i16 /*y*/)
{
    drawViewport->clear(0);
    const GumpShape topLeft(0x36);
    const GumpShape topRight(0x37);
    const GumpShape bottomLeft(0x38);
    const GumpShape bottomRight(0x39);
    const i16 x = static_cast<i16>(SCREEN_WIDTH - 638) >> 1;
    const i16 y = static_cast<i16>(SCREEN_HEIGHT - MAIN_MENU_HEIGHT) >> 1;

    drawShape(x, y, gumpShapes->shapeFrame(topLeft.shape, topLeft.frame));
    drawShape(x + 319, y, gumpShapes->shapeFrame(topRight.shape, topRight.frame));
    drawShape(x, y + (MAIN_MENU_HEIGHT / 2), gumpShapes->shapeFrame(bottomLeft.shape, bottomLeft.frame));
    drawShape(x + 319, y + (MAIN_MENU_HEIGHT / 2), gumpShapes->shapeFrame(bottomRight.shape, bottomRight.frame));
}

MainMenuGump::~MainMenuGump()
{
    jumpToBlack();
    gameMouse->hide();
    popSong();
    mainMenu = nullptr;
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    cycleProcess->paused = 0;
    keyDaemon->setRate(keyRate);
    setRepeatDelay(keyRepeatDelay);
    flushKeys();
    gumpKeys.clear();
}

void MainMenuGump::handleCommand(NewGumpEvent *event)
{
    const i16 choice = event->sender->id.instance();

    choose(choice);
}

void MainMenuGump::handleKeyboard(NewGumpEvent *event)
{
    MenuButton *button;

    switch (event->argument) {
    case ASCII_ESCAPE:
        choose(MAINMENU_RESUME);
        break;
    case KEYCODE_ALT | SCAN_X:
        if (MAIN_MENU_ALT_X_QUITS) {
            jumpToBlack();
            gameMouse->hide();
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, MAINMENU_QUIT, this, parent, 0L)));
            close();
        }
        break;
    case '4':
    case KEYCODE_SCAN | SCAN_LEFT:
        if (menuChoice > 2) {
            button = static_cast<MenuButton *>(children.findByInstanceId(menuChoice - 3));
            button->showPressed();
        }
        break;
    case '6':
    case KEYCODE_SCAN | SCAN_RIGHT:
        if (menuChoice < 3) {
            button = static_cast<MenuButton *>(children.findByInstanceId(menuChoice + 3));
            button->showPressed();
        }
        break;
    case '8':
    case KEYCODE_SCAN | SCAN_UP:
        if (menuChoice != 0) {
            button = static_cast<MenuButton *>(children.findByInstanceId(menuChoice - 1));
            button->showPressed();
        }
        break;
    case '2':
    case KEYCODE_SCAN | SCAN_DOWN:
        if (menuChoice < 5) {
            button = static_cast<MenuButton *>(children.findByInstanceId(menuChoice + 1));
            button->showPressed();
        }
        break;
    case '\r':
        choose(menuChoice);
        break;
    }
}

void MainMenuGump::choose(i16 choice)
{
    /* Each gump the choice opens is done after it has closed. */
    switch (choice) {
    case MAINMENU_LOAD:
        gameMouse->hide();
        openModalGump(new LoadSaveGump(1), static_cast<u32>(-1), [this, choice](s32 result) {
            if (result > 0) {
                jumpToBlack();
                gameMouse->resetShape(MENU_POINTER);
                gameMouse->hide();
                sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, choice, this, parent, 0L)));
                close();
                return;
            }
            paint(0);
            palette.apply();
            gameMouse->resetShape(MENU_POINTER);
            gameMouse->show();
        });
        break;
    case MAINMENU_SAVE:
        if (atStartMenu == 0U) {
            gameMouse->hide();
            openModalGump(new LoadSaveGump(0), static_cast<u32>(-1), [this, choice](s32 result) {
                if (result > 0) {
                    jumpToBlack();
                    gameMouse->resetShape(MENU_POINTER);
                    gameMouse->hide();
                    sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, choice, this, parent, 0L)));
                    close();
                    return;
                }
                paint(0);
                palette.apply();
                gameMouse->resetShape(MENU_POINTER);
                gameMouse->show();
            });
        }
        break;
    case MAINMENU_OPTIONS:
        gameMouse->hide();
        openModalGump(new SettingsGump, static_cast<u32>(-1), [this](s32 /*result*/) {
            if (MAIN_MENU_PLAYS_SOUND) {
                playEffect(MAIN_MENU_SOUND);
            }
            drawViewport->clear(0);
            screenViewport->blit(drawViewport);
            palette.apply();
            gameMouse->resetShape(MENU_POINTER);
            gameMouse->show();
            paint(0);
        });
        break;
    case MAINMENU_CREDITS:
        gameMouse->hide();
        jumpToBlack();
        openCredits([this]() {
            jumpToBlack();
            palette.apply();
            gameMouse->show();
            eventMask |= EVENT_EXECUTE;
            unknown38 = 0;
            flushKeys();
            gumpKeys.clear();
            paint(0);
            return false;
        });
        break;
    case MAINMENU_RESUME:
        if (atStartMenu == 0U) {
            gameMouse->hide();
            jumpToBlack();
            gameMouse->show();
            sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, choice, this, parent, 0L)));
            close();
        }
        break;
    case MAINMENU_NEW_GAME:
        /* A level was chosen: go on as MAINMENU_QUIT with command MAINMENU_NEW_GAME. */
        openModalGump(new DifficultyGump, static_cast<u32>(-1), [this, choice](s32 result) {
            if (result != 0) {
                atStartMenu = 0;
                leave(choice);
                return;
            }
            gameMouse->resetShape(MENU_POINTER);
            gameMouse->show();
            palette.apply();
            paint(0);
        });
        break;
    case MAINMENU_QUIT:
        leave(choice);
        break;
    }
}

void MainMenuGump::leave(i16 choice)
{
    jumpToBlack();
    gameMouse->hide();
    gameMouse->show();
    sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, choice, this, parent, 0L)));
    close();
}

/* UNCALLED: nothing calls it. */
OptionButton::OptionButton(u8 code, u8 on, NewGump *parentGump, GumpShape look)
    : NewGump(NewGumpId(GUMP_BUTTON, code), parentGump, EVENT_LEFT_BUTTON), GumpShape(look)
{
    bounds = dimensions();
    if (on != 0U) {
        choose();
    } else {
        chosen = 0;
        frame = 1;
    }
}

void OptionButton::handleLeftButton(NewGumpEvent *event)
{
    if (event->isMousePress() && (chosen == 0U)) {
        choose();
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
    }
}

void OptionButton::choose()
{
    if (chosen != 0U) {
        return;
    }
    chosen = 1;
    u8 pair = id.instance();
    pair ^= 1;
    OptionButton *other = static_cast<OptionButton *>(parent->children.findByInstanceId(pair));

    if (other != nullptr) {
        other->chosen = 0;
        other->frame = 1;
        other->paint(0);
    }
    frame = 0;
}

void OptionButton::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

OptionsGump::~OptionsGump()
{
    writeOptions();
    jumpToBlack();
    gameMouse->hide();
}

/* The level the difficulty dialog's keys stand on, 1-4. */
i16 &difficultyChoice = gameGlobal<i16>(3);

DifficultyGump::DifficultyGump()
    : DialogGump(NewGumpId(), nullptr, Rect(0, 0, SCREEN_RIGHT, SCREEN_BOTTOM), guiColors->face)
{
    eventMask |= EVENT_KEY;
    jumpToBlack();
    gameMouse->pushShape(DIFFICULTY_POINTER);
    gameMouse->moveTo(320, 400);
    Palette palette;

    palette.load(makePath(nullptr, staticDirectory, "diff.pal", nullptr), 0);
    DifficultyButton *button = new DifficultyButton(1, this, 0x4a, &difficultyChoice, DIFFICULTY_SOUNDS[0]);

    button->bounds.moveTo(static_cast<i16>(bounds.width() - button->bounds.width()) >> 1, 120);
    button = new DifficultyButton(2, this, 0x4b, &difficultyChoice, DIFFICULTY_SOUNDS[1]);
    button->bounds.moveTo(static_cast<i16>(bounds.width() - button->bounds.width()) >> 1, 179);
    button = new DifficultyButton(3, this, 0x4c, &difficultyChoice, DIFFICULTY_SOUNDS[2]);
    button->bounds.moveTo(static_cast<i16>(bounds.width() - button->bounds.width()) >> 1, 238);
    button = new DifficultyButton(4, this, 0x4d, &difficultyChoice, DIFFICULTY_SOUNDS[3]);
    button->bounds.moveTo(static_cast<i16>(bounds.width() - button->bounds.width()) >> 1, 297);
    bounds.moveTo(static_cast<i16>(SCREEN_WIDTH - bounds.width()) >> 1,
                  static_cast<i16>(SCREEN_HEIGHT - bounds.height()) >> 1);
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    killItemProcesses(0, FADE_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    palette.apply();
    gameMouse->show();
    addAcceptedArguments(-1, 0);
}

DifficultyGump::~DifficultyGump()
{
    gameMouse->hide();
    jumpToBlack();
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
}

void DifficultyGump::handleCommand(NewGumpEvent *event)
{
    controlledNpc.difficulty = event->sender->id.instance();
    if (controlledNpc.difficulty == 1) {
        weaponTypes[1].ammoType = 0;
    } else {
        weaponTypes[1].ammoType = 1;
    }
    /* The original closes without setting the modal result; report the
     * clicked level as the Enter path does. */
    sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, controlledNpc.difficulty, this, parent, 0L)));
    close();
}

/* The buttons are each game's DifficultyButtons, whose showPressed it calls
 * through ButtonGump's (their first base). */
void DifficultyGump::handleKeyboard(NewGumpEvent *event)
{
    ButtonGump *button;

    switch (event->argument) {
    case ASCII_ESCAPE:
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, 0L, this, parent, 0L)));
        close();
        break;
    case '8':
    case KEYCODE_SCAN | SCAN_UP:
        if (difficultyChoice > 1) {
            button = static_cast<ButtonGump *>(children.findByInstanceId(difficultyChoice - 1));
            button->showPressed();
        }
        break;
    case '2':
    case KEYCODE_SCAN | SCAN_DOWN:
        if (difficultyChoice < 4) {
            button = static_cast<ButtonGump *>(children.findByInstanceId(difficultyChoice + 1));
            button->showPressed();
        }
        break;
    case '\r':
        controlledNpc.difficulty = difficultyChoice;
        if (difficultyChoice == 1) {
            weaponTypes[1].ammoType = 0;
        } else {
            weaponTypes[1].ammoType = 1;
        }
        sendEvent(&lvalue(NewGumpEvent(EVENT_COMMAND, 0, 0, difficultyChoice, this, parent, 0L)));
        close();
        break;
    }
}

void DifficultyGump::paintSelf(i16 /*x*/, i16 /*y*/)
{
    drawViewport->clear(0);
    const GumpShape title(0x49);
    const Rect size = gumpShapes->dimensions(title.shape, title.frame);

    title.draw(static_cast<i16>(SCREEN_WIDTH - RECT_WIDTH(size)) >> 1, 76);
}

DifficultyButton::DifficultyButton(u8 code, NewGump *parentGump, u16 buttonShape, i16 *selection, i16 buttonSound)
    : ButtonGump(code, parentGump), GumpShape(buttonShape)
{
    choice = selection;
    this->sound = buttonSound;
    bounds = dimensions();
    bounds.right = 320;
    if (code == *choice) {
        showPressed();
    }
}

/* UNCALLED: nothing calls it. */
DifficultyButton::DifficultyButton(u8 code, NewGump *parentGump, u16 buttonShape, i16 *selection)
    : DifficultyButton(code, parentGump, buttonShape, selection, 0)
{
}

void DifficultyButton::paintSelf(i16 x, i16 y)
{
    frame = 0;
    draw(x, y);
    frame = 1;
    draw(x + 170, y);
    if (id.instance() == *choice) {
        drawViewport->rectangle(DIFFICULTY_CHOSEN_COLOUR, bounds.left, bounds.top, bounds.right, bounds.bottom);
    } else if (DIFFICULTY_FRAMES_UNCHOSEN) {
        drawViewport->rectangle(DIFFICULTY_UNCHOSEN_COLOUR, bounds.left, bounds.top, bounds.right, bounds.bottom);
    }
}

void DifficultyButton::showPressed()
{
    DifficultyButton *previousButton = static_cast<DifficultyButton *>(parent->children.findByInstanceId(*choice));

    *choice = id.instance();
    if (previousButton != nullptr) {
        previousButton->paint(0);
    }
    paint(0);
    if (DIFFICULTY_BUTTON_SOUNDS) {
        stopAllEffects();
        playEffect(sound);
    }
}

void DifficultyButton::showReleased() {}

MenuButton::MenuButton(u8 code, NewGump *parentGump, u16 buttonShape, i16 *selection)
    : ButtonGump(code, parentGump), GumpShape(buttonShape)
{
    choice = selection;
    bounds = dimensions();
    if (code == *choice) {
        showPressed();
    }
}

void MenuButton::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

void MenuButton::showPressed()
{
    MenuButton *previousButton = static_cast<MenuButton *>(parent->children.findByInstanceId(*choice));

    *choice = id.instance();
    previousButton->showReleased();
    previousButton->paint(0);
    frame = 1;
    /* The original paints the button twice. */
    paint(0);
    paint(0);
}

void MenuButton::showReleased()
{
    if (id.instance() != *choice) {
        frame = 0;
        paint(0);
    }
}

ArrowButton::ArrowButton(u8 code, NewGump *parentGump, u16 buttonShape)
    : AutoRepeatButtonGump(code, parentGump), GumpShape(buttonShape)
{
    bounds = dimensions();
}

void ArrowButton::paintSelf(i16 x, i16 y)
{
    draw(x, y);
}

/* Both paint twice, as the original does. */
void ArrowButton::showPressed()
{
    frame = 1;
    paint(0);
    paint(0);
}

void ArrowButton::showReleased()
{
    frame = 0;
    paint(0);
    paint(0);
}

/* The options gump's buttons (their codes): the pairs, then the sliders'
 * arrows and Done. */
constexpr int OPTION_ANIMATIONS_ON = 0x50;
constexpr int OPTION_ANIMATIONS_OFF = 0x51;
constexpr int OPTION_FRAME_SKIP_ON = 0x52;
constexpr int OPTION_FRAME_SKIP_OFF = 0x53;
/* The fifth pair: No Regret's hi-colour movies, No Remorse's digital
 * filtering (XAN_MOVIE_OPTIONS); the big video: No Regret's movies doubled, No
 * Remorse's flics in mode 13h. */
constexpr int OPTION_EXTRA_ON = 0x54;
constexpr int OPTION_EXTRA_OFF = 0x55;
constexpr int OPTION_BIG_VIDEO_ON = 0x56;
constexpr int OPTION_BIG_VIDEO_OFF = 0x57;
constexpr int OPTION_BLASTS_LIMITED = 0x58;
constexpr int OPTION_BLASTS_UNLIMITED = 0x59;
constexpr int OPTION_SOUND_DOWN = 0x5c;
constexpr int OPTION_SOUND_UP = 0x5d;
constexpr int OPTION_MUSIC_DOWN = 0x5e;
constexpr int OPTION_MUSIC_UP = 0x5f;
constexpr int OPTION_MOUSE_SLOWER = 0x60;
constexpr int OPTION_MOUSE_FASTER = 0x61;
constexpr int OPTION_DONE = 0x62;
/* Its rows, which the keys move the focus over (optionFocus). */
constexpr int FOCUS_ANIMATIONS = 0;
constexpr int FOCUS_FRAME_SKIP = 1;
constexpr int FOCUS_BIG_VIDEO = 2;
constexpr int FOCUS_BLASTS = 3;
constexpr int FOCUS_EXTRA = 4;
constexpr int FOCUS_SOUND = 5;
constexpr int FOCUS_MUSIC = 6;
constexpr int FOCUS_MOUSE = 7;
constexpr int FOCUS_DONE = 8;

namespace {
/* The volumes of the slider steps, for ASS_SetSoundVolume and
 * ASS_SetMusicVolume. */
u8 soundLevels[23] = {0x00, 0x02, 0x05, 0x08, 0x0b, 0x0e, 0x11, 0x14, 0x17, 0x1a, 0x1d, 0x20,
                      0x22, 0x25, 0x28, 0x2b, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3d, 0x40};
u8 musicLevels[23] = {0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x18,
                      0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x25, 0x27, 0x29, 0x2b, 0x2d, 0x30};
}
/* No Regret's moviehicolor (options.cpp). */
i16 &movieHiColor = gameGlobal<i16>(0);

namespace {
/* The option the keys stand on. */
i16 &optionFocus = gameGlobal<i16>(0);
}
u8 &atStartMenu = gameGlobal<u8>(1);

/* UNCALLED: nothing calls it. */
OptionsGump::OptionsGump() : NewGump(NewGumpId(), nullptr, 0)
{
    flushKeys();
    eventMask |= EVENT_LEFT_BUTTON | EVENT_MOTION | EVENT_COMMAND;
    bounds.set(0, 0, drawViewport->right() - drawViewport->left(), drawViewport->bottom() - drawViewport->top());
    addAcceptedArguments(-1, 0);
    jumpToBlack();
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    palette.load("static\\misc.pal", 0);
    frameSkipChanged = 0;
    soundStep = ASS_GetSoundVolume();
    musicStep = ASS_GetMusicVolume();
    i16 i;

    for (i = 0; i < 23; i++) {
        if (soundLevels[i] >= soundStep) {
            soundStep = i;
            break;
        }
    }
    for (i = 0; i < 23; i++) {
        if (musicLevels[i] >= musicStep) {
            musicStep = i;
            break;
        }
    }
    const i16 x = static_cast<i16>(SCREEN_WIDTH - 638) >> 1;
    const i16 y = static_cast<i16>(SCREEN_HEIGHT - 240) >> 1;
    OptionButton *option =
        new OptionButton(OPTION_ANIMATIONS_ON, static_cast<u8>(animationsStopped == 0), this, GumpShape(0x1f));

    option->bounds.moveTo(x + 34, y + 43);
    option = new OptionButton(OPTION_ANIMATIONS_OFF, static_cast<u8>(animationsStopped == 1), this, GumpShape(0x20));
    option->bounds.moveTo(x + 94, y + 43);
    option = new OptionButton(OPTION_FRAME_SKIP_ON, static_cast<u8>(frameSkip == 3), this, GumpShape(0x21));
    option->bounds.moveTo(x + 34, y + 83);
    option = new OptionButton(OPTION_FRAME_SKIP_OFF, static_cast<u8>(frameSkip != 3), this, GumpShape(0x22));
    option->bounds.moveTo(x + 94, y + 83);
    /* No Regret's LIMITED is lit at 1 only, No Remorse's at any but 0 (its
     * frameskip's word can set more, options.cpp). */
    option =
        new OptionButton(OPTION_BLASTS_LIMITED, BLASTS_LIT_AT_ONE ? static_cast<u8>(limitBlasts == 1) : limitBlasts,
                         this, GumpShape(0x25));
    option->bounds.moveTo(x + 34, y + 163);
    option = new OptionButton(OPTION_BLASTS_UNLIMITED, static_cast<u8>(limitBlasts == 0), this, GumpShape(0x26));
    option->bounds.moveTo(x + 94, y + 163);
    option = new OptionButton(OPTION_EXTRA_ON,
                              XAN_MOVIE_OPTIONS ? static_cast<u8>(hiColorMovies == 1)
                                                : static_cast<u8>(ASS_GetOversampling() != 0),
                              this, GumpShape(0x27));
    option->bounds.moveTo(x + 34, y + 203);
    option = new OptionButton(OPTION_EXTRA_OFF,
                              XAN_MOVIE_OPTIONS ? static_cast<u8>(hiColorMovies == 0)
                                                : static_cast<u8>(ASS_GetOversampling() == 0),
                              this, GumpShape(0x28));
    option->bounds.moveTo(x + 94, y + 203);
    option = new OptionButton(OPTION_BIG_VIDEO_ON,
                              XAN_MOVIE_OPTIONS ? static_cast<u8>(doubleMovies == 2)
                                                : static_cast<u8>(flicVideoMode == MODE_13H),
                              this, GumpShape(0x23));
    option->bounds.moveTo(x + 34, y + 123);
    option = new OptionButton(OPTION_BIG_VIDEO_OFF,
                              XAN_MOVIE_OPTIONS ? static_cast<u8>(doubleMovies == 0)
                                                : static_cast<u8>(flicVideoMode == VESA_640X480),
                              this, GumpShape(0x24));
    option->bounds.moveTo(x + 136, y + 123);
    ArrowButton *arrow = new ArrowButton(OPTION_SOUND_DOWN, this, 0x31);

    arrow->bounds.moveTo(x + 434, y + 26);
    arrow = new ArrowButton(OPTION_SOUND_UP, this, 0x32);
    arrow->bounds.moveTo(x + 596, y + 26);
    arrow = new ArrowButton(OPTION_MUSIC_DOWN, this, 0x31);
    arrow->bounds.moveTo(x + 434, y + 77);
    arrow = new ArrowButton(OPTION_MUSIC_UP, this, 0x32);
    arrow->bounds.moveTo(x + 596, y + 77);
    arrow = new ArrowButton(OPTION_MOUSE_SLOWER, this, 0x31);
    arrow->bounds.moveTo(x + 434, y + 129);
    arrow = new ArrowButton(OPTION_MOUSE_FASTER, this, 0x32);
    arrow->bounds.moveTo(x + 596, y + 129);
    ShapeButtonGump *done = new ShapeButtonGump(OPTION_DONE, this, 0x2b);

    done->bounds.moveTo(x + 550, y + 198);
    if (optionFocus == FOCUS_DONE) {
        done->showPressed();
    }
    killItemProcesses(0, FADE_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    palette.apply();
    paint(1);
    gameMouse->resetShape(MENU_POINTER);
    gameMouse->show();
    playSettingsOpened();
}

void OptionsGump::paintSelf(i16 /*x*/, i16 /*y*/)
{
    [[maybe_unused]] const Rect clip = getClipRect(); /* not used */
    const GumpShape panelTopLeft(0x2c);
    const GumpShape panelTopRight(0x2d);
    const GumpShape panelBottomLeft(0x2e);
    const GumpShape panelBottomRight(0x2f);
    const GumpShape soundLabel(0x29);
    const GumpShape musicLabel(0x2a);
    const i16 x = static_cast<i16>(SCREEN_WIDTH - 638) >> 1;
    const i16 y = static_cast<i16>(SCREEN_HEIGHT - 240) >> 1;

    /* No Regret's options sit in the main menu's frame (OPTIONS_IN_MENU_FRAME). */
    if (OPTIONS_IN_MENU_FRAME) {
        const GumpShape topLeft(0x36);
        const GumpShape topRight(0x37);
        const GumpShape bottomLeft(0x38);
        const GumpShape bottomRight(0x39);
        const i16 menuY = static_cast<i16>(SCREEN_HEIGHT - 330) >> 1;
        addDirtyRect(Rect(x, menuY, x + 637, menuY + 330));
        drawShape(x, menuY, gumpShapes->shapeFrame(topLeft.shape, topLeft.frame));
        drawShape(x + 319, menuY, gumpShapes->shapeFrame(topRight.shape, topRight.frame));
        drawShape(x, menuY + 165, gumpShapes->shapeFrame(bottomLeft.shape, bottomLeft.frame));
        drawShape(x + 319, menuY + 165, gumpShapes->shapeFrame(bottomRight.shape, bottomRight.frame));
    } else {
        addDirtyRect(Rect(x, y, x + 637, y + 240));
    }
    drawShape(x, y, gumpShapes->shapeFrame(panelTopLeft.shape, panelTopLeft.frame));
    drawShape(x + 319, y, gumpShapes->shapeFrame(panelTopRight.shape, panelTopRight.frame));
    drawShape(x, y + 120, gumpShapes->shapeFrame(panelBottomLeft.shape, panelBottomLeft.frame));
    drawShape(x + 319, y + 120, gumpShapes->shapeFrame(panelBottomRight.shape, panelBottomRight.frame));
    soundLabel.draw(x + 467, y + 26);
    musicLabel.draw(x + 464, y + 76);
    const Rect focus = focusRect(optionFocus);

    if (focus.right != 0) {
        drawViewport->rectangle(OPTIONS_FOCUS_COLOUR, focus.left, focus.top, focus.right, focus.bottom);
    }
    {
        const GumpShape knob(0x30);
        Rect area;
        const Rect size = gumpShapes->dimensions(knob.shape, knob.frame);

        area.set(x + (soundStep * 6) + 455, y + 59, 0, 0);
        area.right = RECT_WIDTH(size) + area.left;
        area.bottom = RECT_HEIGHT(size) + area.top;
        knob.draw(area.left, area.top);
        area.left = x + 455;
        area.right = x + SCREEN_RIGHT;
        addDirtyRect(area);
    }
    {
        const GumpShape knob(0x30);
        Rect area;
        const Rect size = gumpShapes->dimensions(knob.shape, knob.frame);

        area.set(x + (musicStep * 6) + 455, y + 110, 0, 0);
        area.right = RECT_WIDTH(size) + area.left;
        area.bottom = RECT_HEIGHT(size) + area.top;
        knob.draw(area.left, area.top);
        area.left = x + 455;
        area.right = x + SCREEN_RIGHT;
        addDirtyRect(area);
    }
    {
        /* The four speeds over the 23 steps: 3/23 in 16.16. */
        const s32 step = 0x2164;
        i16 position = (static_cast<s32>(mouseSpeed) << 16) / step;

        if (mouseSpeed == 3) {
            position--;
        }
        const GumpShape knob(0x30);
        Rect area;
        const Rect size = gumpShapes->dimensions(knob.shape, knob.frame);

        area.set(x + (position * 6) + 455, y + 162, 0, 0);
        area.right = RECT_WIDTH(size) + area.left;
        area.bottom = RECT_HEIGHT(size) + area.top;
        knob.draw(area.left, area.top);
        area.left = x + 455;
        area.right = x + SCREEN_RIGHT;
        addDirtyRect(area);
    }
}

void OptionsGump::handleCommand(NewGumpEvent *event)
{
    const i16 buttonId = event->sender->id.instance();

    switch (buttonId) {
    case OPTION_ANIMATIONS_ON:
    case OPTION_ANIMATIONS_OFF:
        optionFocus = FOCUS_ANIMATIONS;
        break;
    case OPTION_FRAME_SKIP_ON:
    case OPTION_FRAME_SKIP_OFF:
        optionFocus = FOCUS_FRAME_SKIP;
        break;
    case OPTION_BIG_VIDEO_ON:
    case OPTION_BIG_VIDEO_OFF:
        optionFocus = FOCUS_BIG_VIDEO;
        break;
    case OPTION_BLASTS_LIMITED:
    case OPTION_BLASTS_UNLIMITED:
        optionFocus = FOCUS_BLASTS;
        break;
    case OPTION_EXTRA_ON:
    case OPTION_EXTRA_OFF:
        optionFocus = FOCUS_EXTRA;
        break;
    case OPTION_SOUND_DOWN:
    case OPTION_SOUND_UP:
        optionFocus = FOCUS_SOUND;
        break;
    case OPTION_MUSIC_DOWN:
    case OPTION_MUSIC_UP:
        optionFocus = FOCUS_MUSIC;
        break;
    case OPTION_MOUSE_SLOWER:
    case OPTION_MOUSE_FASTER:
        optionFocus = FOCUS_MOUSE;
        break;
    case OPTION_DONE:
        optionFocus = FOCUS_DONE;
        (static_cast<ButtonBaseGump *>(event->sender))->showPressed();
        break;
    }
    paint(0);
    event->sender->paint(0);
    apply(buttonId);
}

Rect OptionsGump::focusRect(i16 focus)
{
    const i16 x = static_cast<i16>(SCREEN_WIDTH - 638) >> 1;
    const i16 y = static_cast<i16>(SCREEN_HEIGHT - 240) >> 1;
    Rect area;

    switch (focus) {
    case FOCUS_ANIMATIONS:
        area.set(x + 21, y + 25, x + 197, y + 56);
        break;
    case FOCUS_FRAME_SKIP:
        area.set(x + 21, y + 65, x + 201, y + 96);
        break;
    case FOCUS_BIG_VIDEO:
        area.set(x + 21, y + 105, x + 224, y + 136);
        break;
    case FOCUS_BLASTS:
        area.set(x + 21, y + 145, x + 231, y + 176);
        break;
    case FOCUS_EXTRA:
        area.set(x + 21, y + 185, x + 283, y + 216);
        break;
    case FOCUS_SOUND:
        area.set(x + 431, y + 24, x + 613, y + 71);
        break;
    case FOCUS_MUSIC:
        area.set(x + 431, y + 74, x + 613, y + 122);
        break;
    case FOCUS_MOUSE:
        area.set(x + 431, y + 127, x + 613, y + 174);
        break;
    default:
        area.set(0, 0, 0, 0);
        break;
    }
    return area;
}

void OptionsGump::handleKeyboard(NewGumpEvent *event)
{
    switch (event->argument) {
    case ASCII_ESCAPE:
        close();
        break;
    case '\r':
        if (optionFocus == FOCUS_DONE) {
            close();
        }
        break;
    case '8':
    case KEYCODE_SCAN | SCAN_UP:
        if (optionFocus != FOCUS_ANIMATIONS) {
            if (optionFocus == FOCUS_DONE) {
                ButtonBaseGump *done = static_cast<ButtonBaseGump *>(children.findByInstanceId(OPTION_DONE));

                done->showReleased();
                done->paint(0);
            }
            optionFocus--;
        }
        paint(0);
        break;
    case '2':
    case KEYCODE_SCAN | SCAN_DOWN:
        if (optionFocus < FOCUS_DONE) {
            optionFocus++;
            if (optionFocus == FOCUS_DONE) {
                ButtonBaseGump *done = static_cast<ButtonBaseGump *>(children.findByInstanceId(OPTION_DONE));

                done->showPressed();
                done->paint(0);
            }
        }
        paint(0);
        break;
    case '4':
    case KEYCODE_SCAN | SCAN_LEFT: {
        i16 optionId = -1;

        switch (optionFocus) {
        case FOCUS_ANIMATIONS:
            optionId = OPTION_ANIMATIONS_ON;
            break;
        case FOCUS_FRAME_SKIP:
            optionId = OPTION_FRAME_SKIP_ON;
            break;
        case FOCUS_BIG_VIDEO:
            optionId = OPTION_BIG_VIDEO_ON;
            break;
        case FOCUS_BLASTS:
            optionId = OPTION_BLASTS_LIMITED;
            break;
        case FOCUS_EXTRA:
            optionId = OPTION_EXTRA_ON;
            break;
        case FOCUS_SOUND:
            optionId = OPTION_SOUND_DOWN;
            break;
        case FOCUS_MUSIC:
            optionId = OPTION_MUSIC_DOWN;
            break;
        case FOCUS_MOUSE:
            optionId = OPTION_MOUSE_SLOWER;
            break;
        }
        if (optionId >= 0) {
            if (optionId < OPTION_SOUND_DOWN) {
                OptionButton *option = static_cast<OptionButton *>(children.findByInstanceId(optionId));

                if (option != nullptr) {
                    option->choose();
                }
            }
            apply(optionId);
        }
        paint(0);
        break;
    }
    case '6':
    case KEYCODE_SCAN | SCAN_RIGHT: {
        i16 optionId = -1;

        switch (optionFocus) {
        case FOCUS_ANIMATIONS:
            optionId = OPTION_ANIMATIONS_OFF;
            break;
        case FOCUS_FRAME_SKIP:
            optionId = OPTION_FRAME_SKIP_OFF;
            break;
        case FOCUS_BIG_VIDEO:
            optionId = OPTION_BIG_VIDEO_OFF;
            break;
        case FOCUS_BLASTS:
            optionId = OPTION_BLASTS_UNLIMITED;
            break;
        case FOCUS_EXTRA:
            optionId = OPTION_EXTRA_OFF;
            break;
        case FOCUS_SOUND:
            optionId = OPTION_SOUND_UP;
            break;
        case FOCUS_MUSIC:
            optionId = OPTION_MUSIC_UP;
            break;
        case FOCUS_MOUSE:
            optionId = OPTION_MOUSE_FASTER;
            break;
        }
        if (optionId >= 0) {
            if (optionId < OPTION_SOUND_UP) {
                OptionButton *option = static_cast<OptionButton *>(children.findByInstanceId(optionId));

                if (option != nullptr) {
                    option->choose();
                }
            }
            apply(optionId);
        }
        paint(0);
        break;
    }
    }
}

void OptionsGump::apply(i16 optionId)
{
    switch (optionId) {
    case OPTION_ANIMATIONS_ON:
        animationsStopped = 0;
        break;
    case OPTION_ANIMATIONS_OFF:
        animationsStopped = 1;
        break;
    case OPTION_BLASTS_LIMITED:
        limitBlasts = 1;
        break;
    case OPTION_BLASTS_UNLIMITED:
        limitBlasts = 0;
        break;
    case OPTION_FRAME_SKIP_ON:
        frameSkip = 3;
        frameSkipChanged = 1;
        break;
    case OPTION_FRAME_SKIP_OFF:
        frameSkip = 0;
        frameSkipChanged = 1;
        break;
    case OPTION_EXTRA_ON:
        if (XAN_MOVIE_OPTIONS) {
            hiColorMovies = 1;
        } else {
            ASS_SetOversampling();
        }
        break;
    case OPTION_EXTRA_OFF:
        if (XAN_MOVIE_OPTIONS) {
            hiColorMovies = 0;
        } else {
            ASS_ClrOversampling();
        }
        break;
    case OPTION_BIG_VIDEO_ON:
        if (XAN_MOVIE_OPTIONS) {
            doubleMovies = 2;
        } else {
            flicVideoMode = MODE_13H;
        }
        break;
    case OPTION_BIG_VIDEO_OFF:
        if (XAN_MOVIE_OPTIONS) {
            doubleMovies = 0;
        } else {
            flicVideoMode = VESA_640X480;
        }
        break;
    case OPTION_DONE:
        close();
        break;
    case OPTION_SOUND_DOWN:
        if (soundStep != 0) {
            soundStep--;
            ASS_SetSoundVolume(soundLevels[soundStep]);
        }
        playEffect(SOUND_VOLUME_SAMPLE);
        paint(0);
        break;
    case OPTION_SOUND_UP:
        if (soundStep < 22) {
            soundStep++;
            ASS_SetSoundVolume(soundLevels[soundStep]);
        }
        playEffect(SOUND_VOLUME_SAMPLE);
        paint(0);
        break;
    case OPTION_MUSIC_DOWN:
        if (musicStep != 0) {
            musicStep--;
            ASS_SetMusicVolume(musicLevels[musicStep]);
        }
        playEffect(SOUND_VOLUME_SAMPLE);
        paint(0);
        break;
    case OPTION_MUSIC_UP:
        if (musicStep < 22) {
            musicStep++;
            ASS_SetMusicVolume(musicLevels[musicStep]);
        }
        playEffect(SOUND_VOLUME_SAMPLE);
        paint(0);
        break;
    case OPTION_MOUSE_SLOWER:
        if (mouseSpeed != 0) {
            mouseSpeed--;
        }
        playEffect(SOUND_VOLUME_SAMPLE);
        paint(0);
        break;
    case OPTION_MOUSE_FASTER:
        if (mouseSpeed < 3) {
            mouseSpeed++;
        }
        playEffect(SOUND_VOLUME_SAMPLE);
        paint(0);
        break;
    }
}

/* The chosen level stays framed until another is chosen. */
