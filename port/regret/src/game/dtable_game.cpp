/* No Regret's tables of the weapons, their ammunition, the bombs and the
 * inventory items, with their names and gump frames (common/src/game/dtable.cpp). */
#include "dtable.h"
#include "gamestate.h"

namespace {
/* Each table has its name in front of it, as the fire types' note does. */
[[maybe_unused]] const char *ammoTableName = "AMMO TABLE";
}
AmmoType (&ammoTypes)[AMMO_TYPES] = gameGlobal<AmmoType[AMMO_TYPES]>({{0, 0, 0, 0},
                                                                      {SHAPE_BA_41_CLIP, 0x14, 0x14, 8},
                                                                      {SHAPE_SG_A1_CLIP, 0xc, 0x1e, 0xa},
                                                                      {SHAPE_RP_32_CLIP, 0x3c, 0x28, 9},
                                                                      {SHAPE_AR_7_CLIP, 8, 0x32, 0xb},
                                                                      {SHAPE_GL_303_CLIP, 0xa, 0x3c, 0xc},
                                                                      {SHAPE_BK_16_CLIP, 0xc, 0x3c, 0x1b},
                                                                      {SHAPE_LNR_81_CLIP, 0xa, 0x3c, 0x1a},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0}});
namespace {
[[maybe_unused]] const char *weaponTableName = "WEAPON TABLE";
}
WeaponType (&weaponTypes)[WEAPON_TYPES] =
    gameGlobal<WeaponType[WEAPON_TYPES]>({{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                          {SHAPE_BA_40, 0x2c, 0xc, 0, 1, 0, 0, 1, 2, 0x2b, 0xb},
                                          {SHAPE_BA_41, 0x2c, 9, 1, 2, 0, 0, 1, 1, 0x2b, 0xb},
                                          {SHAPE_PA_21, 2, 0x14, 0, 1, 0x19, 0, 1, 7, 0, 5},
                                          {SHAPE_EM_4, 0x2f, 0x14, 0, 1, 0xc8, 0, 0, 3, 0, 0xf},
                                          {SHAPE_SG_A1, 0x55, 0x19, 2, 1, 0, 0, 0, 0, 0x19, 2},
                                          {SHAPE_RP_22, 0x2c, 0xa, 3, 2, 0, 0, 0, 6, 0x2b, 1},
                                          {SHAPE_RP_32, 0x2c, 6, 3, 3, 0, 0, 0, 5, 0x2b, 1},
                                          {SHAPE_AR_7, 0x2c, 0xa, 4, 1, 0, 0, 0, 0xc, 0x2b, 0xa},
                                          {SHAPE_GL_303, 0x64, 0x3c, 5, 1, 0, 0, 0, 8, 0x2b, 3},
                                          {SHAPE_PA_31, 2, 0xc, 0, 1, 0x2d, 0, 0, 9, 0, 5},
                                          {SHAPE_PL_1, 0x47, 6, 0, 1, 0x50, 0, 0, 0xa, 0, 6},
                                          {SHAPE_AC_88, 0x55, 0xa, 2, 1, 0, 0, 0, 4, 0x2b, 0xd},
                                          {SHAPE_UV_9, 0x89, 0x23, 0, 1, 0xd2, 0, 0, 0xb, 0, 0xe},
                                          {SHAPE_BK_16, 0x162, 0x14, 6, 1, 0, 0, 0, 0xe, 0x2b, 0x10},
                                          {SHAPE_LNR_81, 0x165, 0xf, 7, 1, 0, 0, 0, 0xf, 0x2b, 0x11},
                                          {SHAPE_XP_5, 0x167, 6, 0, 1, 0x8c, 0, 0, 0x10, 0, 0x14},
                                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
                                          {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}});
namespace {
[[maybe_unused]] const char *miscTableName = "MISC TABLE";
}
InventoryType (&inventoryTypes)[INVENTORY_TYPES] =
    gameGlobal<InventoryType[INVENTORY_TYPES]>({{0, 0},
                                                {SHAPE_MEDICAL_KIT, 0},
                                                {SHAPE_DATA_LINK, 1},
                                                {SHAPE_INHIBITOR, 2},
                                                {SHAPE_IONIC_SHIELD, 3},
                                                {SHAPE_ENERGY_CUBE, 0x19},
                                                {SHAPE_PLASMA_SHIELD, 5},
                                                {SHAPE_DET_PAC, 0x18},
                                                {SHAPE_GRAVITON_SHIELD, 7},
                                                {SHAPE_CHEMICAL_BATTERY, 0x16},
                                                {SHAPE_FUSION_BATTERY, 0x15},
                                                {SHAPE_FISSION_BATTERY, 0x17},
                                                {SHAPE_DISRUPTER, 0x1c},
                                                {SHAPE_SPIDER_MINE, 0x1d},
                                                {SHAPE_DATA_PICK, 0x1e},
                                                {SHAPE_MINE_DET, 0x1f},
                                                {SHAPE_PORTABLE_BETTY, 0x22},
                                                {SHAPE_RADIATION_SHIELD, 0x21},
                                                {SHAPE_VIR_IMAGER, 0x20}});
BombType (&bombTypes)[BOMB_TYPES] = gameGlobal<BombType[BOMB_TYPES]>({{0, 0, 0, 0},
                                                                      {SHAPE_GRENADE, 1, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {0, 0, 0, 0},
                                                                      {SHAPE_EMP_GRENADE, 0, 0, 3},
                                                                      {SHAPE_SPIDER_BOMB, 0, 0, 4},
                                                                      {SHAPE_LAND_MINE, 0, 0, 5},
                                                                      {SHAPE_FUSION_PAC, 0, 0, 6},
                                                                      {0, 0, 0, 0}});
const char *(&weaponNames)[18] = gameGlobal<const char *[18]>({"INVALID", "BA-40", "BA-41", "PA-21", "EM-4", "SG-A1",
                                                               "RP-22", "RP-32", "AR-7", "GL-303", "PA-31", "PL-1",
                                                               "AC-88", "UV-9", "BK-16", "LNR-81", "XP-5", nullptr});
const char *(&inventoryNames)[19] = gameGlobal<const char *[19]>(
    {"INVALID", "MEDICAL KIT", "DATA LINK", "INHIBITOR", "IONIC SHIELD", "ENERGY CUBE", "PLASMA SHIELD", "DET PAC",
     "GRAVITON SHIELD", "CHEMICAL BATTERY", "FUSION BATTERY", "FISSION BATTERY", "DISRUPTER", "SPIDER MINE",
     "DATA PICK", "MINE DET", "PORTABLE BETTY", "RADIATION SHIELD", "VIR IMAGER"});
const char *(&ammoNames)[8] = gameGlobal<const char *[8]>({"INVALID", "BA-41 CLIP", "SG-A1 CLIP", "RP-32 CLIP",
                                                           "AR-7 CLIP", "GL-303 CLIP", "BK-16 CLIP", "LNR-81 CLIP"});
const char *(&bombNames)[8] = gameGlobal<const char *[8]>({"INVALID", "GRENADE", "CONCUSSION GRENADE",
                                                           "NERVE GAS GRENADE", "EMP GRENADE", "SPIDER BOMB",
                                                           "LAND MINE", "FUSION PAC"});
