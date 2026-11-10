/* 1370 (file name unknown): the shape buttons. */
#ifndef PORT_SHAPEBUTTON_H
#define PORT_SHAPEBUTTON_H
#include "button.h"
#include "gumpshap.h"
/* The destructors are the compiler's, as the original's are. */
class ShapeButtonGump : public ButtonGump, protected GumpShape {
public:
    ShapeButtonGump(u16 mode, NewGump *parentGump, u16 shapeValue); /* 1370:0000 */
    void paintSelf(i16 x, i16 y) override;                          /* 1370:0093 */
    void showPressed() override;                                    /* 1370:00cd */
    void showReleased() override;                                   /* 1370:00e9 */
    bool acceptsEvent(NewGumpEvent *event) override;                /* 1370:0105 */
};

class CyclingShapeButtonGump : public CyclingButtonGump, protected GumpShape {
public:
    /* UNCALLED: nothing calls it. */
    CyclingShapeButtonGump(u8 code, NewGump *parentGump, u16 shapeValue); /* 1370:0133 */
    void paintSelf(i16 x, i16 y) override;                                /* 1370:01f0 */
    void showPressed() override;                                          /* 1370:022a */
    /* Inline in the original: its code lies among the compiler's copies of
     * the implicit destructors. */
    void showReleased() override
    {
        const u16 old = current;
        frame = old;
        paint(0);
        current = old;
    }
};

class AutoRepeatShapeButtonGump : public AutoRepeatButtonGump, protected GumpShape {
public:
    /* UNCALLED: nothing calls it. */
    AutoRepeatShapeButtonGump(u8 mode, NewGump *parentGump, u16 shapeValue); /* 1370:0264 */
    void paintSelf(i16 x, i16 y) override;                                   /* 1370:02f7 */
    void showPressed() override;                                             /* 1370:0331 */
    void showReleased() override;                                            /* 1370:034d */
};

ASSERT_SIZE(ShapeButtonGump, 0x3a);
ASSERT_SIZE(CyclingShapeButtonGump, 0x3e);
ASSERT_SIZE(AutoRepeatShapeButtonGump, 0x3e);
#endif
