/* Code121 (file name unknown; No Regret's 13c0:0000-0e11, No Remorse's
 * 13d0:0000-0e6c; the addresses are No Regret's): the load and save gumps,
 * eleven slots over No Regret's static\load.bmp or static\save.bmp, No
 * Remorse's backdrop of four gump shapes. */
#ifndef PORT_LOADSAVE_H
#define PORT_LOADSAVE_H
#include "newgump.h"
#include "gumpshap.h"
#include "palette.h"
#include "viewport.h"
#include "loadsave_game.h"

#include <memory>

/* The load or save gump. Its command is 1 when a slot was chosen (saveSlot)
 * and 0 when not (vtable 1480:6fab). No Remorse's backdrop is gump shapes
 * 40h-43h for loading, 45h-48h for saving, and its palette static\misc.pal
 * (13d0:001e). */
class LoadSaveGump : public NewGump {
public:
    /* No Regret's picture under the slots. */
    Viewport *backdrop;
    u8 loading;
    Palette palette;

    explicit LoadSaveGump(u8 isLoading);
    ~LoadSaveGump() override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    /* No Remorse's draws the backdrop's four quarters in the middle of the
     * screen, x and y not read (13d0:058c). */
    void paintSelf(i16 x, i16 y) override;
};
/* BCC's: No Regret 40h, No Remorse 3Ch (without backdrop). */
ASSERT_SIZE(LoadSaveGump, LOAD_SAVE_GUMP_SIZE);

/* A slot, gump shape 44h: a click chooses it, loading at once or, to save,
 * editing its description (vtable 1480:6f63). */
class SlotGump : public NewGump, protected GumpShape {
public:
    /* Its own. */
    std::unique_ptr<char[]> text; /* the description being typed */
    i16 length;
    u8 editing;
    u8 loading;

    SlotGump(i16 index, u8 isLoading, NewGump *parentGump);
    ~SlotGump() override;
    void handleLeftButton(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override;
    /* Loads the slot, or starts editing its description. */
    void activate();
    /* Makes the slot the selected one. */
    void select();
};
ASSERT_SIZE(SlotGump, 0x41);
#endif
