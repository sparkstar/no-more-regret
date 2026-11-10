/* The terminal in a language other than English (the game's text in other
 * languages): ComputerGump's (compgump.h) typing, pauses, pages, keys and
 * gump, with the text laid out by each character's width, as the original's 20
 * columns of 8 pixels fit only its font. A syllable of the Hangul font
 * (hangul.h) is 10 pixels wide, a character of the game's font 6 8; a row is
 * 160 pixels wide and 10 high (the syllables are 9), a page 11 rows, so that
 * <MORE> fits under it on the screen as the original's does under its 13 rows
 * of 9. Rows break between words; a word wider than a row breaks where the row
 * ends. ^ starts a row, * pauses and is not shown, as in the original. */
#ifndef PORT_TERMGUMP_H
#define PORT_TERMGUMP_H
#include "newgump.h"

#include <cstddef>
#include <vector>

class TerminalGump : public NewGump {
public:
    explicit TerminalGump(const char *text);
    ~TerminalGump() override;
    void execute(NewGumpEvent *event) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;

    /* What one step of the typing does: shows a character at its place, starts a
     * row, or pauses. */
    enum class StepKind : u8 { CHARACTER, ROW, PAUSE };
    struct Step {
        StepKind kind;
        char32_t code;
        /* The place of the character, or the pen's after the step. */
        s16 x;
        s16 row;
    };

private:
    /* The steps of each page. */
    std::vector<std::vector<Step>> pages;
    std::size_t page = 0;
    /* The steps of the page typed so far. */
    std::size_t typed = 0;
    /* Type the rest of the page at once. */
    bool fast = false;
    u8 savedAnimations;

    [[nodiscard]] bool pageTyped() const;
    [[nodiscard]] bool finished() const;
    /* Types the next step of the page. */
    const Step &typeStep();
};

/* The pages of steps a text is typed in (UTF-8, with the original's
 * controls). */
std::vector<std::vector<TerminalGump::Step>> terminalPages(const char *text);

/* Each game's (its compgump.cpp): ComputerGump's waits after a character
 * and at a pause. */
void terminalCharacterWait();
void terminalPauseWait();
#endif
