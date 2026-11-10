/* Off-screen and screen pixel buffers: GRAPH\VPORT.C (1288:0000-0f26). The
 * screen and the current draw target are globals; drawing code swaps
 * drawViewport to render elsewhere. */
#ifndef PORT_VIEWPORT_H
#define PORT_VIEWPORT_H
#include "common.h"
#include "rect.h"
#include "pixels.h"
#include "svgaprim.h"
#include "x32code.h"

#include <memory>

/* Viewport flags. */
constexpr int VIEWPORT_VGA = 1;      /* the mode 13h screen, copied whole */
constexpr int VIEWPORT_LARGE = 2;    /* over 64K: a pixel is reached through its linear address */
constexpr int VIEWPORT_SCREEN = 4;   /* the video memory */
constexpr int VIEWPORT_BORROWED = 8; /* the pixels belong to someone else */
/* The colour that blitMasked leaves out. */
constexpr int TRANSPARENT = 0xff;

class Viewport {
    /* The pixels: the original holds the selector of their memory. */
    u8 *memory_;
    /* The offset of each row from memory_. */
    u32 *rows_;
    s32 left_, top_, right_, bottom_;
    u16 flags_;
    /* The rows it built, its own; a copy (operator=) shares another's
     * rows and pixels and owns neither. The pixels are its own unless
     * they are the screen's or borrowed (flags_). */
    std::unique_ptr<u32[]> ownedRows_;

    /* Fills rows_ for the bounds (1288:0000). */
    void buildRows();
    /* Frees what the viewport owns (1288:017b). */
    void release();
    /* The corners of the isometric rectangle spanned by from and to: top, right,
     * bottom and left; 0 when it is too small (1288:0b81). */
    /* UNCALLED: nothing calls it. */
    bool isoCorners(Point from, Point to, Point *top, Point *right, Point *bottom, Point *left);
    void pixel(s16 x, s16 y, u8 color) { putPixel(this, x, y, color); }
    void hline(s16 x1, s16 y, s16 x2, u8 color) { drawHorizontal(this, x1, y, x2, color); }
    void vline(s16 x, s16 y1, s16 y2, u8 color) { drawVertical(this, x, y1, y2, color); }

public:
    /* Owns nothing until initialized or assigned. */
    Viewport()
    {
        rows_ = nullptr;
        memory_ = nullptr;
    }
    Viewport(u16 viewFlags, s16 left, s16 top, s16 right, s16 bottom, void *pixelBuffer = nullptr)
    {
        initialize(viewFlags, left, top, right, bottom, pixelBuffer);
    }
    ~Viewport();
    u8 *memory() { return memory_; }
    /* What the drawing code reads (pixels.h). */
    ViewportPixels pixels() { return ViewportPixels{memory_, rows_, left_, top_, right_, bottom_}; }
    [[nodiscard]] u16 flags() const { return flags_; }
    /* The pixel bounds; callers read the low words. */
    [[nodiscard]] s16 left() const { return static_cast<s16>(left_); }
    [[nodiscard]] s16 top() const { return static_cast<s16>(top_); }
    [[nodiscard]] s16 right() const { return static_cast<s16>(right_); }
    [[nodiscard]] s16 bottom() const { return static_cast<s16>(bottom_); }
    /* The bounds as stored; addDirtyRect compares them at 32 bits. */
    [[nodiscard]] s32 leftBound() const { return left_; }
    [[nodiscard]] s32 topBound() const { return top_; }
    [[nodiscard]] s32 rightBound() const { return right_; }
    [[nodiscard]] s32 bottomBound() const { return bottom_; }
    /* The size in pixels. */
    [[nodiscard]] s16 width() const { return right() - left() + 1; }
    [[nodiscard]] s16 height() const { return bottom() - top() + 1; }
    /* The pixel bounds as a Rect. */
    [[nodiscard]] Rect bounds() const { return Rect(left(), top(), right(), bottom()); }
    void setBounds(Rect area)
    {
        left_ = area.left;
        top_ = area.top;
        right_ = area.right;
        bottom_ = area.bottom;
    }
    /* Takes buffer, or allocates the pixels when it is 0; a VIEWPORT_SCREEN
     * viewport uses the video memory and becomes screenViewport (1288:0093). */
    void initialize(u16 flags, s16 left, s16 top, s16 right, s16 bottom, void *buffer);
    /* Shares other's pixels (1288:0238). */
    Viewport &operator=(const Viewport &other);
    /* The address of pixel (x,y) (1288:0270). */
    u8 *address(s16 x, s16 y);
    void line(s16 x1, s16 y1, s16 x2, s16 y2, u8 color);
    /* Copies (x1,y1)-(x2,y2) of source to (x,y) (1288:083a); the copy
     * without the pixels of color FFh (1288:0aa4). An all-zero rectangle means
     * the whole source. */
    void blit(Viewport *source, s16 x1 = 0, s16 y1 = 0, s16 x2 = 0, s16 y2 = 0, s16 x = 0, s16 y = 0);
    /* UNCALLED: nothing calls it. */
    void blitMasked(Viewport *source, s16 x1 = 0, s16 y1 = 0, s16 x2 = 0, s16 y2 = 0, s16 x = 0, s16 y = 0);
    /* An all-zero rectangle means the whole viewport. */
    void clear(u8 color, s16 x1 = 0, s16 y1 = 0, s16 x2 = 0, s16 y2 = 0);
    void rectangle(u8 color, s16 x1 = 0, s16 y1 = 0, s16 x2 = 0, s16 y2 = 0);
    /* The outline and the filled inside of the isometric rectangle spanned by
     * (x1,y1) and (x2,y2) (1288:0d71, 0e46). */
    /* UNCALLED: nothing calls it. */
    void isoRectangle(s16 x1, s16 y1, s16 x2, s16 y2, u8 color);
    /* UNCALLED: nothing calls it. */
    void fillIsoRectangle(s16 x1, s16 y1, s16 x2, s16 y2, u8 color);
    /* Moves the VESA window to the bank (1288:0f14). */
    /* UNCALLED: nothing calls it. */
    void setBank(s16 bank);
    /* drawShape and its variants (x32code.h) on this viewport. */
    void drawShape(i16 x, i16 y, ShapeFrame *frame) { ::drawShape(this, x, y, frame); }
    void drawShapeMirrored(i16 x, i16 y, ShapeFrame *frame) { ::drawShapeMirrored(this, x, y, frame); }
    void drawShapeTranslucent(i16 x, i16 y, ShapeFrame *frame, u8 *table)
    {
        ::drawShapeTranslucent(this, x, y, frame, table);
    }
    void drawShapeMirroredTranslucent(i16 x, i16 y, ShapeFrame *frame, u8 *table)
    {
        ::drawShapeMirroredTranslucent(this, x, y, frame, table);
    }
    void drawShapeRemapped(i16 x, i16 y, ShapeFrame *frame, u8 *table)
    {
        ::drawShapeRemapped(this, x, y, frame, table);
    }
    void drawShapeMirroredRemapped(i16 x, i16 y, ShapeFrame *frame, u8 *table)
    {
        ::drawShapeMirroredRemapped(this, x, y, frame, table);
    }
};
ASSERT_SIZE(Viewport, 0x18);

extern Viewport *&screenViewport;
extern Viewport *&drawViewport;
/* The 640x480 VESA screen and the 320x200 mode 13h screen that CRUSYS.C sets up. */
extern Viewport *&svgaScreen;
extern Viewport *&vgaScreen;

/* One-pixel lines on the current draw target in short coordinates. Each
 * expansion keeps its arguments in frame temporaries before widening them. */
inline void drawVLine(s16 x, s16 y1, s16 y2, u8 color)
{
    drawVertical(drawViewport, x, y1, y2, color);
}

inline void drawHLine(s16 x1, s16 y, s16 x2, u8 color)
{
    drawHorizontal(drawViewport, x1, y, x2, color);
}

/* Redirects drawing to another viewport for the object's scope. */
class DrawTarget {
    Viewport *previous;

public:
    explicit DrawTarget(Viewport *target)
    {
        previous = drawViewport;
        drawViewport = target;
    }
    ~DrawTarget() { drawViewport = previous; }
};
ASSERT_SIZE(DrawTarget, 0x4);

#endif
