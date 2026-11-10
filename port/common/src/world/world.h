/* ..\U9UI\WORLD.C (Code2; No Regret's 1008:0000-3f04, No Remorse's
 * 13e8:0000-4029, whose halt names "WORLD.C"; the addresses are No
 * Regret's): the world's gump with the game's keys and mouse, the controlled
 * NPC's items and weapons, choosing among the items nearby, stopping the
 * world while a gump runs full screen, the help pages and the end credits.
 * freezeWorld's and thawWorld's parameters are each game's
 * (world_game.h). */
#ifndef PORT_WORLD_H
#define PORT_WORLD_H
#include "newgump.h"
#include "item.h"
#include "bindings.h"

class Process;

/* WorldGump::modes. Nothing sets 10h, cleared with WORLD_FIRING, nor 40h,
 * which holds the mouse and whose clearing pops the pointer shape. */
/* The right button is held: the controlled NPC walks (execute sends the
 * up arrow key). */
constexpr int WORLD_WALKING = 0x1;
constexpr int WORLD_DRAGGING = 0x2; /* the mouse drags an item */
/* Looks at the item clicked when the left button goes up; nothing sets
 * it. */
constexpr int WORLD_LOOKING = 0x4;
/* The left button is held: the controlled NPC fires (the space key). */
constexpr int WORLD_FIRING = 0x8;
/* A process waits for the item clicked next (waiting, target); nothing in
 * the program sets it. */
constexpr int WORLD_TARGETING = 0x80;
/* G or S chooses among the items nearby (selectItems). */
constexpr int WORLD_SELECTING = 0x100;

/* A node of WorldGump's selection (vtable 1480:0284): an item nearby and its
 * distance from the controlled NPC. */
class SelectionLink : public GumpLink {
public:
    u16 item;
    i16 distance;
};
ASSERT_SIZE(SelectionLink, 0xe);

/* The world's gump, the first modal gump (1008:1074, vtable 1480:029c): the
 * game's keys and the mouse on the map. */
class WorldGump : public NewGump {
public:
    Item unknown34; /* never read */
    u16 unknown36;  /* never used */
    /* The selection picks the items up (G) rather than only showing them
     * (S). */
    u8 grab;
    u16 selected;           /* the item the selection chose */
    SelectionLink *current; /* the node the selection shows */
    GumpList selection;     /* the items nearby, nearest first */
    i16 selectionCount;
    u8 unknown4b[8];  /* never used */
    Process *waiting; /* WORLD_TARGETING's process */
    Item *target;     /* where it wants the item */
    u16 modes;        /* WORLD_* */

    WorldGump();
    void execute(NewGumpEvent *event) override;
    void handleLeftButton(NewGumpEvent *event) override;
    void handleRightButton(NewGumpEvent *event) override;
    void handleMouseMotion(NewGumpEvent *event) override;
    void handleCommand(NewGumpEvent *event) override;
    void handleKeyboard(NewGumpEvent *event) override;
    /* handleKeyboard's after the main menu's keys (the port's): the
     * selection's keys and the others, which go on after the menu has
     * closed. */
    void keyboardRest(NewGumpEvent *event, u8 canAct, Action action);
    /* keyboardRest's keys while the selection is up. */
    void selectionKey(NewGumpEvent *event);
    /* A mouse button's press, double click or release (right: 0 the left,
     * 1 the right): what its input is bound to (bindings.h), the fire and
     * the walk in the mouse's way (fireByMouse, walkByMouse: the original's
     * left and right buttons'), the others as their keys or codes. */
    void mouseButton(u8 right, NewGumpEvent *event);
    void fireByMouse(NewGumpEvent *event);
    void walkByMouse(NewGumpEvent *event);
    /* The main menu during a game: No Regret's F1, No Remorse's Escape
     * (1008:11a9). */
    void gameMenu();
    /* Dragging an item on the map (1008:32ba, 3357, 33a9, 33e4). */
    void startDrag(NewGumpEvent *event);
    void beginDrag(NewGumpEvent *event);
    void endDrag(NewGumpEvent *event);
    void dragTo(NewGumpEvent *event);
    /* G and S: gathers the items near the controlled NPC (1008:3430); G
     * picks them all up at once. */
    void selectItems(u8 take);
    /* Ends the selection with the chosen item (1008:376e) or with none
     * (37ce), and steps it (37ec, 38ba). */
    void useSelection();
    void clearSelection();
    void nextSelection();
    void previousSelection();
    /* The sprite over the item the selection shows (1008:3860, 3880,
     * 3898). */
    void createSelector(i16 x, i16 y, i16 z);
    void freeSelector();
    void showSelector(i16 x, i16 y, i16 z);
};
ASSERT_SIZE(WorldGump, 0x5d);

/* Intrinsic 084: clears movieRestoreSuppressed (1008:0096). */
void allowMovieRestore();
/* The movies leave the palette cycling stopped; No Remorse's PLAYER.C
 * reads it far. */
extern u8 &movieRestoreSuppressed;
/* Ends the stasis and flushes the keys (1008:00ed; 071 I_clrCruStasis); No
 * Remorse's original kept the keys. */
void clearStasis();
/* The controlled NPC is teleporting (1008:0f16, 0f1c, 0f20; 07D, 07B, 07C). */
void clearTeleporting();
void setTeleporting();

/* The controlled NPC readies its weapon, or puts it away in the combat
 * stance: the right button's double click, and READY_WEAPON's inputs
 * (bindings.h). */
void readyWeapon();

/* NPC.C and CONTROL.C call it when the controlled NPC moves during the
 * selection: the selection ends (1008:00f8). */
void avatarMoved();
/* The controlled NPC picks up item (1008:03ab); 0 when it cannot. */
bool pickUp(u16 item, u8 flag);
/* The controlled NPC takes its next weapon (1008:0d71, W) and inventory item
 * (0eb5, O), its previous inventory item (0e54, I). */
void nextWeapon();
/* The controlled NPC takes its previous weapon (1008:0c8e, Q). */
void previousWeapon();
void nextInventoryItem();
void previousInventoryItem();
/* Stop and restart the game; pointer shows the game's mouse pointer
 * (1008:0f68, 0ff1). By default it does, as No Remorse's always does (its
 * take no argument). */
void freezeWorld(u8 pointer = 1);
void thawWorld(u8 pointer = 1);
/* Calls callIntrinsics when worldActive is set (1008:1047). */
void unknown1047();

/* The cheats are on: No Regret's sets it with cheatsEnabled, and F10 and the
 * other cheat keys need it; No Remorse's ~ turns it on and off while
 * cheatsEnabled is set (1480:009b). */
extern u8 &cheatsOn;
/* Set while a sideways mouse move goes to the controller as an arrow key:
 * the NPC turns by mouseTurnAmount and steps at mouseSpeed's pace
 * (1480:009c). */
extern u8 &mouseTurn;
/* The selection is up (1480:009d). */
extern u8 &selectingItems;
/* How far the mouse moved sideways, scaled by mouseSpeed (1480:009e). */
extern i16 &mouseTurnAmount;
/* DISPATCH.C copies the controlled NPC here on Escape; nothing reads it in
 * No Regret, No Remorse's Escape opens the main menu only when it is the
 * Silencer (1480:00a2). */
extern u16 &unknown00a2;
/* crusader.cfg's mousespeed (1480:00a7). */
extern i16 &mouseSpeed;
/* No Remorse's ShapeHandler::getFrame shows the shapes it copies from the CD
 * while it is set. */
extern u8 &cdTransferDisplay;
/* The cheat (No Regret's LOOSECANNON16, No Remorse's JASSICA16) was typed;
 * the hack mover (H) needs it
 * (1480:84d4). */
extern u8 &cheatTyped;
/* The world is frozen: the item animations stop (1480:84d5). */
extern u8 &worldFrozen;
/* Declared in world.cpp in the original; here so that the saved games'
 * schema reaches it. */
/* The end credits (vtable 1480:02e4, intrinsic 145). */
class CreditsProcess : public Process {
public:
    explicit CreditsProcess(Loading loading) : Process(loading) {}
    CreditsProcess() = default;
    void run() override;
};

void suppressMovieRestore();
/* Draws the status display's parts again (No Regret's usecode's intrinsic
 * 0B6). */
void updateInventoryDisplay();
u16 playCredits();
void setStasis();
void resetKeyboardState();
i16 animationsOff();
bool removeInventoryItem(u16 item);
u8 isTeleporting();

/* The tests of pickUp without the picking up: No Remorse's weasel's shop
 * (WEZZY.C) asks it before it sells an item; nothing calls it in No Regret. */
bool canPickUp(u16 item);
/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void unknown1172();
/* UNCALLED: nothing calls it. */
void unknown118e();
/* A key that uses the controlled NPC's first item of shape
 * (WorldGump::handleKeyboard, ITEM_KEYS): a charged one loses a charge each
 * time and goes at the last. */
struct ItemKey {
    Action action;
    u16 shape;
    bool charged;
};
#include "world_game.h"
#endif
