/* ..\U9UI\WORLD.C's help pages (the original declares HelpGump in the
 * module, the port in a header of its own). The pages, their keys and the
 * dialog's start are each game's: No Regret's F3 shows help1.bmp to
 * help5.bmp of the static directory (vtable 1480:0238), No Remorse's F1
 * or? help1.dat and help2.dat and help3.dat the hidden one (vtable
 * 1478:6241). A command or a key turns the page; Escape closes. */
#ifndef PORT_HELP_H
#define PORT_HELP_H
#include "common.h"
#include "bmp.h"
#include "dialog.h"
#include "palette.h"

class HelpGump : public DialogGump {
public:
    u16 unknown47; /* cleared after a key turns the page */
    i16 page;      /* 0-4 */
    Palette palette;
    BmpFile bmp;

    HelpGump();
    ~HelpGump() override;
    /* Nothing: the page is drawn straight on the screen. */
    void paintSelf(i16 x, i16 y) override;
    /* No Regret's does nothing; No Remorse's plays a sound now and then on
     * the hidden page. */
    void execute(NewGumpEvent *event) override;
    void handleLeftButton(NewGumpEvent * /*event*/) override {}
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    /* Shows page (5 wraps to 0); always 1. */
    bool showPage();
};
ASSERT_SIZE(HelpGump, 0x48c);
#endif
