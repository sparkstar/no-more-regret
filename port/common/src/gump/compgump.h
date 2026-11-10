/* Code115 (1390:0000-0aa2, file name unknown): the computer gump, a terminal
 * that types a text out a character at a time in the GUI font over a gump
 * shape. '^' breaks a line, 1 and '*' are not shown, and '*' also pauses;
 * <MORE> waits for a key at the end of a page. */
#ifndef PORT_COMPGUMP_H
#define PORT_COMPGUMP_H
#include "newgump.h"

#include <memory>

constexpr int TEXT_COLUMNS = 20;
constexpr int TEXT_ROWS = 13;

class ComputerGump : public NewGump {
public:
    /* Its own: freed after the destructor's body, which repaints the screen. */
    std::unique_ptr<char[]> text;
    i16 length; /* the characters typed so far */
    u8 unused3a;
    u8 pageFull;
    u16 pageStart;
    Rect unused3e;
    s16 column, row;
    u8 fast; /* type the rest of the page at once */
    u8 savedFlag;

    explicit ComputerGump(const char *source);
    /* UNCALLED: nothing calls it. */
    void unused0000();
    ~ComputerGump() override;
    void execute(NewGumpEvent *event) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    /* Copies source and breaks its lines at TEXT_COLUMNS. */
    void setText(const char *source);
    /* Draws the page up to length and types the next character. */
    void typeText(s16 x, s16 y);
};
ASSERT_SIZE(ComputerGump, 0x4c);

/* Shows text on the computer gump with the world stopped (1390:0005). */
void showComputerText(const char *text);
#endif
