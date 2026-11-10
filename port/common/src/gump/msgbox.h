/* 1350 (file name unknown): MessageBoxGump, a centered dialog showing text
 * above an "Ok" button. */
#ifndef PORT_MESSAGEBOX_H
#define PORT_MESSAGEBOX_H
#include "dialog.h"
class MessageBoxGump : public DialogGump {
    /* One or more lines separated by '\n'; paintSelf splits it in place. */
    const char *text;
    s16 textHeight, textWidth;

public:
    MessageBoxGump(NewGumpId boxId, NewGump *owner, const char *message, u8 boxBackground); /* 1350:0046 */
    /* Clicks on the panel neither drag nor reach DialogGump. */
    void handleLeftButton(NewGumpEvent * /*event*/) override {}
    void handleCommand(NewGumpEvent *event) override;  /* 1350:02b1 */
    void handleKeyboard(NewGumpEvent *event) override; /* 1350:02c8 */
    void paintSelf(i16 x, i16 y) override;             /* 1350:02df */
};
ASSERT_SIZE(MessageBoxGump, 0x4f);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void showMessage(char *message);
#endif
