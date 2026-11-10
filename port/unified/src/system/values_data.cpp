/* Written by port/tools/make_unified_values.py from both games' data files: the calls that link them into the one
 * program (CMakeLists.txt's unified_data); do not edit. */

void linkRegretData_game_control_game();
void linkRegretData_game_dtable_game();
void linkRegretData_game_firetype_game();
void linkRegretData_game_wpncache_game();
void linkRegretData_input_bindings_game();
void linkRegretData_text_translations();
void linkRegretData_usecode_intrinsics();
void linkRegretData_world_savefields();
void linkRemorseData_game_control_game();
void linkRemorseData_game_dtable_game();
void linkRemorseData_game_firetype_game();
void linkRemorseData_game_wpncache_game();
void linkRemorseData_input_bindings_game();
void linkRemorseData_text_translations();
void linkRemorseData_usecode_intrinsics();
void linkRemorseData_world_savefields();

/* Never called: the program links this object, and with it every data file. */
void linkGameData();

void linkGameData()
{
    linkRegretData_game_control_game();
    linkRegretData_game_dtable_game();
    linkRegretData_game_firetype_game();
    linkRegretData_game_wpncache_game();
    linkRegretData_input_bindings_game();
    linkRegretData_text_translations();
    linkRegretData_usecode_intrinsics();
    linkRegretData_world_savefields();
    linkRemorseData_game_control_game();
    linkRemorseData_game_dtable_game();
    linkRemorseData_game_firetype_game();
    linkRemorseData_game_wpncache_game();
    linkRemorseData_input_bindings_game();
    linkRemorseData_text_translations();
    linkRemorseData_usecode_intrinsics();
    linkRemorseData_world_savefields();
}
