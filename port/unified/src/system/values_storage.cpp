/* Written by port/tools/make_unified_values.py from both games' value headers: the one program's value variables, which
 * the game it plays fills (values.cpp); do not edit. */

#include "cache.h"
#include "cache_game.h"
#include "shaphand.h"
#include "shaphand_game.h"
#include "display.h"
#include "display_game.h"
#include "activity.h"
#include "activity_game.h"
#include "animfrm.h"
#include "animfrm_game.h"
#include "animprim.h"
#include "animprim_game.h"
#include "attack.h"
#include "attack_game.h"
#include "boboboom.h"
#include "boboboom_game.h"
#include "control.h"
#include "control_game.h"
#include "crusader.h"
#include "crusader_game.h"
#include "dtable.h"
#include "dtable_game.h"
#include "explode.h"
#include "explode_game.h"
#include "firetype.h"
#include "firetype_game.h"
#include "sprite.h"
#include "sprite_game.h"
#include "wpncache.h"
#include "wpncache_game.h"
#include "cafont.h"
#include "cafont_game.h"
#include "compgump.h"
#include "compgump_game.h"
#include "credits.h"
#include "credits_game.h"
#include "dragitem.h"
#include "dragitem_game.h"
#include "loadsave.h"
#include "loadsave_game.h"
#include "mainmenu.h"
#include "mainmenu_game.h"
#include "settings.h"
#include "settings_game.h"
#include "status.h"
#include "status_game.h"
#include "widgets.h"
#include "widgets_game.h"
#include "bindings.h"
#include "bindings_game.h"
#include "joydrv.h"
#include "joydrv_game.h"
#include "alert.h"
#include "alert_game.h"
#include "gravity.h"
#include "gravity_game.h"
#include "item.h"
#include "item_game.h"
#include "npc.h"
#include "npc_game.h"
#include "preload.h"
#include "preload_game.h"
#include "kernel.h"
#include "kernel_game.h"
#include "movie.h"
#include "movie_game.h"
#include "dsfxman.h"
#include "dsfxman_game.h"
#include "music.h"
#include "music_game.h"
#include "sndcache.h"
#include "sndcache_game.h"
#include "sndsrc.h"
#include "sndsrc_game.h"
#include "cprotmem.h"
#include "cprotmem_game.h"
#include "profile.h"
#include "translate.h"
#include "translate_game.h"
#include "searches.h"
#include "crusys.h"
#include "crusys_game.h"
#include "gamemain_game.h"
#include "mapjump.h"
#include "mapjump_game.h"
#include "options.h"
#include "options_game.h"
#include "savefields.h"
#include "savefields_game.h"
#include "savegame.h"
#include "savegame_game.h"
#include "world.h"
#include "world_game.h"

bool MEMORY_PROFILER;
bool CACHE_KEEPS_FREE;
bool SWAP_REBUILD_HALTS;
bool SHAPES_FROM_SWAP;
u8 ALL_SHAPES;
int PICKUP_SPARE_COLUMNS;
u16 ACTIVITY_SIZE;
u16 SURRENDER_SIZE;
bool IDLE_TIMING;
bool STANDING_PUTS_WEAPON_AWAY;
bool SURRENDER_SPEAKS_ONCE;
int FRAME_SOUND_MASK;
u16 ANIM_PRIMITIVE_SIZE;
bool LEDGE_HOLDS_CONTROLLED_NPC;
bool BLOCKED_MOVE_SEARCH;
u16 ATTACK_SIZE;
bool TIMED_FEINT;
u16 BOSS_SHAPE;
i16 BOSS_FIRE_DELAY;
bool BOSS_GRENADES;
bool ATTACK_NO_REGRET;
std::span<const i16> MALE_CRIES;
std::span<const i16> FEMALE_CRIES;
bool SHAPE_SOUNDS;
u16 AUTO_FIRER_RATE;
u16 CHEAT_ITEM_QUALITY;
std::array<u16, 5> WATCHFUL_SHAPES;
std::span<const u16> NON_COMBAT_ANIMS;
bool AUTO_FIRER_USES_AMMO;
bool RETICLE_HIDDEN_WHILE_HELD;
i16 FIRST_RETICLE_STYLE;
int CONTROLLER_BLOCK;
bool BUSY_NPC_TURNS;
bool ROLL_FORWARD_STEPS;
bool AUTO_FIRE_STOPS_EMPTY;
std::span<const u16> UNARMED_SHOOTERS;
bool RP_32_NEVER_DRY;
std::array<i16, 13> NUMBER_KEY_WEAPONS;
std::array<i16, 6> WEAPON_LIST_1;
std::array<i16, 6> WEAPON_LIST_2;
std::array<i16, 4> WEAPON_LIST_3;
bool WEAPON_LISTS;
bool NUMBER_KEYS_SELECT_WEAPONS;
std::span<const CheatGift> CHEAT_GIFTS;
std::span<const u16> CHEAT_CLIPS;
bool CHEAT_GIVES_CREDITS;
u32 CHEAT_CREDITS;
bool CHEAT_IMMORTALITY_ALONE;
bool DEBUG_KEY_FLUSHES;
bool SHAPE_5D6_IS_DEAD;
bool RETICLE_STYLE_KEY;
bool RETURN_KEY_ENDS_STASIS;
bool JUMP_UNREADIES_WEAPON;
bool FIRE_INTERRUPTS_BUSY;
bool JASSICA_JOKE;
bool MORE_CHEAT_CODES;
bool MOVE_NO_REGRET;
i16 BOSS_SHIELD_START;
std::span<const u16> ROBOT_SHAPES;
u16 DEATH_SOUND;
bool DEATH_CLEARS_STASIS;
std::span<const i16> KEPT_ANIMS;
bool SMALL_WEAPON_ANIMS;
bool WEAK_WEAPONS_TAKE_TYPE_DAMAGE;
bool NPCS_AIM_AT_ATTACK_TARGET;
bool TAKE_HIT_NO_REGRET;
i16 HURL_X_NO_DIRECTION;
i16 HURL_Y_NO_DIRECTION;
int INVENTORY_TYPES;
int SHAPE_CREDITS;
std::span<AmmoType> ammoTypes;
std::span<WeaponType> weaponTypes;
std::span<InventoryType> inventoryTypes;
std::span<BombType> bombTypes;
std::span<const char *> weaponNames;
std::span<const char *> inventoryNames;
std::span<const char *> ammoNames;
std::span<const char *> bombNames;
bool EXPLOSION_ALERTS;
bool EXPLOSION_SOUND_AT_ITEM;
std::span<const u16> GRENADE_BLASTS;
std::span<const u16> ROCKET_BLASTS;
bool MORE_HIT_SOUNDS;
int FIRE_TYPES;
std::span<FireType> fireTypes;
bool SHOT_HITS_AS_ITEM;
bool SILENCER_SPRITE;
bool AIM_PAST_RETICLE;
bool ROBOTS_AIM_EXACTLY;
bool OTHER_SHOOTERS_SPREAD;
std::span<OverlayWeapon> overlayWeapons;
std::array<s16, CA_FONTS> CHAR_SPACINGS;
bool CHARACTER_WAITS_A_TICK;
u16 CHARACTER_WAIT;
u16 PAUSE_WAIT;
const char *CREDITS_CHEAT_MOVIE;
bool DRAG_USES_SLOT_7;
u16 LOAD_SAVE_GUMP_SIZE;
bool LOAD_SAVE_PICTURE;
u8 SELECTED_SLOT_COLOUR;
i16 SLOTS_X;
i16 SLOTS_Y;
u16 LOAD_SAVE_POINTER;
i16 MAIN_MENU_HEIGHT;
bool MAIN_MENU_PLAYS_SOUND;
u16 MAIN_MENU_SOUND;
bool MAIN_MENU_ALT_X_QUITS;
u16 DIFFICULTY_BUTTON_SIZE;
u8 DIFFICULTY_CHOSEN_COLOUR;
bool DIFFICULTY_FRAMES_UNCHOSEN;
u8 DIFFICULTY_UNCHOSEN_COLOUR;
bool DIFFICULTY_BUTTON_SOUNDS;
std::array<i16, 4> DIFFICULTY_SOUNDS;
bool OPTIONS_IN_MENU_FRAME;
u8 OPTIONS_FOCUS_COLOUR;
bool BLASTS_LIT_AT_ONE;
Rect SETTINGS_AREA;
s16 SETTINGS_ROW_GAP;
bool SETTINGS_OPEN_PLAYS_SOUND;
u16 SETTINGS_OPEN_SOUND;
u8 STATUS_PAINT_MODE;
bool STATUS_NAMES;
i16 STATUS_AMMO_FONT;
i16 STATUS_AMMO_X;
i16 STATUS_CLIPS_X;
i16 STATUS_AMMO_Y;
u16 STATUS_BAR_LENGTH;
i16 STATUS_BAR_X;
i16 STATUS_BAR_Y;
i16 STATUS_BAR_HEIGHT;
bool STATUS_NO_HEALTH_SHAPE;
u8 WIDGET_FACE;
u8 WIDGET_EDGE;
u8 WIDGET_WARNING_FACE;
u8 WIDGET_WARNING_EDGE;
u8 WIDGET_DIM_FACE;
u8 WIDGET_DIM_EDGE;
u8 WIDGET_DIM_WARNING_FACE;
u8 WIDGET_DIM_WARNING_EDGE;
u8 WIDGET_FOCUS;
std::span<const ActionDefault> GAME_ACTION_DEFAULTS;
std::size_t GAME_ACTION_DEFAULT_COUNT;
std::span<const ActionDefault> GAME_MODERN_BINDINGS;
std::size_t GAME_MODERN_BINDING_COUNT;
bool CALIBRATE_ONCE;
std::array<i16, 16> JOYSTICK_ACTIONS;
i16 JOY_CONTROL;
i16 JOY_ALT;
i16 JOY_SHIFT;
i16 JOY_KEYS;
bool JOY_JUMPS;
i16 JOY_JUMP;
bool JOY_REPEATED_KEYS;
bool JOY_SHIFTS_TAKE_CONTROL;
bool ARROWS_PRESSED_AFTER_FIRE;
bool ALERT_ONLY_ON_CHANGE;
bool ALERT_SWAPS_LIGHTS;
bool CONTROLLED_FALL_AT_ONCE;
u16 GRAVITY_PROCESS_SIZE;
bool FALL_CHECKED_FIRST;
std::span<const u16> CRATE_SHAPES;
bool ON_SCREEN_WHOLE_HEIGHT;
bool DEAD_NPCS_RESTART;
std::span<const StrongerWeapon> STRONGER_WEAPONS;
i16 DEFAULT_STRONGER_WEAPON;
bool EGG_MARKS_ARMED;
bool WEAPONS_ONLY_FOR_CONTAINERS;
int EQUIPMENT_SLOTS;
int NPC_DATA_SIZE;
SlotRange WEAPON_SLOTS;
SlotRange UTILITY_SLOTS;
SlotRange PACK_SLOTS;
u16 BASE_MAP;
std::span<const u16> KEPT_NPC_SHAPES;
std::span<const u16> FEMALE_SHAPES;
bool TELEPORT_COMPACTS_MEMORY;
bool TELEPORT_RESTARTS_TRACKER;
bool FEINTING_NPC_MAY_CHANGE;
bool NEW_ACTIVITY_PUTS_WEAPON_AWAY;
bool ROLLING_THUNDER;
std::span<const i16> KNEELING_ANIMS;
bool ANIM_16_AND_SLIDE_RIGHT;
bool ROLL_FORWARD_DODGES;
bool KNEELING_MOVES_HAND_BACK;
bool COMBAT_RUN_DODGES;
std::span<const u16> RETICLE_NPC_SHAPES;
bool PRELOAD_BY_SHAPES;
int KEY_QUEUE;
int KERNEL_ITEMS;
bool XAN_MOVIE_HOST;
SubtitleStyle SUBTITLE_STYLE;
bool SOUND_FLEX_TAGGED;
int SOUND_FLEX_MODE;
std::span<const char *const> SONG_NAMES;
i16 SONG_FILES;
std::span<const u16> REGION_SONGS;
std::span<const char *const> CHRISTMAS_SONGS;
std::span<const SongTurns> SONG_TURNS;
int SOUND_CACHE_SPARE;
int LAST_SOUND;
u32 SOUND_FRAGMENT_SIZE;
u16 FIRST_FRAGMENT_PERCENT;
bool SHARE_BY_MEMORY_SIZE;
u32 FIRST_MINIMUM;
const char *PROFILE_TITLE;
const char *PROFILE_NAME;
std::uint32_t PROFILE_MEMORY;
std::span<const Translations> USECODE_TRANSLATION_TABLES;
std::size_t USECODE_TRANSLATION_TABLE_COUNT;
std::span<const Translations> SUBTITLE_TRANSLATION_TABLES;
std::size_t SUBTITLE_TRANSLATION_TABLE_COUNT;
u16 SEARCH_VTABLES[5];
const char *GAME_TITLE;
const char *MISSING_CD_MESSAGE;
const char *HELP_MESSAGE;
bool CHEAT_OPTION;
u8 TITLE_BAR_COLOUR;
u8 COPYRIGHT_COLOUR;
bool TITLE_SPACED;
bool PRODUCTION_LINE;
const char *PRODUCTION;
const char *GOODBYE;
const char *GOODBYE_WITH_CHEATS;
bool SHUTDOWN_AFTER_START;
bool SHUTDOWN_ENDS_FADES;
bool START_CHECKS_FILES;
const char *LOADING_STEP;
bool ANCHOR_PROCESS;
const char *GAME_VERSION;
const char *GAME_PROGRAM_NAME;
const char *COPYRIGHT_LINE;
bool JUMP_SHOWS_LOADING;
bool JUMP_COMPACTS_MEMORY;
bool JUMP_RESETS_SOUND_CACHE;
bool JUMP_MOVES_TO_EGG;
std::array<MovieRequest, 2> INTRO_MOVIES;
u16 INTRO_PAUSE;
bool START_HOLDS_SILENCER;
const char *CFG_LINE_END;
const char *GOG_OPTIONS;
bool XAN_MOVIE_OPTIONS;
bool INSTALL_OPTIONS;
bool VIOLENCE_OPTION;
bool FRAME_SKIP_SPILLS;
i16 TUNES_READ;
i16 TUNES_WRITTEN;
bool TUNES_BY_MISSION;
std::span<const SaveName> SAVE_NAMES;
u16 SAVE_NAME_COUNT;
std::span<const SaveClass> SAVE_CLASSES;
u16 SAVE_CLASS_COUNT;
u16 ORIGINAL_DS;
u16 ORIGINAL_UNIT_FILE;
u16 ORIGINAL_STACK;
u16 ORIGINAL_FIRST_TACTIC;
u32 ORIGINAL_BIOS_VECTOR;
u16 SUBSCRIBER_LIST_VTABLE;
std::span<const SaveGlobal> CONTROLLER_GLOBALS;
u16 CONTROLLER_GLOBAL_COUNT;
i16 SAVE_FLEX_ENTRIES;
std::span<const SavedObject> GAME_SAVED_OBJECTS;
std::span<const u16> UNSTACKED_SHAPES;
u16 SELECT_SOUND;
bool SELECT_SOUND_WAITS;
bool CREDITS_END_TIDILY;
bool THAW_REDRAWS_SPRITES;
bool THAW_ENDS_TELEPORT;
bool INTRINSICS_SHOP;
const char *INTRINSICS_TEXT;
bool HELP_PAGE_SOUNDS;
bool WEAPON_KEYS_SILENCER_ONLY;
bool PREVIOUS_WEAPON_SETS_STANCE;
std::array<const char *, 5> HELP_FILES;
i16 HELP_PAGES;
bool HELP_PAGES_WRAP;
bool HELP_BUTTONS;
u16 HELP_SOUND;
bool HELP_ARROW_KEYS;
u16 QUIT_SOUND;
u16 QUICK_LOAD_SOUND;
u16 QUICK_SAVE_SOUND;
u16 VERSION_SOUND;
Point QUESTION_YES;
Point QUESTION_NO;
Point NOTICE_PLACE;
std::span<const u16> DETONATED_SHAPES;
std::span<const ItemKey> ITEM_KEYS;
std::span<const u16> UNCHARGED_ITEMS;
bool ITEM_USED_FIRST;
i16 VERSION_FRAGMENTS;
u16 KEPT_WEAPON;
bool TARGETING_KEY;
bool POSITION_KEY;
bool MORE_DEBUG_KEYS;
bool WORLD_KEYS_NO_REGRET;
