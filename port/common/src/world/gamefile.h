/* The game's directories and files, variables of the module of main (Code11,
 * 1050:0073-00a5, data 1480:0900-0abe). Every module reaches them in DGROUP
 * (PUSH DS; PUSH 0912h) except MusicProcess, whose source declared them far
 * (PUSH SEG, MOV AX,SEG; MOV ES,AX). The target of MusicProcess's PUSH SEG
 * for the sound directory is 1480:0912, so that directory was a variable of
 * its own. */
#ifndef PORT_GAMEFILE_H
#define PORT_GAMEFILE_H
#include "common.h"

/* The data drive, the CD ("d:", crusader.cfg's cdletter changes the letter;
 * 1480:0974), the movies' directory (crusader.cfg's flicpath, 0 without one;
 * 1480:0978) and the game's files (1480:097c). Code5 reads flicPath as a
 * variable of its own (MOV AX,[097Ah]), not as an element of gameFiles. The
 * directories under the drive (1480:0900, 0909, 0912, 091b, 0924): a DOS
 * name each, but usecode is a path of up to 80 characters, which the
 * command line can give (CRUSYS.C copies 30 of them). */
extern char *&gameDrive;
extern char *&flicPath;
extern const char *(&gameFiles)[17];
extern char (&staticDirectory)[9];
extern char (&savegameDirectory)[9];
extern char (&soundDirectory)[9];
extern char (&editorDirectory)[9];
extern char (&usecodeDirectory)[80];
/* "Copyright (c) 1996 Origin Systems." and "All Rights Reserved."
 * (1480:09c0). */
extern const char *(&copyright)[2];

/* shapes.flx */
constexpr int GAME_SHAPES = 0;
/* fixed.dat, the maps' items */
constexpr int GAME_FIXED = 2;
/* nonfixed.dat */
constexpr int GAME_NONFIXED = 3;
/* glob.flx */
constexpr int GAME_GLOB = 4;
/* xformpal.dat, the colour tables */
constexpr int GAME_XFORMPAL = 5;
/* fonts.flx */
constexpr int GAME_FONTS = 8;
/* eusecode.flx: its first letter is the language's. */
constexpr int GAME_USECODE = 9;
/* anim.dat */
constexpr int GAME_ANIM = 10;
/* gamepal.pal */
constexpr int GAME_PALETTE = 13;
/* typeflag.dat */
constexpr int GAME_TYPEFLAG = 14;
/* regret: the program's name (regret.map). */
constexpr int GAME_PROGRAM = 16;

/* The original's main, up to its kernel's loop: makes the game's system
 * object, which starts the game (crusys.cpp's CruSystem::startup). */
int gameMain(int argc, char *argv[]);
/* A step of the game: the start's, until it has started the game, then a
 * pass of the kernel (runKernel). The system layer calls it after
 * gameMain until the run ends (platform/host.h's hostEnd). */
void gameStep();
/* What the game holds released at the run's end, however it ends
 * (platform/host.h's hostEnd; crusys.cpp). */
void gameRelease();
#endif
