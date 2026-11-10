/* Borland's conio.h for the port: the text console of the start and the halts. */
#ifndef PORT_CONIO_H
#define PORT_CONIO_H
enum COLORS {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};
enum text_modes { LASTMODE = -1, BW40 = 0, C40, BW80, C80, MONO = 7, C4350 = 64 };

/* The text screen is the host's (host.h). */

#endif
