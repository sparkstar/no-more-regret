/* The port's (experimental): the world beyond the 640x480 screen's sides,
 * drawn for the wide screen (screen.h's screenSetWide) after each frame the
 * camera draws. The game's rules stay the 640x480 screen's: the sides read
 * the map and change nothing of it (the items' frames, their flags, the
 * display list, the random numbers), so a run is the same with them or
 * without. They draw what the camera would there: the fast area's items as
 * the display list takes them and orders them (DL_ItemNode's place, order
 * and draw), then the sprites. The decorations there, which the game
 * animates only on the screen (animateItems), are drawn at frames of the
 * sides' own, of the animation's time and their shape's speed, and of no
 * random number of the game's. */
#ifndef PORT_WIDESIDES_H
#define PORT_WIDESIDES_H

class BasePalette;

/* The sides of the camera's frame, drawn and shown; nothing when the wide
 * screen is off. */
void drawWideSides();
/* The sides covered (shown black) while a gump over the whole screen is
 * open on the frozen world (the game menu, the help, the settings,
 * loading, saving) or the screen is not the world's (a movie on black or
 * one that clears it, the credits, the intro and the start menu), drawn
 * again when the world goes on. */
void coverWideSides();
/* The sides greyed as a movie greys the screen behind it (No Remorse's
 * movies on the greyed world, toFixedGray with the palette before the
 * movie's): the last drawn turned into the grey entries the movie's
 * palette keeps, and kept so until ungreyWideSides, when the camera's next
 * frame draws them again. Nothing when they are covered or not drawn. */
void greyWideSides(BasePalette *palette);
void ungreyWideSides();
/* The sides' pixels freed (gameRelease); the screen keeps the last shown,
 * as it keeps the last frame. */
void closeWideSides();
#endif
