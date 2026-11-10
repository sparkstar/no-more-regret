/* No Remorse's weapons' overlay types (wpncache.h, 1478:2af8). No XP-5, BK-16 or LNR-81; BA-40 first. */
#include "wpncache.h"
#include "gamestate.h"
#include "dtable.h"

OverlayWeapon (&overlayWeapons)[OVERLAY_WEAPONS] = gameGlobal<OverlayWeapon[OVERLAY_WEAPONS]>({{SHAPE_BA_40, 0},
                                                                                               {SHAPE_BA_41, 0},
                                                                                               {SHAPE_PA_21, 1},
                                                                                               {SHAPE_EM_4, 2},
                                                                                               {SHAPE_SG_A1, 4},
                                                                                               {SHAPE_RP_22, 2},
                                                                                               {SHAPE_RP_32, 2},
                                                                                               {SHAPE_AR_7, 3},
                                                                                               {SHAPE_GL_303, 3},
                                                                                               {SHAPE_PA_31, 2},
                                                                                               {SHAPE_PL_1, 3},
                                                                                               {SHAPE_AC_88, 3},
                                                                                               {SHAPE_UV_9, 3}});
