/* WEZZY.C (13e0:0000-1a3d, No Remorse only): the weasel's shop, where the
 * player buys weapons, ammunition and items with the credits of the
 * controlled NPC's card before a mission. */
#ifndef PORT_WEZZY_H
#define PORT_WEZZY_H
#include "newgump.h"

/* A ware of static\stuff.dat. The file holds 142h bytes a level: a count and
 * WEASEL_ITEMS of these. */
struct WeaselItem {
    /* "W" a weapon, "I" an item; the shop does not read it. */
    char id[4];
    u16 unknown4, unknown6;
    u16 shape; /* 0 when the ware is not for sale */
    i16 cost;
    u16 number;
    u16 unknown0e;
};
ASSERT_SIZE(WeaselItem, 0x10);

constexpr int WEASEL_ITEMS = 20;
/* The purchases the shop keeps until the player confirms them. */
constexpr int WEASEL_PURCHASES = 0x80;

/* WeaselGump::state: what the weasel does or asks. step runs the state,
 * nothing sets WEASEL_POOR. */
constexpr int WEASEL_GREETING = 0;
constexpr int WEASEL_LEAVING = 1;   /* the player goes without buying */
constexpr int WEASEL_CONFIRM = 2;   /* "Are you sure you want to buy this?" */
constexpr int WEASEL_CANCELLED = 3; /* the purchases were not confirmed */
constexpr int WEASEL_BOUGHT = 4;
constexpr int WEASEL_POOR = 5;
constexpr int WEASEL_BROWSING = 6;
constexpr int WEASEL_CLOSING = 7;
constexpr int WEASEL_ANYTHING_ELSE = 8; /* "Do you want anything else?" */

/* The instances of the shop's buttons: its commands. */
constexpr int WEASEL_NO = 0;
constexpr int WEASEL_YES = 1;
constexpr int WEASEL_BUY = 2;
constexpr int WEASEL_EXIT = 3;
constexpr int WEASEL_PREVIOUS = 4;
constexpr int WEASEL_NEXT = 5;
constexpr int WEASEL_BLANK = 6;
constexpr int WEASEL_AMMO = 7;    /* shows the ammunition, level 1's wares */
constexpr int WEASEL_WEAPONS = 8; /* back to the level's wares */

/* The shop, a modal gump over the stopped world (13e0:005c, vtable
 * 1478:5ffa). The purchases wait in purchases until the player confirms
 * them; deliver then gives them to the controlled NPC. 28Bh bytes. */
class WeaselGump : public NewGump {
public:
    u16 level; /* the wares shown */
    /* The level whose wares WEASEL_AMMO replaced, -1 for none. */
    i16 savedLevel;
    u16 unknown38; /* never read */
    /* The credits left after the purchases. */
    s32 credits;
    u16 purchases[WEASEL_PURCHASES];
    u16 unknown13e, unknown140; /* never read */
    i16 state;                  /* WEASEL_* */
    i16 current;                /* the ware shown */
    i16 count;                  /* of items */
    u8 pending;                 /* purchases wait for the confirmation */
    u8 delivered;               /* a confirmation delivered purchases */
    u8 purchaseCount;
    WeaselItem items[WEASEL_ITEMS];

    explicit WeaselGump(u16 level);
    ~WeaselGump() override;
    /* Shows only the yes and no buttons (13e0:0486). */
    void showQuestion();
    /* Shows the buttons of the wares (13e0:0608). */
    void showBrowsing();
    void paintSelf(i16 x, i16 y) override;
    /* Reads the wares of level from stuff.dat (13e0:0b1c). */
    void loadCatalog(u16 level);
    void unknown0c70();
    void unknown0c75();
    /* Repaints and keeps current in the wares (13e0:0c7a). */
    void clampItem();
    /* Plays one of count movies in names at random (13e0:0cbb). */
    void playMovie(const char *const *names, i16 count);
    void unknown0e51();
    /* UNCALLED: nothing calls it. */
    void unknown0e56();
    /* Runs state: the shop sends itself a command (13e0:0e5b). */
    void step();
    /* A command from the shop runs state, one from a button chooses it
     * (13e0:0e9a). */
    void handleCommand(NewGumpEvent *event) override;
    void execute(NewGumpEvent *event) override;
    /* Carries out the button with instance button (13e0:1098). */
    void choose(i16 button);
    void handleKeyboard(NewGumpEvent *event) override;
    /* The ware shown is among the purchases (13e0:157d). Nothing calls it. */
    /* UNCALLED: nothing calls it. */
    bool isPurchased();
    /* The purchases (13e0:15b7) and the NPC's items (15f3) of shape. */
    i16 purchasedCount(u16 shape);
    i16 ownedCount(u16 shape);
    /* One more of shape stays under the limit: 1 of a weapon, 20 of
     * ammunition and spider bombs, 10 of the rest (13e0:166d). */
    bool canBuy(u16 shape);
    /* Buys the ware shown when the credits and the limits allow it
     * (13e0:174a). */
    void buy();
    /* Gives the purchases to the controlled NPC and sets its credits
     * (13e0:1878). */
    void deliver();
    /* The next (13e0:1939) and the previous (198b) ware for sale. */
    void next();
    void previous();
};
ASSERT_SIZE(WeaselGump, 0x28b);

/* Set once the weasel has said his first words; the saved game keeps it
 * (object 91, saveGame; 1478:5f2f). */
extern u8 &weaselVisited;
/* Runs the shop with the wares of level (13e0:0000). Only WORLD.C's
 * callIntrinsics calls it, and never. */
void showWeaselShop(u16 level);
#endif
