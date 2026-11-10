/* The terminal in a language other than English. See termgump.h. */
#include "termgump.h"
#include "cafont.h"
#include "camera.h"
#include "crusys.h"
#include "dispatch.h"
#include "dsfxman.h"
#include "gametime.h"
#include "gumpshap.h"
#include "hangul.h"
#include "joydrv.h"
#include "kernel.h"
#include "mousectl.h"
#include "u9mouse.h"
#include "viewport.h"
#include "input.h"
#include "utf8.h"

namespace {

/* ComputerGump's: its gump, where the text starts in it, the width of the
 * original's 20 columns, its font and the colour of that font's ink. */
const u16 TERMINAL_SHAPE = 0x1e;
const s16 SECOND_FRAME = 160;
const s16 TEXT_X = 41;
const s16 TEXT_Y = 38;
const s16 ROW_WIDTH = 160;
const i16 TERMINAL_FONT = 6;
const u8 INK = 68;
/* The layout of the other languages: the rows' height and a page's rows
 * (11 rows of 10 and <MORE> fill what 13 rows of 9 and <MORE> did). */
const s16 ROW_HEIGHT = 10;
const s16 PAGE_ROWS = 11;
/* The sound of a typed character (ComputerGump's). */
const i16 TYPING_EFFECT = 0x33;

/* The code points of UTF-8 text; a byte that starts no character is '?'. */
std::vector<char32_t> codePoints(const char *text)
{
    std::vector<char32_t> out;
    const u8 *at = reinterpret_cast<const u8 *>(text);
    while (*at != 0U) {
        const u8 first = *at;
        const int extra = utf8Following(first);
        if (extra < 0) {
            out.push_back('?');
            at++;
            continue;
        }
        char32_t code = extra == 0 ? first : first & (0x3f >> extra);
        int i = 1;
        for (; i <= extra && (at[i] & 0xc0) == 0x80; i++) {
            code = code << 6 | (at[i] & 0x3f);
        }
        if (i <= extra) {
            out.push_back('?');
            at += i;
            continue;
        }
        out.push_back(code);
        at += extra + 1;
    }
    return out;
}

/* The width a character takes: a syllable of the Hangul font's, else the
 * glyph of font 6 (a character it has not as '?'). */
s16 characterWidth(char32_t code)
{
    if (hangulGlyph(code) != nullptr) {
        return HANGUL_ADVANCE;
    }
    CAFont font(TERMINAL_FONT);
    return font.glyphWidth(code < 0x80 ? static_cast<u8>(code) : static_cast<u8>('?'));
}

/* Draws a character at (x, y): a syllable of the Hangul font, else font 6's
 * glyph. */
void drawCharacter(char32_t code, s16 x, s16 y)
{
    if (const HangulGlyph *glyph = hangulGlyph(code)) {
        drawHangul(*glyph, x, y, INK);
        return;
    }
    CAFont font(TERMINAL_FONT);
    font.putChar(x, y, code < 0x80 ? static_cast<u8>(code) : static_cast<u8>('?'));
}

}

std::vector<std::vector<TerminalGump::Step>> terminalPages(const char *text)
{
    using Step = TerminalGump::Step;
    using StepKind = TerminalGump::StepKind;
    std::vector<char32_t> codes = codePoints(text);
    std::vector<Step> steps;
    s16 x = 0;
    s16 row = 0;
    auto newRow = [&]() {
        x = 0;
        row++;
    };
    std::size_t i = 0;
    while (i < codes.size()) {
        char32_t code = codes[i];
        if (code == '^') {
            newRow();
            steps.push_back(Step{StepKind::ROW, code, x, row});
            i++;
            continue;
        }
        if (code == '*') {
            steps.push_back(Step{StepKind::PAUSE, code, x, row});
            i++;
            continue;
        }
        if (code == 1) {
            i++;
            continue;
        }
        if (code == ' ') {
            const s16 width = characterWidth(code);
            if (x + width > ROW_WIDTH) {
                /* The space a row breaks at is not shown. */
                newRow();
            } else {
                steps.push_back(Step{StepKind::CHARACTER, code, x, row});
                x += width;
            }
            i++;
            continue;
        }
        /* A word: up to the next space or ^, on the next row when it does not
         * fit the rest of this one. */
        std::size_t end = i;
        s16 wordWidth = 0;
        for (; end < codes.size() && codes[end] != ' ' && codes[end] != '^'; end++) {
            if (codes[end] != '*' && codes[end] != 1) {
                wordWidth += characterWidth(codes[end]);
            }
        }
        if (x > 0 && x + wordWidth > ROW_WIDTH) {
            newRow();
        }
        for (; i < end; i++) {
            code = codes[i];
            if (code == '*') {
                steps.push_back(Step{StepKind::PAUSE, code, x, row});
                continue;
            }
            if (code == 1) {
                continue;
            }
            const s16 width = characterWidth(code);
            if (x > 0 && x + width > ROW_WIDTH) {
                newRow();
            }
            steps.push_back(Step{StepKind::CHARACTER, code, x, row});
            x += width;
        }
    }
    std::vector<std::vector<Step>> pages;
    for (const Step &step : steps) {
        const std::size_t number = static_cast<std::size_t>(step.row / PAGE_ROWS);
        while (pages.size() <= number) {
            pages.emplace_back();
        }
        pages[number].push_back(step);
    }
    /* No page after the text's last character. */
    while (pages.size() > 1) {
        bool shown = false;
        for (const Step &step : pages.back()) {
            shown = shown || (step.kind == StepKind::CHARACTER && step.code != ' ');
        }
        if (shown) {
            break;
        }
        pages.pop_back();
    }
    if (pages.empty()) {
        pages.emplace_back();
    }
    return pages;
}

TerminalGump::TerminalGump(const char *text) : NewGump(NewGumpId(), nullptr, 0), pages(terminalPages(text))
{
    savedAnimations = animationsHeld;
    animationsHeld = 1;
    if (joystickReady != 0U) {
        do {
            pollJoystick();
        } while ((joystickState & JOYSTICK_BUTTONS) != 0);
    }
    eventMask |= EVENT_EXECUTE | EVENT_LEFT_BUTTON | EVENT_KEY;
    addAcceptedArguments(-1, 0);
    GumpShape image(TERMINAL_SHAPE);
    bounds = image.dimensions();
    image.frame = 1;
    bounds.bottom += image.height();
    bounds.moveTo(static_cast<i16>(drawViewport->right() - drawViewport->left() + 1 - RECT_WIDTH(bounds)) >> 1,
                  static_cast<i16>(drawViewport->bottom() - drawViewport->top() + 1 - RECT_HEIGHT(bounds)) >> 1);
    gameMouse->hide();
    inputMousePlace(0, 0);
    worldActive = 0;
    paint(0);
}

TerminalGump::~TerminalGump()
{
    animationsHeld = savedAnimations;
    worldActive = 1;
    gameMouse->show();
    gameCamera->repaint();
    gameCamera->run();
    flushKeys();
    gumpKeys.clear();
}

bool TerminalGump::pageTyped() const
{
    return typed >= pages[page].size();
}

bool TerminalGump::finished() const
{
    return page + 1 >= pages.size() && pageTyped();
}

const TerminalGump::Step &TerminalGump::typeStep()
{
    return pages[page][typed++];
}

void TerminalGump::paintSelf(i16 x, i16 y)
{
    GumpShape image(TERMINAL_SHAPE);
    image.draw(x, y);
    image.frame = 1;
    image.draw(x, y + SECOND_FRAME);
    const std::vector<Step> &steps = pages[page];
    s16 penX = 0;
    s16 penRow = 0;
    for (std::size_t i = 0; i < typed; i++) {
        const Step &step = steps[i];
        const s16 row = step.row % PAGE_ROWS;
        if (step.kind == StepKind::CHARACTER) {
            drawCharacter(step.code, x + TEXT_X + step.x, y + TEXT_Y + (row * ROW_HEIGHT));
            penX = step.x + characterWidth(step.code);
        } else {
            penX = step.x;
        }
        penRow = row;
    }
    CAFont font(TERMINAL_FONT);
    if (!pageTyped()) {
        if (!fast) {
            font.putChar(x + TEXT_X + penX, y + TEXT_Y + (penRow * ROW_HEIGHT), '[');
        }
        return;
    }
    if (!finished()) {
        font.draw(x + TEXT_X, y + TEXT_Y + (PAGE_ROWS * ROW_HEIGHT), "<MORE>");
    }
}

void TerminalGump::execute(NewGumpEvent * /*unused*/)
{
    if (joystickReady != 0U) {
        pollJoystick();
        if ((joystickState & JOYSTICK_BUTTONS) != 0) {
            handleKeyboard(&lvalue(NewGumpEvent(EVENT_KEY, 0, 0, ASCII_ESCAPE)));
            return;
        }
    }
    if (pageTyped()) {
        paint(1);
        return;
    }
    if (fast) {
        while (!pageTyped()) {
            typeStep();
        }
        fast = false;
        paint(1);
        return;
    }
    const Step &step = typeStep();
    paint(1);
    if (step.kind == StepKind::PAUSE) {
        terminalPauseWait();
        return;
    }
    stopEffect(TYPING_EFFECT);
    playEffect(TYPING_EFFECT);
    terminalCharacterWait();
}

void TerminalGump::handleLeftButton(NewGumpEvent *event)
{
    if (event->isMousePress()) {
        if (!finished()) {
            fast = true;
            return;
        }
        close();
    }
}

void TerminalGump::handleKeyboard(NewGumpEvent *event)
{
    if (pageTyped() && !finished()) {
        page++;
        typed = 0;
        return;
    }
    if (event->argument != ASCII_ESCAPE && event->argument != '\r') {
        return;
    }
    if (!finished()) {
        if (event->argument == ASCII_ESCAPE) {
            fast = true;
        }
        return;
    }
    close();
}
