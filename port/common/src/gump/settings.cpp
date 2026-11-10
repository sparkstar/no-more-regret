/* The port's settings gump and the settings both games have. See
 * settings.h. */
#include "settings.h"
#include "gamestate.h"
#include "settings_game.h"
#include "mainmenu_game.h"
#include "widgets_game.h"
#include "asylum.h"
#include "clock.h"
#include "control.h"
#include "dispatch.h"
#include "dsfxman.h"
#include "enhanced.h"
#include "fade.h"
#include "keysgump.h"
#include "filespec.h"
#include "gamefile.h"
#include "gametime.h"
#include "gumpshap.h"
#include "kernel.h"
#include "options.h"
#include "options_game.h"
#include "flicplay.h"
#include "movie.h"
#include "supervga.h"
#include "priority.h"
#include "screen.h"
#include "sndmgr.h"
#include "u9mouse.h"
#include "viewport.h"
#include "world.h"

#include <algorithm>
#include <array>
#include <type_traits>

namespace {

/* The volumes of the sliders' steps, for ASS_SetSoundVolume and
 * ASS_SetMusicVolume: the options gump's tables, the same in both games. */
const std::array<u8, 23> SOUND_LEVELS = {0x00, 0x02, 0x05, 0x08, 0x0b, 0x0e, 0x11, 0x14, 0x17, 0x1a, 0x1d, 0x20,
                                         0x22, 0x25, 0x28, 0x2b, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3d, 0x40};
const std::array<u8, 23> MUSIC_LEVELS = {0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0d, 0x0f, 0x11, 0x13, 0x15, 0x18,
                                         0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x25, 0x27, 0x29, 0x2b, 0x2d, 0x30};
/* The mouse's speeds (mouseSpeed). */
const i16 MOUSE_SPEEDS = 4;
/* The sound of a slider's step (the options gump's). */
const i16 STEP_SOUND = 0x98;
/* The layout: between the columns, a heading and its choices; the room
 * inside the area's edge; the focus frame's round its row. Between the rows
 * each game's (SETTINGS_ROW_GAP). */
const s16 COLUMN_GAP = 48;
const s16 HEADING_GAP = 3;
const s16 AREA_MARGIN = 10;
const s16 FOCUS_MARGIN = 3;

/* The row or the Done button the keys stand on, kept from one opening to
 * the next (as the options gump's). */
i16 &settingsFocus = gameGlobal<i16>(0);

}

const char *inLanguage(const Words &words, Language language)
{
    return language == Language::KOREAN ? words.korean : words.english;
}

i16 volumeStep(std::span<const u8> levels, i16 volume)
{
    for (std::size_t i = 0; i < levels.size(); i++) {
        if (levels[i] >= volume) {
            return static_cast<i16>(i);
        }
    }
    return static_cast<i16>(levels.size() - 1);
}

i16 nextFocus(i16 focus, i16 count, i16 direction)
{
    return std::clamp(static_cast<i16>(focus + direction), static_cast<i16>(0), static_cast<i16>(count - 1));
}

std::vector<SettingChoice> onOffChoices()
{
    return {SettingChoice{Words{"ON", "켬"}, false}, SettingChoice{Words{"OFF", "끔"}, true}};
}

Setting animationsSetting()
{
    return Setting{Words{"ANIMATIONS", "애니메이션"},
                   onOffChoices(),
                   0,
                   []() { return static_cast<i16>(animationsStopped == 0U ? 0 : 1); },
                   [](i16 choice) { animationsStopped = static_cast<u8>(choice == 0 ? 0 : 1); },
                   nullptr};
}

Setting frameSkipSetting()
{
    return Setting{
        Words{"FRAME SKIP", "프레임 건너뛰기"},
        onOffChoices(),
        0,
        []() { return static_cast<i16>(frameSkip == 3 ? 0 : 1); },
        [](i16 choice) { frameSkip = static_cast<std::remove_reference_t<decltype(frameSkip)>>(choice == 0 ? 3 : 0); },
        nullptr};
}

Setting blastsSetting()
{
    return Setting{Words{"LIMIT BLASTS", "폭발 제한"},
                   onOffChoices(),
                   0,
                   []() { return static_cast<i16>(limitBlasts != 0U ? 0 : 1); },
                   [](i16 choice) { limitBlasts = static_cast<u8>(choice == 0 ? 1 : 0); },
                   nullptr};
}

Setting soundVolumeSetting()
{
    return Setting{Words{"SOUND VOLUME", "효과음 크기"},
                   {},
                   static_cast<i16>(SOUND_LEVELS.size()),
                   []() { return volumeStep(SOUND_LEVELS, ASS_GetSoundVolume()); },
                   [](i16 step) {
                       ASS_SetSoundVolume(SOUND_LEVELS[static_cast<std::size_t>(step)]);
                       playEffect(STEP_SOUND);
                   },
                   nullptr};
}

Setting musicVolumeSetting()
{
    return Setting{Words{"MUSIC VOLUME", "음악 크기"},
                   {},
                   static_cast<i16>(MUSIC_LEVELS.size()),
                   []() { return volumeStep(MUSIC_LEVELS, ASS_GetMusicVolume()); },
                   [](i16 step) {
                       ASS_SetMusicVolume(MUSIC_LEVELS[static_cast<std::size_t>(step)]);
                       playEffect(STEP_SOUND);
                   },
                   nullptr};
}

Setting mouseSetting()
{
    return Setting{Words{"MOUSE RESPONSE", "마우스 반응"},
                   {},
                   MOUSE_SPEEDS,
                   []() { return mouseSpeed; },
                   [](i16 step) {
                       mouseSpeed = step;
                       playEffect(STEP_SOUND);
                   },
                   nullptr};
}

Setting languageSetting()
{
    /* Each language by its own name. */
    return Setting{Words{"LANGUAGE", "언어"},
                   {SettingChoice{Words{"ENGLISH", "ENGLISH"}, false}, SettingChoice{Words{"한국어", "한국어"}, false}},
                   0,
                   []() { return static_cast<i16>(gameLanguage == Language::KOREAN ? 1 : 0); },
                   [](i16 choice) { gameLanguage = choice == 1 ? Language::KOREAN : Language::ENGLISH; },
                   nullptr};
}

Setting keysSetting()
{
    /* The key settings (keysgump.h): the keys' preset and each action's
     * inputs. */
    return Setting{Words{"CONTROLS", "조작"},
                   {SettingChoice{Words{"SETTINGS", "설정"}, false}},
                   0,
                   []() { return static_cast<i16>(0); },
                   [](i16 /*choice*/) {},
                   []() -> NewGump * { return new KeysGump; }};
}

Setting soundDepthSetting()
{
    /* The depth of the output that plays; before it, the port's or the
     * DMA channel's (asylum.h). A change plays the slider's step in the
     * new depth. */
    return Setting{Words{"SOUND", "소리"},
                   {SettingChoice{Words{"8 BIT", "8비트"}, false}, SettingChoice{Words{"16 BIT", "16비트"}, false}},
                   0,
                   []() {
                       u16 bits = ASS_GetOutputBits();
                       if (bits == 0U) {
                           bits = ASS_GetDepth();
                       }
                       if (bits == 0U) {
                           bits = static_cast<s16>(soundDma) >= 4 ? 16 : 8;
                       }
                       return static_cast<i16>(bits == 16 ? 1 : 0);
                   },
                   [](i16 choice) {
                       chooseSoundBits(choice == 1 ? 16 : 8);
                       playEffect(STEP_SOUND);
                   },
                   nullptr};
}

Setting subtitlesSetting()
{
    return Setting{Words{"SUBTITLES", "자막"},
                   onOffChoices(),
                   0,
                   []() { return static_cast<i16>(movieSubtitles != 0U ? 0 : 1); },
                   [](i16 choice) { movieSubtitles = static_cast<u16>(choice == 0 ? 1 : 0); },
                   nullptr};
}

/* The machine's speed the passes give (clock.h's MachineSpeed): how fast
 * the walks, the people and the usecode's steps go. */
Setting gameSpeedSetting()
{
    return Setting{Words{"GAME SPEED", "게임 속도"},
                   {SettingChoice{Words{"SLOW", "느리게"}, false}, SettingChoice{Words{"NORMAL", "보통"}, false},
                    SettingChoice{Words{"FAST", "빠르게"}, false}},
                   0,
                   []() { return static_cast<i16>(clockMachineSpeed()); },
                   [](i16 choice) { clockSetMachineSpeed(static_cast<MachineSpeed>(choice)); },
                   nullptr};
}

/* The port's experimental wide screen: off, the world
 * beside the 640x480 screen, or that dimmed. */
Setting wideScreenSetting()
{
    /* No wider than the column's other headings: a wider column moves the
     * other, which the player's recordings click. */
    return Setting{Words{"WIDE (TEST)", "와이드(실험)"},
                   {SettingChoice{Words{"OFF", "끔"}, true}, SettingChoice{Words{"ON", "켬"}, false},
                    SettingChoice{Words{"DIM", "어둡게"}, false}},
                   0,
                   []() { return static_cast<i16>(screenWide()); },
                   [](i16 choice) { screenSetWide(static_cast<u8>(choice)); },
                   nullptr};
}

namespace {

/* The options gump's LARGE and SMALL: No Regret's movies doubled or not
 * (doubleMovies), No Remorse's in mode 13h, which the screen enlarges, or in
 * 640x480 (flicVideoMode; XAN_MOVIE_OPTIONS). */
Setting videoSetting()
{
    return Setting{Words{"VIDEO", "영상"},
                   {SettingChoice{Words{"LARGE", "크게"}, false}, SettingChoice{Words{"SMALL", "작게"}, true}},
                   0,
                   []() {
                       if (XAN_MOVIE_OPTIONS) {
                           return static_cast<i16>(doubleMovies == 2 ? 0 : 1);
                       }
                       return static_cast<i16>(flicVideoMode == static_cast<u32>(MODE_13H) ? 0 : 1);
                   },
                   [](i16 choice) {
                       if (XAN_MOVIE_OPTIONS) {
                           doubleMovies = static_cast<i16>(choice == 0 ? 2 : 0);
                       } else {
                           flicVideoMode = choice == 0 ? static_cast<u32>(MODE_13H) : static_cast<u32>(VESA_640X480);
                       }
                   },
                   nullptr};
}

/* No Regret's options gump's HI COLOR VIDEO: the movies in 16-bit colour. */
Setting hiColorSetting()
{
    return Setting{Words{"HI COLOR VIDEO", "하이컬러 영상"},
                   onOffChoices(),
                   0,
                   []() { return static_cast<i16>(hiColorMovies != 0U ? 0 : 1); },
                   [](i16 choice) { hiColorMovies = static_cast<u8>(choice == 0 ? 1 : 0); },
                   nullptr};
}

/* No Remorse's options gump's DIGITAL FILTERING: the sound's oversampling. */
Setting filteringSetting()
{
    return Setting{Words{"DIGITAL FILTERING", "디지털 필터"},
                   onOffChoices(),
                   0,
                   []() { return static_cast<i16>(ASS_GetOversampling() != 0U ? 0 : 1); },
                   [](i16 choice) {
                       if (choice == 0) {
                           ASS_SetOversampling();
                       } else {
                           ASS_ClrOversampling();
                       }
                   },
                   nullptr};
}

}

/* The options gump's settings and the port's: each game's fifth is its own
 * (XAN_MOVIE_OPTIONS: No Regret's hi-colour movies, No Remorse's
 * filtering). */
SettingColumns gameSettingColumns()
{
    return SettingColumns{{animationsSetting(), frameSkipSetting(), videoSetting(), blastsSetting(),
                           XAN_MOVIE_OPTIONS ? hiColorSetting() : filteringSetting(), subtitlesSetting(),
                           wideScreenSetting()},
                          {soundVolumeSetting(), musicVolumeSetting(), mouseSetting(), languageSetting(), keysSetting(),
                           soundDepthSetting(), gameSpeedSetting()}};
}

void playSettingsOpened()
{
    if (SETTINGS_OPEN_PLAYS_SOUND) {
        playEffect(SETTINGS_OPEN_SOUND);
    }
}

SettingsGump::SettingsGump()
    : NewGump(NewGumpId(), nullptr, 0), layout_(Axis::HORIZONTAL, COLUMN_GAP, Align::CENTER),
      corner_(Axis::VERTICAL, 0, Align::END), done_(nullptr)
{
    flushKeys();
    eventMask |= EVENT_LEFT_BUTTON | EVENT_MOTION | EVENT_COMMAND;
    bounds.set(0, 0, static_cast<s16>(drawViewport->right() - drawViewport->left()),
               static_cast<s16>(drawViewport->bottom() - drawViewport->top()));
    addAcceptedArguments(-1, 0);
    jumpToBlack();
    drawViewport->clear(0);
    screenViewport->blit(drawViewport);
    palette_.load(makePath(nullptr, staticDirectory, "misc.pal", nullptr), 0);
    const SettingColumns columns = gameSettingColumns();
    LayoutBox &left = layout_.add<LayoutBox>(Align::START, Axis::VERTICAL, SETTINGS_ROW_GAP, Align::START);
    for (const Setting &setting : columns.left) {
        addRow(left, setting);
    }
    LayoutBox &right = layout_.add<LayoutBox>(Align::START, Axis::VERTICAL, SETTINGS_ROW_GAP, Align::START);
    for (const Setting &setting : columns.right) {
        addRow(right, setting);
    }
    /* Done in the area's bottom right corner, as the keys gump's, apart
     * from the columns' rows. */
    done_ = new DoneButton(this);
    corner_.add<GumpNode>(Align::END, done_);
    settingsFocus = std::min(settingsFocus, static_cast<i16>(rows_.size()));
    if (settingsFocus == static_cast<i16>(rows_.size())) {
        done_->showPressed();
    }
    refresh();
    killItemProcesses(0, FADE_TYPE, Priority(PRIORITY_EQUAL | BASE_LEVEL));
    palette_.apply();
    paint(1);
    gameMouse->resetShape(MENU_POINTER);
    gameMouse->show();
    playSettingsOpened();
}

SettingsGump::~SettingsGump()
{
    writeOptions();
    jumpToBlack();
    gameMouse->hide();
}

void SettingsGump::addRow(LayoutBox &column, const Setting &setting)
{
    const std::size_t index = rows_.size();
    const bool slider = setting.choices.empty();
    /* A slider takes its column's width. */
    LayoutBox &row =
        column.add<LayoutBox>(slider ? Align::FILL : Align::START, Axis::VERTICAL, HEADING_GAP, Align::START);
    Row entry{setting, nullptr, nullptr, nullptr, nullptr};
    Widget *control = nullptr;
    if (setting.opens) {
        entry.heading = new Label(this, HEADING_FONT, Ink{WIDGET_FACE, WIDGET_EDGE});
        entry.button = new TextButton(this);
        entry.button->pressed = [this, index]() { open(index); };
        row.add<WidgetNode>(Align::START, entry.heading);
        row.add<WidgetNode>(Align::START, entry.button);
        control = entry.button;
    } else if (slider) {
        entry.slider = new Slider(this, setting.steps);
        entry.slider->changed = [this, index](i16 value) { change(index, value); };
        row.add<WidgetNode>(Align::FILL, entry.slider);
        control = entry.slider;
    } else {
        entry.heading = new Label(this, HEADING_FONT, Ink{WIDGET_FACE, WIDGET_EDGE});
        entry.choices = new ChoiceRow(this);
        entry.choices->changed = [this, index](i16 value) { change(index, value); };
        row.add<WidgetNode>(Align::START, entry.heading);
        row.add<WidgetNode>(Align::START, entry.choices);
        control = entry.choices;
    }
    control->touched = [this, index]() { setFocus(static_cast<i16>(index)); };
    rows_.push_back(std::move(entry));
}

void SettingsGump::change(std::size_t row, i16 value)
{
    rows_[row].setting.write(value);
    refresh();
    paint(0);
}

void SettingsGump::open(std::size_t row)
{
    openModalGump(rows_[row].setting.opens(), static_cast<u32>(-1), [this](s32 /*result*/) {
        refresh();
        paint(1);
    });
}

void SettingsGump::refresh()
{
    for (Row &row : rows_) {
        const i16 value = row.setting.read();
        const char *heading = inLanguage(row.setting.heading, gameLanguage);
        if (row.button != nullptr) {
            row.heading->setText(heading);
            row.button->setText(inLanguage(row.setting.choices[0].words, gameLanguage));
            continue;
        }
        if (row.slider != nullptr) {
            row.slider->setHeading(heading);
            row.slider->setStep(value);
            continue;
        }
        row.heading->setText(heading);
        std::vector<ChoiceRow::Choice> choices;
        for (const SettingChoice &choice : row.setting.choices) {
            choices.push_back(ChoiceRow::Choice{inLanguage(choice.words, gameLanguage), choice.warning});
        }
        row.choices->setChoices(std::move(choices));
        row.choices->setChosen(value);
    }
    const Rect area(
        static_cast<s16>(SETTINGS_AREA.left + AREA_MARGIN), static_cast<s16>(SETTINGS_AREA.top + AREA_MARGIN),
        static_cast<s16>(SETTINGS_AREA.right - AREA_MARGIN), static_cast<s16>(SETTINGS_AREA.bottom - AREA_MARGIN));
    layout_.place(area);
    corner_.place(area);
}

void SettingsGump::setFocus(i16 place)
{
    const auto doneFocus = static_cast<i16>(rows_.size());
    if (settingsFocus == doneFocus && place != doneFocus) {
        done_->showReleased();
    }
    settingsFocus = place;
    if (settingsFocus == doneFocus) {
        done_->showPressed();
    }
    paint(0);
}

Rect SettingsGump::focusFrame()
{
    if (settingsFocus < 0 || settingsFocus >= static_cast<i16>(rows_.size())) {
        return Rect(0, 0, 0, 0);
    }
    const Row &row = rows_[static_cast<std::size_t>(settingsFocus)];
    Rect frame = row.slider != nullptr ? row.slider->bounds : row.heading->bounds;
    if (row.choices != nullptr) {
        frame.extend(row.choices->bounds);
    }
    if (row.button != nullptr) {
        frame.extend(row.button->bounds);
    }
    return Rect(static_cast<s16>(frame.left - FOCUS_MARGIN), static_cast<s16>(frame.top - FOCUS_MARGIN),
                static_cast<s16>(frame.right + FOCUS_MARGIN), static_cast<s16>(frame.bottom + FOCUS_MARGIN));
}

void SettingsGump::paintSelf(i16 /*x*/, i16 /*y*/)
{
    /* The main menu's picture, as MainMenuGump::paintSelf draws it. */
    drawViewport->clear(0);
    const i16 left = (SCREEN_WIDTH - 638) >> 1;
    const i16 top = (SCREEN_HEIGHT - MAIN_MENU_HEIGHT) >> 1;
    const i16 half = MAIN_MENU_HEIGHT / 2;
    drawShape(left, top, gumpShapes->shapeFrame(0x36, 0));
    drawShape(static_cast<i16>(left + 319), top, gumpShapes->shapeFrame(0x37, 0));
    drawShape(left, static_cast<i16>(top + half), gumpShapes->shapeFrame(0x38, 0));
    drawShape(static_cast<i16>(left + 319), static_cast<i16>(top + half), gumpShapes->shapeFrame(0x39, 0));
    const Rect frame = focusFrame();
    if (frame.right != 0) {
        drawViewport->rectangle(WIDGET_FOCUS, frame.left, frame.top, frame.right, frame.bottom);
    }
}

void SettingsGump::handleCommand(NewGumpEvent *event)
{
    if (event->sender == done_) {
        settingsFocus = static_cast<i16>(rows_.size());
        close();
    }
}

void SettingsGump::handleKeyboard(NewGumpEvent *event)
{
    const auto places = static_cast<i16>(rows_.size() + 1);
    const bool onRow = settingsFocus < static_cast<i16>(rows_.size());
    switch (event->argument) {
    case ASCII_ESCAPE:
        close();
        break;
    case '\r':
        if (!onRow) {
            close();
        } else if (rows_[static_cast<std::size_t>(settingsFocus)].button != nullptr) {
            rows_[static_cast<std::size_t>(settingsFocus)].button->activate();
        }
        break;
    case '8':
    case KEYCODE_SCAN | SCAN_UP:
        setFocus(nextFocus(settingsFocus, places, -1));
        break;
    case '2':
    case KEYCODE_SCAN | SCAN_DOWN:
        setFocus(nextFocus(settingsFocus, places, 1));
        break;
    case '4':
    case KEYCODE_SCAN | SCAN_LEFT:
        if (onRow) {
            Row &row = rows_[static_cast<std::size_t>(settingsFocus)];
            if (row.slider != nullptr) {
                row.slider->step(-1);
            } else if (row.choices != nullptr) {
                row.choices->step(-1);
            }
        }
        break;
    case '6':
    case KEYCODE_SCAN | SCAN_RIGHT:
        if (onRow) {
            Row &row = rows_[static_cast<std::size_t>(settingsFocus)];
            if (row.slider != nullptr) {
                row.slider->step(1);
            } else if (row.choices != nullptr) {
                row.choices->step(1);
            }
        }
        break;
    default:
        break;
    }
}
