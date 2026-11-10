/* 12f8 (file name unknown): DialogGump, the draggable panel behind the GUI
 * dialogs. */
#ifndef PORT_DIALOG_H
#define PORT_DIALOG_H
#include "draggabl.h"
#include "gumpfont.h"
class DialogGump : public DraggableGump {
protected:
    u8 color;
    /* gumpFont unless setFont created a private font. */
    GumpFont *font;
    /* Paint a flat panel instead of a raised one. */
    u8 flat;

public:
    DialogGump(NewGumpId gumpId, NewGump *owner, Rect area, u8 dialogBackground); /* 12f8:024b */
    ~DialogGump() override;                                                       /* 12f8:02e4 */
    /* UNCALLED: nothing calls it. */
    void setFont(i16 index);                                                         /* 12f8:036b */
    void paintSelf(i16 x, i16 y) override;                                           /* 12f8:038a */
    void handleLeftButton(NewGumpEvent *event) override;                             /* 12f8:044a */
    [[gnu::format(printf, 4, 5)]] void print(i16 x, i16 y, const char *format, ...); /* 12f8:0463 */
};
ASSERT_SIZE(DialogGump, 0x47);
#endif
