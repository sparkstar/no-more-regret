/* The port's: the shapes (static\shapes.flx) the code names by number,
 * besides the tables' weapons and items (dtable.h). Named for what their
 * frames show, and where the usecode has a class of the shape (an item's
 * class is its shape's), by the class's name (in the comments). A shape the
 * code takes in one game only is that game's; the others are the same in
 * both. */
#ifndef PORT_SHAPES_H
#define PORT_SHAPES_H

/* The bosses (each game's BOSS_SHAPE), where the code takes them in their
 * own game only: No Regret's robot (D_ROBO), No Remorse's Vargas (VARGAS). */
constexpr int SHAPE_BOSS_ROBOT = 0x5b1;
constexpr int SHAPE_VARGAS = 0x3ac;

/* The shots (fireWeapon's sprites, a frame for each fire type and
 * direction) and what their hits show (fireExplosion). */
constexpr int SHAPE_SHOT = 0x1d9;
constexpr int SHAPE_BULLET_HIT = 0x1d8; /* sparks */
constexpr int SHAPE_SMALL_HIT = 0x537;  /* a small flash */
constexpr int SHAPE_PL_1_HIT = 0x578;   /* a blue ring */
constexpr int SHAPE_UV_9_HIT = 0x56b;   /* red rings */
constexpr int SHAPE_EM_4_HIT = 0x59b;   /* rings of white sparks */
constexpr int SHAPE_BK_16_HIT = 0x643;  /* No Regret's: a blue ring */
constexpr int SHAPE_LNR_81_HIT = 0x642; /* No Regret's: a green glow */
constexpr int SHAPE_XP_5_HIT = 0x641;   /* No Regret's: orange rings */
/* The smoke a GL-303 grenade leaves behind it. */
constexpr int SHAPE_GRENADE_TRAIL = 0x426;
/* No Regret's spider bomb crawling once set off (CSPID; SHAPE_SPIDER_BOMB is
 * the one carried), which the NPCs shoot at (attack.cpp, activity.cpp). */
constexpr int SHAPE_CRAWLING_SPIDER = 0x584;
/* The controlled NPC's target reticle (showReticle). */
constexpr int SHAPE_RETICLE = 0x59a;
/* A shape of no frames whose size canExistAt sets to test another's room. */
constexpr int SHAPE_FIT_PROBE = 0x31a;

/* The shields' sprites: the Silencer's shields of types 1 and 2 flash at
 * the hit, of types 3 and 4 show a sphere around him (Npc::shieldType); the
 * flares of the robots' and No Regret's boss's shields; No Regret's sparks
 * around the Silencer shocked (FIRE_ELECTRIC). */
constexpr int SHAPE_SHIELD_FLASH = 0x5a9;
constexpr int SHAPE_SHIELD_SPHERE = 0x52b;
constexpr int SHAPE_SHIELD_FLARE = 0x4f1;
constexpr int SHAPE_SHOCK_SPARKS = 0x62b;

/* The shapes an NPC dies in (takeHit). */
constexpr int SHAPE_BURNING = 0x576;           /* KORESH: running in flames */
constexpr int SHAPE_BURNING_GIBS = 0x656;      /* D_FLAMEX */
constexpr int SHAPE_GIBS = 0x278;              /* MEATY: No Regret's, FIRE_XP_5 */
constexpr int SHAPE_PLASMA_DEATH = 0x58f;      /* D_PLASMA: FIRE_PL_1 */
constexpr int SHAPE_SKELETON_DEATH = 0x596;    /* D_SKELET: FIRE_UV_9 */
constexpr int SHAPE_EM_DEATH = 0x59c;          /* D_EM_GUY: FIRE_EM_4 */
constexpr int SHAPE_LNR_DEATH = 0x62d;         /* D_LNR: No Regret's, FIRE_LNR_81 */
constexpr int SHAPE_FROZEN = 0x5d6;            /* FREEZ: No Regret's, FIRE_BK_16 and 18 */
constexpr int SHAPE_FROZEN_SHATTERING = 0x5ef; /* FREEZEXP: No Regret's */

/* NPCs: the workers that surrender, */
constexpr int SHAPE_MAINTENANCE_MAN = 0x2f5; /* MAINTMAN */
constexpr int SHAPE_CHEMSUIT_WORKER = 0x2f6; /* CHEMSUIT */
constexpr int SHAPE_OFFICE_WORKER = 0x2f7;   /* OFFWORK */
constexpr int SHAPE_SCIENTIST = 0x344;       /* SCIENTIS */
constexpr int SHAPE_SEATED_LADY = 0x597;     /* SEATLADY (No Regret's class) */
/* No Regret's other people (Npc::playHurtSound), */
constexpr int SHAPE_CONGRESS = 0x2df;      /* CONGRESS */
constexpr int SHAPE_FREIGHTER = 0x384;     /* FREIGHTE */
constexpr int SHAPE_MINER = 0x4e0;         /* NMINER */
constexpr int SHAPE_SEATED_WORKER = 0x595; /* SEATWORK */
/* the armoured guards, */
constexpr int SHAPE_STORM_TROOPER = 0x371;   /* STORMTRO */
constexpr int SHAPE_ELITE_GUARD = 0x4d1;     /* ELITE */
constexpr int SHAPE_ELITE_GRENADIER = 0x528; /* EGRENADE */
/* No Regret's guards, each kind with two kneeling shapes and the kneeling
 * one's death (D_LMC2, D_ENFOR2, D_HQ2, D_STORM2), */
constexpr int SHAPE_LMC_GUARD = 0x308; /* LMC_GUAR */
constexpr int SHAPE_LMC_GUARD_KNEELING_A = 0x5f0;
constexpr int SHAPE_LMC_GUARD_KNEELING_B = 0x2c3;
constexpr int SHAPE_LMC_GUARD_KNEELING_DEATH = 0x5d5;
constexpr int SHAPE_ENFORCER = 0x1b4;
constexpr int SHAPE_ENFORCER_KNEELING_A = 0x625;
constexpr int SHAPE_ENFORCER_KNEELING_B = 0x626;
constexpr int SHAPE_ENFORCER_KNEELING_DEATH = 0x62e;
constexpr int SHAPE_HQ_GUARD = 0x57a; /* HQGUARD */
constexpr int SHAPE_HQ_GUARD_KNEELING_A = 0x5ff;
constexpr int SHAPE_HQ_GUARD_KNEELING_B = 0x5d7;
constexpr int SHAPE_HQ_GUARD_KNEELING_DEATH = 0x606;
constexpr int SHAPE_STORM_TROOPER_KNEELING_A = 0x62f;
constexpr int SHAPE_STORM_TROOPER_KNEELING_B = 0x630;
constexpr int SHAPE_STORM_TROOPER_KNEELING_DEATH = 0x631;
/* and the robots. */
constexpr int SHAPE_THERMATRON = 0x338; /* THERMATR */
constexpr int SHAPE_ANDROID = 0x383;
constexpr int SHAPE_CHEDITRON = 0x385; /* CHEDITRO */
constexpr int SHAPE_VETRON = 0x4e6;
constexpr int SHAPE_AVATRON = 0x30c;  /* No Regret's */
constexpr int SHAPE_CAMETRON = 0x319; /* No Regret's */
constexpr int SHAPE_CRYOTRON = 0x5e2; /* No Regret's */

/* The alarms, which an alert switches on (setAlertActive): the alarm
 * switches (ALARM_EW, ALARM_NS, MALRM_EW, MALRM_NS) and boxes, and No
 * Regret's lights, which change shape (two kinds, off and on). */
constexpr int SHAPE_ALARM_EW = 0x21;
constexpr int SHAPE_ALARM_NS = 0x49;
constexpr int SHAPE_MALRM_EW = 0x271;
constexpr int SHAPE_MALRM_NS = 0x174;
constexpr int SHAPE_ALARM_BOX = 0x477;
constexpr int SHAPE_ALARM_LIGHT_A_OFF = 0x663;
constexpr int SHAPE_ALARM_LIGHT_A_ON = 0x660;
constexpr int SHAPE_ALARM_LIGHT_B_OFF = 0x662;
constexpr int SHAPE_ALARM_LIGHT_B_ON = 0x661;

#endif
