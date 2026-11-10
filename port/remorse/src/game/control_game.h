/* No Remorse's values of the controlled NPC (common/src/game/control.cpp). */
#ifndef PORT_CONTROL_GAME_H
#define PORT_CONTROL_GAME_H
#include <array>
#include "common.h"
#include "animprim.h"
#include "dtable.h"

/* The auto firer's shots a second while the fire key is held with the PL-1
 * (AutoFirer::AutoFirer); none of them uses energy, only each fire animation
 * the key starts (Controller::fire), 45. */
inline constexpr u16 AUTO_FIRER_RATE = 6;
/* The quality of the items the cheat gives (giveItem). */
inline constexpr u16 CHEAT_ITEM_QUALITY = 100;
/* The NPCs a shot alerts only when they can see the controlled NPC
 * (alertNpcs). */
inline constexpr std::array<u16, 5> WATCHFUL_SHAPES = {0x2f5, 0x2f6, 0x2f7, 0x595, 0x597};
/* The controlled NPC's last animations that are not a combat stance
 * (inCombatStance). */
inline constexpr std::array<u16, 9> NON_COMBAT_ANIMS = {
    ANIM_WALK,          ANIM_STAND,         ANIM_QUICK_JUMP,  ANIM_RUN, ANIM_COMBAT_RUN_SMALL, ANIM_COMBAT_RUN_LARGE,
    ANIM_UNREADY_SMALL, ANIM_UNREADY_LARGE, ANIM_ELECTROCUTED};
/* Whether the auto firer's shots use no energy (AutoFirer::event). */
inline constexpr bool AUTO_FIRER_USES_AMMO = false;
/* The reticle shows while the Silencer is held too (No Regret's stays away
 * at a healing or charging station and an egg's teleport) (showReticle). */
inline constexpr bool RETICLE_HIDDEN_WHILE_HELD = false;
/* The reticle's one look (reticleStyle). */
inline constexpr i16 FIRST_RETICLE_STYLE = 0;
/* The bytes of the controller's globals in a saved game (CONTROLLER_GLOBALS,
 * control_game.cpp): the original's data segment from fireHeld to the word
 * after targetItem, weaponKeys1's first. */
inline constexpr int CONTROLLER_BLOCK = 75;
/* The controller's saved block's room: CONTROLLER_BLOCK here; the one program's build takes the larger of
 * the two games'. */
inline constexpr int CONTROLLER_BLOCK_CAPACITY = CONTROLLER_BLOCK;
/* The arrow keys turn the busy controlled NPC a step every 12 ticks
 * (Controller::turn): No Regret's. */
inline constexpr bool BUSY_NPC_TURNS = false;
/* The forward roll is a step (stepAllowed): No Regret's, which has the roll. */
inline constexpr bool ROLL_FORWARD_STEPS = false;
/* The fire key does nothing with the auto firer on and no ammunition left
 * (Controller::fire): No Regret's. */
inline constexpr bool AUTO_FIRE_STOPS_EMPTY = false;
/* The controlled NPCs that fire with no weapon, the robots (Controller::fire): none in No Remorse. */
inline constexpr std::array<u16, 0> UNARMED_SHOOTERS = {};
/* On difficulty 1 and 2 the RP-32 never runs dry: the controlled NPC is given
 * a clip and reloads (Controller::fire): No Regret's. */
inline constexpr bool RP_32_NEVER_DRY = false;
/* The weapons of the number keys 1-0, - and = (handleKey), which the cheat
 * gives too (cheatKey), to -1 (weaponKeys): No Regret's, none here. */
inline constexpr std::array<i16, 13> NUMBER_KEY_WEAPONS = {-1};
/* The cheat's lists of weapons, to -1 (weaponKeys1-3, WEAPON_LISTS). */
inline constexpr std::array<i16, 6> WEAPON_LIST_1 = {SHAPE_UV_9,   SHAPE_PA_31, SHAPE_AR_7,
                                                     SHAPE_GL_303, SHAPE_RP_32, -1};
inline constexpr std::array<i16, 6> WEAPON_LIST_2 = {SHAPE_BA_40, SHAPE_BA_41, SHAPE_RP_22,
                                                     SHAPE_PA_21, SHAPE_PL_1,  -1};
inline constexpr std::array<i16, 4> WEAPON_LIST_3 = {SHAPE_SG_A1, SHAPE_AC_88, SHAPE_EM_4, -1};
/* The cheat gives one of the three lists, Ctrl the second, Alt the third,
 * else the first (cheatKey): No Remorse's; No Regret's gives the number keys'
 * weapons. */
inline constexpr bool WEAPON_LISTS = true;
/* The number keys select the weapons of NUMBER_KEY_WEAPONS, and the volume
 * keys (- and =) need Ctrl (handleKey): No Regret's. */
inline constexpr bool NUMBER_KEYS_SELECT_WEAPONS = false;
/* The items the cheat gives up to their count when the controlled NPC has
 * none, and the clips it gives one each of (cheatKey). */
inline constexpr std::array<CheatGift, 4> CHEAT_GIFTS = {
    {{SHAPE_SPIDER_BOMB, 20}, {SHAPE_LAND_MINE, 20}, {SHAPE_DET_PAC, 20}, {SHAPE_MEDICAL_KIT, 20}}};
inline constexpr std::array<u16, 5> CHEAT_CLIPS = {
    {SHAPE_BA_41_CLIP, SHAPE_RP_32_CLIP, SHAPE_AR_7_CLIP, SHAPE_GL_303_CLIP, SHAPE_SG_A1_CLIP}};
/* The cheat sets the credits to 30000 (cheatKey). */
inline constexpr bool CHEAT_GIVES_CREDITS = true;
inline constexpr u32 CHEAT_CREDITS = 30000;
/* Ctrl with the cheat key only turns immortality on or off: No Regret's; No
 * Remorse's revives and gives the items too (cheatKey). */
inline constexpr bool CHEAT_IMMORTALITY_ALONE = false;
/* The debugger's key (Ctrl+Alt+Escape) empties the key queue first: No
 * Regret's (cheatKey). */
inline constexpr bool DEBUG_KEY_FLUSHES = false;
/* The controlled NPC turned to shape 5D6h (takeHit's damage types 16 and 18)
 * counts as dead: the cheat revives it and the move keys do nothing
 * (cheatKey, Controller::keyboard): No Regret's. */
inline constexpr bool SHAPE_5D6_IS_DEAD = false;
/* Ctrl+R cycles the reticle's looks (handleKey): No Regret's. */
inline constexpr bool RETICLE_STYLE_KEY = false;
/* The key that takes back the Silencer ends a stasis, and with the Silencer
 * controlled and the game running only empties the key queue (handleKey): No
 * Regret's; No Remorse's takes it back from another NPC only. */
inline constexpr bool RETURN_KEY_ENDS_STASIS = false;
/* The jump key with a weapon drawn unreadies it first (Controller::keyboard):
 * No Remorse's. */
inline constexpr bool JUMP_UNREADIES_WEAPON = true;
/* The fire key breaks into the busy NPC's animation when it has ammunition
 * and is not firing already (Controller::keyboard): No Regret's. */
inline constexpr bool FIRE_INTERRUPTS_BUSY = false;
/* JASSICA16, No Remorse's cheat code, is a joke: the cheats were changed,
 * the Silencer comes back and the game jumps to map E6h with the Christmas
 * songs (typeCheat): No Regret's; No Remorse's turns the cheat keys on and
 * off. */
inline constexpr bool JASSICA_JOKE = false;
/* No Regret's other cheat codes (typeCheat): LOOSECANNON16 turns the cheat
 * keys on and off, PIX Backspace's screenshots (Controller::keyboard), MEMORY
 * the memory profiler, VIDTEST a loop of two movies. */
inline constexpr bool MORE_CHEAT_CODES = false;
/* Controller::move's rules: No Regret's (true) have the kneeling walk (the
 * strafe keys and Alt's moves of a kneeling NPC take the kneeling
 * animations), the forward roll (Ctrl and a move key, also out of a run),
 * the jump forward (the forward keys of a kneeling or running NPC), the
 * kneeling NPC's own animations (isKneelAnim), a busy NPC's animation
 * broken into for the new one, a stop that waits for animations 19h and 27h
 * and a stop of the run that readies the weapon of an NPC other than the
 * Silencer; No Remorse's (false) slide and roll toward the side nearer the
 * key's direction (Alt and Ctrl with a move key, SIDE_STEP), stand the
 * kneeling NPC up with its strafe keys, unready the weapon with Ctrl and up
 * (the Silencer then jumps), and only turn a busy NPC. */
inline constexpr bool MOVE_NO_REGRET = false;
/* The boss's shield as the game starts (bossShield). */
inline constexpr i16 BOSS_SHIELD_START = 5000;
#endif
