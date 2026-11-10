/* No Regret's fire types (firetype.h): the note in front of the table and the
 * table (1480:2b2e). */
#include "firetype.h"
#include "gamestate.h"

namespace {
[[maybe_unused]] const char *fireTypeNote =
    "FIRE TYPE TABLE - FTYPE,LOW DMG,HIGH DMG,RANGE,NUM SHOTS,SHIELD COST,SHIELD MASK,"
    "ACCURATE,CELLS/RND,RND DUR,NEAR SPRITE - This string is for the benefit of hackers.";
}
FireType (&fireTypes)[FIRE_TYPES] =
    gameGlobal<FireType[FIRE_TYPES]>({{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                      {1, 0x12, 0x19, 0, 1, 0x82, 7, 0, 0xc, 0x3e8, 0},
                                      {2, 0xf, 0x14, 0, 0xa, 0x50, 7, 0, 0xc, 0x3e8, 0},
                                      {3, 0xc8, 0xc8, 0xa, 1, 0, 0, 1, 3, 0x3e8, 1},
                                      {4, 0x96, 0x96, 0xa, 1, 0, 0, 0, 1, 0x3e8, 1},
                                      {5, 0x2d, 0x3c, 0, 1, 0x190, 6, 0, 4, 0x3e8, 1},
                                      {6, 0x19, 0x1e, 0, 1, 0x12c, 6, 1, 4, 0x3e8, 1},
                                      {7, 5, 5, 0, 1, 0, 0, 0, 1, 0x3e8, 1},
                                      {8, 5, 5, 0, 1, 0, 0, 0, 1, 0x3e8, 0},
                                      {9, 0xa, 0xf, 0, 1, 0x12c, 6, 1, 1, 0x64, 1},
                                      {0xa, 0x2d, 0x50, 7, 3, 0, 0, 0, 3, 0x64, 1},
                                      {0xb, 0xe, 0x14, 0, 1, 0x50, 7, 0, 0xc, 0x3e8, 0},
                                      {0xc, 5, 5, 0, 1, 0, 0, 0, 1, 0x3e8, 0},
                                      {0xd, 0xf, 0x14, 0, 0xa, 0x50, 7, 0, 0xc, 0x3e8, 0},
                                      {0xe, 0xfa, 0xfa, 4, 1, 0x9c4, 4, 1, 3, 0x3e8, 1},
                                      {0xf, 0x32, 0x4b, 4, 1, 0x2ee, 4, 0, 3, 0x3e8, 1},
                                      {0x10, 0x4b, 0x64, 3, 1, 0x1f4, 4, 0, 3, 0x3e8, 1},
                                      {0x11, 0x7d, 0x96, 0, 1, 0x4e2, 4, 0, 3, 0x3e8, 1},
                                      {0x12, 0x14, 0x14, 0, 1, 0, 0, 0, 3, 0x3e8, 1},
                                      {0x13, 5, 5, 0, 1, 0, 0, 0, 3, 0x3e8, 1},
                                      {0x14, 0x64, 0x7d, 0, 1, 0x384, 4, 0, 3, 0x3e8, 1},
                                      {0x15, 0x1e, 0x32, 0, 1, 0x15e, 7, 0, 3, 0x3e8, 1},
                                      {0x16, 0x2d, 0x50, 7, 1, 0, 0, 1, 3, 0x3e8, 1}});
