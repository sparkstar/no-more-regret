/* The port's key settings. See keysgump.h. */
#include "keysgump.h"
#include "host.h"
#include "gamestate.h"
#include "widgets_game.h"
#include "language.h"
#include "viewport.h"

#include <algorithm>
#include <cctype>

namespace {

/* The layout: the room inside the screen's edge, between the lines, between
 * the title and the preset, between a name's column and the inputs, the
 * focus frame's round its row. */
const s16 SCREEN_MARGIN = 20;
const s16 LINE_GAP = 10;
const s16 ROW_GAP = 4;
const s16 TITLE_GAP = 24;
const s16 NAME_GAP = 16;
const s16 FOCUS_MARGIN = 2;
/* The inputs' least room on a button. */
const s16 INPUTS_WIDTH = 300;

/* The group whose tab is chosen, kept from one opening to the next. */
i16 &keysGroup = gameGlobal<i16>(0);

struct ActionEntry {
    Action action;
    Words words;
};

/* Every action's words, in the enum's order. */
constexpr ActionEntry ACTION_WORDS[] = {
    {Action::NONE, Words{"", ""}},
    {Action::RUN, Words{"RUN", "달리기"}},
    {Action::RUN_LOCK, Words{"RUN LOCK", "달리기 고정"}},
    {Action::COMBAT_STEP, Words{"COMBAT STEP", "전투 걸음"}},
    {Action::ROLL, Words{"ROLL", "구르기"}},
    {Action::MOVE_FORWARD, Words{"FORWARD", "앞으로"}},
    {Action::MOVE_BACK, Words{"BACK", "뒤로"}},
    {Action::TURN_LEFT, Words{"TURN LEFT", "왼쪽으로 돌기"}},
    {Action::TURN_RIGHT, Words{"TURN RIGHT", "오른쪽으로 돌기"}},
    {Action::ADVANCE, Words{"ADVANCE", "전진"}},
    {Action::RETREAT, Words{"RETREAT", "후퇴"}},
    {Action::SLIDE_LEFT, Words{"SIDESTEP LEFT", "왼쪽 옆걸음"}},
    {Action::SLIDE_RIGHT, Words{"SIDESTEP RIGHT", "오른쪽 옆걸음"}},
    {Action::ROLL_LEFT, Words{"ROLL LEFT", "왼쪽 구르기"}},
    {Action::ROLL_RIGHT, Words{"ROLL RIGHT", "오른쪽 구르기"}},
    {Action::ROLL_FORWARD, Words{"ROLL FORWARD", "앞구르기"}},
    {Action::JUMP_FORWARD, Words{"JUMP FORWARD", "앞으로 점프"}},
    {Action::KNEEL, Words{"KNEEL", "웅크리기"}},
    {Action::MOVE_NORTHWEST, Words{"NORTHWEST", "북서쪽으로"}},
    {Action::MOVE_SOUTHEAST, Words{"SOUTHEAST", "남동쪽으로"}},
    {Action::MOVE_SOUTHWEST, Words{"SOUTHWEST", "남서쪽으로"}},
    {Action::MOVE_NORTHEAST, Words{"NORTHEAST", "북동쪽으로"}},
    {Action::MOVE_WEST, Words{"WEST", "서쪽으로"}},
    {Action::MOVE_NORTH, Words{"NORTH", "북쪽으로"}},
    {Action::MOVE_SOUTH, Words{"SOUTH", "남쪽으로"}},
    {Action::MOVE_EAST, Words{"EAST", "동쪽으로"}},
    {Action::NUMPAD_FORWARD, Words{"KEYPAD FORWARD", "키패드 앞으로"}},
    {Action::NUMPAD_BACK, Words{"KEYPAD BACK", "키패드 뒤로"}},
    {Action::NUMPAD_TURN_LEFT, Words{"KEYPAD TURN LEFT", "키패드 왼쪽으로 돌기"}},
    {Action::NUMPAD_TURN_RIGHT, Words{"KEYPAD TURN RIGHT", "키패드 오른쪽으로 돌기"}},
    {Action::NUMPAD_SLIDE_LEFT, Words{"KEYPAD SIDESTEP LEFT", "키패드 왼쪽 옆걸음"}},
    {Action::NUMPAD_SLIDE_RIGHT, Words{"KEYPAD SIDESTEP RIGHT", "키패드 오른쪽 옆걸음"}},
    {Action::NUMPAD_ROLL_LEFT, Words{"KEYPAD ROLL LEFT", "키패드 왼쪽 구르기"}},
    {Action::NUMPAD_ROLL_RIGHT, Words{"KEYPAD ROLL RIGHT", "키패드 오른쪽 구르기"}},
    {Action::FIRE, Words{"FIRE", "발사"}},
    {Action::JUMP, Words{"JUMP", "점프"}},
    {Action::READY_WEAPON, Words{"READY WEAPON", "무기 꺼내기 넣기"}},
    {Action::KEYPAD_USE, Words{"KEYPAD USE", "키패드 사용"}},
    {Action::KEYPAD_NEXT_WEAPON, Words{"KEYPAD NEXT WEAPON", "키패드 다음 무기"}},
    {Action::KEYPAD_NEXT_ITEM, Words{"KEYPAD NEXT ITEM", "키패드 다음 아이템"}},
    {Action::KEYPAD_SELECT, Words{"KEYPAD SELECT ITEMS", "키패드 아이템 고르기"}},
    {Action::CAMERA, Words{"CAMERA BACK", "카메라 되돌리기"}},
    {Action::RETURN_CONTROL, Words{"RETURN CONTROL", "조종 끝내기"}},
    {Action::MUSIC_LOUDER, Words{"MUSIC LOUDER", "음악 크게"}},
    {Action::MUSIC_QUIETER, Words{"MUSIC QUIETER", "음악 작게"}},
    {Action::RETICLE_STYLE, Words{"RETICLE STYLE", "조준선 모양"}},
    {Action::SELECT_WEAPON_1, Words{"WEAPON 1", "무기 1"}},
    {Action::SELECT_WEAPON_2, Words{"WEAPON 2", "무기 2"}},
    {Action::SELECT_WEAPON_3, Words{"WEAPON 3", "무기 3"}},
    {Action::SELECT_WEAPON_4, Words{"WEAPON 4", "무기 4"}},
    {Action::SELECT_WEAPON_5, Words{"WEAPON 5", "무기 5"}},
    {Action::SELECT_WEAPON_6, Words{"WEAPON 6", "무기 6"}},
    {Action::SELECT_WEAPON_7, Words{"WEAPON 7", "무기 7"}},
    {Action::SELECT_WEAPON_8, Words{"WEAPON 8", "무기 8"}},
    {Action::SELECT_WEAPON_9, Words{"WEAPON 9", "무기 9"}},
    {Action::SELECT_WEAPON_10, Words{"WEAPON 10", "무기 10"}},
    {Action::SELECT_WEAPON_11, Words{"WEAPON 11", "무기 11"}},
    {Action::SELECT_WEAPON_12, Words{"WEAPON 12", "무기 12"}},
    {Action::QUIT, Words{"QUIT GAME", "게임 끝내기"}},
    {Action::CANCEL, Words{"CANCEL", "취소"}},
    {Action::MENU, Words{"MENU", "메뉴"}},
    {Action::HELP, Words{"HELP", "도움말"}},
    {Action::OPTIONS, Words{"OPTIONS", "설정"}},
    {Action::QUICK_LOAD, Words{"QUICK LOAD", "빠른 불러오기"}},
    {Action::QUICK_SAVE, Words{"QUICK SAVE", "빠른 저장"}},
    {Action::LOAD, Words{"LOAD", "불러오기"}},
    {Action::SAVE, Words{"SAVE", "저장"}},
    {Action::DETONATE, Words{"DETONATE", "폭탄 터뜨리기"}},
    {Action::USE_ENERGY_CUBE, Words{"ENERGY CUBE", "에너지 큐브"}},
    {Action::USE_MEDKIT, Words{"MEDKIT", "의료 키트"}},
    {Action::USE_VIR_IMAGER, Words{"VIRTUAL IMAGER", "가상 이미저"}},
    {Action::USE_MINE_DETECTOR, Words{"MINE DETECTOR", "지뢰 탐지기"}},
    {Action::USE_RADIATION_SHIELD, Words{"RADIATION SHIELD", "방사선 차폐"}},
    {Action::TARGETING_RETICLE, Words{"TARGETING RETICLE", "조준선 켜기 끄기"}},
    {Action::DATALINK, Words{"DATALINK", "데이터링크"}},
    {Action::DROP_WEAPON, Words{"DROP WEAPON", "무기 버리기"}},
    {Action::SHOW_POSITION, Words{"SHOW POSITION", "위치 보기"}},
    {Action::CALIBRATE_JOYSTICK, Words{"CALIBRATE JOYSTICK", "조이스틱 보정"}},
    {Action::NEXT_SONG, Words{"NEXT SONG", "다음 곡"}},
    {Action::VERSION, Words{"VERSION", "판 정보"}},
    {Action::PREVIOUS_WEAPON, Words{"PREVIOUS WEAPON", "이전 무기"}},
    {Action::NEXT_WEAPON, Words{"NEXT WEAPON", "다음 무기"}},
    {Action::USE_ITEM, Words{"USE ITEM", "아이템 쓰기"}},
    {Action::PREVIOUS_ITEM, Words{"PREVIOUS ITEM", "이전 아이템"}},
    {Action::NEXT_ITEM, Words{"NEXT ITEM", "다음 아이템"}},
    {Action::SELECT_ITEMS, Words{"SELECT ITEMS", "아이템 고르기"}},
    {Action::GRAB_ITEMS, Words{"GRAB ITEMS", "고르며 집기"}},
    {Action::SELECTION_PREVIOUS, Words{"SELECTING PREVIOUS", "고르는 중 이전"}},
    {Action::SELECTION_NEXT, Words{"SELECTING NEXT", "고르는 중 다음"}},
    {Action::SELECTION_CHOOSE, Words{"SELECTING CHOOSE", "고르는 중 고르기"}},
    {Action::SELECTION_USE, Words{"SELECTING USE", "고르는 중 쓰기"}},
    {Action::SELECTION_CANCEL, Words{"SELECTING CANCEL", "고르는 중 그만두기"}},
    {Action::REWIND, Words{"REWIND", "되감기"}},
    {Action::SAVE_STATE, Words{"SAVE STATE", "상태 저장"}},
    {Action::LOAD_STATE, Words{"LOAD STATE", "상태 불러오기"}},
    {Action::FULLSCREEN, Words{"FULL SCREEN", "전체 화면"}},
    {Action::TAKE_OVER, Words{"TAKE OVER REPLAY", "재생 이어받기"}},
    {Action::FAST_FORWARD, Words{"FAST FORWARD", "빨리 감기"}},
    {Action::PAUSE, Words{"PAUSE", "정지"}},
};
static_assert(std::size(ACTION_WORDS) == static_cast<std::size_t>(Action::COUNT), "an action's words each");

/* Whether each action's words are at its place: actionWords takes them by
 * the action's number. */
constexpr bool wordsInOrder()
{
    for (std::size_t index = 0; index < std::size(ACTION_WORDS); index++) {
        if (ACTION_WORDS[index].action != static_cast<Action>(index)) {
            return false;
        }
    }
    return true;
}
static_assert(wordsInOrder(), "the actions' words in the enum's order");

/* The texts of the gump. */
const Words TITLE{"CONTROLS", "조작"};
const Words ORIGINAL_PRESET{"ORIGINAL", "원래대로"};
const Words MODERN_PRESET{"MODERN", "현대식"};
const Words MOUSE_ROLL{"MOUSE ROLL", "마우스 구르기"};
const Words ROLL_ON{"ON", "켬"};
const Words ROLL_OFF{"OFF", "끔"};
const Words HINT{"CLICK AN ACTION, THEN PRESS AN INPUT TO ADD IT. RIGHT CLICK CLEARS.",
                 "동작을 누르고 입력을 주면 더합니다. 오른쪽 클릭은 지우기."};
const Words WAIT_HINT{"PRESS A KEY, A MOUSE BUTTON, A PAD BUTTON OR A STICK. ESC CLEARS.",
                      "키, 마우스 버튼, 패드 버튼이나 스틱을 누르세요. ESC는 지우기."};
const Words WAITING{"PRESS AN INPUT", "입력을 누르세요"};
const Words NO_INPUT{"NONE", "없음"};

/* The words of an input of enhanced.cfg's text (inputsLabel): capitals, a
 * space for an underscore, Num Lock's state. */
std::string inputWords(const std::string &input)
{
    std::string base = input;
    std::string state;
    const std::size_t at = base.find('@');
    if (at != std::string::npos) {
        state = base.substr(at + 1) == "numlock" ? " NUM ON" : " NUM OFF";
        base = base.substr(0, at);
    }
    for (char &c : base) {
        c = (c == '_') ? ' ' : static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return base + state;
}

/* A text cut to width in the font, with dots at its end. */
std::string fitted(const std::string &text, s16 width)
{
    if (textExtent(CHOICE_FONT, text).width <= width) {
        return text;
    }
    std::string cut = text;
    while (!cut.empty() && textExtent(CHOICE_FONT, cut + "...").width > width) {
        /* A UTF-8 character's bytes at once. */
        do {
            cut.pop_back();
        } while (!cut.empty() && (static_cast<unsigned char>(cut.back()) & 0xc0) == 0x80);
        if (!cut.empty() && (static_cast<unsigned char>(cut.back()) & 0xc0) == 0xc0) {
            cut.pop_back();
        }
    }
    return cut + "...";
}

Ink faceInk()
{
    return Ink{WIDGET_FACE, WIDGET_EDGE};
}

Ink dimInk()
{
    return Ink{WIDGET_DIM_FACE, WIDGET_DIM_EDGE};
}

Ink warningInk()
{
    return Ink{WIDGET_WARNING_FACE, WIDGET_WARNING_EDGE};
}

}

namespace {

/* The groups' actions and the groups (actionGroups), constants. */
constexpr Action MOVE_ACTIONS[] = {Action::MOVE_FORWARD, Action::MOVE_BACK,  Action::TURN_LEFT,    Action::TURN_RIGHT,
                                   Action::RUN,          Action::RUN_LOCK,   Action::COMBAT_STEP,  Action::ROLL,
                                   Action::ADVANCE,      Action::RETREAT,    Action::SLIDE_LEFT,   Action::SLIDE_RIGHT,
                                   Action::ROLL_LEFT,    Action::ROLL_RIGHT, Action::ROLL_FORWARD, Action::JUMP,
                                   Action::JUMP_FORWARD, Action::KNEEL};
constexpr Action KEYPAD_ACTIONS[] = {
    Action::MOVE_NORTH,        Action::MOVE_NORTHEAST,     Action::MOVE_EAST,        Action::MOVE_SOUTHEAST,
    Action::MOVE_SOUTH,        Action::MOVE_SOUTHWEST,     Action::MOVE_WEST,        Action::MOVE_NORTHWEST,
    Action::NUMPAD_FORWARD,    Action::NUMPAD_BACK,        Action::NUMPAD_TURN_LEFT, Action::NUMPAD_TURN_RIGHT,
    Action::NUMPAD_SLIDE_LEFT, Action::NUMPAD_SLIDE_RIGHT, Action::NUMPAD_ROLL_LEFT, Action::NUMPAD_ROLL_RIGHT,
    Action::KEYPAD_USE,        Action::KEYPAD_NEXT_WEAPON, Action::KEYPAD_NEXT_ITEM, Action::KEYPAD_SELECT};
constexpr Action WEAPONS_ACTIONS[] = {Action::FIRE,
                                      Action::READY_WEAPON,
                                      Action::PREVIOUS_WEAPON,
                                      Action::NEXT_WEAPON,
                                      Action::SELECT_WEAPON_1,
                                      Action::SELECT_WEAPON_2,
                                      Action::SELECT_WEAPON_3,
                                      Action::SELECT_WEAPON_4,
                                      Action::SELECT_WEAPON_5,
                                      Action::SELECT_WEAPON_6,
                                      Action::SELECT_WEAPON_7,
                                      Action::SELECT_WEAPON_8,
                                      Action::SELECT_WEAPON_9,
                                      Action::SELECT_WEAPON_10,
                                      Action::SELECT_WEAPON_11,
                                      Action::SELECT_WEAPON_12,
                                      Action::DROP_WEAPON,
                                      Action::TARGETING_RETICLE,
                                      Action::RETICLE_STYLE,
                                      Action::DETONATE};
constexpr Action ITEMS_ACTIONS[] = {Action::USE_ITEM,          Action::PREVIOUS_ITEM,        Action::NEXT_ITEM,
                                    Action::SELECT_ITEMS,      Action::GRAB_ITEMS,           Action::USE_MEDKIT,
                                    Action::USE_ENERGY_CUBE,   Action::USE_RADIATION_SHIELD, Action::USE_VIR_IMAGER,
                                    Action::USE_MINE_DETECTOR, Action::SELECTION_PREVIOUS,   Action::SELECTION_NEXT,
                                    Action::SELECTION_CHOOSE,  Action::SELECTION_USE,        Action::SELECTION_CANCEL};
constexpr Action GAME_ACTIONS[] = {Action::MENU,
                                   Action::CANCEL,
                                   Action::OPTIONS,
                                   Action::HELP,
                                   Action::QUICK_SAVE,
                                   Action::QUICK_LOAD,
                                   Action::SAVE,
                                   Action::LOAD,
                                   Action::QUIT,
                                   Action::CAMERA,
                                   Action::RETURN_CONTROL,
                                   Action::DATALINK,
                                   Action::SHOW_POSITION,
                                   Action::MUSIC_LOUDER,
                                   Action::MUSIC_QUIETER,
                                   Action::NEXT_SONG,
                                   Action::CALIBRATE_JOYSTICK,
                                   Action::VERSION,
                                   Action::REWIND,
                                   Action::FAST_FORWARD,
                                   Action::PAUSE,
                                   Action::SAVE_STATE,
                                   Action::LOAD_STATE,
                                   Action::FULLSCREEN,
                                   Action::TAKE_OVER};
constexpr ActionGroup ACTION_GROUPS[] = {
    {Words{"MOVE", "이동"}, MOVE_ACTIONS},       {Words{"KEYPAD", "키패드"}, KEYPAD_ACTIONS},
    {Words{"WEAPONS", "무기"}, WEAPONS_ACTIONS}, {Words{"ITEMS", "아이템"}, ITEMS_ACTIONS},
    {Words{"GAME", "게임"}, GAME_ACTIONS},
};

}

std::span<const ActionGroup> actionGroups()
{
    return ACTION_GROUPS;
}

Words actionWords(Action action)
{
    return ACTION_WORDS[static_cast<std::size_t>(action)].words;
}

std::string inputsLabel(const std::string &inputs)
{
    std::vector<std::string> words;
    std::size_t at = 0;
    while (at < inputs.size()) {
        std::size_t end = inputs.find(' ', at);
        if (end == std::string::npos) {
            end = inputs.size();
        }
        if (end > at) {
            words.push_back(inputs.substr(at, end - at));
        }
        at = end + 1;
    }
    auto has = [&words](const std::string &word) { return std::ranges::find(words, word) != words.end(); };
    std::string label;
    for (const std::string &word : words) {
        std::string shown;
        const char c = word[0];
        const bool small = c >= 'a' && c <= 'z';
        const bool capital = c >= 'A' && c <= 'Z';
        if (word.size() == 1 && small) {
            shown = std::string(1, static_cast<char>(c - 'a' + 'A'));
        } else if (word.size() == 1 && capital) {
            /* A capital whose small letter is there too is shown with it. */
            if (has(std::string(1, static_cast<char>(c - 'A' + 'a')))) {
                continue;
            }
            shown = "SHIFT+" + word;
        } else if (word.size() == 1) {
            shown = word;
        } else {
            shown = inputWords(word);
        }
        label += label.empty() ? shown : ", " + shown;
    }
    return label;
}

void KeysGump::ActionButton::setAction(Action shown, s16 nameWidth)
{
    action_ = shown;
    nameWidth_ = nameWidth;
}

Extent KeysGump::ActionButton::preferredExtent()
{
    return Extent{static_cast<s16>(nameWidth_ + NAME_GAP + INPUTS_WIDTH), textExtent(CHOICE_FONT, "A").height};
}

void KeysGump::ActionButton::paintSelf(i16 x, i16 y)
{
    if (action_ == Action::NONE) {
        return;
    }
    drawText(CHOICE_FONT, inLanguage(actionWords(action_), gameLanguage), x, y, faceInk());
    const auto inputsX = static_cast<s16>(x + nameWidth_ + NAME_GAP);
    const auto room = static_cast<s16>(width() - nameWidth_ - NAME_GAP);
    if (capturingAction() == action_) {
        drawText(CHOICE_FONT, fitted(inLanguage(WAITING, gameLanguage), room), inputsX, y, warningInk());
        return;
    }
    std::string inputs = inputsLabel(boundText(action_));
    if (inputs.empty()) {
        inputs = inLanguage(NO_INPUT, gameLanguage);
    }
    drawText(CHOICE_FONT, fitted(inputs, room), inputsX, y, dimInk());
}

void KeysGump::ActionButton::handleLeftButton(NewGumpEvent *event)
{
    if (event->isMousePress() && action_ != Action::NONE) {
        keys_->pressed(row_, false);
    }
}

void KeysGump::ActionButton::handleRightButton(NewGumpEvent *event)
{
    if (event->isMousePress() && action_ != Action::NONE) {
        keys_->pressed(row_, true);
    }
}

KeysGump::KeysGump()
    : NewGump(NewGumpId(), nullptr, 0), layout_(Axis::VERTICAL, LINE_GAP, Align::START), title_(nullptr),
      preset_(nullptr), mouseRollLabel_(nullptr), mouseRoll_(nullptr), groups_(nullptr), hint_(nullptr), done_(nullptr)
{
    eventMask |= EVENT_LEFT_BUTTON | EVENT_RIGHT_BUTTON | EVENT_EXECUTE | EVENT_COMMAND;
    bounds.set(0, 0, static_cast<s16>(drawViewport->right() - drawViewport->left()),
               static_cast<s16>(drawViewport->bottom() - drawViewport->top()));
    addAcceptedArguments(-1, 0);
    LayoutBox &top = layout_.add<LayoutBox>(Align::START, Axis::HORIZONTAL, TITLE_GAP, Align::START);
    title_ = new Label(this, HEADING_FONT, faceInk());
    top.add<WidgetNode>(Align::END, title_);
    preset_ = new ChoiceRow(this);
    preset_->touched = [this]() { endWait(); };
    preset_->changed = [this](i16 chosen) {
        setKeyPreset(chosen == 1 ? KeyPreset::MODERN : KeyPreset::ORIGINAL);
        refresh();
        paint(0);
    };
    top.add<WidgetNode>(Align::END, preset_);
    /* Whether the mouse rolls, beside the preset: the rows below stay
     * where they were. */
    mouseRollLabel_ = new Label(this, CHOICE_FONT, faceInk());
    top.add<WidgetNode>(Align::END, mouseRollLabel_);
    mouseRoll_ = new ChoiceRow(this);
    mouseRoll_->touched = [this]() { endWait(); };
    mouseRoll_->changed = [this](i16 chosen) {
        setMouseRoll(chosen == 0);
        refresh();
        paint(0);
    };
    top.add<WidgetNode>(Align::END, mouseRoll_);
    groups_ = new ChoiceRow(this);
    groups_->touched = [this]() { endWait(); };
    groups_->changed = [this](i16 chosen) {
        showGroup(chosen);
        paint(0);
    };
    layout_.add<WidgetNode>(Align::START, groups_);
    LayoutBox &list = layout_.add<LayoutBox>(Align::FILL, Axis::VERTICAL, ROW_GAP, Align::START);
    std::size_t rows = 0;
    for (const ActionGroup &group : actionGroups()) {
        rows = std::max(rows, group.actions.size());
    }
    for (std::size_t row = 0; row < rows; row++) {
        auto *button = new ActionButton(this, row);
        buttons_.push_back(button);
        list.add<WidgetNode>(Align::FILL, button);
    }
    layout_.add<Stretch>(Align::START);
    /* The hint's line, then Done at the right. */
    hint_ = new Label(this, CHOICE_FONT, dimInk());
    layout_.add<WidgetNode>(Align::START, hint_);
    done_ = new DoneButton(this);
    layout_.add<GumpNode>(Align::END, done_);
    keysGroup = std::clamp(keysGroup, static_cast<i16>(0), static_cast<i16>(actionGroups().size() - 1));
    showGroup(keysGroup);
    paint(1);
}

KeysGump::~KeysGump()
{
    stopCapture();
}

void KeysGump::showGroup(i16 group)
{
    endWait();
    keysGroup = group;
    focus_ = 0;
    refresh();
}

void KeysGump::refresh()
{
    title_->setText(inLanguage(TITLE, gameLanguage));
    preset_->setChoices({ChoiceRow::Choice{inLanguage(ORIGINAL_PRESET, gameLanguage), false},
                         ChoiceRow::Choice{inLanguage(MODERN_PRESET, gameLanguage), false}});
    preset_->setChosen(static_cast<i16>(keyPreset() == KeyPreset::MODERN ? 1 : 0));
    mouseRollLabel_->setText(inLanguage(MOUSE_ROLL, gameLanguage));
    mouseRoll_->setChoices({ChoiceRow::Choice{inLanguage(ROLL_ON, gameLanguage), false},
                            ChoiceRow::Choice{inLanguage(ROLL_OFF, gameLanguage), false}});
    mouseRoll_->setChosen(static_cast<i16>(mouseRoll() ? 0 : 1));
    std::vector<ChoiceRow::Choice> tabs;
    s16 nameWidth = 0;
    for (const ActionGroup &group : actionGroups()) {
        tabs.push_back(ChoiceRow::Choice{inLanguage(group.words, gameLanguage), false});
        for (const Action action : group.actions) {
            nameWidth =
                std::max(nameWidth, textExtent(CHOICE_FONT, inLanguage(actionWords(action), gameLanguage)).width);
        }
    }
    groups_->setChoices(std::move(tabs));
    groups_->setChosen(keysGroup);
    /* The group's actions that the game has, in the rows from the top; the
     * debug features' only with them (host.h). */
    std::vector<Action> shown;
    for (const Action action : actionGroups()[static_cast<std::size_t>(keysGroup)].actions) {
        if (gameHasAction(action) && (!debugAction(action) || hostDebugFeatures())) {
            shown.push_back(action);
        }
    }
    for (std::size_t row = 0; row < buttons_.size(); row++) {
        buttons_[row]->setAction(row < shown.size() ? shown[row] : Action::NONE, nameWidth);
    }
    focus_ = std::clamp(focus_, static_cast<i16>(0), static_cast<i16>(std::max<std::size_t>(shown.size(), 1) - 1));
    waitShown_ = capturingAction() != Action::NONE;
    hint_->setText(inLanguage(waitShown_ ? WAIT_HINT : HINT, gameLanguage));
    layout_.place(Rect(SCREEN_MARGIN, SCREEN_MARGIN, static_cast<s16>(bounds.right - SCREEN_MARGIN),
                       static_cast<s16>(bounds.bottom - SCREEN_MARGIN)));
}

void KeysGump::wait(std::size_t row)
{
    focus_ = static_cast<i16>(row);
    startCapture(buttons_[row]->action());
    refresh();
    paint(0);
}

void KeysGump::endWait()
{
    if (capturingAction() == Action::NONE) {
        return;
    }
    stopCapture();
    refresh();
    paint(0);
}

void KeysGump::pressed(std::size_t row, bool right)
{
    const Action action = buttons_[row]->action();
    if (capturingAction() != Action::NONE) {
        /* The waiting action's own button gives the mouse's button; another
         * ends the wait. */
        if (capturingAction() == action) {
            captureDevice(right ? Device::MOUSE_RIGHT : Device::MOUSE_LEFT);
            refresh();
            paint(0);
        } else {
            endWait();
        }
        return;
    }
    focus_ = static_cast<i16>(row);
    if (right) {
        clearInputs(action);
        refresh();
        paint(0);
        return;
    }
    wait(row);
}

void KeysGump::execute(NewGumpEvent * /*event*/)
{
    /* A wait the keys or the other devices ended. */
    if (waitShown_ && capturingAction() == Action::NONE) {
        refresh();
        paint(0);
    }
}

void KeysGump::handleLeftButton(NewGumpEvent *event)
{
    if (event->isMousePress()) {
        endWait();
    }
}

void KeysGump::handleRightButton(NewGumpEvent *event)
{
    if (event->isMousePress()) {
        endWait();
    }
}

void KeysGump::handleCommand(NewGumpEvent *event)
{
    if (event->sender == done_) {
        close();
    }
}

void KeysGump::handleKeyboard(NewGumpEvent *event)
{
    i16 rows = 0;
    for (const ActionButton *button : buttons_) {
        if (button->action() != Action::NONE) {
            rows++;
        }
    }
    const auto groups = static_cast<i16>(actionGroups().size());
    switch (event->argument) {
    case ASCII_ESCAPE:
        close();
        break;
    case '\r':
        if (focus_ < rows) {
            wait(static_cast<std::size_t>(focus_));
        }
        break;
    case '8':
    case KEYCODE_SCAN | SCAN_UP:
        focus_ = std::max(static_cast<i16>(focus_ - 1), static_cast<i16>(0));
        paint(0);
        break;
    case '2':
    case KEYCODE_SCAN | SCAN_DOWN:
        focus_ = std::min(static_cast<i16>(focus_ + 1), static_cast<i16>(std::max(rows - 1, 0)));
        paint(0);
        break;
    case '4':
    case KEYCODE_SCAN | SCAN_LEFT:
        showGroup(static_cast<i16>((keysGroup + groups - 1) % groups));
        paint(0);
        break;
    case '6':
    case KEYCODE_SCAN | SCAN_RIGHT:
        showGroup(static_cast<i16>((keysGroup + 1) % groups));
        paint(0);
        break;
    default:
        break;
    }
}

Rect KeysGump::focusFrame()
{
    if (focus_ < 0 || static_cast<std::size_t>(focus_) >= buttons_.size() ||
        buttons_[static_cast<std::size_t>(focus_)]->action() == Action::NONE) {
        return Rect(0, 0, 0, 0);
    }
    const Rect &row = buttons_[static_cast<std::size_t>(focus_)]->bounds;
    return Rect(static_cast<s16>(row.left - FOCUS_MARGIN), static_cast<s16>(row.top - FOCUS_MARGIN),
                static_cast<s16>(row.right + FOCUS_MARGIN), static_cast<s16>(row.bottom + FOCUS_MARGIN));
}

void KeysGump::paintSelf(i16 /*x*/, i16 /*y*/)
{
    drawViewport->clear(0);
    const Rect frame = focusFrame();
    if (frame.right != 0) {
        drawViewport->rectangle(WIDGET_FOCUS, frame.left, frame.top, frame.right, frame.bottom);
    }
}
