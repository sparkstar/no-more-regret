/* The port's: the sound effects the code plays by number (sound\sound.flx's
 * objects, sound n object n), named for what they are where they are
 * played. The comments give the archive's own names (its object 0: a long
 * and a name ending in 0 a sound). A number the code plays in one game
 * only names that game's sound. */
#ifndef PORT_SOUNDS_H
#define PORT_SOUNDS_H

/* The controlled NPC's weapon and the hits. */
constexpr int SOUND_DRY_FIRE = 0x2a;       /* CLICK: fire held with no ammunition to reload */
constexpr int SOUND_SHIELD_ABSORBS = 0x48; /* ELEC3A: a shield takes a hit's damage */
constexpr int SOUND_HURT_FEMALE = 0xd8;    /* FW04 */
constexpr int SOUND_HURT_MALE = 0x8f;      /* DETHUM05: also the Silencer landing after a fall */
/* No Regret's boss: its shield gone (ROBO7). */
constexpr int SOUND_BOSS_SHIELD_DOWN = 0x1eb;
/* No Regret's hits of fireExplosion's fire types 5, 6, 17 and 21. */
constexpr int SOUND_LASER_HIT_1 = 0x71;  /* LASRHIT1 */
constexpr int SOUND_LASER_HIT_2 = 0x164; /* LASRHIT2 */
constexpr int SOUND_PL_1_HIT = 0x206;    /* PL1_HIT1 */
constexpr int SOUND_LNR_81_HIT = 0x203;  /* LNRHIT3 */
constexpr int SOUND_UV_9_SHOT = 0x20a;   /* UV9SHOT2 */

/* The medical kit and the energy's recharge: the loop while they run, and
 * a sound for each point healed or charged. */
constexpr int SOUND_HEALING = 0xdb;       /* HEALING */
constexpr int SOUND_HEAL_POINT = 0xba;    /* HEAL1 */
constexpr int SOUND_CHARGING = 0xa4;      /* FUSNCORE */
constexpr int SOUND_CHARGE_POINT = 0x10b; /* RECHARGE */

/* The gumps. */
constexpr int SOUND_KEYPAD_DIGIT = 0x3b;    /* BUTTON3A */
constexpr int SOUND_KEYPAD_ERASE = 0x3a;    /* BUTTON2A */
constexpr int SOUND_TERMINAL_TYPING = 0x33; /* BEEP5C: a computer's text typed out, a character */
constexpr int SOUND_VOLUME_SAMPLE = 0x98;   /* FFBRIDGE: the options' effects volume changed */
/* No Remorse's weasel refuses a purchase (NOTCLOSE). */
constexpr int SOUND_WEASEL_REFUSES = 0xb0;

/* The cheat codes: No Remorse's (NC15), No Regret's answer to it (ROBO1),
 * No Regret's own (COMP36). */
constexpr int SOUND_CHEATS_TOGGLED = 0x103;
constexpr int SOUND_OLD_CHEAT_JOKE = 0x1e5;
constexpr int SOUND_CHEAT_CODE = 0x190;

#endif
