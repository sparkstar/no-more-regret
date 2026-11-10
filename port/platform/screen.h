/* The game's screen: its modes, its palette and the vertical retrace. The
 * system layer keeps it (port/system/video.cpp) and the window shows it
 * (window.cpp); a wait for the retrace takes the clock's time. */
#ifndef PORT_PLATFORM_SCREEN_H
#define PORT_PLATFORM_SCREEN_H
#include <cstdint>

/* The game's screen, mode 101h's 640x480 (and the movies' 111h): its width
 * and height in pixels, its last column and row. */
constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;
constexpr int SCREEN_RIGHT = SCREEN_WIDTH - 1;
constexpr int SCREEN_BOTTOM = SCREEN_HEIGHT - 1;

/* The modes, by the game's numbers for them (the BIOS's and VESA's in the
 * original): 3 the text of 80 columns, 13h 320x200 of 256 colours;
 * supervga.h's 101h 640x480, 103h 800x600 and 105h 1024x768 of 256 colours
 * and the movies' 111h 640x480 of 65536. The pixels are the video memory's
 * (svgaprim.h, x32code.h). */

/* The video memory the game draws on, which its videoMemory and
 * svgaMemory take as their places when the program starts (svgaprim.h,
 * x32code.h): the VGA's 64K at A000h, of mode 13h, and the SuperVGA's
 * frame buffer, which view32.cpp draws on flat. */
std::uint8_t *screenVgaMemory();
std::uint8_t *screenSvgaMemory();

/* The mode now. */
std::uint16_t screenMode();
/* The scan lines of the text's characters: 16 for 25 rows, 8 for 50
 * (host.h's hostTextMode sets them). */
std::uint16_t screenFontHeight();
/* Sets mode, its screen cleared unless keep; false for a mode the screen
 * has not. */
bool screenSetMode(std::uint16_t mode, bool keep);
/* A VESA mode's width and height in pixels; false for a mode the screen
 * has not. */
bool screenModeSize(std::uint16_t mode, std::uint16_t *width, std::uint16_t *height);

/* count colours from index first (wrapping at 256), three bytes each: red,
 * green and blue of 6 bits (the higher bits are dropped). */
void screenSetColors(std::uint8_t first, std::uint16_t count, const std::uint8_t *rgb);
/* The colours as the screen has them. */
void screenGetColors(std::uint8_t first, std::uint16_t count, std::uint8_t *rgb);
/* Waits for the start of the next vertical retrace, 60 a second (70 in
 * mode 13h and the text): while in one to its end, then to the next. Each
 * look at it takes a tick of the interval timer, as the original's read of
 * the VGA's status took one on its bus. */
void screenWaitRetrace();

/* The port's wide screen (experimental, the settings' WIDE SCREEN): the
 * world beyond the 640x480 screen's sides, which the game draws beside
 * it while it shows the map; its rules stay the 640x480 screen's. 0
 * off, 1 the sides shown, 2 shown dimmed. */
void screenSetWide(std::uint8_t mode);
std::uint8_t screenWide();
/* The width of the picture with the sides for its 480 rows, the shape of
 * the window's display (640 when off). */
std::uint16_t screenWideWidth();
/* The sides the game drew: width x 480 pixels of the palette, the 640x480
 * screen's columns in their middle not read; taken at once. Shown beside
 * the screen in mode 101h until another or 0 (the map no longer drawn). */
void screenShowSides(const std::uint8_t *pixels, std::uint16_t width);
#endif
