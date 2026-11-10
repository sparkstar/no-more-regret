/* Written by port/tools/make_unified_values.py from regret/src's value headers: regret's values, in a namespace of
 * their own, which fill the one program's when it plays regret; do not edit. */

#ifndef PORT_VALUES_REGRET_H
#define PORT_VALUES_REGRET_H
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

namespace regret_values {

inline constexpr u16 ACTIVITY_SIZE = 0x48;
inline constexpr bool AIM_PAST_RETICLE = true;
inline constexpr bool ALERT_ONLY_ON_CHANGE = true;
inline constexpr bool ALERT_SWAPS_LIGHTS = true;
inline constexpr u8 ALL_SHAPES = 1;
inline constexpr bool ANCHOR_PROCESS = true;
inline constexpr bool ANIM_16_AND_SLIDE_RIGHT = true;
inline constexpr u16 ANIM_PRIMITIVE_SIZE = 0x5e;
inline constexpr bool ARROWS_PRESSED_AFTER_FIRE = false;
inline constexpr bool ATTACK_NO_REGRET = true;
inline constexpr u16 ATTACK_SIZE = 0x9c;
inline constexpr u16 AUTO_FIRER_RATE = 12;
inline constexpr bool AUTO_FIRER_USES_AMMO = true;
inline constexpr bool AUTO_FIRE_STOPS_EMPTY = true;
inline constexpr u16 BASE_MAP = 25;
inline constexpr bool BLASTS_LIT_AT_ONE = true;
inline constexpr bool BLOCKED_MOVE_SEARCH = true;
inline constexpr i16 BOSS_FIRE_DELAY = 0x1e;
inline constexpr bool BOSS_GRENADES = true;
inline constexpr u16 BOSS_SHAPE = 0x5b1;
inline constexpr i16 BOSS_SHIELD_START = 1500;
inline constexpr bool BUSY_NPC_TURNS = true;
inline constexpr bool CACHE_KEEPS_FREE = true;
inline constexpr bool CALIBRATE_ONCE = true;
inline constexpr const char *CFG_LINE_END = "\r\n";
inline constexpr u16 CHARACTER_WAIT = 30;
inline constexpr bool CHARACTER_WAITS_A_TICK = false;
inline constexpr std::array<s16, CA_FONTS> CHAR_SPACINGS = {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                            1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
inline constexpr int SHAPE_BK_16_CLIP = 0x615;
inline constexpr int SHAPE_LNR_81_CLIP = 0x614;
inline constexpr std::array<u16, 6> CHEAT_CLIPS = {
    {SHAPE_RP_32_CLIP, SHAPE_AR_7_CLIP, SHAPE_GL_303_CLIP, SHAPE_SG_A1_CLIP, SHAPE_BK_16_CLIP, SHAPE_LNR_81_CLIP}};
inline constexpr u32 CHEAT_CREDITS = 0;
inline constexpr int SHAPE_VIR_IMAGER = 0x4ed;
inline constexpr int SHAPE_SPIDER_MINE = 0x11e;
inline constexpr int SHAPE_MINE_DET = 0x61c;
inline constexpr int SHAPE_PORTABLE_BETTY = 0x63e;
inline constexpr int SHAPE_DISRUPTER = 0x616;
inline constexpr int SHAPE_RADIATION_SHIELD = 0x191;
inline constexpr int SHAPE_DATA_PICK = 0x613;
inline constexpr std::array<CheatGift, 13> CHEAT_GIFTS = {{{SHAPE_SPIDER_BOMB, 20},
                                                           {SHAPE_LAND_MINE, 20},
                                                           {SHAPE_DET_PAC, 20},
                                                           {SHAPE_MEDICAL_KIT, 20},
                                                           {SHAPE_VIR_IMAGER, 1},
                                                           {SHAPE_INHIBITOR, 1},
                                                           {SHAPE_ENERGY_CUBE, 20},
                                                           {SHAPE_SPIDER_MINE, 20},
                                                           {SHAPE_MINE_DET, 20},
                                                           {SHAPE_PORTABLE_BETTY, 20},
                                                           {SHAPE_DISRUPTER, 20},
                                                           {SHAPE_RADIATION_SHIELD, 20},
                                                           {SHAPE_DATA_PICK, 20}}};
inline constexpr bool CHEAT_GIVES_CREDITS = false;
inline constexpr bool CHEAT_IMMORTALITY_ALONE = true;
inline constexpr u16 CHEAT_ITEM_QUALITY = 99;
inline constexpr bool CHEAT_OPTION = false;
inline constexpr std::array<const char *, 1> CHRISTMAS_SONGS = {"xmas"};
inline constexpr bool COMBAT_RUN_DODGES = true;
inline constexpr bool CONTROLLED_FALL_AT_ONCE = true;
inline constexpr int CONTROLLER_BLOCK = 79;
inline constexpr int CONTROLLER_BLOCK_CAPACITY = CONTROLLER_BLOCK;
inline constexpr u8 COPYRIGHT_COLOUR = RED;
inline constexpr const char *COPYRIGHT_LINE = "Copyright (c) 1996 Origin Systems.";
inline constexpr std::array<u16, 0> CRATE_SHAPES{};
inline constexpr const char *CREDITS_CHEAT_MOVIE = nullptr;
inline constexpr bool CREDITS_END_TIDILY = true;
inline constexpr bool DEAD_NPCS_RESTART = false;
inline constexpr bool DEATH_CLEARS_STASIS = true;
inline constexpr u16 DEATH_SOUND = 0x194;
inline constexpr bool DEBUG_KEY_FLUSHES = true;
inline constexpr i16 DEFAULT_STRONGER_WEAPON = 15;
inline constexpr std::array<u16, 2> DETONATED_SHAPES = {{SHAPE_DET_PAC, SHAPE_PORTABLE_BETTY}};
inline constexpr u16 DIFFICULTY_BUTTON_SIZE = 0x40;
inline constexpr bool DIFFICULTY_BUTTON_SOUNDS = true;
inline constexpr u8 DIFFICULTY_CHOSEN_COLOUR = 0xf1;
inline constexpr bool DIFFICULTY_FRAMES_UNCHOSEN = true;
inline constexpr std::array<i16, 4> DIFFICULTY_SOUNDS = {0x19d, 0x19e, 0x19f, 0x1a0};
inline constexpr u8 DIFFICULTY_UNCHOSEN_COLOUR = 0xee;
inline constexpr bool DRAG_USES_SLOT_7 = false;
inline constexpr bool EGG_MARKS_ARMED = true;
inline constexpr int EQUIPMENT_SLOTS = 0x2d;
inline constexpr int EQUIPMENT_SLOTS_CAPACITY = EQUIPMENT_SLOTS;
inline constexpr bool EXPLOSION_ALERTS = true;
inline constexpr bool EXPLOSION_SOUND_AT_ITEM = true;
inline constexpr bool FALL_CHECKED_FIRST = true;
inline constexpr bool FEINTING_NPC_MAY_CHANGE = true;
inline constexpr std::array<i16, 4> FEMALE_CRIES = {0x3d, 0x77, 0x210, -1};
inline constexpr std::array<u16, 1> FEMALE_SHAPES = {0x597};
inline constexpr bool FIRE_INTERRUPTS_BUSY = true;
inline constexpr int FIRE_TYPES = 0x17;
inline constexpr u16 FIRST_FRAGMENT_PERCENT = 53;
inline constexpr u32 FIRST_MINIMUM = 0;
inline constexpr i16 FIRST_RETICLE_STYLE = 3;
inline constexpr bool FRAME_SKIP_SPILLS = false;
inline constexpr int FRAME_SOUND_MASK = 0x3ff;
inline constexpr const char *GAME_PROGRAM_NAME = "regret";
inline constexpr std::array<SavedObject, 0> GAME_SAVED_OBJECTS{};
inline constexpr const char *GAME_TITLE = "CRUSADER: No Regret ";
inline constexpr const char *GAME_VERSION = "V1.06";
inline constexpr const char *GOG_OPTIONS =
    "irq=5\ndma=5\nport=0x220\nsoundcard=3\nsound=on\nmusic=on\nmoviehicolor=true\n"
    "bigvideo=true\nsubtitles=true\ncontrol=0\ncdletter=D\nflicpath=flics\n";
inline constexpr const char *GOODBYE = "No pity. No mercy. No regret.\n\r";
inline constexpr const char *GOODBYE_WITH_CHEATS = "No pity. No mercy. No regret.\n\r";
inline constexpr u16 GRAVITY_PROCESS_SIZE = 0x43;
inline constexpr std::array<u16, 4> GRENADE_BLASTS = {0x326, 0x320, 0x321, 0x323};
inline constexpr bool HELP_ARROW_KEYS = false;
inline constexpr bool HELP_BUTTONS = false;
inline constexpr std::array<const char *, 5> HELP_FILES = {"help1.bmp", "help2.bmp", "help3.bmp", "help4.bmp",
                                                           "help5.bmp"};
inline constexpr const char *HELP_MESSAGE = nullptr;
inline constexpr i16 HELP_PAGES = 5;
inline constexpr bool HELP_PAGES_WRAP = true;
inline constexpr bool HELP_PAGE_SOUNDS = false;
inline constexpr u16 HELP_SOUND = 0x1a3;
inline constexpr i16 HURL_X_NO_DIRECTION = -2;
inline constexpr i16 HURL_Y_NO_DIRECTION = static_cast<i16>(0x8800);
inline constexpr bool IDLE_TIMING = true;
inline constexpr bool INSTALL_OPTIONS = false;
inline constexpr bool INTRINSICS_SHOP = false;
inline constexpr const char *INTRINSICS_TEXT = "No Regret!";
inline constexpr std::array<MovieRequest, 2> INTRO_MOVIES = {
    MovieRequest{.name = "ORIGIN", .mayDouble = false, .restorePalette = false},
    MovieRequest{.name = "ANIM01", .restorePalette = false}};
inline constexpr u16 INTRO_PAUSE = 30;
inline constexpr int INVENTORY_TYPES = 0x13;
inline constexpr std::array<ItemKey, 6> ITEM_KEYS = {{{Action::USE_ENERGY_CUBE, SHAPE_ENERGY_CUBE, true},
                                                      {Action::USE_VIR_IMAGER, SHAPE_VIR_IMAGER, false},
                                                      {Action::USE_MINE_DETECTOR, SHAPE_MINE_DET, true},
                                                      {Action::USE_RADIATION_SHIELD, SHAPE_RADIATION_SHIELD, false},
                                                      {Action::DATALINK, SHAPE_DATA_LINK, false},
                                                      {Action::USE_MEDKIT, SHAPE_MEDICAL_KIT, true}}};
inline constexpr bool ITEM_USED_FIRST = false;
inline constexpr bool JASSICA_JOKE = true;
inline constexpr std::array<i16, 16> JOYSTICK_ACTIONS = {{0, 10, 2, 7, 11, -1, 8, 5, 1, 9, -1, -1, -1}};
inline constexpr i16 JOY_ALT = 3;
inline constexpr i16 JOY_CONTROL = 2;
inline constexpr i16 JOY_JUMP = 1;
inline constexpr bool JOY_JUMPS = true;
inline constexpr i16 JOY_KEYS = 5;
inline constexpr bool JOY_REPEATED_KEYS = true;
inline constexpr i16 JOY_SHIFT = 4;
inline constexpr bool JOY_SHIFTS_TAKE_CONTROL = true;
inline constexpr bool JUMP_COMPACTS_MEMORY = true;
inline constexpr bool JUMP_MOVES_TO_EGG = true;
inline constexpr bool JUMP_RESETS_SOUND_CACHE = false;
inline constexpr bool JUMP_SHOWS_LOADING = true;
inline constexpr bool JUMP_UNREADIES_WEAPON = false;
inline constexpr std::array<i16, 8> KEPT_ANIMS = {
    ANIM_COMBAT_STAND_SMALL, ANIM_UNREADY_SMALL, ANIM_READY_SMALL,    ANIM_FIRE_SMALL,
    ANIM_ADVANCE_SMALL,      ANIM_RETREAT_SMALL, ANIM_STOP_RUN_SMALL, -1};
inline constexpr std::array<u16, 0> KEPT_NPC_SHAPES{};
inline constexpr u16 KEPT_WEAPON = SHAPE_RP_32;
inline constexpr int KERNEL_ITEMS = 0x2800;
inline constexpr int KEY_QUEUE = 48;
inline constexpr int KEY_QUEUE_CAPACITY = KEY_QUEUE;
inline constexpr std::array<i16, 16> KNEELING_ANIMS = {
    ANIM_KNEEL_START,      ANIM_KNEELING_SMALL,      ANIM_KNEELING_LARGE,       ANIM_KNEEL_FIRE_SMALL,
    ANIM_KNEEL_FIRE_LARGE, ANIM_COMBAT_ROLL_LEFT,    ANIM_COMBAT_ROLL_RIGHT,    ANIM_KNEEL_ROLL_LEFT,
    ANIM_KNEEL_ROLL_RIGHT, ANIM_BRIGHT_KNEEL_FIRE,   ANIM_ROLL_FORWARD,         ANIM_KNEELING_RETREAT,
    ANIM_KNEELING_ADVANCE, ANIM_KNEELING_SLIDE_LEFT, ANIM_KNEELING_SLIDE_RIGHT, -1};
inline constexpr bool KNEELING_MOVES_HAND_BACK = true;
inline constexpr int LAST_SOUND = 1024;
inline constexpr bool LEDGE_HOLDS_CONTROLLED_NPC = true;
inline constexpr const char *LOADING_STEP = "\xb1";
inline constexpr u16 LOAD_SAVE_GUMP_SIZE = 0x40;
inline constexpr bool LOAD_SAVE_PICTURE = true;
inline constexpr u16 LOAD_SAVE_POINTER = 0x28;
inline constexpr bool MAIN_MENU_ALT_X_QUITS = true;
inline constexpr i16 MAIN_MENU_HEIGHT = 330;
inline constexpr bool MAIN_MENU_PLAYS_SOUND = true;
inline constexpr u16 MAIN_MENU_SOUND = 0x1a1;
inline constexpr std::array<i16, 5> MALE_CRIES = {0x24f, 0x250, 0x201, 0x200, -1};
inline constexpr bool MEMORY_PROFILER = true;
inline constexpr const char *MISSING_CD_MESSAGE = "\n\rCANNOT FIND THE CRUSADER: No Regret CD.\n\r";
inline constexpr bool MORE_CHEAT_CODES = true;
inline constexpr bool MORE_DEBUG_KEYS = false;
inline constexpr bool MORE_HIT_SOUNDS = true;
inline constexpr bool MOVE_NO_REGRET = true;
inline constexpr bool NEW_ACTIVITY_PUTS_WEAPON_AWAY = true;
inline constexpr std::array<u16, 10> NON_COMBAT_ANIMS = {ANIM_WALK,
                                                         ANIM_STAND,
                                                         ANIM_QUICK_JUMP,
                                                         ANIM_RUN,
                                                         ANIM_COMBAT_RUN_SMALL,
                                                         ANIM_COMBAT_RUN_LARGE,
                                                         ANIM_UNREADY_SMALL,
                                                         ANIM_UNREADY_LARGE,
                                                         ANIM_ELECTROCUTED,
                                                         ANIM_ROLL_FORWARD};
inline constexpr Point NOTICE_PLACE = Point(0x4a, 0x40);
inline constexpr bool NPCS_AIM_AT_ATTACK_TARGET = true;
inline constexpr int NPC_DATA_SIZE = 0x97;
inline constexpr bool NUMBER_KEYS_SELECT_WEAPONS = true;
inline constexpr int SHAPE_BK_16 = 0x5f6;
inline constexpr int SHAPE_XP_5 = 0x198;
inline constexpr int SHAPE_LNR_81 = 0x5f5;
inline constexpr std::array<i16, 13> NUMBER_KEY_WEAPONS = {
    SHAPE_RP_32, SHAPE_SG_A1,  SHAPE_AC_88, SHAPE_PA_31,  SHAPE_PL_1, SHAPE_EM_4, SHAPE_BK_16,
    SHAPE_XP_5,  SHAPE_LNR_81, SHAPE_AR_7,  SHAPE_GL_303, SHAPE_UV_9, -1};
inline constexpr bool ON_SCREEN_WHOLE_HEIGHT = false;
inline constexpr u8 OPTIONS_FOCUS_COLOUR = 0x74;
inline constexpr bool OPTIONS_IN_MENU_FRAME = true;
inline constexpr bool OTHER_SHOOTERS_SPREAD = false;
inline constexpr SlotRange PACK_SLOTS = {21, 44};
inline constexpr u16 PAUSE_WAIT = 1800;
inline constexpr int PICKUP_SPARE_COLUMNS = 3;
inline constexpr bool POSITION_KEY = false;
inline constexpr bool PRELOAD_BY_SHAPES = true;
inline constexpr bool PREVIOUS_WEAPON_SETS_STANCE = true;
inline constexpr const char *PRODUCTION = "A Loose Cannon Production";
inline constexpr bool PRODUCTION_LINE = true;
inline constexpr std::uint32_t PROFILE_MEMORY = 15014912;
inline constexpr char PROFILE_NAME[] = "regret";
inline constexpr char PROFILE_TITLE[] = "Crusader: No Regret";
inline constexpr Point QUESTION_NO = Point(0x63, 0x4e);
inline constexpr Point QUESTION_YES = Point(0x2c, 0x4e);
inline constexpr u16 QUICK_LOAD_SOUND = 0x192;
inline constexpr u16 QUICK_SAVE_SOUND = 0x193;
inline constexpr u16 QUIT_SOUND = 0x17c;
inline constexpr std::array<u16, 26> REGION_SONGS = {0, 1,  10, 2,  0, 3,  11, 4,  16, 5,  20, 6, 0,
                                                     7, 13, 8,  15, 9, 12, 10, 19, 14, 21, 0,  0, 2};
inline constexpr bool RETICLE_HIDDEN_WHILE_HELD = true;
inline constexpr std::array<u16, 0> RETICLE_NPC_SHAPES = {};
inline constexpr bool RETICLE_STYLE_KEY = true;
inline constexpr bool RETURN_KEY_ENDS_STASIS = true;
inline constexpr bool ROBOTS_AIM_EXACTLY = true;
inline constexpr std::array<u16, 14> ROBOT_SHAPES = {0x4c8, 0x338, 0x45d, 0x2cb, 0x4e6, 0x383, 0x385,
                                                     0x1cd, 0x5e6, 0x319, 0x30c, 0x5e2, 0x5b1, 0x5ba};
inline constexpr std::array<u16, 4> ROCKET_BLASTS = {0x31c, 0x31f, 0x321, 0x31f};
inline constexpr bool ROLLING_THUNDER = true;
inline constexpr bool ROLL_FORWARD_DODGES = true;
inline constexpr bool ROLL_FORWARD_STEPS = true;
inline constexpr bool RP_32_NEVER_DRY = true;
inline constexpr i16 SAVE_FLEX_ENTRIES = 0xc00;
inline constexpr u16 SEARCH_VTABLES[5] = {0x0290, 0x7190, 0x07b4, 0x12d8, 0x08d2};
inline constexpr u8 SELECTED_SLOT_COLOUR = 0x6f;
inline constexpr u16 SELECT_SOUND = 0x1a7;
inline constexpr bool SELECT_SOUND_WAITS = true;
inline constexpr Rect SETTINGS_AREA = Rect(10, 94, 627, 385);
inline constexpr bool SETTINGS_OPEN_PLAYS_SOUND = true;
inline constexpr u16 SETTINGS_OPEN_SOUND = 0x1a2;
inline constexpr s16 SETTINGS_ROW_GAP = 8;
inline constexpr bool SHAPES_FROM_SWAP = false;
inline constexpr bool SHAPE_5D6_IS_DEAD = true;
inline constexpr int SHAPE_CREDITS = 0x112;
inline constexpr bool SHAPE_SOUNDS = true;
inline constexpr bool SHARE_BY_MEMORY_SIZE = true;
inline constexpr bool SHOT_HITS_AS_ITEM = false;
inline constexpr bool SHUTDOWN_AFTER_START = true;
inline constexpr bool SHUTDOWN_ENDS_FADES = true;
inline constexpr bool SILENCER_SPRITE = true;
inline constexpr i16 SLOTS_X = 53;
inline constexpr i16 SLOTS_Y = 44;
inline constexpr bool SMALL_WEAPON_ANIMS = true;
inline constexpr i16 SONG_FILES = 22;
inline constexpr i16 SONG_FILES_CAPACITY = SONG_FILES;
inline constexpr std::array<const char *, 23> SONG_NAMES = {
    nullptr,  "ninth", "phil",   "straight", "party",  "demo", "stint",  "mk",  "space2", "d3",    "space", "rhythm",
    "intent", "m03",   "silver", "m01",      "techno", "cred", "regret", "m13", "retro",  "metal", nullptr};
inline constexpr std::array<SongTurns, 0> SONG_TURNS{};
inline constexpr int SONG_TURNS_CAPACITY = 0;
inline constexpr int SOUND_CACHE_SPARE = (15000 + 4096);
inline constexpr int SOUND_FLEX_MODE = FILE_READ;
inline constexpr bool SOUND_FLEX_TAGGED = false;
inline constexpr u32 SOUND_FRAGMENT_SIZE = 315000;
inline constexpr bool STANDING_PUTS_WEAPON_AWAY = false;
inline constexpr bool START_CHECKS_FILES = true;
inline constexpr bool START_HOLDS_SILENCER = true;
inline constexpr i16 STATUS_AMMO_FONT = 8;
inline constexpr i16 STATUS_AMMO_X = 38;
inline constexpr i16 STATUS_AMMO_Y = 8;
inline constexpr i16 STATUS_BAR_HEIGHT = 11;
inline constexpr u16 STATUS_BAR_LENGTH = 64;
inline constexpr i16 STATUS_BAR_X = 35;
inline constexpr i16 STATUS_BAR_Y = 8;
inline constexpr i16 STATUS_CLIPS_X = 90;
inline constexpr bool STATUS_NAMES = true;
inline constexpr bool STATUS_NO_HEALTH_SHAPE = true;
inline constexpr u8 STATUS_PAINT_MODE = 0;
inline constexpr std::array<StrongerWeapon, 14> STRONGER_WEAPONS = {
    StrongerWeapon{0x1b4, 4, 13, 9},  StrongerWeapon{0x528, 3, 9, 8},   StrongerWeapon{0x371, 3, 9, 10},
    StrongerWeapon{0x4d1, 2, 4, 11},  StrongerWeapon{0x383, 3, 10, 7},  StrongerWeapon{0x4e6, 3, 5, 11},
    StrongerWeapon{0x385, 4, 8, 9},   StrongerWeapon{0x338, 3, 5, 7},   StrongerWeapon{0x2cb, 2, 3, 7},
    StrongerWeapon{0x308, 2, 10, 11}, StrongerWeapon{0x57a, 2, 13, 15}, StrongerWeapon{0x30c, 2, 4, 15},
    StrongerWeapon{0x319, 2, 8, 9},   StrongerWeapon{0x5e2, 0, 14, 14}};
inline constexpr SubtitleStyle SUBTITLE_STYLE = {.font = 3,
                                                 .stripWidth = 640,
                                                 .stripHeight = 15,
                                                 .stripColour = 1,
                                                 .centreWidth = 640,
                                                 .shift = 0,
                                                 .textY = 10,
                                                 .twoTone = true,
                                                 .screenX = 0,
                                                 .screenY = 420,
                                                 .gap = 0,
                                                 .room = 0,
                                                 .lowest = 0,
                                                 .hiColourTextY = 8,
                                                 .hiColourLines = 10,
                                                 .ink = 0};
inline constexpr u16 SURRENDER_SIZE = 0x4a;
inline constexpr bool SURRENDER_SPEAKS_ONCE = false;
inline constexpr bool SWAP_REBUILD_HALTS = true;
inline constexpr bool TAKE_HIT_NO_REGRET = true;
inline constexpr bool TARGETING_KEY = false;
inline constexpr bool TELEPORT_COMPACTS_MEMORY = true;
inline constexpr bool TELEPORT_RESTARTS_TRACKER = false;
inline constexpr bool THAW_ENDS_TELEPORT = false;
inline constexpr bool THAW_REDRAWS_SPRITES = true;
inline constexpr bool TIMED_FEINT = false;
inline constexpr u8 TITLE_BAR_COLOUR = RED;
inline constexpr bool TITLE_SPACED = false;
inline constexpr bool TUNES_BY_MISSION = false;
inline constexpr i16 TUNES_READ = 22;
inline constexpr i16 TUNES_WRITTEN = 22;
inline constexpr std::array<u16, 4> UNARMED_SHOOTERS = {0x1cd, 0x4c8, 0x5e6, 0x5ba};
inline constexpr std::array<u16, 7> UNCHARGED_ITEMS = {{SHAPE_DATA_LINK, SHAPE_INHIBITOR, SHAPE_GRAVITON_SHIELD,
                                                        SHAPE_PLASMA_SHIELD, SHAPE_IONIC_SHIELD, SHAPE_VIR_IMAGER,
                                                        SHAPE_RADIATION_SHIELD}};
inline constexpr std::array<u16, 5> UNSTACKED_SHAPES = {SHAPE_IONIC_SHIELD, SHAPE_PLASMA_SHIELD, SHAPE_GRAVITON_SHIELD,
                                                        SHAPE_VIR_IMAGER, SHAPE_RADIATION_SHIELD};
inline constexpr SlotRange UTILITY_SLOTS = {13, 20};
inline constexpr i16 VERSION_FRAGMENTS = 5;
inline constexpr u16 VERSION_SOUND = 0x190;
inline constexpr bool VIOLENCE_OPTION = true;
inline constexpr std::array<u16, 5> WATCHFUL_SHAPES = {0x2f5, 0x2f6, 0x2f7, 0x384, 0x344};
inline constexpr bool WEAK_WEAPONS_TAKE_TYPE_DAMAGE = true;
inline constexpr bool WEAPONS_ONLY_FOR_CONTAINERS = true;
inline constexpr bool WEAPON_KEYS_SILENCER_ONLY = false;
inline constexpr bool WEAPON_LISTS = false;
inline constexpr std::array<i16, 6> WEAPON_LIST_1 = {-1};
inline constexpr std::array<i16, 6> WEAPON_LIST_2 = {-1};
inline constexpr std::array<i16, 4> WEAPON_LIST_3 = {-1};
inline constexpr SlotRange WEAPON_SLOTS = {1, 12};
inline constexpr u8 WIDGET_DIM_EDGE = 154;
inline constexpr u8 WIDGET_DIM_FACE = 148;
inline constexpr u8 WIDGET_DIM_WARNING_EDGE = 230;
inline constexpr u8 WIDGET_DIM_WARNING_FACE = 2;
inline constexpr u8 WIDGET_EDGE = 143;
inline constexpr u8 WIDGET_FACE = 144;
inline constexpr u8 WIDGET_FOCUS = 0x74;
inline constexpr u8 WIDGET_WARNING_EDGE = 224;
inline constexpr u8 WIDGET_WARNING_FACE = 1;
inline constexpr bool WORLD_KEYS_NO_REGRET = true;
inline constexpr bool XAN_MOVIE_HOST = true;
inline constexpr bool XAN_MOVIE_OPTIONS = true;

}
#endif
