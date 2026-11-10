/* No Regret's values of the sprites and shots (common/src/game/sprite.cpp). */
#ifndef PORT_SPRITE_GAME_H
#define PORT_SPRITE_GAME_H

/* An NPC a shot hits takes the hit from the shot's source, the NPC that fired it (SuperSprite::hit). */
inline constexpr bool SHOT_HITS_AS_ITEM = false;
/* A sprite of shape 352h, 609h, 1F2h, 3D5h or 62Bh started at the controlled
 * NPC follows it, the controller listening to it (silencerSprite;
 * createSpriteProcess). */
inline constexpr bool SILENCER_SPRITE = true;
/* A shot at the reticle aims six times as far as the reticle from the firer
 * (launchAtReticle). */
inline constexpr bool AIM_PAST_RETICLE = true;
/* A robot's (an NPC that is not organic) shots go where it aims
 * (SuperSprite::launch). */
inline constexpr bool ROBOTS_AIM_EXACTLY = true;
/* A shot from what is not an NPC (a turret) spreads less, a fifth, at a
 * kneeling controlled NPC, else by three times the dodge factor
 * (SuperSprite::launch): No Remorse's. */
inline constexpr bool OTHER_SHOOTERS_SPREAD = false;
#endif
