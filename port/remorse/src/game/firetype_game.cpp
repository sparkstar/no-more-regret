/* No Remorse's fire types (firetype.h): the note in front of the table and the
 * table (1478:252a). */
#include "firetype.h"
#include "gamestate.h"

namespace {
/* The note is short and the table has 16 types (1478:2526); the damages
 * and shield costs differ from No Regret's. */
[[maybe_unused]] const char *fireTypeNote = "FIRE TYPE TABLE";
}
FireType (&fireTypes)[FIRE_TYPES] = gameGlobal<FireType[FIRE_TYPES]>({{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                                                      {1, 0x12, 0x19, 0, 1, 0x82, 7, 0, 0xc, 0x3e8, 0},
                                                                      {2, 0xa, 0x11, 0, 8, 0x50, 7, 0, 0xc, 0x3e8, 0},
                                                                      {3, 0xc8, 0xc8, 0xa, 1, 0, 0, 1, 3, 0x3e8, 1},
                                                                      {4, 0x96, 0x96, 0xa, 1, 0, 0, 0, 1, 0x3e8, 1},
                                                                      {5, 0x19, 0x2d, 0, 1, 0x190, 6, 0, 4, 0x3e8, 1},
                                                                      {6, 0xf, 0x23, 0, 1, 0x177, 6, 1, 4, 0x3e8, 1},
                                                                      {7, 5, 5, 0, 1, 0, 0, 0, 1, 0x3e8, 1},
                                                                      {8, 5, 5, 0, 1, 0, 0, 0, 1, 0x3e8, 0},
                                                                      {9, 0xa, 0xf, 0, 1, 0x12c, 6, 1, 1, 0x64, 1},
                                                                      {0xa, 0x2d, 0x50, 0xa, 3, 0, 0, 0, 3, 0x64, 1},
                                                                      {0xb, 0xe, 0x14, 0, 1, 0x50, 7, 0, 0xc, 0x3e8, 0},
                                                                      {0xc, 5, 5, 0, 1, 0, 0, 0, 1, 0x3e8, 0},
                                                                      {0xd, 0xa, 0x11, 0, 5, 0x50, 7, 0, 0xc, 0x3e8, 0},
                                                                      {0xe, 0xfa, 0xfa, 4, 1, 0x9c4, 4, 1, 3, 0x3e8, 1},
                                                                      {0xf, 0x23, 0x37, 4, 1, 0x2ee, 4, 0, 3, 0x3e8,
                                                                       1}});
