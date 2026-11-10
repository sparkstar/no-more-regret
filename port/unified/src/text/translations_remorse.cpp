/* Written by port/tools/make_unified_values.py from remorse/src/text/translations.cpp: remorse's data in its namespace
 * in the one program; do not edit. */

#include "values.h"
#include "translate.h"
#include <iterator>
#include "values_remorse.h"

namespace remorse_values {
#include "../../../remorse/src/text/translations.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRemorseData_text_translations();

void linkRemorseData_text_translations() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (playingRemorse) {
        ::USECODE_TRANSLATION_TABLES = remorse_values::USECODE_TRANSLATION_TABLES;
        ::USECODE_TRANSLATION_TABLE_COUNT = remorse_values::USECODE_TRANSLATION_TABLE_COUNT;
        ::SUBTITLE_TRANSLATION_TABLES = remorse_values::SUBTITLE_TRANSLATION_TABLES;
        ::SUBTITLE_TRANSLATION_TABLE_COUNT = remorse_values::SUBTITLE_TRANSLATION_TABLE_COUNT;
    }
    return true;
}();

}
