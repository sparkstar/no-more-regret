/* 1328 (file name unknown): DraggableGump, a gump the left button drags as an
 * off-screen image. */
#ifndef PORT_DRAGGABLE_H
#define PORT_DRAGGABLE_H
#include "newgump.h"
#include "viewport.h"
class DraggableGump : public NewGump {
    /* While dragging: the gump tree painted over transparent color 0xff,
     * and the screen pixels it currently covers. */
    Viewport *image;
    Viewport *background;
    u8 dragging;
    s16 grabX, grabY;

public:
    /* UNCALLED: nothing calls it. */
    DraggableGump(NewGumpId gumpId, NewGump *owner);      /* 1328:0000 */
    ~DraggableGump() override;                            /* 1328:006b */
    void handleMouseMotion(NewGumpEvent *event) override; /* 1328:00d7 */
    void handleLeftButton(NewGumpEvent *event) override;  /* 1328:0267 */
    void paintSelf(i16 x, i16 y) override;                /* 1328:064d */
};
ASSERT_SIZE(DraggableGump, 0x41);
#endif
