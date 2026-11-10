/* Written by port/tools/make_unified_values.py from regret/src/text/translations.cpp: regret's data in its namespace in
 * the one program; do not edit. */

#include "values.h"
#include "translate.h"
#include <iterator>
#include "values_regret.h"

namespace regret_values {
#include "../../../regret/src/text/translations.cpp"
}

/* Called by linkGameData (values_data.cpp) so that the program links this file. */
void linkRegretData_text_translations();

void linkRegretData_text_translations() {}

namespace {

[[maybe_unused]] const bool POINTED = [] {
    if (!playingRemorse) {
        ::USECODE_TRANSLATION_TABLES = regret_values::USECODE_TRANSLATION_TABLES;
        ::USECODE_TRANSLATION_TABLE_COUNT = regret_values::USECODE_TRANSLATION_TABLE_COUNT;
        ::SUBTITLE_TRANSLATION_TABLES = regret_values::SUBTITLE_TRANSLATION_TABLES;
        ::SUBTITLE_TRANSLATION_TABLE_COUNT = regret_values::SUBTITLE_TRANSLATION_TABLE_COUNT;
    }
    return true;
}();

}
