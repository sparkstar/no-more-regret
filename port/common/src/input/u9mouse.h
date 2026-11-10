/* U9MOUSE.C (Code122, 13c8), the parts the game uses: the game's mouse
 * controller. U9MouseControlProcess's constructor (13c8:0000) puts itself
 * here; the GUI modules use it as a MouseCtl. */
#ifndef PORT_U9MOUSE_H
#define PORT_U9MOUSE_H
#include "mousectl.h"

/* 1480:6ff4 */
extern MouseCtl *&gameMouse;

/* The game's mouse controller: it reads the pointers of mouse.shp and puts
 * itself in gameMouse (13c8:0000). The arrow shapes point away from the
 * centre, in eight directions. */
class U9MouseControlProcess : public MouseCtl {
public:
    explicit U9MouseControlProcess(Loading loading) : MouseCtl(loading) {}
    /* 1 with the pointer in the box around the centre, 2 outside it or near
     * the screen's edges; frameAt adds 8 frames for 2. */
    i16 range;
    i16 direction;        /* the frame frameAt picked */
    i16 centreX, centreY; /* 320,200 */

    U9MouseControlProcess();
    void load(File *file) override
    {
        MouseCtl::load(file);
        gameMouse = this;
    }
    i16 frameAt(s16 x, s16 y) override;
    /* Sets range for the pointer at (x,y) and returns it. */
    i16 checkRange(s16 x, s16 y);
    /* The direction of the arrow at the pointer, 0-7, or 0 for other shapes;
     * not called. */
    /* UNCALLED: nothing calls it. */
    i16 arrowDirection();
    i16 shape() { return shapes[depth]; }
};
ASSERT_SIZE(U9MouseControlProcess, 0x1a0);

#endif
