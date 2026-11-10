/* Written by port/tools/make_unified_values.py from remorse/src's value headers: remorse's values, in a namespace of
 * their own, which fill the one program's when it plays remorse; do not edit. */

#ifndef PORT_VALUES_REMORSE_H
#define PORT_VALUES_REMORSE_H
#include "common.h"
#include <array>
#include "animprim.h"
#include "dtable.h"
#include "cafont.h"
#include "rect.h"
#include "subtitles.h"
#include "file.h"
#include <cstdint>
#include <conio.h>
#include "movie.h"
#include "savegame.h"
#include "wezzy.h"
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
#include "animprim_game.h"
#include "attack.h"
#include "attack_game.h"
#include "boboboom.h"
#include "boboboom_game.h"
#include "control.h"
#include "control_game.h"
#include "crusader.h"
#include "crusader_game.h"
#include "dtable_game.h"
#include "explode.h"
#include "explode_game.h"
#include "firetype.h"
#include "firetype_game.h"
#include "sprite.h"
#include "sprite_game.h"
#include "wpncache.h"
#include "wpncache_game.h"
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
#include "savegame_game.h"
#include "world.h"
#include "world_game.h"

namespace remorse_values {

inline constexpr u16 ACTIVITY_SIZE = 0x42;
inline constexpr bool AIM_PAST_RETICLE = false;
inline constexpr bool ALERT_ONLY_ON_CHANGE = false;
inline constexpr bool ALERT_SWAPS_LIGHTS = false;
inline constexpr u8 ALL_SHAPES = 0;
inline constexpr bool ANCHOR_PROCESS = false;
inline constexpr bool ANIM_16_AND_SLIDE_RIGHT = false;
inline constexpr u16 ANIM_PRIMITIVE_SIZE = 0x5b;
inline constexpr bool ARROWS_PRESSED_AFTER_FIRE = true;
inline constexpr bool ATTACK_NO_REGRET = false;
inline constexpr u16 ATTACK_SIZE = 0x98;
inline constexpr u16 AUTO_FIRER_RATE = 6;
inline constexpr bool AUTO_FIRER_USES_AMMO = false;
inline constexpr bool AUTO_FIRE_STOPS_EMPTY = false;
inline constexpr u16 BASE_MAP = 40;
inline constexpr bool BLASTS_LIT_AT_ONE = false;
inline constexpr bool BLOCKED_MOVE_SEARCH = false;
inline constexpr i16 BOSS_FIRE_DELAY = 0xf;
inline constexpr bool BOSS_GRENADES = false;
inline constexpr u16 BOSS_SHAPE = 0x3ac;
inline constexpr i16 BOSS_SHIELD_START = 5000;
inline constexpr bool BUSY_NPC_TURNS = false;
inline constexpr bool CACHE_KEEPS_FREE = false;
inline constexpr bool CALIBRATE_ONCE = false;
inline constexpr const char *CFG_LINE_END = "\n\r";
inline constexpr u16 CHARACTER_WAIT = 0;
inline constexpr bool CHARACTER_WAITS_A_TICK = true;
inline constexpr std::array<s16, CA_FONTS> CHAR_SPACINGS = {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                            1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
inline constexpr std::array<u16, 5> CHEAT_CLIPS = {
    {SHAPE_BA_41_CLIP, SHAPE_RP_32_CLIP, SHAPE_AR_7_CLIP, SHAPE_GL_303_CLIP, SHAPE_SG_A1_CLIP}};
inline constexpr u32 CHEAT_CREDITS = 30000;
inline constexpr std::array<CheatGift, 4> CHEAT_GIFTS = {
    {{SHAPE_SPIDER_BOMB, 20}, {SHAPE_LAND_MINE, 20}, {SHAPE_DET_PAC, 20}, {SHAPE_MEDICAL_KIT, 20}}};
inline constexpr bool CHEAT_GIVES_CREDITS = true;
inline constexpr bool CHEAT_IMMORTALITY_ALONE = false;
inline constexpr u16 CHEAT_ITEM_QUALITY = 100;
inline constexpr bool CHEAT_OPTION = true;
inline constexpr std::array<const char *, 0> CHRISTMAS_SONGS{};
inline constexpr bool COMBAT_RUN_DODGES = false;
inline constexpr bool CONTROLLED_FALL_AT_ONCE = false;
inline constexpr int CONTROLLER_BLOCK = 75;
inline constexpr int CONTROLLER_BLOCK_CAPACITY = CONTROLLER_BLOCK;
inline constexpr u8 COPYRIGHT_COLOUR = YELLOW;
inline constexpr const char *COPYRIGHT_LINE = "Copyright (c) 1995 Origin Systems.";
inline constexpr std::array<u16, 8> CRATE_SHAPES = {0x432, 0x5a0, 0x1fd, 0x1fe, 0x8f, 0x59f, 0x2b3, 0x2ca};
inline constexpr const char *CREDITS_CHEAT_MOVIE = "z03";
inline constexpr bool CREDITS_END_TIDILY = false;
inline constexpr bool DEAD_NPCS_RESTART = true;
inline constexpr bool DEATH_CLEARS_STASIS = false;
inline constexpr u16 DEATH_SOUND = 9;
inline constexpr bool DEBUG_KEY_FLUSHES = false;
inline constexpr i16 DEFAULT_STRONGER_WEAPON = 7;
inline constexpr std::array<u16, 1> DETONATED_SHAPES = {{SHAPE_DET_PAC}};
inline constexpr u16 DIFFICULTY_BUTTON_SIZE = 0x3e;
inline constexpr bool DIFFICULTY_BUTTON_SOUNDS = false;
inline constexpr u8 DIFFICULTY_CHOSEN_COLOUR = 0x44;
inline constexpr bool DIFFICULTY_FRAMES_UNCHOSEN = false;
inline constexpr std::array<i16, 4> DIFFICULTY_SOUNDS = {0, 0, 0, 0};
inline constexpr u8 DIFFICULTY_UNCHOSEN_COLOUR = 0;
inline constexpr bool DRAG_USES_SLOT_7 = true;
inline constexpr bool EGG_MARKS_ARMED = false;
inline constexpr int EQUIPMENT_SLOTS = 0x1e;
inline constexpr int EQUIPMENT_SLOTS_CAPACITY = EQUIPMENT_SLOTS;
inline constexpr bool EXPLOSION_ALERTS = false;
inline constexpr bool EXPLOSION_SOUND_AT_ITEM = false;
inline constexpr bool FALL_CHECKED_FIRST = false;
inline constexpr bool FEINTING_NPC_MAY_CHANGE = false;
inline constexpr std::array<i16, 3> FEMALE_CRIES = {0xb, 0xa, -1};
inline constexpr std::array<u16, 2> FEMALE_SHAPES = {0x597, 0x3ac};
inline constexpr bool FIRE_INTERRUPTS_BUSY = false;
inline constexpr int FIRE_TYPES = 0x10;
inline constexpr u16 FIRST_FRAGMENT_PERCENT = 57;
inline constexpr u32 FIRST_MINIMUM = 0x180000L;
inline constexpr i16 FIRST_RETICLE_STYLE = 0;
inline constexpr bool FRAME_SKIP_SPILLS = true;
inline constexpr int FRAME_SOUND_MASK = 0xff;
inline constexpr const char *GAME_PROGRAM_NAME = "crusader";
inline constexpr std::array<SavedObject, 1> GAME_SAVED_OBJECTS{
    {{91, []() -> u8 * { return &weaselVisited; }, 1, "weaselVisited"}}};
inline constexpr const char *GAME_TITLE = "CRUSADER: No Remorse -";
inline constexpr const char *GAME_VERSION = "V1.21";
inline constexpr const char *GOG_OPTIONS =
    "irq=5\ndma=1\nport=0x220\nsoundcard=3\nsound=on\nmusic=on\nfullinstall=true\n"
    "bigvideo=true\nsubtitles=true\ncdletter=D\nflicpath=\\flics\nnewvideo=true\n";
inline constexpr const char *GOODBYE = "CRUSADER: No Remorse\n\r";
inline constexpr const char *GOODBYE_WITH_CHEATS = "No pity. No mercy. No remorse.\n\r";
inline constexpr u16 GRAVITY_PROCESS_SIZE = 0x42;
inline constexpr std::array<u16, 3> GRENADE_BLASTS = {0x326, 0x320, 0x321};
inline constexpr bool HELP_ARROW_KEYS = true;
inline constexpr bool HELP_BUTTONS = true;
inline constexpr std::array<const char *, 5> HELP_FILES = {"help1.dat", "help2.dat", nullptr, nullptr, "help3.dat"};
inline constexpr const char *HELP_MESSAGE = "You DO need help!\n\r";
inline constexpr i16 HELP_PAGES = 2;
inline constexpr bool HELP_PAGES_WRAP = false;
inline constexpr bool HELP_PAGE_SOUNDS = true;
inline constexpr u16 HELP_SOUND = 0;
inline constexpr i16 HURL_X_NO_DIRECTION = -2;
inline constexpr i16 HURL_Y_NO_DIRECTION = 0x88;
inline constexpr bool IDLE_TIMING = false;
inline constexpr bool INSTALL_OPTIONS = true;
inline constexpr bool INTRINSICS_SHOP = true;
inline constexpr const char *INTRINSICS_TEXT = "FART ...TRY... -laurie (Have fun, Jely)";
inline constexpr std::array<MovieRequest, 2> INTRO_MOVIES = {
    MovieRequest{.name = "T01", .background = MovieBackground::GREY, .holdMusic = true},
    MovieRequest{.name = "T02", .background = MovieBackground::GREY, .holdMusic = true}};
inline constexpr u16 INTRO_PAUSE = 0;
inline constexpr int INVENTORY_TYPES = 0xc;
inline constexpr std::array<ItemKey, 2> ITEM_KEYS = {
    {{Action::USE_ENERGY_CUBE, SHAPE_ENERGY_CUBE, true}, {Action::USE_MEDKIT, SHAPE_MEDICAL_KIT, true}}};
inline constexpr bool ITEM_USED_FIRST = true;
inline constexpr bool JASSICA_JOKE = false;
inline constexpr std::array<i16, 16> JOYSTICK_ACTIONS = {{0, 1, 3, 2, 4, 9, -1, 6, -1, -1, -1, -1, -1}};
inline constexpr i16 JOY_ALT = 2;
inline constexpr i16 JOY_CONTROL = 1;
inline constexpr i16 JOY_JUMP = -1;
inline constexpr bool JOY_JUMPS = false;
inline constexpr i16 JOY_KEYS = 4;
inline constexpr bool JOY_REPEATED_KEYS = false;
inline constexpr i16 JOY_SHIFT = 3;
inline constexpr bool JOY_SHIFTS_TAKE_CONTROL = false;
inline constexpr bool JUMP_COMPACTS_MEMORY = false;
inline constexpr bool JUMP_MOVES_TO_EGG = false;
inline constexpr bool JUMP_RESETS_SOUND_CACHE = true;
inline constexpr bool JUMP_SHOWS_LOADING = false;
inline constexpr bool JUMP_UNREADIES_WEAPON = true;
inline constexpr std::array<i16, 7> KEPT_ANIMS = {ANIM_COMBAT_STAND_SMALL,
                                                  ANIM_UNREADY_SMALL,
                                                  ANIM_READY_SMALL,
                                                  ANIM_FIRE_SMALL,
                                                  ANIM_ADVANCE_SMALL,
                                                  ANIM_RETREAT_SMALL,
                                                  -1};
inline constexpr std::array<u16, 5> KEPT_NPC_SHAPES = {0x3ac, 0x4dc, 0x524, 0x4d9, 0x4e0};
inline constexpr u16 KEPT_WEAPON = SHAPE_BA_40;
inline constexpr int KERNEL_ITEMS = 0x2400;
inline constexpr int KEY_QUEUE = 32;
inline constexpr int KEY_QUEUE_CAPACITY = KEY_QUEUE;
inline constexpr std::array<i16, 11> KNEELING_ANIMS = {ANIM_KNEEL_START,
                                                       ANIM_KNEELING_SMALL,
                                                       ANIM_KNEELING_LARGE,
                                                       ANIM_KNEEL_FIRE_SMALL,
                                                       ANIM_KNEEL_FIRE_LARGE,
                                                       ANIM_COMBAT_ROLL_LEFT,
                                                       ANIM_COMBAT_ROLL_RIGHT,
                                                       ANIM_KNEEL_ROLL_LEFT,
                                                       ANIM_KNEEL_ROLL_RIGHT,
                                                       ANIM_BRIGHT_KNEEL_FIRE,
                                                       -1};
inline constexpr bool KNEELING_MOVES_HAND_BACK = false;
inline constexpr int LAST_SOUND = 512;
inline constexpr bool LEDGE_HOLDS_CONTROLLED_NPC = false;
inline constexpr const char *LOADING_STEP = ".";
inline constexpr u16 LOAD_SAVE_GUMP_SIZE = 0x3c;
inline constexpr bool LOAD_SAVE_PICTURE = false;
inline constexpr u16 LOAD_SAVE_POINTER = 0x25;
inline constexpr bool MAIN_MENU_ALT_X_QUITS = false;
inline constexpr i16 MAIN_MENU_HEIGHT = 240;
inline constexpr bool MAIN_MENU_PLAYS_SOUND = false;
inline constexpr u16 MAIN_MENU_SOUND = 0;
inline constexpr std::array<i16, 4> MALE_CRIES = {0x65, 0x66, 0x67, -1};
inline constexpr bool MEMORY_PROFILER = false;
inline constexpr const char *MISSING_CD_MESSAGE = nullptr;
inline constexpr bool MORE_CHEAT_CODES = false;
inline constexpr bool MORE_DEBUG_KEYS = true;
inline constexpr bool MORE_HIT_SOUNDS = false;
inline constexpr bool MOVE_NO_REGRET = false;
inline constexpr bool NEW_ACTIVITY_PUTS_WEAPON_AWAY = false;
inline constexpr std::array<u16, 9> NON_COMBAT_ANIMS = {
    ANIM_WALK,          ANIM_STAND,         ANIM_QUICK_JUMP,  ANIM_RUN, ANIM_COMBAT_RUN_SMALL, ANIM_COMBAT_RUN_LARGE,
    ANIM_UNREADY_SMALL, ANIM_UNREADY_LARGE, ANIM_ELECTROCUTED};
inline constexpr Point NOTICE_PLACE = Point(0x5b, 0x2f);
inline constexpr bool NPCS_AIM_AT_ATTACK_TARGET = false;
inline constexpr int NPC_DATA_SIZE = 0x79;
inline constexpr bool NUMBER_KEYS_SELECT_WEAPONS = false;
inline constexpr std::array<i16, 13> NUMBER_KEY_WEAPONS = {-1};
inline constexpr bool ON_SCREEN_WHOLE_HEIGHT = true;
inline constexpr u8 OPTIONS_FOCUS_COLOUR = 0xc;
inline constexpr bool OPTIONS_IN_MENU_FRAME = false;
inline constexpr bool OTHER_SHOOTERS_SPREAD = true;
inline constexpr SlotRange PACK_SLOTS = {11, 25};
inline constexpr u16 PAUSE_WAIT = 4800;
inline constexpr int PICKUP_SPARE_COLUMNS = 2;
inline constexpr bool POSITION_KEY = true;
inline constexpr bool PRELOAD_BY_SHAPES = false;
inline constexpr bool PREVIOUS_WEAPON_SETS_STANCE = false;
inline constexpr const char *PRODUCTION = "";
inline constexpr bool PRODUCTION_LINE = false;
inline constexpr std::uint32_t PROFILE_MEMORY = 15000576;
inline constexpr char PROFILE_NAME[] = "remorse";
inline constexpr char PROFILE_TITLE[] = "Crusader: No Remorse";
inline constexpr Point QUESTION_NO = Point(0x87, 0x2f);
inline constexpr Point QUESTION_YES = Point(0x37, 0x2f);
inline constexpr u16 QUICK_LOAD_SOUND = 0x107;
inline constexpr u16 QUICK_SAVE_SOUND = 0x108;
inline constexpr u16 QUIT_SOUND = 0x109;
inline constexpr std::array<u16, 0> REGION_SONGS{};
inline constexpr bool RETICLE_HIDDEN_WHILE_HELD = false;
inline constexpr std::array<u16, 1> RETICLE_NPC_SHAPES = {0x580};
inline constexpr bool RETICLE_STYLE_KEY = false;
inline constexpr bool RETURN_KEY_ENDS_STASIS = false;
inline constexpr bool ROBOTS_AIM_EXACTLY = false;
inline constexpr std::array<u16, 7> ROBOT_SHAPES = {0x4c8, 0x338, 0x45d, 0x2cb, 0x4e6, 0x383, 0x385};
inline constexpr std::array<u16, 3> ROCKET_BLASTS = {0x31c, 0x31f, 0x322};
inline constexpr bool ROLLING_THUNDER = false;
inline constexpr bool ROLL_FORWARD_DODGES = false;
inline constexpr bool ROLL_FORWARD_STEPS = false;
inline constexpr bool RP_32_NEVER_DRY = false;
inline constexpr i16 SAVE_FLEX_ENTRIES = 0x400;
inline constexpr u16 SEARCH_VTABLES[5] = {0x0600, 0x6600, 0x04de, 0x0eda, 0x0608};
inline constexpr u8 SELECTED_SLOT_COLOUR = 0xfd;
inline constexpr u16 SELECT_SOUND = 0xb0;
inline constexpr bool SELECT_SOUND_WAITS = false;
inline constexpr Rect SETTINGS_AREA = Rect(11, 132, 628, 349);
inline constexpr bool SETTINGS_OPEN_PLAYS_SOUND = false;
inline constexpr u16 SETTINGS_OPEN_SOUND = 0;
inline constexpr s16 SETTINGS_ROW_GAP = 5;
inline constexpr bool SHAPES_FROM_SWAP = true;
inline constexpr bool SHAPE_5D6_IS_DEAD = false;
inline constexpr int SHAPE_BLAST_PAC = 0x39b;
inline constexpr int SHAPE_CONCUSSION_GRENADE = 0x34e;
inline constexpr int SHAPE_CREDITS = 0x4ed;
inline constexpr int SHAPE_NERVE_GAS_GRENADE = 0x34f;
inline constexpr bool SHAPE_SOUNDS = false;
inline constexpr bool SHARE_BY_MEMORY_SIZE = false;
inline constexpr bool SHOT_HITS_AS_ITEM = true;
inline constexpr bool SHUTDOWN_AFTER_START = false;
inline constexpr bool SHUTDOWN_ENDS_FADES = false;
inline constexpr bool SILENCER_SPRITE = false;
inline constexpr i16 SLOTS_X = 72;
inline constexpr i16 SLOTS_Y = 43;
inline constexpr bool SMALL_WEAPON_ANIMS = false;
inline constexpr i16 SONG_FILES = 20;
inline constexpr i16 SONG_FILES_CAPACITY = SONG_FILES;
inline constexpr std::array<const char *, 21> SONG_NAMES = {nullptr, "m01", "m02",  "m03",  "m04",  "m05",   "m06",
                                                            "m07",   "m08", "m09",  "m10",  "m11",  "m12",   "m13",
                                                            "m14",   "m15", "m16a", "cred", "menu", "buyme", nullptr};
inline constexpr std::array<SongTurns, 1> SONG_TURNS{{{16, {"m16a", "m16b", "m16c"}}}};
inline constexpr int SONG_TURNS_CAPACITY = 1;
inline constexpr int SOUND_CACHE_SPARE = 4096;
inline constexpr int SOUND_FLEX_MODE = FILE_READWRITE;
inline constexpr bool SOUND_FLEX_TAGGED = true;
inline constexpr u32 SOUND_FRAGMENT_SIZE = 300000;
inline constexpr bool STANDING_PUTS_WEAPON_AWAY = true;
inline constexpr bool START_CHECKS_FILES = false;
inline constexpr bool START_HOLDS_SILENCER = false;
inline constexpr i16 STATUS_AMMO_FONT = 15;
inline constexpr i16 STATUS_AMMO_X = 30;
inline constexpr i16 STATUS_AMMO_Y = 11;
inline constexpr i16 STATUS_BAR_HEIGHT = 13;
inline constexpr u16 STATUS_BAR_LENGTH = 66;
inline constexpr i16 STATUS_BAR_X = 34;
inline constexpr i16 STATUS_BAR_Y = 7;
inline constexpr i16 STATUS_CLIPS_X = 75;
inline constexpr bool STATUS_NAMES = false;
inline constexpr bool STATUS_NO_HEALTH_SHAPE = false;
inline constexpr u8 STATUS_PAINT_MODE = 1;
inline constexpr std::array<StrongerWeapon, 13> STRONGER_WEAPONS = {
    StrongerWeapon{0x2fd, 4, 12, 3}, StrongerWeapon{0x319, 4, 12, 3}, StrongerWeapon{0x384, 3, 5, 10},
    StrongerWeapon{0x1b4, 4, 13, 9}, StrongerWeapon{0x528, 3, 9, 8},  StrongerWeapon{0x371, 3, 9, 10},
    StrongerWeapon{0x4d1, 2, 4, 11}, StrongerWeapon{0x383, 3, 10, 7}, StrongerWeapon{0x4e6, 3, 5, 11},
    StrongerWeapon{0x385, 4, 8, 9},  StrongerWeapon{0x338, 3, 5, 7},  StrongerWeapon{0x2cb, 2, 3, 7},
    StrongerWeapon{0x3ac, 2, 9, 13}};
inline constexpr SubtitleStyle SUBTITLE_STYLE = {.font = 4,
                                                 .stripWidth = 601,
                                                 .stripHeight = 16,
                                                 .stripColour = 0,
                                                 .centreWidth = 600,
                                                 .shift = 4,
                                                 .textY = 2,
                                                 .twoTone = false,
                                                 .screenX = 20,
                                                 .screenY = -1,
                                                 .gap = 8,
                                                 .room = 10,
                                                 .lowest = 470,
                                                 .hiColourTextY = 0,
                                                 .hiColourLines = 0,
                                                 .ink = 8};
inline constexpr u16 SURRENDER_SIZE = 0x45;
inline constexpr bool SURRENDER_SPEAKS_ONCE = true;
inline constexpr bool SWAP_REBUILD_HALTS = false;
inline constexpr bool TAKE_HIT_NO_REGRET = false;
inline constexpr bool TARGETING_KEY = true;
inline constexpr bool TELEPORT_COMPACTS_MEMORY = false;
inline constexpr bool TELEPORT_RESTARTS_TRACKER = true;
inline constexpr bool THAW_ENDS_TELEPORT = true;
inline constexpr bool THAW_REDRAWS_SPRITES = false;
inline constexpr bool TIMED_FEINT = true;
inline constexpr u8 TITLE_BAR_COLOUR = BLUE;
inline constexpr bool TITLE_SPACED = true;
inline constexpr bool TUNES_BY_MISSION = true;
inline constexpr i16 TUNES_READ = 17;
inline constexpr i16 TUNES_WRITTEN = 18;
inline constexpr std::array<u16, 0> UNARMED_SHOOTERS = {};
inline constexpr std::array<u16, 5> UNCHARGED_ITEMS = {
    {SHAPE_DATA_LINK, SHAPE_INHIBITOR, SHAPE_GRAVITON_SHIELD, SHAPE_PLASMA_SHIELD, SHAPE_IONIC_SHIELD}};
inline constexpr std::array<u16, 3> UNSTACKED_SHAPES = {SHAPE_IONIC_SHIELD, SHAPE_PLASMA_SHIELD, SHAPE_GRAVITON_SHIELD};
inline constexpr SlotRange UTILITY_SLOTS = {6, 10};
inline constexpr i16 VERSION_FRAGMENTS = 4;
inline constexpr u16 VERSION_SOUND = 0x103;
inline constexpr bool VIOLENCE_OPTION = false;
inline constexpr std::array<u16, 5> WATCHFUL_SHAPES = {0x2f5, 0x2f6, 0x2f7, 0x595, 0x597};
inline constexpr bool WEAK_WEAPONS_TAKE_TYPE_DAMAGE = false;
inline constexpr bool WEAPONS_ONLY_FOR_CONTAINERS = false;
inline constexpr bool WEAPON_KEYS_SILENCER_ONLY = true;
inline constexpr bool WEAPON_LISTS = true;
inline constexpr std::array<i16, 6> WEAPON_LIST_1 = {SHAPE_UV_9,   SHAPE_PA_31, SHAPE_AR_7,
                                                     SHAPE_GL_303, SHAPE_RP_32, -1};
inline constexpr std::array<i16, 6> WEAPON_LIST_2 = {SHAPE_BA_40, SHAPE_BA_41, SHAPE_RP_22,
                                                     SHAPE_PA_21, SHAPE_PL_1,  -1};
inline constexpr std::array<i16, 4> WEAPON_LIST_3 = {SHAPE_SG_A1, SHAPE_AC_88, SHAPE_EM_4, -1};
inline constexpr SlotRange WEAPON_SLOTS = {1, 5};
inline constexpr u8 WIDGET_DIM_EDGE = 139;
inline constexpr u8 WIDGET_DIM_FACE = 45;
inline constexpr u8 WIDGET_DIM_WARNING_EDGE = 16;
inline constexpr u8 WIDGET_DIM_WARNING_FACE = 94;
inline constexpr u8 WIDGET_EDGE = 28;
inline constexpr u8 WIDGET_FACE = 12;
inline constexpr u8 WIDGET_FOCUS = 0xc;
inline constexpr u8 WIDGET_WARNING_EDGE = 26;
inline constexpr u8 WIDGET_WARNING_FACE = 14;
inline constexpr bool WORLD_KEYS_NO_REGRET = false;
inline constexpr bool XAN_MOVIE_HOST = false;
inline constexpr bool XAN_MOVIE_OPTIONS = false;

}
#endif
