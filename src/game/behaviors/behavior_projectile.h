#ifndef GAME_BEHAVIOR_PROJECTILE_H
#define GAME_BEHAVIOR_PROJECTILE_H

#include "behaviors.h"

/* Shared projectile module (Wave N2). A projectile is a lightweight runtime
   Entity (FourCC "PROJ") spawned mid-frame: it integrates along a straight
   line, raycasts through SOLID geometry to stop at walls, and on overlapping
   a valid target applies damage, then despawns. It is the common path for
   enemy bolts and the player's thrown baseball (and Wave N3 player combat). */

#define PROJ_TEAM_ENEMY  0  /* fired by an enemy; damages the player */
#define PROJ_TEAM_PLAYER 1  /* fired by the player; defeats enemies   */

/* Spawn a projectile at (ox,oy,oz) travelling along (dx,dy,dz) (need not be
   normalized) at `speed` units/sec, living up to `ttl` seconds, dealing
   `damage` on hit. Returns the new entity, or NULL on failure. */
Entity *projectile_spawn(World *w, float ox, float oy, float oz,
                         float dx, float dy, float dz,
                         int team, float speed, int damage, float ttl);

extern const EntityVTable vt_projectile;  /* FourCC "PROJ" */

#endif
