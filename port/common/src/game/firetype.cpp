/* FIRETYPE.C (Code39, 1130:062d-069e, No Regret's addresses): the fire
 * types, what each kind of shot does; the table is the game's
 * (<game>/src/game/firetype_game.cpp). */
#include "firetype.h"
#include "utility.h"

/* Halts when there is no such type and then returns nothing. */
FireType *fireType(u16 type)
{
    for (i16 i = 0; i < FIRE_TYPES; i++) {
        if (fireTypes[i].type == type) {
            return &fireTypes[i];
        }
    }
    halt();
}

i16 damageOf(i16 type)
{
    FireType *fire = fireType(type);
    return randomBetween(fire->lowDamage, fire->highDamage);
}
