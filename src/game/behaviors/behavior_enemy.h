#ifndef GAME_BEHAVIOR_ENEMY_H
#define GAME_BEHAVIOR_ENEMY_H

#include "behaviors.h"

/* Apply a hit to `e` if it is a live enemy. Returns 1 when the hit landed
   on an enemy (so the caller — a player projectile — can despawn), 0 if `e`
   is not an enemy or is already knocked out. Public so behavior_projectile.c
   (and Wave N3 player combat) can defeat enemies without knowing the family. */
int behavior_enemy_take_hit(Entity *e, int dmg);

extern const EntityVTable vt_yokian;  /* 3SOL / 3GUA / 3SPY — C3DYokian family */

#endif
