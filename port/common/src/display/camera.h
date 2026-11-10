/* Code52 (file name unknown; CAMERA.C would sort before DISPLAY.C): the game's
 * camera, a BaseCamera that follows the map. It keeps the display list's grid
 * over the view, builds the item nodes of the fast area, draws each frame and
 * the debug displays. */
#ifndef PORT_CAMERA_H
#define PORT_CAMERA_H
#include "basecam.h"
#include "dlitem.h"

constexpr int CAMERA_TYPE = 0x219;

class File;
class Item;
class Viewport;

class Camera : public BaseCamera {
public:
    explicit Camera(Loading loading) : BaseCamera(loading) {}
    /* Becomes gameCamera. */
    Camera();
    void run() override;
    /* Clears gameCamera first. */
    void fail(u32 failResult) override;
    /* The camera state is global: 14 values. */
    void load(File *file) override;
    void save(File *file) override;
    void afterLoad() override;
    /* Draws what the scroll uncovered, or the whole map area. */
    void flush() override;
    /* Draws area again; with only set, the gumps and the dirty list only. */
    void invalidate(Rect area, u8 only) override;
    /* Invalidates the map area. */
    virtual void repaint();
    /* Draws the frame's changes and copies them to the screen. */
    virtual void drawFrame();
    /* Nothing. */
    virtual void unknown0040();
};
/* No Remorse's PLAYER.C reads it far. */
extern Camera *&gameCamera;
/* The map is drawn; the camera flushes nothing while it is 0. */
extern u8 &mapDrawing;

/* The item the camera follows. */
extern u16 &followedItem;
/* Code9 asks for the nodes to be built again. */
extern u8 &rebuildRequested;
/* Cheats: the snap eggs' areas and the chunk grid; WORLD.C's
 * Alt-F7 and F7 turn them on and off. */
extern u8 &showSnaps;
extern u8 &showChunks;
/* Set while ITEMCACH.C clears the map chunks: the items it moves are not
 * redrawn. */
extern u8 &clearingChunks;
/* The map's scroll offset on the screen. */
extern i16 &scrollX;
extern i16 &scrollY;

/* Makes the camera. */
void createCamera();
/* Intrinsic 03F: repaints the map and draws a frame. */
void repaintCamera();
/* The map point of screen point x,y. */
void screenToMap(i16 x, i16 y, u16 *mapX, u16 *mapY);
/* The map point under screen point x,y, on top of the item there; with item
 * and inside, the item and whether the point is over its footprint. */
void mapPointAt(i16 x, i16 y, u16 *mapX, u16 *mapY, u8 *z, Item *item, u8 *inside);
/* Moves the camera to x,y,z of map. */
void moveCamera(u16 x, u16 y, u8 z, u16 map);
void setFollowedItem(u16 item);
/* Builds the nodes and the screen again. */
void rebuildScreen();
/* The map point the camera looks at. */
u16 cameraX();
u16 cameraY();
u8 cameraZ();
/* The height above which items are not drawn. */
i16 cameraLevel();
/* Draws a box of xsize by ysize at x,y,z in color, for the egg display. */
void showBox(u16 x, u16 y, u8 z, i16 xsize, i16 ysize, u8 color);

/* Nothing calls these, in the original either. */
/* UNCALLED: nothing calls it. */
void moveCameraBy(i16 dx, i16 dy, char dz);
/* UNCALLED: nothing calls it. */
void findRoof(Item item);
/* UNCALLED: nothing calls it. */
bool followedItemMoved();
#endif
