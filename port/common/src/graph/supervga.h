/* GRAPH\SUPERVGA.C: VESA SuperVGA modes. The Rect module
 * before it in the segment is rect.h. */
#ifndef PORT_SUPERVGA_H
#define PORT_SUPERVGA_H
#include "common.h"
#include "svgaprim.h"

/* VESA modes, 256 colors unless noted. */
constexpr int VESA_640X480 = 0x101;
constexpr int VESA_800X600 = 0x103;
constexpr int VESA_1024X768 = 0x105;
constexpr int VESA_640X480_64K = 0x111; /* 65536 colors, No Regret's movies */

/* INT 10h AX=4F00h: the controller information (the port asks for none). */
struct VesaInfo {
    char signature[4]; /* "VESA" */
    u8 unknown04[252];
};
/* INT 10h AX=4F01h: a mode's information (the port's from screen.h's
 * size). */
struct VesaModeInfo {
    u8 unknown00[4];
    /* The step of the window position, in KB. */
    u16 granularity;
    u8 unknown06[12];
    u16 width;
    u16 height;
    u8 unknown16[234];
};
ASSERT_SIZE(VesaInfo, 0x100);
ASSERT_SIZE(VesaModeInfo, 0x100);

class SuperVga {
public:
    u16 unknown0;
    /* The VESA mode selectMode chose; 0 for none. */
    u16 mode;
    VesaModeInfo modeInfo;
    VesaInfo vbeInfo;
    /* 1 when the window granularity is 64K, else 16. */
    u16 bankStep;

    SuperVga();
    /* UNCALLED: nothing calls it. */
    explicit SuperVga(u16 mode);
    /* Chooses a VESA mode; 0 when the controller or the mode is missing. */
    bool selectMode(u16 mode);
    /* The chosen mode's size and window granularity; -1 (0 for the
     * granularity) when there is none. */
    [[nodiscard]] s16 width() const;
    [[nodiscard]] s16 height() const;
    [[nodiscard]] u16 granularity() const;
    /* Switches to the chosen mode; halts when there is none or it fails. */
    void setMode() const;
};
ASSERT_SIZE(SuperVga, 0x206);
#endif
