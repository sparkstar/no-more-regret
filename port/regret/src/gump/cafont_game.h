/* No Regret's values of the fonts (common/src/gump/cafont.cpp). */
#ifndef PORT_CAFONT_GAME_H
#define PORT_CAFONT_GAME_H
#include "cafont.h"

#include <array>

/* What each font adds to a glyph's width. */
inline constexpr std::array<s16, CA_FONTS> CHAR_SPACINGS = {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                            1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
