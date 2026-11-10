/* The port's settings gump, in place of the options gump (mainmenu.h): the
 * options gump's settings and the port's (the language, the keys' preset,
 * the subtitles, the sound's depth, the experimental wide screen), in the
 * port's widgets (widgets.h) laid out (layout.h) over the main menu's
 * picture, each game's own setting by its values (options_game.h,
 * settings_game.h). Closing it writes crusader.cfg, as closing the options
 * gump does, and with it the port's enhanced.cfg (enhanced.h: the language,
 * the sound's depth, the keys' preset). */
#ifndef PORT_SETTINGS_H
#define PORT_SETTINGS_H
#include "language.h"
#include "layout.h"
#include "newgump.h"
#include "palette.h"
#include "shapebtn.h"
#include "widgets.h"

#include <functional>
#include <span>
#include <vector>

/* A text in the port's languages (language.h). */
struct Words {
    const char *english;
    const char *korean;
};
[[nodiscard]] const char *inLanguage(const Words &words, Language language);

/* A choice of a setting; a warning is drawn red when chosen, as the
 * originals' OFF. */
struct SettingChoice {
    Words words;
    bool warning;
};

/* A setting of the gump: a heading over a row of choices, or a slider of
 * steps (no choices). Its value, the index of a choice or a step, is read
 * from the game and written to it. With opens, a heading over a button (its
 * first choice's words) that opens the gump opens makes, after which the
 * settings are read again. */
struct Setting {
    Words heading;
    std::vector<SettingChoice> choices;
    i16 steps;
    std::function<i16()> read;
    std::function<void(i16)> write;
    std::function<NewGump *()> opens;
};

/* The two columns of settings; Done is apart from them, in the area's
 * bottom right corner. */
struct SettingColumns {
    std::vector<Setting> left;
    std::vector<Setting> right;
};

/* The columns: the options gump's settings, each game's fifth its own, and
 * the port's (settings.cpp). */
[[nodiscard]] SettingColumns gameSettingColumns();
/* Plays the sound the options gump opens with, when the game has one
 * (settings_game.h). */
void playSettingsOpened();
[[nodiscard]] Setting animationsSetting();
[[nodiscard]] Setting frameSkipSetting();
[[nodiscard]] Setting blastsSetting();
[[nodiscard]] Setting soundVolumeSetting();
[[nodiscard]] Setting musicVolumeSetting();
[[nodiscard]] Setting mouseSetting();
[[nodiscard]] Setting languageSetting();
[[nodiscard]] Setting keysSetting();
[[nodiscard]] Setting soundDepthSetting();
[[nodiscard]] Setting subtitlesSetting();
[[nodiscard]] Setting wideScreenSetting();
[[nodiscard]] Setting gameSpeedSetting();
/* The ON and OFF choices of a setting that is on or off. */
[[nodiscard]] std::vector<SettingChoice> onOffChoices();

/* The step of a table of levels a volume has: the first level at or above
 * it, the last for one above them all. */
[[nodiscard]] i16 volumeStep(std::span<const u8> levels, i16 volume);
/* The focus the keys move to from focus among count places (the settings'
 * rows, then the Done button): up (-1) or down (1), stopping at the ends. */
[[nodiscard]] i16 nextFocus(i16 focus, i16 count, i16 direction);

class SettingsGump : public NewGump {
public:
    SettingsGump();
    ~SettingsGump() override;
    SettingsGump(const SettingsGump &) = delete;
    SettingsGump &operator=(const SettingsGump &) = delete;
    SettingsGump(SettingsGump &&) = delete;
    SettingsGump &operator=(SettingsGump &&) = delete;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;

private:
    /* A setting and its widgets: a heading and a row of choices, or a
     * button, or a slider, whose heading is its own. */
    struct Row {
        Setting setting;
        Label *heading;
        ChoiceRow *choices;
        Slider *slider;
        TextButton *button;
    };
    void addRow(LayoutBox &column, const Setting &setting);
    /* Writes a row's new value, refreshes and paints. */
    void change(std::size_t row, i16 value);
    /* Opens a button's gump; the settings read again when it closes. */
    void open(std::size_t row);
    /* The widgets' texts in the language and values from the game, and
     * their places again. */
    void refresh();
    void setFocus(i16 place);
    /* The frame round the row the keys stand on; all 0 for none (Done). */
    [[nodiscard]] Rect focusFrame();
    Palette palette_;
    std::vector<Row> rows_;
    /* The columns, and Done in the bottom right corner of the same area. */
    LayoutBox layout_;
    LayoutBox corner_;
    ShapeButtonGump *done_;
};
#endif
