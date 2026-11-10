/* No Remorse's values of the sprites and shots (common/src/game/sprite.cpp). */
#ifndef PORT_SPRITE_GAME_H
#define PORT_SPRITE_GAME_H

/* An NPC a shot hits takes the hit from the shot's item, not its source (No Regret's names the NPC that fired it)
 * (SuperSprite::hit). */
inline constexpr bool SHOT_HITS_AS_ITEM = true;
/* A sprite of shape 352h, 609h, 1F2h, 3D5h or 62Bh started at the controlled
 * NPC follows it, the controller listening to it (silencerSprite;
 * createSpriteProcess): not No Remorse's. */
inline constexpr bool SILENCER_SPRITE = false;
/* A shot at the reticle aims six times as far as the reticle from the firer
 * (launchAtReticle); No Remorse's aims at the reticle. */
inline constexpr bool AIM_PAST_RETICLE = false;
/* A robot's (an NPC that is not organic) shots go where it aims: No Regret's
 * (SuperSprite::launch). */
inline constexpr bool ROBOTS_AIM_EXACTLY = false;
/* A shot from what is not an NPC (a turret) spreads less, a fifth, at a
 * kneeling controlled NPC, else by three times the dodge factor
 * (SuperSprite::launch). */
inline constexpr bool OTHER_SHOOTERS_SPREAD = true;
#endif
