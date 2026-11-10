/* 10c8:0616-06d6, file name unknown: FixedFlex, static\fixed.dat. See
 * itemdam.h. Its destructor is the compiler's. The order of the segment's NE
 * relocations shows a module of its own after ITEMDAM.C. */
#include "itemdam.h"
#include "gamefile.h"
#include "filespec.h"
#include "file.h"

FixedFlex::FixedFlex()
    : FastFlex(makePath(gameDrive, staticDirectory, gameFiles[GAME_FIXED], nullptr), FILE_READ, 0x100)
{
}
