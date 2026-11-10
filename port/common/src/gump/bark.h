/* BARK.C (1390:0000-0977, No Remorse only): the bark gump, an item's words
 * in a box over it for a while, kept clear of the other barks. */
#ifndef PORT_BARK_H
#define PORT_BARK_H
#include "irgump.h"

/* How many places the barks can take on the screen. */
constexpr int BARK_SLOTS = 30;

/* An item's text in font 4, a page (a rectangle of formatText) at a time,
 * in the free place nearest the item (vtable 1478:54e0). 73h bytes. */
class BarkGump : public ItemRelativeGump {
public:
    i16 fontIndex;
    /* The formatted text; its pages end in 0. */
    char *text;
    /* Its place in the slots, -1 for none. */
    i16 slot;
    /* Where the page shown begins in text, FFFFh before the first. */
    u16 page;
    u8 unknown64;
    /* The item's top on the screen. */
    i16 screenX, screenY;
    /* The length of text with its 0. */
    u16 length;
    /* The item's frame on the screen: the bark stays off it. */
    Rect itemArea;

    /* A left click shows the next page when click; after time seconds the
     * gump closes (1390:0000). */
    /* UNCALLED: nothing calls it. */
    BarkGump(NewGump *parent, u16 item, char *text, u8 click, i16 time);
    ~BarkGump() override; /* 1390:02c2 */
    /* Moves to the free place nearest the item (1390:032c). */
    void place();
    /* A command shows the next page (1390:05ce). */
    void handleCommand(NewGumpEvent *event) override;
    /* Frees the gump's slot (1390:05e1). */
    void releaseSlot();
    /* Closing only turns the page (1390:0639). */
    void close() override;
    /* Shows the next page, or closes after the last (1390:064c). */
    void advance();
    void update(i16 show) override; /* 1390:07d3 */
    /* A press turns the page, a release goes to the parent (1390:080a). */
    void handleLeftButton(NewGumpEvent *event) override;
    void paintSelf(i16 x, i16 y) override; /* 1390:085e */
    /* Closes the layer 2 item gumps of the world gump (1390:08c0). */
    void slot17() override;
    void slot16() override; /* 1390:092f */
};
ASSERT_SIZE(BarkGump, 0x73);
#endif
